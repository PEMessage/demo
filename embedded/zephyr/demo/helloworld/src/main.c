#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>

int main(void) {
    while (true) {
        printk("Hello World!\n");
        k_msleep(1000);
    }
    return 0;
}
