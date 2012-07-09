/*
 * TI AM3359 ADC driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/input/ti_tscadc.h>
#include <linux/delay.h>
#include <linux/device.h>

static ssize_t do_adc_sample(struct device *, struct device_attribute *, char *);
static DEVICE_ATTR(ain1, S_IRUGO, do_adc_sample, NULL);
static DEVICE_ATTR(ain2, S_IRUGO, do_adc_sample, NULL);
static DEVICE_ATTR(ain3, S_IRUGO, do_adc_sample, NULL);
static DEVICE_ATTR(ain4, S_IRUGO, do_adc_sample, NULL);
static DEVICE_ATTR(ain5, S_IRUGO, do_adc_sample, NULL);
static DEVICE_ATTR(ain6, S_IRUGO, do_adc_sample, NULL);
static DEVICE_ATTR(ain7, S_IRUGO, do_adc_sample, NULL);
static DEVICE_ATTR(ain0, S_IRUGO, do_adc_sample, NULL);

/* Memory mapped registers here have incorrect offsets!
 * Correct after referring TRM */
#define TSCADC_REG_IRQEOI			0x020
#define TSCADC_REG_RAWIRQSTATUS			0x024
#define TSCADC_REG_IRQSTATUS			0x028
#define TSCADC_REG_IRQENABLE			0x02C
#define TSCADC_REG_IRQWAKEUP			0x034
#define TSCADC_REG_CTRL				0x040
#define TSCADC_REG_ADCFSM			0x044
#define TSCADC_REG_CLKDIV			0x04C
#define TSCADC_REG_SE				0x054
#define TSCADC_REG_IDLECONFIG			0x058
#define TSCADC_REG_CHARGECONFIG			0x05C
#define TSCADC_REG_CHARGEDELAY			0x060
#define TSCADC_REG_STEPCONFIG(n)		(0x64 + ((n-1) * 8))
#define TSCADC_REG_STEPDELAY(n)			(0x68 + ((n-1) * 8))
#define TSCADC_REG_STEPCONFIG13			0x0C4
#define TSCADC_REG_STEPDELAY13			0x0C8
#define TSCADC_REG_STEPCONFIG14			0x0CC
#define TSCADC_REG_STEPDELAY14			0x0D0
#define TSCADC_REG_FIFO0CNT			0xE4
#define TSCADC_REG_FIFO0THR			0xE8
#define TSCADC_REG_FIFO1CNT			0xF0
#define TSCADC_REG_FIFO1THR			0xF4
#define TSCADC_REG_FIFO0			0x100
#define TSCADC_REG_FIFO1			0x200

/*	Register Bitfields	*/
#define TSCADC_IRQWKUP_ENB			BIT(0)
#define TSCADC_STPENB_STEPENB_TOUCHSCREEN	0x7FFF
#define TSCADC_STPENB_STEPENB_GENERAL		0x0400
#define TSCADC_IRQENB_FIFO0THRES		BIT(2)
#define TSCADC_IRQENB_FIFO0OVERRUN		BIT(3)
#define TSCADC_IRQENB_FIFO1THRES		BIT(5)
#define TSCADC_IRQENB_EOS			BIT(1)
#define TSCADC_IRQENB_PENUP			BIT(9)
#define TSCADC_STEPCONFIG_MODE_HWSYNC		0x2
#define TSCADC_STEPCONFIG_MODE_SWCONT		0x1
#define TSCADC_STEPCONFIG_MODE_SWONESHOT	0x0
#define TSCADC_STEPCONFIG_2SAMPLES_AVG		(1 << 4)
#define TSCADC_STEPCONFIG_NO_AVG		0
#define TSCADC_STEPCONFIG_XPP			BIT(5)
#define TSCADC_STEPCONFIG_XNN			BIT(6)
#define TSCADC_STEPCONFIG_YPP			BIT(7)
#define TSCADC_STEPCONFIG_YNN			BIT(8)
#define TSCADC_STEPCONFIG_XNP			BIT(9)
#define TSCADC_STEPCONFIG_YPN			BIT(10)
#define TSCADC_STEPCONFIG_RFP			(1 << 12)
#define TSCADC_STEPCONFIG_INM			(1 << 18)
#define TSCADC_STEPCONFIG_INP_4			(1 << 19)
#define TSCADC_STEPCONFIG_INP			(1 << 20)
#define TSCADC_STEPCONFIG_INP_5			(1 << 21)
#define TSCADC_STEPCONFIG_FIFO1			(1 << 26)
#define TSCADC_STEPCONFIG_IDLE_INP		(1 << 22)
#define TSCADC_STEPCONFIG_OPENDLY		0x018
#define TSCADC_STEPCONFIG_SAMPLEDLY		0x88
#define TSCADC_STEPCONFIG_Z1			(3 << 19)
#define TSCADC_STEPCHARGE_INM_SWAP		BIT(16)
#define TSCADC_STEPCHARGE_INM			BIT(15)
#define TSCADC_STEPCHARGE_INP_SWAP		BIT(20)
#define TSCADC_STEPCHARGE_INP			BIT(19)
#define TSCADC_STEPCHARGE_RFM			(1 << 23)
#define TSCADC_STEPCHARGE_DELAY			0x1
#define TSCADC_CNTRLREG_TSCSSENB		BIT(0)
#define TSCADC_CNTRLREG_STEPID			BIT(1)
#define TSCADC_CNTRLREG_STEPCONFIGWRT		BIT(2)
#define TSCADC_CNTRLREG_TSCENB			BIT(7)
#define TSCADC_CNTRLREG_4WIRE			(0x1 << 5)
#define TSCADC_CNTRLREG_5WIRE			(0x1 << 6)
#define TSCADC_CNTRLREG_8WIRE			(0x3 << 5)
#define TSCADC_ADCFSM_STEPID			0x10
#define TSCADC_ADCFSM_FSM			BIT(5)

#define ADC_CLK					3000000

#define MAX_12BIT				((1 << 12) - 1)

struct adc {
	struct clk *tsc_ick;
	void __iomem *tsc_base;
};

static unsigned int adc_readl(struct adc *ts, unsigned int reg)
{
	return readl(ts->tsc_base + reg);
}

static void adc_writel(struct adc *tsc, unsigned int reg,
		unsigned int val)
{
	writel(val, tsc->tsc_base + reg);
}

static void adc_step_config(struct adc *ts_dev, int channel)
{
	unsigned int stepconfig = 0, delay = 0;

	/*
	 * Step Configuration
	 * software-enabled one shot
	 * 2 sample averaging
	 * sample channel 1 (SEL_INP mux bits = 0)
	 */
	stepconfig = TSCADC_STEPCONFIG_MODE_SWONESHOT
			| TSCADC_STEPCONFIG_2SAMPLES_AVG 
			| ((channel) << 19);

	delay = TSCADC_STEPCONFIG_SAMPLEDLY | TSCADC_STEPCONFIG_OPENDLY;

	adc_writel(ts_dev, TSCADC_REG_STEPCONFIG(10), stepconfig);
	adc_writel(ts_dev, TSCADC_REG_STEPDELAY(10), delay);

	/* Get the ball rolling, this will trigger the FSM to step through
	 * as soon as TSC_ADC_SS is turned on */
	adc_writel(ts_dev, TSCADC_REG_SE, TSCADC_STPENB_STEPENB_GENERAL);
}

static ssize_t do_adc_sample(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct adc *ts_dev;
	int channel_num, output;
	int fifo0count = 0;
	int read_sample = 0;
	int poll_limit = 5000;
	int poll_limit_ctr = 0;

	memset(buf, 0, PAGE_SIZE);

	ts_dev = dev_get_drvdata(dev);

	if (strncmp(attr->attr.name, "ain", 3)) {
		printk("Invalid ain num\n");
		return -EINVAL;
	}

	channel_num = attr->attr.name[3] - 0x30;
	if (channel_num > 7 || channel_num < 0) {
		printk("Invalid channel_num=%d\n", channel_num);
		return -EINVAL;
	}

	adc_step_config(ts_dev, channel_num);

	/* spin until theres something in the FIFO or we decide */
	/* the device isn't going to respond */
	do {
		fifo0count = adc_readl(ts_dev, TSCADC_REG_FIFO0CNT);
		poll_limit_ctr++;
	} while (!fifo0count && (poll_limit_ctr < poll_limit));

	while ((poll_limit_ctr < poll_limit) && (fifo0count--)) {
		read_sample = adc_readl(ts_dev, TSCADC_REG_FIFO0) & 0xfff;
		// printk("polling sample: %d: %x\n", fifo0count, read_sample);
	}
	
	if(poll_limit_ctr > poll_limit) {
		printk("Poll limit hit; could not read ADC value.\n");
	}

	output = sprintf(buf, "%d\n", read_sample);

	return output;
}

/*
 * The functions for inserting/removing driver as a module.
 */
static int __devinit adc_probe(struct platform_device *pdev)
{
	struct adc *ts_dev;
	int err;
	int clk_value;
	int clock_rate, irqenable, ctrl;
	struct resource *res;
	struct clk *clk;

	printk("dev addr = %p\n", &pdev->dev);
	printk("pdev addr = %p\n", pdev);

	err = device_create_file(&pdev->dev, &dev_attr_ain1);
	err |= device_create_file(&pdev->dev, &dev_attr_ain2);
	err |= device_create_file(&pdev->dev, &dev_attr_ain3);
	err |= device_create_file(&pdev->dev, &dev_attr_ain4);
	err |= device_create_file(&pdev->dev, &dev_attr_ain5);
	err |= device_create_file(&pdev->dev, &dev_attr_ain6);
	err |= device_create_file(&pdev->dev, &dev_attr_ain7);
	err |= device_create_file(&pdev->dev, &dev_attr_ain0);

	if(err) {
		dev_err(&pdev->dev, "couldn't create sysfs entries.\n");
		return err;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource defined.\n");
		return -EINVAL;
	}

	/* Allocate memory for device */
	ts_dev = kzalloc(sizeof(struct adc), GFP_KERNEL);
	if (!ts_dev) {
		dev_err(&pdev->dev, "failed to allocate memory.\n");
		return -ENOMEM;
	}

	res = request_mem_region(res->start, resource_size(res), pdev->name);
	if (!res) {
		dev_err(&pdev->dev, "failed to reserve registers.\n");
		err = -EBUSY;
		goto err_free_mem;
	}

	ts_dev->tsc_base = ioremap(res->start, resource_size(res));
	if (!ts_dev->tsc_base) {
		dev_err(&pdev->dev, "failed to map registers.\n");
		err = -ENOMEM;
		goto err_release_mem;
	}

	ts_dev->tsc_ick = clk_get(&pdev->dev, "adc_tsc_ick");
	if (IS_ERR(ts_dev->tsc_ick)) {
		dev_err(&pdev->dev, "failed to get ADC ick\n");
		goto err_unmap_regs;
	}
	clk_enable(ts_dev->tsc_ick);

	clk = clk_get(&pdev->dev, "adc_tsc_fck");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get ADC fck\n");
		err = PTR_ERR(clk);
		goto err_unmap_regs;
	}
	clock_rate = clk_get_rate(clk);
	clk_value = clock_rate / ADC_CLK;
	if (clk_value < 7) {
		dev_err(&pdev->dev, "clock input less than min clock requirement\n");
		goto err_fail;
	}
	/* TSCADC_CLKDIV needs to be configured to the value minus 1 */
	clk_value = clk_value - 1;
	adc_writel(ts_dev, TSCADC_REG_CLKDIV, clk_value);

	/* Set the control register bits - 12.5.44 TRM */
	ctrl = TSCADC_CNTRLREG_STEPCONFIGWRT | TSCADC_CNTRLREG_STEPID;
	ctrl |= TSCADC_CNTRLREG_TSCSSENB;

	/* ADC configuration */
	adc_writel(ts_dev, TSCADC_REG_FIFO0THR, 0);

	/* don't use interrupts */
	irqenable = 0;
	adc_writel(ts_dev, TSCADC_REG_IRQENABLE, irqenable);

	/* Turn on TSC_ADC */
	adc_writel(ts_dev, TSCADC_REG_CTRL, ctrl);

	platform_set_drvdata(pdev, ts_dev);
	return 0;

err_fail:
	clk_disable(ts_dev->tsc_ick);
	clk_put(ts_dev->tsc_ick);
err_unmap_regs:
	iounmap(ts_dev->tsc_base);
err_release_mem:
	release_mem_region(res->start, resource_size(res));
err_free_mem:
	kfree(ts_dev);
	printk(KERN_ERR "Fatal error, shutting down ADC\n");
	return err;
}

static int __devexit adc_remove(struct platform_device *pdev)
{
	struct adc *ts_dev = platform_get_drvdata(pdev);
	struct resource *res;

	device_remove_file(&pdev->dev, &dev_attr_ain1);
	device_remove_file(&pdev->dev, &dev_attr_ain2);
	device_remove_file(&pdev->dev, &dev_attr_ain3);
	device_remove_file(&pdev->dev, &dev_attr_ain4);
	device_remove_file(&pdev->dev, &dev_attr_ain5);
	device_remove_file(&pdev->dev, &dev_attr_ain6);
	device_remove_file(&pdev->dev, &dev_attr_ain7);
	device_remove_file(&pdev->dev, &dev_attr_ain0);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	iounmap(ts_dev->tsc_base);
	release_mem_region(res->start, resource_size(res));

	clk_disable(ts_dev->tsc_ick);
	clk_put(ts_dev->tsc_ick);

	kfree(ts_dev);

	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver adc_driver = {
		.probe = adc_probe,
		.remove = __devexit_p(adc_remove),
		.driver = {
			.name = "ain",
			.owner = THIS_MODULE,
		},
};

static int __init ti_adc_init(void)
{
	return platform_driver_register(&adc_driver);
}
module_init(ti_adc_init);

static void __exit ti_adc_exit(void)
{
	platform_driver_unregister(&adc_driver);
}
module_exit(ti_adc_exit);

MODULE_DESCRIPTION("TI AM3359 ADC driver");
MODULE_AUTHOR("Jeffrey Myers <jmyers@criticallink.com>");
MODULE_LICENSE("GPL");

