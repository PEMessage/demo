#define single_sharp(x) 123 #x 123
#define double_sharp(x) 123 ##x 123
single_sharp(456);  // 123 "456" 123;  // trun into string
double_sharp(456); // 123456 123;   // cat to previous token
double_sharp(456,789); // oversize -> error: macro "double_sharp" passed 2 arguments, but takes just 1


#define cat(x,y) x ## y
// The 'real' cat 2 token
cat(apple,banana); // applebanana
cat(apple,xx banana); // applexx banana
cat(apple, banana);  //applebanana

#define va(...) 123 __VA_ARGS__ 123
va(456,789); //  123 456,789 123;
va(456, 789); // 123 456, 789 123; // extra space


// https://gcc.gnu.org/onlinedocs/gcc/Variadic-Macros.html
#define LOG(m, fmt, ...) (void)LOG_DEMO(LOG_ERROR, LABAL[m], fmt, __VA_ARGS__)
LOG(1, "%s", example_string)
LOG(1, "%s")
// result: __VA_ARGS__ exists
// (void)LOG_DEMO(LOG_ERROR, LABAL[1], "%s", example_string,2,3,4)
// result: __VA_ARGS__ not exists
// (void)LOG_DEMO(LOG_ERROR, LABAL[1], "%s", ) //  FAIL !!!
#define LOG_double_sharp(m, fmt, ...) (void)LOG_DEMO(LOG_ERROR, LABAL[m], fmt, ##__VA_ARGS__)
LOG_double_sharp(1, "%s", example_string)
LOG_double_sharp(1, "%s")
// result: __VA_ARGS__ exists
// (void)LOG_DEMO(LOG_ERROR, LABAL[1], "%s", example_string,2,3,4)
// result: __VA_ARGS__ not exists (will eat ',' before __VA_ARGS__)
// (void)LOG_DEMO(LOG_ERROR, LABAL[1], "%s") // SUCCESS !! 

// Since C++20 we have a better way
#define log(format, ...) printf("LOG: " format __VA_OPT__(,) __VA_ARGS__)

log("%d%f", 1, .2);    // -> printf("LOG: %d%f", 1, .2);
log("hello world");    // -> printf("LOG: hello world");
log("hello world", );  // -> printf("LOG: hello world");


#define REGISTER_SYSTEM_ABILITY_BY_ID(abilityClassName, systemAbilityId, runOnCreate) \
    const bool abilityClassName##_##RegisterResult = \
    SystemAbility::MakeAndRegisterAbility(new abilityClassName(systemAbilityId, runOnCreate));

REGISTER_SYSTEM_ABILITY_BY_ID(TestService, TESTSERVICE_SA_ID, true)
// const bool TestService_RegisterResult = SystemAbility::MakeAndRegisterAbility(new TestService(TESTSERVICE_SA_ID, true));

#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define FILE_LINE __FILE__ ":" STRINGIFY(__LINE__)
FILE_LINE
// 直接使用STR(__LINE__)会得到：
// STR(__LINE__) → "__LINE__"
// 使用STRINGIFY(__LINE__)会：
// STRINGIFY(__LINE__) → STR(123) → "123"  // 假设当前行号是123
void test_func() {
    printf("Hello world :" STR(__func__));
    // printk("Hello world :" "__func__"); // do not met our requirement
    // __func__ is a local variable, not a macro
}


// https://github.com/netcan/recipes/blob/master/cpp/metaproggramming/MetaMacro.hpp
// https://zhuanlan.zhihu.com/p/165993590
// https://pfultz2.com/blog/2012/07/31/reflection-in-under-100-lines/
// 用于C++反射
// DEFINE_STRUCT(Point,
//     (double) x,
//     (double) y)
#define PARE(...) __VA_ARGS__
#define EAT(...)
#define PAIR(x) PARE x // PAIR((int) x) => PARE(int) x => int x
#define STRIP(x) EAT x // STRIP((int) x) => EAT(int) x => x

PAIR((int) x)
STRIP((int) x)



// 预扫描 与 ## 拼接
// See: https://zhuanlan.zhihu.com/p/152354031
//      https://bot-man-jl.github.io/articles/2020/Macro-Programming-Art/macro-meta.cc
//      https://gcc.gnu.org/onlinedocs/cpp/Argument-Prescan.html
// 1. 在进入宏函数前，所有 宏参数 会先进行一次 预扫描 (prescan)，
//    完全展开 未用于 拼接标识符 或 获取字面量 的所有参数
// 2. 在宏函数展开时，用（预扫描展开后的）参数替换 展开目标里的 同名符号
// 3. 在宏函数展开后，替换后的文本会进行 二次扫描(scan twice)，继续展开 结果里出现的宏
#if 1
    #define FOO(SYMBOL) foo_ ## SYMBOL
    #define BAR() bar

    FOO(bar)    // -> foo_bar
    FOO(BAR())  // -> foo_BAR()
    #undef FOO
    #undef BAR
#endif
#if 1
    #define PP_CONCAT(A, B) PP_CONCAT_IMPL(A, B)
    #define PP_CONCAT_IMPL(A, B) A##B

    #define FOO(N) PP_CONCAT(foo_, N)

    FOO(bar)    // -> foo_bar
    FOO(BAR())  // -> foo_bar
#endif
