#define pr_fmt(fmt) KBUILD_MODNAME " [%s-%d] " ": "  fmt, __func__, __LINE__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module that prints at init");

static int __init simple_init(void)
{
    pr_info("Hello, Kernel World!\n");
    return 0;
}

static void __exit simple_exit(void)
{
    pr_info("Goodbye, Kernel World!\n");
}

module_init(simple_init);
module_exit(simple_exit);
