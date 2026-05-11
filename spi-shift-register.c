/* 
 * spi-shift-register.c - 8 bit shift register driver
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/string.h>

#include <linux/spi/spi.h>

#include <asm/errno.h>

#define DEVICE_NAME		"my_shift_register"

#define SHIFT_REG_MAX_INPUT	20

static ssize_t shift_reg_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static int shift_reg_open(struct inode *inode, struct file *file);

struct shift_register_data {
	struct spi_device *spi;
	struct mutex lock;
	struct delayed_work work;
	struct miscdevice miscdev;

	u16 pattern;
};

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.write = shift_reg_write,
	.open = shift_reg_open,
};

static void write_spi_output_mask(struct shift_register_data *data, u8 value);

static int shift_reg_open(struct inode *inode, struct file *file)
{
	struct miscdevice *mdev = file->private_data;
	struct shift_register_data *data = container_of(mdev, struct shift_register_data, miscdev);

	file->private_data = data;

	return 0;
}

static void blink_led_pattern(struct shift_register_data *data)
{
	u16 pattern = data->pattern;

	while (pattern) {
		write_spi_output_mask(data, pattern & 0x0F);
		pattern >>= 4;
	}
}

static ssize_t shift_reg_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct shift_register_data *data = file->private_data;
	char *kbuf, *p, *token;
	size_t len;
	int ret = 0;
	u8 value;
	u16 pattern = 0;

	if (!data || !data->spi)
		return -ENODEV;

	len = min_t(size_t, count, SHIFT_REG_MAX_INPUT);

	kbuf = memdup_user_nul(buf, len);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	if (len > 0 && kbuf[len - 1] == '\n')
		kbuf[--len] = '\0';

	p = kbuf;

	while((token = strsep(&p, " \t\n")) != NULL) {
		if (*token == '\0')
			continue;

		ret = kstrtou8(token, 2, &value);
		if (ret)
			goto out;

		if (value > 0x0F) {
			ret = -EINVAL;
			goto out;
		}

		pattern = (pattern << 4) | value;
	} 

	mutex_lock(&data->lock);
	data->pattern = pattern;
	mutex_unlock(&data->lock);

	schedule_delayed_work(&data->work, msecs_to_jiffies(250));

	ret = count;
out:
	kfree(kbuf);
	return ret;
}

static void write_spi_output_mask(struct shift_register_data *data, u8 value)
{
	int ret;

	u8 value_padded = (value << 1) & 0x1E;

	ret = spi_write(data->spi, &value_padded, 1);
	if (ret)
		dev_err(&data->spi->dev, "spi_write failed: %d\n", ret);

	msleep(500);
}


static void worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct shift_register_data *data = container_of(dwork, struct shift_register_data, work);

	if (!data || !data->spi) {
		pr_err("shift-register: invalid data/spi pointer\n");
		return;
	}

	u16 pattern;

	mutex_lock(&data->lock);
	pattern = data->pattern;
	mutex_unlock(&data->lock);

	blink_led_pattern(data);

	schedule_delayed_work(&data->work, 0);
}

static int device_probe(struct spi_device *dev)
{
	struct shift_register_data *data;
	int ret;

	data = devm_kzalloc(&dev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->spi = dev;

	mutex_init(&data->lock);
	spi_set_drvdata(dev, data);

	dev->bits_per_word = 8;
	dev->mode = SPI_MODE_0;

	ret = spi_setup(dev);
	if (ret) {
		dev_err(&dev->dev, "spi_setup failed: %d\n", ret);
		return ret;
	}

	data->miscdev.name = DEVICE_NAME;
	data->miscdev.minor = MISC_DYNAMIC_MINOR;
	data->miscdev.fops = &fops;
	data->miscdev.parent = &dev->dev;

	ret = misc_register(&data->miscdev);
	if (ret) {
		dev_err(&dev->dev, "Failed to initialize shift reg misc device driver");
		return ret;
	}

	INIT_DELAYED_WORK(&data->work, worker);
	dev_info(&dev->dev, "probe finished\n");

	return 0;
}

static void device_remove(struct spi_device *dev)
{
	struct shift_register_data *data = spi_get_drvdata(dev);

	cancel_delayed_work_sync(&data->work);
	misc_deregister(&data->miscdev);
}

static const struct of_device_id shift_register_of_match[] = {
	{ .compatible = "myvendor,my-shift-register" },
	{ }
};

static struct spi_driver shift_register_driver = {
	.probe = device_probe,
	.remove = device_remove,
	.driver = {
		.name = DEVICE_NAME,
		.of_match_table = shift_register_of_match,
	}
};
MODULE_DEVICE_TABLE(of, shift_register_of_match);

static int __init led_init(void)
{
	int ret = 0;

	ret = spi_register_driver(&shift_register_driver);
	if (ret) {
		pr_err("Unable to register (%s) driver: error %d\n", DEVICE_NAME, ret);
		return ret;
	}
	pr_info("(%s) driver registered\n", DEVICE_NAME);

	return 0;
}

static void __exit led_exit(void)
{
	spi_unregister_driver(&shift_register_driver);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple example led platform driver");

