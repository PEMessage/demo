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
