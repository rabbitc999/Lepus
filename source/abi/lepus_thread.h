/**
 * @file lepus_thread.h
 * @brief 跨平台 C 线程创建与回收接口。
 */
#ifndef LEPUS_THREAD_H
#define LEPUS_THREAD_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct LepusThread LepusThread;
typedef void (*LepusThreadEntry)(void *);
/**
 * @brief 创建执行一次回调的宿主线程，失败时不接管回调参数。
 * @param entry 线程入口，必须在返回前处理自身错误。
 * @param argument 调用方管理的入口参数。
 * @param output 成功时接收独占线程句柄，失败时置空。
 * @return 成功返回零，否则返回平台错误码。
 */
int lepus_thread_create(LepusThreadEntry entry, void *argument, LepusThread **output);
/**
 * @brief 等待线程结束并释放句柄，成功后该句柄失效。
 * @param thread 先前成功创建且尚未等待的线程。
 * @return 成功返回零，失败时保留句柄供调用方处理。
 */
int lepus_thread_join(LepusThread *thread);
#ifdef __cplusplus
}
#endif
#endif
