#include "template_binding.hpp"
#include "../lepus_ast.hpp"
#include "../literals/string_escape.hpp"
#include <stdexcept>

namespace lepus_macro
{

/**
 * @brief 解析并核对一个宏重复的片段变量及等长来源。
 * @param nodes AST 节点池。
 * @param node 单来源或成组重复节点。
 * @param inputs 当前层可见的宏实参。
 * @param variables 接收迭代变量名。
 * @param arguments 接收各来源的片段编号。
 * @param error 接收来源或形状错误。
 * @return 各来源均可迭代时返回真。
 */
bool ReadMacroRepetition(const Memory::Deque<MNode> &nodes, const MNode &node, const std::map<std::string, int> &inputs,
                         std::vector<std::string> &variables, std::vector<const Memory::Vector<int> *> &arguments, std::string &error)
{
    std::vector<std::string> sources;
    variables.clear();
    arguments.clear();
    if (node.kids.size() == 3)
    {
        for (const int variable : nodes[node.kids[1]].kids) variables.emplace_back(nodes[variable].s.data(), nodes[variable].s.size());
        for (const int source : nodes[node.kids[2]].kids) sources.emplace_back(nodes[source].s.data(), nodes[source].s.size());
    }
    else
    {
        sources.emplace_back(node.s2.data(), node.s2.size());
        variables.emplace_back(node.s.data(), node.s.size());
    }
    if (variables.size() != sources.size() || variables.empty()) return error = "宏成组重复的变量与来源数量必须一致", false;
    std::set<std::string> unique;
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        if (!unique.insert(variables[index]).second) return error = "宏成组重复的变量不能重名", false;
        const auto &source = sources[index];
        if (!inputs.count(source) || nodes[inputs.at(source)].kind != MNode::MacroVariadic)
            return error = "宏循环必须遍历变长片段参数：" + source, false;
        arguments.push_back(&nodes[inputs.at(source)].kids);
        if (arguments.back()->size() != arguments.front()->size()) return error = "宏成组重复的来源长度必须一致", false;
    }
    return true;
}

/**
 * @brief 根据片段类别或原文选择宏分支。
 * @param nodes AST 节点池。
 * @param node 匹配或正则分支节点。
 * @param inputs 当前层可见的宏实参。
 * @param matcher 用于正则匹配的缓存。
 * @param error 接收无效形参或模式错误。
 * @return 选中的模板编号，无匹配时为负数。
 */
int SelectMacroBranch(const Memory::Deque<MNode> &nodes, const MNode &node, const std::map<std::string, int> &inputs,
                      RegexMatcher *matcher, std::string &error)
{
    const auto name = std::string(node.s.data(), node.s.size());
    if (!inputs.count(name)) return error = "宏编译期分支引用了不存在的形参：" + name, -1;
    const auto &argument = nodes[inputs.at(name)];
    int selected = -1;
    if (node.kind == MNode::MacroMatch)
    {
        for (const int branchId : nodes[node.kids[0]].kids)
        {
            const auto &branch = nodes[branchId];
            int category = branch.s == "expr" ? 0 : branch.s == "ident" ? 1 :
                                                branch.s == "type"      ? 2 :
                                                branch.s == "block"     ? 3 :
                                                branch.s == "stmt"      ? 4 :
                                                branch.s == "pattern"   ? 5 :
                                                                          -1;
            if (category < 0 || (argument.kind == MNode::MacroArgument && argument.kids.size() == lepus_macro::FragmentAlternativeCount && argument.kids[category] >= 0))
            {
                selected = branch.kids[0];
                break;
            }
        }
    }
    else
    {
        if (!matcher || argument.kind != MNode::MacroArgument) return error = "宏正则匹配缺少原始实参或模式缓存", -1;
        std::string pattern;
        if (!lepus_literal::decodeString(std::string_view(node.s2.data(), node.s2.size()), pattern, error)) return -1;
        const auto result = matcher->match(std::string_view(argument.s.data(), argument.s.size()), pattern, error);
        if (result == RegexResult::Invalid) return error = "宏正则模式非法：" + error, -1;
        selected = node.kids[result == RegexResult::Matched ? 0 : 1];
    }
    return selected;
}

/**
 * @brief 查找需要编译期局部绑定准备的模板。
 * @param nodes 语法节点池。
 * @param root 模板子树。
 * @param depth 当前深度。
 * @return 子树中有宏常量声明时返回真。
 */
bool ContainsMacroLet(const Memory::Deque<MNode> &nodes, int root, std::size_t depth)
{
    if (root < 0 || depth >= 256) return false;
    const auto &node = nodes[root];
    if (node.kind == MNode::MacroLet) return true;
    if ((node.kind == MNode::Block || node.kind == MNode::Case) && ContainsMacroLet(nodes, static_cast<int>(node.i), depth + 1))
        return true;
    return std::any_of(node.kids.begin(), node.kids.end(), [&](int child) { return ContainsMacroLet(nodes, child, depth + 1); });
}

/**
 * @brief 保存节点池及卫生解析器的引用。
 * @param nodes 编译器节点池。
 * @param resolver 本次编译共用的卫生解析器。
 * @param types 可选的当前类型池。
 */
TemplateBinding::TemplateBinding(Memory::Deque<MNode> &nodes, HygieneResolver &resolver, Memory::Vector<std::shared_ptr<MType>> *types) : nodes_(nodes), resolver_(resolver), types_(types) {}

/**
 * @brief 克隆宏体并建立词法绑定，错误时回滚克隆节点。
 * @param definition 宏定义编号。
 * @param definitionScope 宏定义所在作用域。
 * @param result 接收独立模板及形参映射。
 * @param error 接收错误文本。
 * @param inputs 本次调用的形参到实参映射。
 * @param matcher 共享正则缓存。
 * @param prepare 已知实参与具体类型后处理元数据控制的回调。
 * @param bindBody 是否处理宏体。
 * @return 成功返回真。
 */
bool TemplateBinding::bind(int definition, ScopeId definitionScope, BoundTemplate &result, std::string &error,
                           const std::map<std::string, int> *inputs, RegexMatcher *matcher, bool bindBody,
                           const std::function<bool(int, const std::map<std::string, int> &, std::string &)> &prepare)
{
    const auto start = nodes_.size();
    const auto typeStart = types_ ? types_->size() : 0;
    result = {};
    error.clear();
    parameters_.clear();
    typeDeclarations_.clear();
    memberDeclarations_.clear();
    patternParameters_.clear();
    inputs_ = inputs;
    result_ = &result;
    iterationInputs_ = inputs ? *inputs : std::map<std::string, int>{};
    if (inputs) inputs_ = &iterationInputs_;
    matcher_ = matcher;
    prepare_ = prepare;
    try
    {
        if (definition < 0 || static_cast<std::size_t>(definition) >= start || nodes_[definition].kind != MNode::MacroDefinition || nodes_[definition].kids.size() != 3)
            throw std::runtime_error("宏定义结构无效");
        const auto definitionNode = nodes_[definition];
        const auto scope = resolver_.createScope(definitionScope);
        parameterScope_ = scope;
        const int group = definitionNode.kids[0];
        if (group >= 0)
        {
            if (static_cast<std::size_t>(group) >= start || nodes_[group].kind != MNode::MacroParameters)
                throw std::runtime_error("宏参数组结构无效");
            Memory::Vector<int> groups = { group };
            if (!nodes_[group].kids.empty() && nodes_[nodes_[group].kids[0]].kind == MNode::MacroParameters) groups = nodes_[group].kids;
            for (const int parameterGroup : groups)
                for (const int parameter : nodes_[parameterGroup].kids)
                {
                    if (parameter < 0 || static_cast<std::size_t>(parameter) >= start || nodes_[parameter].kind != MNode::MacroParameter)
                        throw std::runtime_error("宏形参结构无效");
                    const auto &name = nodes_[parameter].s;
                    const auto identity = resolver_.declare(std::string_view(name.data(), name.size()), scope, definitionNode.syntaxContext);
                    if (!identity) throw std::runtime_error("宏形参重复：" + std::string(name.data(), name.size()));
                    result.parameters.emplace(std::string(name.data(), name.size()), identity);
                    parameters_.insert(identity);
                    if (nodes_[parameter].s2 == "pattern") patternParameters_.insert(identity);
                }
        }
        if (!bindBody)
        {
            result.root = definitionNode.kids[1];
            return true;
        }
        AstSubstitution substitution(nodes_, 1000000, types_);
        result.root = substitution.instantiate(definitionNode.kids[1], {}, definitionNode.syntaxContext, error);
        if (result.root < 0) throw std::runtime_error(error);
        visit(result.root, resolver_.createScope(scope), 0);
        return true;
    } catch (const std::exception &failure)
    {
        nodes_.resize(start);
        if (types_) types_->resize(typeStart);
        result = {};
        error = failure.what();
        return false;
    }
}

/**
 * @brief 登记声明身份，保留用户名称用于诊断。
 * @param id 声明节点。
 * @param scope 词法作用域。
 */
void TemplateBinding::declare(int id, ScopeId scope)
{
    auto &node = nodes_[id];
    const auto parameter = resolver_.resolve(std::string_view(node.s.data(), node.s.size()), scope, node.syntaxContext);
    if (parameters_.count(parameter)) node.nameParameter = parameter;
    node.bindingIdentity = resolver_.declare(std::string_view(node.s.data(), node.s.size()), scope, node.syntaxContext);
    if (!node.bindingIdentity) throw std::runtime_error("宏体同一作用域重复声明：" + std::string(node.s.data(), node.s.size()));
}

/**
 * @brief 遍历模式中的绑定叶子，不将构造器或标量模式名称误当声明。
 * @param id 模式编号。
 * @param scope 分支或声明所在作用域。
 * @param depth 当前模式深度。
 */
void TemplateBinding::bindPattern(int id, ScopeId scope, std::size_t depth)
{
    if (id < 0) return;
    if (depth >= 256) throw std::runtime_error("宏模式绑定超过语法深度预算");
    auto &node = nodes_[id];
    if (node.kind == MNode::Var)
    {
        const auto parameter = resolver_.resolve(std::string_view(node.s.data(), node.s.size()), scope, node.syntaxContext);
        if (patternParameters_.count(parameter))
        {
            node.kind = MNode::MacroHole;
            node.bindingIdentity = parameter;
        }
        else if (node.s != "_")
            declare(id, scope);
        return;
    }
    if (node.kind == MNode::List || node.kind == MNode::Tuple)
        for (const int child : node.kids) bindPattern(child, scope, depth + 1);
    else
        visit(id, scope, depth + 1);
}

/**
 * @brief 根据 AST 的真实结构边绑定形参引用和局部变量。
 * @param id 当前节点。
 * @param scope 当前作用域。
 * @param depth 遍历深度。
 */
void TemplateBinding::visit(int id, ScopeId scope, std::size_t depth)
{
    if (id < 0) return;
    if (static_cast<std::size_t>(id) >= nodes_.size() || depth >= 256) throw std::runtime_error("宏绑定遇到无效或过深的语法节点");
    auto &node = nodes_[id];
    if (node.kind == MNode::MacroDefinition) throw std::runtime_error("嵌套宏定义尚未接入模板绑定");
    if (node.kind == MNode::MacroTemplate && node.s2 == "__lepus_field_iteration") scope = resolver_.createScope(scope);
    if (node.kind == MNode::MacroTemplate && node.kids.size() == 2)
    {
        std::map<Memory::String, int> declarations;
        const auto children = nodes_[node.kids[0]].kids;
        for (const int child : children)
        {
            auto &entry = nodes_[child];
            const bool associated = entry.kind == MNode::AssociatedDecl || entry.kind == MNode::AssociatedDef;
            const bool member = associated || entry.kind == MNode::Field || entry.kind == MNode::MethodDef;
            if (!member && entry.kind != MNode::FuncDef && entry.kind != MNode::StructDecl && entry.kind != MNode::EnumDecl && entry.kind != MNode::InterfaceDecl) continue;
            if (entry.bindingIdentity) continue;
            const auto previous = declarations.find(entry.s);
            if (previous != declarations.end() && (entry.kind == MNode::FuncDef || entry.kind == MNode::MethodDef) && nodes_[previous->second].kind == entry.kind)
            {
                entry.bindingIdentity = nodes_[previous->second].bindingIdentity;
                entry.nameParameter = nodes_[previous->second].nameParameter;
            }
            else
            {
                declare(child, scope);
                declarations.emplace(entry.s, child);
            }
            if (member) memberDeclarations_.insert(entry.bindingIdentity);
            else if (entry.kind != MNode::FuncDef) typeDeclarations_.insert(entry.bindingIdentity);
            if (associated) typeDeclarations_.insert(entry.bindingIdentity);
        }
    }

    if (node.kind == MNode::MacroFieldFor || node.kind == MNode::MacroCompileIf)
    {
        std::string error;
        // 重复体的控制条件在绑定期间才准备；保留外层局部身份，不能误读同名全局常量。
        std::function<void(int, std::size_t)> bindConstants = [&](int current, std::size_t level) {
            if (current < 0) return;
            if (level >= 256) throw std::runtime_error("编译期局部常量绑定超过深度预算");
            auto &value = nodes_[current];
            if (value.kind == MNode::Var)
            {
                const auto identity = resolver_.resolve(std::string_view(value.s.data(), value.s.size()), scope, value.syntaxContext);
                if (identity && !parameters_.count(identity)) value.bindingIdentity = identity;
            }
            for (const int child : value.kids) bindConstants(child, level + 1);
        };
        if (!node.kids.empty()) bindConstants(node.kids[0], depth + 1);
        if (!prepare_ || !inputs_ || !prepare_(id, *inputs_, error)) throw std::runtime_error(error.empty() ? "编译期控制缺少类型化准备器" : error);
        visit(id, scope, depth + 1);
        return;
    }
    if (node.kind == MNode::MacroFor)
    {
        std::vector<std::string> variables;
        std::vector<const Memory::Vector<int> *> arguments;
        std::string error;
        if (!inputs_ || !ReadMacroRepetition(nodes_, node, *inputs_, variables, arguments, error))
            throw std::runtime_error(error.empty() ? "宏循环缺少实参" : error);
        const int body = node.kids[0];
        MNode list;
        list.kind = MNode::List;
        const int items = static_cast<int>(nodes_.size());
        nodes_.push_back(std::move(list));
        // 准备器已选择并执行过这一轮，只补齐片段身份与卫生作用域。
        const bool prepared = node.i > 0;
        const bool prepareIteration = !prepared && ContainsMacroLet(nodes_, body);
        const std::size_t first = prepared ? static_cast<std::size_t>(node.i - 1) : 0;
        const std::size_t end = prepared ? first + 1 : arguments.front()->size();
        if (end > arguments.front()->size()) throw std::runtime_error("宏循环迭代超出来源范围");
        const auto previous = iterationInputs_;
        for (std::size_t item = first; item < end; ++item)
        {
            const auto iterationScope = resolver_.createScope(scope);
            for (std::size_t index = 0; index < variables.size(); ++index)
            {
                const auto &variable = variables[index];
                const int argument = (*arguments[index])[item];
                const auto identity = resolver_.declare(variable, iterationScope, node.syntaxContext);
                parameters_.insert(identity);
                if (nodes_[argument].kind == MNode::MacroArgument && nodes_[argument].kids.size() == lepus_macro::FragmentAlternativeCount && nodes_[argument].kids[5] >= 0 && nodes_[argument].kids[0] < 0)
                    patternParameters_.insert(identity);
                result_->iterations.emplace(identity, argument);
                iterationInputs_[variable] = argument;
            }
            AstSubstitution copy(nodes_, 1000000, types_);
            std::string error;
            const int expanded = prepared ? body : copy.instantiate(body, {}, node.syntaxContext, error);
            if (expanded < 0) throw std::runtime_error(error);
            if (prepareIteration && (!prepare_ || !prepare_(expanded, iterationInputs_, error)))
                throw std::runtime_error(error.empty() ? "$let 缺少编译期准备器" : error);
            visit(expanded, resolver_.createScope(iterationScope), depth + 1);
            nodes_[items].kids.push_back(expanded);
        }
        iterationInputs_ = previous;
        node.kind = MNode::MacroTemplate;
        node.kids = { items, -1 };
        node.s.clear();
        node.s2.clear();
        node.i = 0;
        return;
    }
    if (node.kind == MNode::MacroMatch || node.kind == MNode::MacroRegex)
    {
        if (!inputs_) throw std::runtime_error("宏编译期分支缺少实参");
        std::string error;
        const int selected = SelectMacroBranch(nodes_, node, *inputs_, matcher_, error);
        if (!error.empty()) throw std::runtime_error(error);
        if (selected >= 0)
        {
            std::string error;
            if (ContainsMacroLet(nodes_, selected) && (!prepare_ || !prepare_(selected, iterationInputs_, error)))
                throw std::runtime_error(error.empty() ? "$let 缺少编译期准备器" : error);
            node = nodes_[selected];
        }
        else
        {
            const int empty = static_cast<int>(nodes_.size());
            nodes_.push_back(MNode{});
            node.kind = MNode::MacroTemplate;
            node.kids = { empty, -1 };
            node.s.clear();
            node.s2.clear();
        }
    }
    if (types_ && node.type >= 0)
    {
        auto type = bindType(types_->at(node.type), scope, node.syntaxContext, 0);
        node.type = static_cast<int>(types_->size());
        types_->push_back(std::move(type));
    }
    if (node.kind == MNode::MacroArgument && node.kids.size() == lepus_macro::FragmentAlternativeCount && node.kids[1] >= 0)
    {
        const auto &identifier = nodes_[node.kids[1]];
        const auto identity = resolver_.resolve(std::string_view(identifier.s.data(), identifier.s.size()), scope, node.syntaxContext);
        if (parameters_.count(identity)) node.bindingIdentity = identity;
    }
    if (node.kind == MNode::Var)
    {
        node.bindingIdentity = resolver_.resolve(std::string_view(node.s.data(), node.s.size()), scope, node.syntaxContext);
        if (parameters_.count(node.bindingIdentity)) node.kind = MNode::MacroHole;
        return;
    }
    if (node.kind == MNode::EnumCtor && types_)
    {
        const auto identity = resolver_.resolve(std::string_view(node.s.data(), node.s.size()), scope, node.syntaxContext);
        if (parameters_.count(identity) || typeDeclarations_.count(identity))
        {
            auto type = Memory::makeShared<MType>();
            type->kind = MType::Struct;
            type->name = node.s;
            type->macroBinding = identity;
            node.type = static_cast<int>(types_->size());
            types_->push_back(std::move(type));
            node.kind = MNode::TypeQualifiedRef;
            node.s.clear();
        }
    }
    if ((node.kind == MNode::Member || node.kind == MNode::Arrow || node.kind == MNode::EnumCtor || node.kind == MNode::TypeQualifiedRef) && node.s2.rfind("__lepus_metadata_member:", 0) != 0)
    {
        const auto &name = node.kind == MNode::EnumCtor || node.kind == MNode::TypeQualifiedRef ? node.s2 : node.s;
        const auto identity = resolver_.resolve(std::string_view(name.data(), name.size()), scope, node.syntaxContext);
        if (parameters_.count(identity)) node.nameParameter = identity;
        else if (memberDeclarations_.count(identity)) node.bindingIdentity = identity;
    }
    if (node.kind == MNode::Block)
    {
        visit(static_cast<int>(node.i), resolver_.createScope(scope), depth + 1);
        return;
    }
    if (node.kind == MNode::FuncDef || node.kind == MNode::MethodDef || node.kind == MNode::StructDecl || node.kind == MNode::EnumDecl || node.kind == MNode::InterfaceDecl || node.kind == MNode::For || node.kind == MNode::GenericLambda)
        scope = resolver_.createScope(scope);
    if (node.kind == MNode::Destructure && !node.i)
    {
        visit(node.kids[1], scope, depth + 1);
        bindPattern(node.kids[0], scope, depth + 1);
        return;
    }
    if (node.kind == MNode::Case)
    {
        scope = resolver_.createScope(scope);
        if (node.i >= 0)
        {
            auto &pattern = nodes_[static_cast<int>(node.i)];
            if (pattern.kind == MNode::EnumCtor || pattern.kind == MNode::TypeQualifiedRef)
            {
                const auto payloads = pattern.kids;
                pattern.kids.clear();
                visit(static_cast<int>(node.i), scope, depth + 1);
                pattern.kids = payloads;
                for (const int payload : payloads) bindPattern(payload, scope, depth + 1);
            }
            else if (pattern.kind == MNode::Call && pattern.kids.size() == 2)
            {
                visit(pattern.kids[0], scope, depth + 1);
                bindPattern(pattern.kids[1], scope, depth + 1);
            }
            else
                visit(static_cast<int>(node.i), scope, depth + 1);
        }
        visit(node.guard, scope, depth + 1);
        for (const int child : node.kids) visit(child, scope, depth + 1);
        return;
    }
    if (node.kind == MNode::Foreach)
    {
        if (node.kids.size() < 2u || node.kids.size() > 3u) throw std::runtime_error("宏体遍历循环结构无效");
        visit(node.kids[0], scope, depth + 1);
        scope = resolver_.createScope(scope);
        declare(id, scope);
        visit(node.kids[1], scope, depth + 1);
        if (node.kids.size() > 2u) visit(node.kids[2], scope, depth + 1);
        return;
    }
    const bool declarationGroup = node.kind == MNode::Declare && node.kids.size() == 1 && node.kids[0] >= 0 && nodes_[node.kids[0]].kind == MNode::List;
    for (const int child : node.kids) visit(child, scope, depth + 1);
    if (node.kind == MNode::GenericLambda && node.type2 >= 0) visit(node.type2, scope, depth + 1);
    for (const int annotation : node.annotations) visit(annotation, scope, depth + 1);
    visit(node.guard, scope, depth + 1);
    const bool indexedChild = node.kind == MNode::Case || node.kind == MNode::NewPtr || ((node.kind == MNode::Declare || node.kind == MNode::Field) && !declarationGroup && node.i >= 0) || (node.kind == MNode::Index && node.s2 == "__lepus_bare_index");
    if (indexedChild) visit(static_cast<int>(node.i), scope, depth + 1);
    if ((node.kind == MNode::Declare && !declarationGroup) || node.kind == MNode::Params) declare(id, scope);
}

/**
 * @brief 标记类型语法中引用的宏形参，类型参数与表达式参数仍共用同一声明身份。
 * @param type 原始类型描述。
 * @param scope 当前词法作用域。
 * @param context 当前语法上下文。
 * @param depth 类型递归深度。
 * @return 独立的已绑定类型描述。
 */
std::shared_ptr<MType> TemplateBinding::bindType(const std::shared_ptr<MType> &type, ScopeId scope, ContextId context, std::size_t depth)
{
    if (!type) return nullptr;
    if (depth >= 256) throw std::runtime_error("宏类型绑定超过类型深度预算");
    auto result = Memory::makeShared<MType>(*type);
    if (type->kind == MType::Struct && type->genericBase.empty())
    {
        const auto identity = resolver_.resolve(std::string_view(type->name.data(), type->name.size()), scope, context);
        if (parameters_.count(identity) || typeDeclarations_.count(identity))
            result->macroBinding = identity;
        else
        {
            const auto split = type->name.find("::");
            if (split != Memory::String::npos)
            {
                const auto parameter = resolver_.resolve(std::string_view(type->name.data(), split), scope, context);
                if (parameters_.count(parameter))
                {
                    result->macroBinding = parameter;
                    result->genericMember = type->name.substr(split + 2);
                }
            }
        }
    }
    result->elem = bindType(type->elem, scope, context, depth + 1);
    result->elem2 = bindType(type->elem2, scope, context, depth + 1);
    for (auto &parameter : result->paramTypes) parameter = bindType(parameter, scope, context, depth + 1);
    for (auto &argument : result->genericArguments) argument = bindType(argument, scope, context, depth + 1);
    return result;
}
}  // namespace lepus_macro
