#ifndef __ASSERT_STATIC_H__
#define __ASSERT_STATIC_H__

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    // C11 or later: use static_assert
    #define ASSERT_STATIC(expr, msg) static_assert(expr, msg)
#elif defined(__GNUC__) || defined(__clang__)
    // GCC or Clang: use _Static_assert
    #define ASSERT_STATIC(expr, msg) _Static_assert(expr, msg)
#else
    // Fallback for older compilers: use a hack with typedef and array size
    #define ASSERT_STATIC(expr, msg) typedef char static_assert_##msg[(expr) ? 1 : -1]
#endif


#endif
