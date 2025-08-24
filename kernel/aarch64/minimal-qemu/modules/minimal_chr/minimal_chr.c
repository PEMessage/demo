/*
        kzalloc()/kmalloc()
        kfree()
        cdev_init()
        cdev_add()
        cdev_del()
        struct file_operations
*/
// Test:
// insmod minimal_chr.ko
// mknod minimalchr0 c 230 0
// mknod minimalchr1 c 230 1
// cat minimalchr1
// cat minimalchr0
//
#include "linux/printk.h"
#include <linux/cdev.h>
#include <linux/fs.h> //注册函数
#include <linux/module.h>
#include <linux/slab.h>

#define DEFAULT_MAJOR 230
#define DEFAULT_MINOR 0
#define DEFAULT_NR 2
#define DEFAULT_NAME "minimal_chr"

int major = DEFAULT_MAJOR;
int minor = DEFAULT_MINOR;
int nr = DEFAULT_NR;
char *name = DEFAULT_NAME;

bool config_sysfs = true;
bool config_rw = true;
int config_rw_size = 512;

module_param(major, int, S_IRUGO);
module_param(minor, int, S_IRUGO);
module_param(nr, int, S_IRUGO);
module_param(name, charp, S_IRUGO);

module_param(config_sysfs, bool, S_IRUGO);
module_param(config_rw, bool, S_IRUGO);
module_param(config_rw_size, int, S_IRUGO);

dev_t major_minor; // 高12位是主设备号，低20位是次设备号

struct private_dev { // 实际的字符设备结构，类似于面向对象的继承
        // Kernel Object
        struct cdev cdev;

        // Buffer & mutex [optional]
        char *buffer;
        ssize_t size;
        ssize_t max_size;



};

// sysfs
struct class *cls; // add cls

static int do_open(struct inode *inode, struct file *filp) {

    // file->private_data = ...
    struct private_dev *pdev = container_of(inode->i_cdev, struct private_dev, cdev);
    filp->private_data = pdev;

    printk(KERN_INFO "do open: inode is %d, minor is %d\n", iminor(inode),
           MINOR(inode->i_cdev->dev));
    return 0;
}
static ssize_t do_read(struct file *filp, char __user *buf, size_t count,
                       loff_t *f_pos) {
    struct private_dev *pdev = filp->private_data;
    ssize_t bytes_to_read = 0;
    printk(KERN_INFO "do read\n");

    if (!config_rw) {
        return 0;
    }

    // ------ config_rw ------
    // Check if we're at the end of the buffer
    if (*f_pos >= pdev->size) { return 0; } // EOF
    bytes_to_read = min(count, (size_t)(pdev->size - *f_pos));
    if (copy_to_user(buf, pdev->buffer + *f_pos, bytes_to_read)) {
        return -EFAULT; // Failed to copy to userspace
    }
    *f_pos += bytes_to_read;

    /* read will not decrease buffer size(compare to write) */

    return bytes_to_read;
}

static ssize_t do_write(struct file *filp, const char __user *buf, size_t count,
                        loff_t *f_pos) {

    struct private_dev *pdev = filp->private_data;
    ssize_t bytes_to_write = 0;
    printk(KERN_INFO "do write\n");

    if (!config_rw) {
        return count; // 不能返回0，否则会不停的写
    } 

    // ------ config_rw ------
    // Check if we're at the end of the buffer
    if (*f_pos >= pdev->max_size) { return -ENOSPC; } // No space left
    bytes_to_write = min(count, (size_t)(pdev->max_size - *f_pos));
    if (copy_from_user(pdev->buffer + *f_pos, buf, bytes_to_write)) {
        return -EFAULT; // Failed to copy from userspace
    }
    *f_pos += bytes_to_write;

    if (*f_pos > pdev->size) {
        pdev->size = *f_pos;
    }

    return  bytes_to_write;
}

static int do_release(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "do release\n");

    /* struct private_dev *pridev = filp->private_data; */

    return 0;
}

struct file_operations fops = {
    // 字符设备的操作函数
    .owner = THIS_MODULE,
    .read = do_read,
    .write = do_write,
    .open = do_open,
    .release = do_release,
};

struct private_dev *pdev;

static int __init mc_do_init(void) {
    int ret = 0;
    int i = 0;
    int j = 0;
    pr_info("Enter mc_do_init\n");

    // 1. register dev_t
    // --------------------------------------
    // Using `cat /proc/devices` to check

    BUG_ON(!(ret == 0));
    if (major) {
        major_minor = MKDEV(major, minor);
        ret = register_chrdev_region(major_minor, nr,
                                     name); // 使用指定的设备号分配
    } else {
        ret = alloc_chrdev_region(&major_minor, minor, nr,
                                  name); // 动态分配主设备号
        major = MAJOR(major_minor);
        minor = MINOR(major_minor);
    }
    if (ret < 0) {
        printk(KERN_WARNING "can't get major %d\n", major);
        goto step1_fail;
    }

    pr_info("major is: %d\n", major);
    pr_info("minor is: %d\n", minor);
    pr_info("name is: %s\n", name);
    pr_info("nr is: %d\n", nr);
    pr_info("major_minor is: %d\n", major_minor);
    pr_info("config_sysfs is: %d\n", config_sysfs);

    // 2. kzalloc private_dev
    // 2. cdev_init and cdev_add
    // --------------------------------------
    pdev = kzalloc(sizeof(struct private_dev) * nr, GFP_KERNEL);
    if (!pdev) {
        pr_err("Fail to alloc pdev space, nr is %d", nr);
        ret = -ENOMEM;
        goto step2_fail;
    }

    // 3. cdev_init and cdev_add
    // --------------------------------------
    BUG_ON(!(i == 0));
    for (i = 0; i < nr; i++) {
        cdev_init(&pdev[i].cdev, &fops);
        ret = cdev_add(&pdev[i].cdev, MKDEV(major, minor + i), 1);
        // NOTE: How could I error handle ret
        if (ret < 0) {
            pr_err("cdev_add failed for minor %d: %d\n", minor + i, ret);
            goto step3_fail;
        }
    }


    // 4. sysfs[optional]: class_create, device_create
    // using `mdev -s` to refresh and auto gen device node
    // --------------------------------------
    if (config_sysfs) {
        // NOTE !!! if missing a newline character
        // it gets buffered and may not appear immediately in the kernel log
        pr_info("create sysfs...\n");
        cls = class_create(THIS_MODULE, DEFAULT_NAME);
        for (int x=0; x < nr; x++) {
            device_create(cls, NULL, MKDEV(major, minor + x), NULL, DEFAULT_NAME "%d", x);
        }
    } else {
        pr_info("config_sysfs disabled: %d\n", config_sysfs);
    }

    // 5. rw[optional]: 
    // --------------------------------------
    if (config_rw) {
        pr_info("create buffer...\n");
        for (j=0; j < nr; j++) {
            pdev[j].buffer = kmalloc(config_rw_size, GFP_KERNEL);
            if (!pdev[j].buffer) {
                pr_err("Fail to create buffer[%d]\n", j);
                goto step5_fail;
            }
            pdev[j].max_size = config_rw_size;
            pdev[j].size = 0;
        }
    } else {
        pr_info("config_rw disabled: %d\n", config_rw);
    }


/* success: */
    return 0;

step5_fail:
    for(int x = 0; x < j; x++) {
        kfree(pdev[j].buffer);
    }
/* step4_fail: */
    /* pass */
step3_fail:
    // Clean up all cdevs that were successfully added
    for (int x = 0; x < i; x++) {
        cdev_del(&pdev[x].cdev);
    }
    kfree(pdev);
    pdev = NULL;
step2_fail:
    unregister_chrdev_region(major_minor, nr);
step1_fail:
    return ret;
}

static void __exit mc_do_exit(void) {
    pr_info("Enter mc_do_exit\n");

    if(config_sysfs) {
        for (int i = 0; i < nr; i++) {
            device_destroy(cls, MKDEV(major, minor + i));
        }
        class_destroy(cls);
    }

    unregister_chrdev_region(major_minor, nr); // 移除模块时释放设备号
    for (int j = 0; j < nr; j++) {
        cdev_del(&pdev[j].cdev);
    }
    kfree(pdev);
    pdev = NULL;
}

module_init(mc_do_init);
module_exit(mc_do_exit);

MODULE_LICENSE("Dual BSD/GPL"); // 许可 GPL、GPL v2、Dual
                                // MPL/GPL、Proprietary(专有)等,没有内核会提示
MODULE_AUTHOR("Your");
