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

#include <linux/spi/spi.h>

#include <asm/errno.h>

#define DEVICE_NAME		"my_shift_register"
#define FIRST_LED_MASK		0x02
#define SECOND_LED_MASK		0x04
#define THIRD_LED_MASK		0x08
#define FOURTH_LED_MASK		0x10

struct shift_register_data {
	struct spi_device *spi;
	struct mutex lock;
	struct delayed_work work;
	bool on;

	u8 pattern;
};

static void write_spi_output_mask(struct shift_register_data *data, u8 value)
{
	int ret;

	ret = spi_write(data->spi, &value, 1);
	if (ret)
		dev_err(&data->spi->dev, "spi_write failed: %d\n", ret);

	msleep(1000);
}

static void blink_led_pattern(struct shift_register_data *data)
{
	if (data->on) {
		write_spi_output_mask(data, FOURTH_LED_MASK + THIRD_LED_MASK);
		write_spi_output_mask(data, SECOND_LED_MASK + FIRST_LED_MASK);
	}
}

static void worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct shift_register_data *data = container_of(dwork, struct shift_register_data, work);

	if (!data || !data->spi) {
		pr_err("shift-register: invalid data/spi pointer\n");
		return;
	}

	mutex_lock(&data->lock);

	blink_led_pattern(data);

	mutex_unlock(&data->lock);

	schedule_delayed_work(&data->work, 0);
}

static int device_probe(struct spi_device *dev)
{
	struct shift_register_data *data;
	int ret;

	dev_info(&dev->dev, "probe called\n");

	data = devm_kzalloc(&dev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->spi = dev;
	data->on = true;

	mutex_init(&data->lock);
	spi_set_drvdata(dev, data);

	dev->bits_per_word = 8;
	dev->mode = SPI_MODE_0;

	ret = spi_setup(dev);
	if (ret) {
		dev_err(&dev->dev, "spi_setup failed: %d\n", ret);
		return ret;
	}

	INIT_DELAYED_WORK(&data->work, worker);
	schedule_delayed_work(&data->work, msecs_to_jiffies(250));

	dev_info(&dev->dev, "probe finished\n");

	return 0;
}

static void device_remove(struct spi_device *dev)
{
	struct shift_register_data *data = spi_get_drvdata(dev);

	cancel_delayed_work_sync(&data->work);
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

