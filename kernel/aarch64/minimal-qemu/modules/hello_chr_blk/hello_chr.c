/*
*  	register_chrdev_region()
*	alloc_chrdev_region()
*	unregister_chrdev_region()
        MAJOR()
        MINOR()
        MKDEV()
*/
#include <linux/module.h>

// #include<linux/types.h>	//dev_t
// #include<linux/kdev_t.h>	//MAJOR/MINOR/MKDEV
#include <linux/fs.h> //注册函数

#define DEFAULT_MAJOR 0
#define DEFAULT_MINOR 0
#define DEFAULT_NR 2

int major = DEFAULT_MAJOR;
int minor = DEFAULT_MINOR;
int nr = DEFAULT_NR;

module_param(major, int, S_IRUGO);
module_param(minor, int, S_IRUGO);
module_param(nr, int, S_IRUGO);

dev_t major_minor; // 高12位是主设备号，低20位是次设备号

// Using `cat /proc/devices` to check
static int __init hello_init(void) {
    int ret;

    pr_info("Enter hello_init\n");

    if (major) {
        major_minor = MKDEV(major, minor);
        ret = register_chrdev_region(major_minor, nr,
                                     "hello_chr"); // 使用指定的设备号分配
    } else {
        ret = alloc_chrdev_region(&major_minor, minor, nr,
                                  "hello_chr"); // 动态分配主设备号
        major = MAJOR(major_minor);
        minor = MINOR(major_minor);
    }
    if (ret < 0) {
        printk(KERN_WARNING "can't get major %d\n", major);
        return ret;
    }

    pr_info("major is: %d\n", major);
    pr_info("minor is: %d\n", minor);
    pr_info("nr is: %d\n", nr);
    pr_info("major_minor is: %d\n", major_minor);
    return 0;
}

static void __exit hello_exit(void) {
    pr_info("Enter hello_exit\n");
    unregister_chrdev_region(major_minor, nr); // 移除模块时释放设备号
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("Dual BSD/GPL"); // 许可 GPL、GPL v2、Dual
                                // MPL/GPL、Proprietary(专有)等,没有内核会提示
MODULE_AUTHOR("Your");
