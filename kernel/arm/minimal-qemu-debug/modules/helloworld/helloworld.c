#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module that prints at init");

static int __init simple_init(void)
{
    printk(KERN_INFO "Hello, Kernel World!\n");
    return 0;
}

static void __exit simple_exit(void)
{
    printk(KERN_INFO "Goodbye, Kernel World!\n");
}

module_init(simple_init);
module_exit(simple_exit);
