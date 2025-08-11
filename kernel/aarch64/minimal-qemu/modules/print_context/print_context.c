#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");

static struct timer_list my_timer;

static void print_current_context(void)
{
    if (in_interrupt()) {
        if (in_irq()) {
            printk(KERN_INFO "Context: Hard interrupt\n");
        } else if (in_softirq()) {
            printk(KERN_INFO "Context: Soft interrupt\n");
        }
    } else {
        struct task_struct *task = current;
        printk(KERN_INFO "Context: Process context\n");
        printk(KERN_INFO "  PID: %d\n", task->pid);
        printk(KERN_INFO "  Command: %s\n", task->comm);
    }
}

static void timer_callback(struct timer_list *t)
{
    printk(KERN_INFO "Timer interrupt triggered\n");
    print_current_context();
}

static int __init context_demo_init(void)
{
    printk(KERN_INFO "Context demo module loaded\n");
    
    // 初始化定时器
    timer_setup(&my_timer, timer_callback, 0);
    
    // 设置1秒后触发定时器
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
    
    // 打印初始上下文（进程上下文）
    print_current_context();
    return 0;
}

static void __exit context_demo_exit(void)
{
    del_timer(&my_timer);
    printk(KERN_INFO "Context demo module unloaded\n");
    print_current_context();
}

module_init(context_demo_init);
module_exit(context_demo_exit);
