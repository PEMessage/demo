#include "hitrace_meter.h"
/**
 * Track the beginning of a context.
 */
void StartTrace(uint64_t tag, const std::string& name, float limit) {}
void StartTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, const char* customArgs) {}
void StartTraceDebug(bool isDebug, uint64_t tag, const std::string& name, float limit) {}
void StartTraceArgs(uint64_t tag, const char* fmt, ...) {}
void StartTraceArgsEx(HiTraceOutputLevel level, uint64_t tag, const char* customArgs, const char* fmt, ...) {}
void StartTraceArgsDebug(bool isDebug, uint64_t tag, const char* fmt, ...) {}
void StartTraceWrapper(uint64_t tag, const char* name) {}

/**
 * Track the end of a context.
 */
void FinishTrace(uint64_t tag) {}
void FinishTraceEx(HiTraceOutputLevel level, uint64_t tag) {}
void FinishTraceDebug(bool isDebug, uint64_t tag) {}

/**
 * Track the beginning of an asynchronous event.
 */
void StartAsyncTrace(uint64_t tag, const std::string& name, int32_t taskId, float limit) {}
void StartAsyncTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, int32_t taskId,
    const char* customCategory, const char* customArgs) {}
void StartAsyncTraceDebug(bool isDebug, uint64_t tag, const std::string& name, int32_t taskId, float limit) {}
void StartAsyncTraceArgs(uint64_t tag, int32_t taskId, const char* fmt, ...) {}
void StartAsyncTraceArgsEx(HiTraceOutputLevel level, uint64_t tag, int32_t taskId,
    const char* customCategory, const char* customArgs, const char* fmt, ...) {}
void StartAsyncTraceArgsDebug(bool isDebug, uint64_t tag, int32_t taskId, const char* fmt, ...) {}
void StartAsyncTraceWrapper(uint64_t tag, const char* name, int32_t taskId) {}

/**
 * Track the beginning of an hitrace chain event.
 */
struct HiTraceIdStruct;
void StartTraceChain(uint64_t tag, const struct HiTraceIdStruct* hiTraceId, const char* name) {}

/**
 * Track the end of an asynchronous event.
 */
void FinishAsyncTrace(uint64_t tag, const std::string& name, int32_t taskId) {}
void FinishAsyncTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, int32_t taskId) {}
void FinishAsyncTraceDebug(bool isDebug, uint64_t tag, const std::string& name, int32_t taskId) {}
void FinishAsyncTraceArgs(uint64_t tag, int32_t taskId, const char* fmt, ...) {}
void FinishAsyncTraceArgsEx(HiTraceOutputLevel level, uint64_t tag, int32_t taskId, const char* fmt, ...) {}
void FinishAsyncTraceArgsDebug(bool isDebug, uint64_t tag, int32_t taskId, const char* fmt, ...) {}
void FinishAsyncTraceWrapper(uint64_t tag, const char* name, int32_t taskId) {}

/**
 * Track the middle of a context. Match the previous function of StartTrace before it.
 */
void MiddleTrace(uint64_t tag, const std::string& beforeValue, const std::string& afterValue) {}
void MiddleTraceDebug(bool isDebug, uint64_t tag, const std::string& beforeValue, const std::string& afterValue) {}

/**
 * Track the 64-bit integer counter value.
 */
void CountTrace(uint64_t tag, const std::string& name, int64_t count) {}
void CountTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, int64_t count) {}
void CountTraceDebug(bool isDebug, uint64_t tag, const std::string& name, int64_t count) {}
void CountTraceWrapper(uint64_t tag, const char* name, int64_t count) {}

bool IsTagEnabled(uint64_t tag) { return false; }
