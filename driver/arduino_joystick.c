#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>

#define IRQ_PIN 17

struct arduino_joystick {
	struct input_dev *input;
	struct i2c_client *client;
	char phys[32];
	int irq;
};

static irqreturn_t arduino_joystick_soft_irq(int irq, void *handle)
{
	struct arduino_joystick *joystick = handle;
	char data[8];

	//printk(KERN_INFO "arduino_joystick soft irq\n");
	i2c_master_recv(joystick->client, data, 8);
	/*
	for(int i = 0; i < 8; i++)
		printk(KERN_INFO "%x\n", data[i]);
	*/
	if(data[0] == 0)
		input_report_key(joystick->input, BTN_A, 1);
	else
		input_report_key(joystick->input, BTN_A, 0);
	if(data[1] == 0)
		input_report_key(joystick->input, BTN_B, 1);
	else
		input_report_key(joystick->input, BTN_B, 0);
	if(data[2] == 0)
		input_report_key(joystick->input, BTN_X, 1);
	else
		input_report_key(joystick->input, BTN_X, 0);
	if(data[3] == 0)
		input_report_key(joystick->input, BTN_Y, 1);
	else
		input_report_key(joystick->input, BTN_Y, 0);
	input_report_abs(joystick->input, ABS_X, (data[5] << 8) + data[4]);
	input_report_abs(joystick->input, ABS_Y, (data[7] << 8) + data[6]);

	input_sync(joystick->input);
	return IRQ_HANDLED;
}

static irqreturn_t arduino_joystick_hard_irq(int irq, void *handle)
{
	struct arduino_joystick *joystick = handle;

	//printk(KERN_INFO "arduino_joystick hard irq\n");

	return IRQ_WAKE_THREAD;
}

static int arduino_joystick_open(struct input_dev *input_dev)
{

	return 0;
}

static void arduino_joystick_close(struct input_dev *input_dev)
{
}

static int __devinit arduino_joystick_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct arduino_joystick *joystick;
	struct input_dev *input_dev;
	int irq_gpio;
	int err;

	printk(KERN_NOTICE "arduino_joystick  probe\n");

	joystick = kzalloc(sizeof(struct arduino_joystick), GFP_KERNEL);
	input_dev = input_allocate_device();
	if(!joystick || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	joystick->client = client;
	joystick->input = input_dev;
	snprintf(joystick->phys, sizeof(joystick->phys), "%s/input0", dev_name(&client->dev));

	input_dev->name = "arduino_joystick";
	input_dev->phys = joystick->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->open = arduino_joystick_open;
	input_dev->close = arduino_joystick_close;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	__set_bit(BTN_A, input_dev->keybit);
	__set_bit(BTN_B, input_dev->keybit);
	__set_bit(BTN_X, input_dev->keybit);
	__set_bit(BTN_Y, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_X, 0, 1023, 4, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 1023, 4, 0);

	err = gpio_request(IRQ_PIN, "arduino_joystick");
	if(err < 0) {
		printk(KERN_NOTICE "gpio_request error\n");
		goto err_free_mem;
	}
	irq_gpio = gpio_to_irq(IRQ_PIN);
	if(irq_gpio < 0) {
		printk(KERN_NOTICE "gpio_to_irq error\n");
		err = irq_gpio;
		goto err_free_mem;
	}
	err = request_threaded_irq(irq_gpio, arduino_joystick_hard_irq, arduino_joystick_soft_irq, IRQF_TRIGGER_FALLING, "arduino_joystick", joystick);
	if(err < 0) {
		printk(KERN_NOTICE "request_threaded_irq error\n");
		goto err_free_mem;
	}
	joystick->irq = irq_gpio;

	err = input_register_device(input_dev);
	if(err < 0) {
		printk(KERN_NOTICE "input_register_device error\n");
		goto err_free_irq;
	}
	
	i2c_set_clientdata(client, joystick);
	return 0;

err_free_irq:
	free_irq(joystick->irq, joystick);
	gpio_free(IRQ_PIN);

err_free_mem:
	input_free_device(input_dev);
	kfree(joystick);
	return err;
}

static int __devexit arduino_joystick_remove(struct i2c_client *client)
{
	struct arduino_joystick *joystick;

	joystick = i2c_get_clientdata(client);
	free_irq(joystick->irq, joystick);
	gpio_free(IRQ_PIN);
	input_unregister_device(joystick->input);
	kfree(joystick);

	return 0;
}

static const struct i2c_device_id arduino_joystick_idtable[] = {
	{ "arduino_joystick", 0},
	{ }
};

MODULE_DEVICE_TABLE(i2c, arduino_joystick_idtable);

static struct i2c_driver arduino_joystick_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "arduino_joystick"
	},
	.id_table = arduino_joystick_idtable,
	.probe = arduino_joystick_probe,
	.remove = __devexit_p(arduino_joystick_remove)
};

module_i2c_driver(arduino_joystick_driver);

MODULE_DESCRIPTION("arduino_joystick driver");
MODULE_LICENSE("GPL");
