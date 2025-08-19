#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module that prints at init");


static char *prompt = "This is default prompt message";
static int number = 77;
module_param(prompt, charp, S_IRUGO);
module_param(number, int,S_IRUGO);

static int numbers[] = {11, 22, 33};
static int size = sizeof(numbers)/sizeof(int);
module_param_array(numbers, int, &size, S_IRUGO);


// Test method: `insmod helloworld_parameter.ko prompt='New message' number=88 numbers=1,2,3`
static int __init simple_init(void)
{
    pr_info("Hello, Kernel World!\n");
    {
        pr_info("  prompt is: %s\n", prompt);
        pr_info("  number is: %d\n", number);
        pr_info("  numbers len size: %d\n", size);
    }
    for (int i = 0; i < size ; i++ ) {
        pr_info("  numbers[%d] is: %d\n", i, numbers[i]);

    }
    return 0;
}

static void __exit simple_exit(void)
{
    pr_info("Goodbye, Kernel World!\n");
}

module_init(simple_init);
module_exit(simple_exit);
