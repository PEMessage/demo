#include <mutex>
#include <cstdio>

/* Macro-controlled: with -DHIDDEN the lib is compiled with -fvisibility=hidden,
 * and only symbols carrying the API macro are re-exported; g_mutex stays hidden. */
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

extern "C" API void a_say(const char* msg) {
    std::lock_guard<std::mutex> lk(g_mutex); /* internal only; the public API never exposes the mutex */
    printf("  [A.say] '%s'  uses mutex @ %p\n", msg, (void*)&g_mutex);
}
