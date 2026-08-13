#include <mutex>
#include <cstdio>

#ifdef HIDDEN
#define API __attribute__((visibility("default")))
#else
#define API
#endif

#ifdef MUTEX_STATIC
static std::mutex g_mutex;
#elif defined(MUTEX_NAMESPACE)
namespace { std::mutex g_mutex; }
#else
std::mutex g_mutex;
#endif

extern "C" API void b_say(const char* msg) {
    std::lock_guard<std::mutex> lk(g_mutex);
    printf("  [B.say] '%s'  uses mutex @ %p\n", msg, (void*)&g_mutex);
}
