/**
 * @file lepus_abi_registry.cpp
 * @brief 公开 C 注册句柄，内部只使用现有 NativeRegistry。
 */
#include "lepus_abi_registry.hpp"
#include "lepus_abi_internal.h"
#include "lepus_abi_runtime.hpp"
#include "ircode/runtime_internal.hpp"
#include <cstring>
#include <exception>

namespace
{
/**
 * @brief 将注册失败转换为无分配的稳定诊断。
 * @param status 注册状态。
 * @param error 调用方诊断缓冲。
 * @return 输入状态。
 */
int registrationStatus(int status, LepusAbiError *error)
{
    const char *message = status == LepusAbiSealed    ? "registry is sealed" :
                          status == LepusAbiDuplicate ? "duplicate native name or symbol" :
                          status == LepusAbiOk        ? nullptr :
                                                        "native registration failed";
    return lepus_abi_status(error, status, message);
}

/** @brief 恢复借用的输入帧，失败时清空输出，纯标量调用提供局部错误通道。 */
struct FrameScope
{
    LepusAbiFrame &frame;
    const LepusAbiFrame before;
    LepusAbiRuntime local;
    LepusAbiRuntime *attached;
    bool succeeded = false;
    /**
     * @brief 暂时为没有语言上下文的帧接入错误通道。
     * @param value 已验证的调用帧。
     */
    explicit FrameScope(LepusAbiFrame &value) : frame(value), before(value), attached(value.runtime ? value.runtime : &local)
    { frame.runtime = attached; }
    /** @brief 恢复原始输入字段；调用方只接收合法输出。 */
    ~FrameScope()
    {
        frame.runtime = before.runtime;
        frame.slots = before.slots;
        frame.kinds = before.kinds;
        frame.count = before.count;
        if (!succeeded)
        {
            frame.result = 0;
            frame.resultIsFloat = 0;
        }
    }
};
}  // namespace

/**
 * @brief 创建拥有单个运行时函数表的 C 句柄，分配异常转换为错误。
 * @param error 可选的调用方诊断缓冲。
 * @return 句柄或空指针。
 */
extern "C" LepusAbiRegistry *lepus_abi_registry_create(LepusAbiError *error)
try
{
    Memory::AllocationScope allocation(nullptr);
    auto owned = Memory::makeCHeapUnique<runtime::NativeRegistry>();
    auto *registry = Memory::makeCHeapUnique<LepusAbiRegistry>(LepusAbiRegistry{ std::move(owned), nullptr }).release();
    registry->backend = registry->owned.get();
    lepus_abi_status(error, LepusAbiOk, nullptr);
    return registry;
} catch (...)
{
    lepus_abi_status(error, LepusAbiHostFailure, "registry allocation failed");
    return nullptr;
}

/**
 * @brief 销毁拥有句柄；借用视图不拥有表，也不能由此释放。
 * @param registry create 创建的句柄，允许为空。
 */
extern "C" void lepus_abi_registry_destroy(LepusAbiRegistry *registry)
{
    if (registry && registry->owned) Memory::destroyCHeap(registry);
}

/**
 * @brief 校验并原子登记整个模块，失败不改变既有回调。
 * @param registry 未封存的函数表。
 * @param module 模块描述。
 * @param error 可选的调用方诊断缓冲。
 * @return 稳定状态码。
 */
extern "C" int32_t lepus_abi_registry_register(LepusAbiRegistry *registry, const LepusAbiModuleDescriptor *module, LepusAbiError *error)
try
{
    if (!registry || !registry->backend) return lepus_abi_status(error, LepusAbiInvalidArgument, "missing registry");
    if (registry->backend->sealed()) return registrationStatus(LepusAbiSealed, error);
    const auto checked = lepus_abi_check_module(module, error);
    if (checked != LepusAbiOk) return checked;
    return registrationStatus(registry->backend->addDirectModule(module->records, module->count), error);
} catch (...)
{
    return lepus_abi_status(error, LepusAbiHostFailure, "native registration allocation failed");
}

/**
 * @brief 封存同一运行时表，C 与私有 C++ 注册路径同时受约束。
 * @param registry 函数表句柄。
 * @param error 可选的调用方诊断缓冲。
 * @return 稳定状态码。
 */
extern "C" int32_t lepus_abi_registry_seal(LepusAbiRegistry *registry, LepusAbiError *error)
{
    if (!registry || !registry->backend) return lepus_abi_status(error, LepusAbiInvalidArgument, "missing registry");
    registry->backend->seal();
    return lepus_abi_status(error, LepusAbiOk, nullptr);
}

/**
 * @brief 解析直接 C 入口，不公开 C++ 值或容器。
 * @param registry 函数表句柄。
 * @param name 完整名称。
 * @param entry 接收入口，失败时为空。
 * @param arity 可选的参数数量接收器。
 * @param error 可选的调用方诊断缓冲。
 * @return 稳定状态码。
 */
extern "C" int32_t lepus_abi_registry_resolve(const LepusAbiRegistry *registry, const char *name, LepusAbiEntry *entry, int32_t *arity, LepusAbiError *error)
try
{
    if (entry) *entry = nullptr;
    if (arity) *arity = 0;
    if (!registry || !registry->backend || !name || !*name || !entry)
        return lepus_abi_status(error, LepusAbiInvalidArgument, "invalid native lookup");
    const auto *found = registry->backend->find(name);
    if (!found || !found->direct) return lepus_abi_status(error, LepusAbiNotFound, "C entry not found");
    *entry = found->direct;
    if (arity) *arity = found->directArity;
    return lepus_abi_status(error, LepusAbiOk, nullptr);
} catch (...)
{
    return lepus_abi_status(error, LepusAbiHostFailure, "native lookup allocation failed");
}

/**
 * @brief 执行受检查的直接入口，隔离异常并恢复输入帧。
 * @param registry 函数表句柄。
 * @param name 完整名称。
 * @param frame 调用方持有的帧。
 * @param error 可选的调用方诊断缓冲。
 * @return 稳定状态码；失败时不返回部分输出。
 */
extern "C" int32_t lepus_abi_registry_call(const LepusAbiRegistry *registry, const char *name, LepusAbiFrame *frame, LepusAbiError *error)
try
{
    if (frame)
    {
        frame->result = 0;
        frame->resultIsFloat = 0;
    }
    if (!registry || !registry->backend || !name || !*name)
        return lepus_abi_status(error, LepusAbiInvalidArgument, "invalid native lookup");
    const auto *entry = registry->backend->find(name);
    if (!entry) return lepus_abi_status(error, LepusAbiNotFound, "native entry not found");
    if (!entry->direct && entry->arity != std::size_t(-1) && (!frame || frame->count < 0 || static_cast<std::uint64_t>(frame->count) != entry->arity))
        return lepus_abi_status(error, LepusAbiInvalidArgument, "argument count mismatch");
    auto checked = lepus_abi_check_frame(frame, entry->direct ? entry->directArity : -1, error);
    if (checked != LepusAbiOk) return checked;
    FrameScope scope(*frame);
    if (scope.attached->context && !scope.attached->context->memoryFault.empty())
        return lepus_abi_status(error, LepusAbiHostFailure, scope.attached->context->memoryFault.c_str());
    if (scope.attached->context && !scope.attached->context->error.empty())
        lepus_abi_mark_failure(frame, scope.attached->context->error.c_str());
    if (scope.attached->status == LepusAbiOk)
    {
        try
        {
            if (entry->direct)
                entry->direct(frame);
            else if (!scope.attached->context || !entry->callback)
                lepus_abi_mark_failure(frame, "native callback requires a runtime context");
            else
            {
                /* 迁移期复用原表的私有运行时回调；旧模板绑定删除后不产生此分支的标准库入口。 */
                using namespace runtime;
                Memory::Vector<NativeValue> values;
                values.reserve(static_cast<std::size_t>(frame->count));
                for (std::int64_t at = 0; at < frame->count; ++at)
                {
                    if (frame->kinds[at] == LepusAbiSlotFloat)
                    {
                        double real = 0;
                        std::memcpy(&real, &frame->slots[at], sizeof(real));
                        values.push_back(NativeValue::fromFloat(real));
                    }
                    else
                        values.push_back(NativeValue::fromInt(frame->slots[at]));
                }
                auto &context = *scope.attached->context;
                runtimeDetail::NativeCallContextImpl call(context, context.invoker);
                const auto returned = entry->callback(call, values);
                if (returned.isFloat())
                    lepus_abi_return_real(frame, returned.floating);
                else if (returned.isText())
                    lepus_abi_return_integer(frame, lepus_abi_string(frame, returned.text.data(), returned.text.size()));
                else
                    lepus_abi_return_integer(frame, returned.integer);
            }
        } catch (const std::exception &failure)
        {
            frame->runtime = scope.attached;
            lepus_abi_mark_failure(frame, failure.what());
        } catch (...)
        {
            frame->runtime = scope.attached;
            lepus_abi_mark_failure(frame, "native entry threw an exception");
        }
    }
    if (frame->runtime != scope.attached || frame->slots != scope.before.slots || frame->kinds != scope.before.kinds || frame->count != scope.before.count)
    {
        frame->runtime = scope.attached;
        lepus_abi_mark_failure(frame, "native entry modified frame inputs");
    }
    if (scope.attached->context && !scope.attached->context->error.empty())
        lepus_abi_mark_failure(frame, scope.attached->context->error.c_str());
    if (scope.attached->context && !scope.attached->context->memoryFault.empty())
        return lepus_abi_status(error, LepusAbiHostFailure, scope.attached->context->memoryFault.c_str());
    if (scope.attached->status != LepusAbiOk)
        return lepus_abi_status(error, scope.attached->status, scope.attached->fallback ? scope.attached->fallback : scope.attached->diagnostic.c_str());
    checked = lepus_abi_check_result(frame, error);
    scope.succeeded = checked == LepusAbiOk;
    return checked;
} catch (...)
{
    if (frame)
    {
        frame->result = 0;
        frame->resultIsFloat = 0;
    }
    return lepus_abi_status(error, LepusAbiHostFailure, "native call failed");
}
