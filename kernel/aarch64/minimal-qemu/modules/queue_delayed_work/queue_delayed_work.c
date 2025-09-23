#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#define DELAY_MS 2000  // 2 second delay

static struct workqueue_struct *my_wq;
static struct delayed_work my_delayed_work;

// Work function that will be executed after the delay
static void my_delayed_work_fn(struct work_struct *work)
{
    pr_info("Delayed work executed after %d ms\n", DELAY_MS);
}

static int __init my_module_init(void)
{
    pr_info("Delayed work module loaded\n");
    
    // Create a workqueue
    // See: https://docs.kernel.org/core-api/workqueue.html
    //     While the combination of @max_active of 1 and WQ_UNBOUND used to achieve this behavior,
    //     this is no longer the case. Use alloc_ordered_workqueue() instead.
    // my_wq = create_singlethread_workqueue("my_delayed_workqueue");
    my_wq = alloc_ordered_workqueue("my_delayed_workqueue", 0 /*extra flag for alloc_workqueue, not used set to 0*/);

    if (!my_wq) {
        pr_err("Failed to create workqueue\n");
        return -ENOMEM;
    }
    
    // Initialize the delayed work
    INIT_DELAYED_WORK(&my_delayed_work, my_delayed_work_fn);
    
    // Queue the work with a delay
    queue_delayed_work(my_wq, &my_delayed_work, msecs_to_jiffies(DELAY_MS));
    
    pr_info("Delayed work queued to run in %d ms\n", DELAY_MS);
    return 0;
}

static void __exit my_module_exit(void)
{
    // Cancel any pending work before exiting
    cancel_delayed_work(&my_delayed_work);
    
    // Flush and destroy the workqueue
    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);
    
    pr_info("Delayed work module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple demo of queue_delayed_work");
