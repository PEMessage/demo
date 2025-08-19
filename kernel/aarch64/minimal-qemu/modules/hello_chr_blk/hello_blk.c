/*
*	register_blkdev()
*	unregister_blkdev()
*/

#include "linux/printk.h"
#include <linux/module.h>

#include<linux/module.h>

//#include<linux/types.h>	//dev_t
//#include<linux/kdev_t.h>	//MAJOR/MINOR/MKDEV
#include<linux/fs.h>		//注册函数
#include <linux/blkdev.h>

#define DEFAULT_MAJOR 230

int major = DEFAULT_MAJOR;

module_param(major, int, S_IRUGO);


// Using `cat /proc/devices` to check
static int __init hello_init(void) {
    int ret;

    pr_info("Enter hello_init\n");

    ret = register_blkdev(major,"hello_blk");

    if (ret < 0) {
        pr_warn("can't get major %d\n", major);
        return ret;
    }

    pr_info("major is: %d\n", major);
    return 0;
}

static void __exit hello_exit(void) {
    pr_info("Enter hello_exit\n");
    unregister_blkdev(major, "hello_blk");	//移除模块时释放设备号
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("Dual BSD/GPL"); // 许可 GPL、GPL v2、Dual
                                // MPL/GPL、Proprietary(专有)等,没有内核会提示
MODULE_AUTHOR("Your");
