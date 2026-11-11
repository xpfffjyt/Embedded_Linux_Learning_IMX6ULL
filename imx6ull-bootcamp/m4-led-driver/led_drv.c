/*
 * ebf_led.c - M4: a GPIO LED character driver driven by the device tree.
 *
 * This is the smallest driver that teaches the three things you must internalise
 * when coming from MCU land:
 *
 *   1. HARDWARE IS DESCRIBED IN THE DEVICE TREE, NOT IN C.
 *      The pin number, mux mode and polarity live in the .dts. The driver only
 *      asks for "the GPIO of this device" via devm_gpiod_get().
 *
 *   2. THE KERNEL MATCHES DRIVERS BY 'compatible'.
 *      If the string here does not match the dts byte for byte, probe() is
 *      never called and /dev/ebf_led never appears. Check with:
 *          dmesg | grep ebf
 *
 *   3. YOU MUST EXPOSE A FILE INTERFACE.
 *      A char device + file_operations + cdev + class/device_create is the
 *      classic path from "kernel object" to "/dev/ebf_led you can echo into".
 *
 * Build: make KDIR=$HOME/imx6ull/kernel/linux-imx
 *
 * On the board:
 *   insmod led_drv.ko
 *   ls -l /dev/ebf_led
 *   echo 1 > /dev/ebf_led      # LED on
 *   echo 0 > /dev/ebf_led      # LED off
 *   cat /dev/ebf_led           # read back 0/1
 *   rmmod led_drv
 *
 * NOTE: teaching code. The single global instance, missing locking for
 * concurrent writers, and the dangling file->private_data after rmmod are all
 * deliberate - see the exercises in the roadmap (mutex, poll, sysfs, input).
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>

#define DEV_NAME	"ebf_led"

struct ebf_led_dev {
	struct device		*dev;
	struct cdev		cdev;
	dev_t			devno;
	struct class		*cls;
	struct gpio_desc	*gpiod;
};

static struct ebf_led_dev *led_dev;

static int ebf_led_open(struct inode *inode, struct file *file)
{
	file->private_data = led_dev;
	return 0;
}

static ssize_t ebf_led_read(struct file *file, char __user *buf,
			    size_t count, loff_t *ppos)
{
	struct ebf_led_dev *d = file->private_data;
	char kbuf[4];
	int val, n;

	val = gpiod_get_value(d->gpiod);	/* 0 or 1 */
	n = scnprintf(kbuf, sizeof(kbuf), "%d\n", val ? 1 : 0);

	if (*ppos >= n)
		return 0;

	if (count > (size_t)(n - *ppos))
		count = (size_t)(n - *ppos);

	if (copy_to_user(buf, kbuf + *ppos, count))
		return -EFAULT;

	*ppos += count;
	return count;
}

static ssize_t ebf_led_write(struct file *file, const char __user *buf,
			     size_t count, loff_t *ppos)
{
	struct ebf_led_dev *d = file->private_data;
	char kbuf[8];

	if (count == 0)
		return 0;

	if (count > sizeof(kbuf) - 1)
		count = sizeof(kbuf) - 1;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	kbuf[count] = '\0';

	if (kbuf[0] == '1') {
		gpiod_set_value(d->gpiod, 1);
		dev_info(d->dev, "LED on\n");
	} else if (kbuf[0] == '0') {
		gpiod_set_value(d->gpiod, 0);
		dev_info(d->dev, "LED off\n");
	} else {
		return -EINVAL;
	}

	return count;
}

static const struct file_operations ebf_led_fops = {
	.owner		= THIS_MODULE,
	.open		= ebf_led_open,
	.read		= ebf_led_read,
	.write		= ebf_led_write,
	.llseek		= default_llseek,
};

static int ebf_led_probe(struct platform_device *pdev)
{
	struct ebf_led_dev *d;
	int ret;

	d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	d->dev = &pdev->dev;

	/*
	 * con_id == NULL -> the DT property named "gpios".
	 * GPIOD_OUT_LOW -> initialise to logical 0 (LED off, polarity handled
	 * by GPIO_ACTIVE_LOW in the dts - you never touch the raw level).
	 */
	d->gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
	if (IS_ERR(d->gpiod)) {
		ret = PTR_ERR(d->gpiod);
		dev_err(&pdev->dev, "failed to get gpios: %d\n", ret);
		return ret;
	}

	ret = alloc_chrdev_region(&d->devno, 0, 1, DEV_NAME);
	if (ret < 0) {
		dev_err(&pdev->dev, "alloc_chrdev_region failed: %d\n", ret);
		return ret;
	}

	cdev_init(&d->cdev, &ebf_led_fops);
	d->cdev.owner = THIS_MODULE;

	ret = cdev_add(&d->cdev, d->devno, 1);
	if (ret < 0) {
		dev_err(&pdev->dev, "cdev_add failed: %d\n", ret);
		goto err_region;
	}

	d->cls = class_create(THIS_MODULE, DEV_NAME);
	if (IS_ERR(d->cls)) {
		ret = PTR_ERR(d->cls);
		goto err_cdev;
	}

	/* udev/mdev creates /dev/ebf_led from this. */
	if (!device_create(d->cls, &pdev->dev, d->devno, d, DEV_NAME)) {
		ret = -EINVAL;
		goto err_class;
	}

	led_dev = d;
	platform_set_drvdata(pdev, d);

	dev_info(&pdev->dev, "registered, major=%d minor=%d, /dev/%s ready\n",
		 MAJOR(d->devno), MINOR(d->devno), DEV_NAME);

	return 0;

err_class:
	class_destroy(d->cls);
err_cdev:
	cdev_del(&d->cdev);
err_region:
	unregister_chrdev_region(d->devno, 1);
	return ret;
}

static int ebf_led_remove(struct platform_device *pdev)
{
	struct ebf_led_dev *d = platform_get_drvdata(pdev);

	if (!d)
		return 0;

	/* devm_* resources (gpiod, kzalloc) are released automatically after
	 * this returns; cdev/class/device are NOT, so undo them by hand and in
	 * reverse order of creation. */
	device_destroy(d->cls, d->devno);
	class_destroy(d->cls);
	cdev_del(&d->cdev);
	unregister_chrdev_region(d->devno, 1);

	led_dev = NULL;
	return 0;
}

static const struct of_device_id ebf_led_of_match[] = {
	{ .compatible = "ebf,ebf-led" },	/* must match the dts exactly */
	{ }
};
MODULE_DEVICE_TABLE(of, ebf_led_of_match);

static struct platform_driver ebf_led_driver = {
	.probe	= ebf_led_probe,
	.remove	= ebf_led_remove,
	.driver	= {
		.name		= "ebf_led",
		.of_match_table	= ebf_led_of_match,
	},
};

module_platform_driver(ebf_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("you");
MODULE_DESCRIPTION("i.MX6ULL GPIO LED char driver with device tree");
MODULE_VERSION("0.1");
