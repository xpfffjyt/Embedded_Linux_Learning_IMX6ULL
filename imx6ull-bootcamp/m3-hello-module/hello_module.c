/*
 * hello_module.c - M3: your first out-of-tree kernel module.
 *
 * What it demonstrates:
 *   1. module_init / module_exit  (vs. main() in MCU firmware)
 *   2. printk via pr_info         (vs. printf - there is no libc in the kernel)
 *   3. module parameters          (runtime-configurable via /sys/module/...)
 *
 * Build:
 *   make KDIR=$HOME/imx6ull/kernel/linux-imx
 *
 * On the board:
 *   insmod hello_module.ko count=3 who="EBF"
 *   dmesg | tail -5
 *   lsmod | grep hello
 *   cat /sys/module/hello_module/parameters/count
 *   rmmod hello_module
 *
 * Lesson: a module has no main(), no libc, no floating point by default, and
 * crashing it can take down the whole system. That is the price of running in
 * kernel space.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/jiffies.h>

static int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "How many times to print the banner (1..100)");

static char *who = "world";
module_param(who, charp, 0644);
MODULE_PARM_DESC(who, "Name to greet");

static int __init hello_init(void)
{
	int i;

	if (count < 1 || count > 100) {
		pr_err("hello_module: count %d out of range, clamping to 1\n", count);
		count = 1;
	}

	for (i = 0; i < count; i++)
		pr_info("hello_module: hello, %s (%d/%d)\n", who, i + 1, count);

	/* jiffies is the kernel tick counter - the closest thing to SysTick. */
	pr_info("hello_module: loaded, jiffies=%lu, HZ=%d\n", jiffies, HZ);

	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello_module: bye\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");		/* non-GPL taints the kernel and blocks many APIs */
MODULE_AUTHOR("you");
MODULE_DESCRIPTION("First out-of-tree module for the i.MX6ULL board");
MODULE_VERSION("0.1");
