#include <linux/amba/bus.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>
//#include <linux/mux.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/system.h>
#include <asm/pmu.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/unistd.h>
#include <linux/bug.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#define DTS_COMP_GPS_POWER_NAME "huawei,gps_power"
#define GPS_VDDIO_LDO_27V 2700000

typedef struct gps_bcm_info {
	struct gpio       gpioid_en;
	struct clk          *clk;
	struct gpio       gpioid_power;
	struct gpio       gpioid_hostwake;
	bool is_hostwake;
	/* Begin: Added for E911 */
//	struct gpio       gpioid_refclk;
	/* End: Added for E911 */
} GPS_BCM_INFO;

/// gps daemon will access "/dev/gps_geofence_wake"
#define HOST_WAKE_MODULE_NAME "gps_geofence_wake"

/// driver structure for HOST_WAKE module
struct gps_geofence_wake{
    /// irq from gpio_to_irq()
    int irq;
    /// HOST_WAKE_GPIO
    int host_req_pin;
    /// misc driver structure
    struct miscdevice misc;
    /// wake_lock
    struct wake_lock wake_lock;
};
static struct gps_geofence_wake g_geofence_wake;

static int gps_geofence_wake_open(struct inode *inode, struct file *filp)
{
    printk("%s\n", __func__);
    return 0;
}

static int gps_geofence_wake_release(struct inode *inode, struct file *filp)
{
    printk("%s\n", __func__);
    return 0;
}

static long gps_geofence_wake_ioctl( struct file *filp,
        unsigned int cmd, unsigned long arg)
{
    printk("%s\n", __func__);
    return 0;
}

static const struct file_operations gps_geofence_wake_fops = {
    .owner = THIS_MODULE,
    .open = gps_geofence_wake_open,
    .release = gps_geofence_wake_release,
    .unlocked_ioctl = gps_geofence_wake_ioctl
};

/// set/reset wake lock by HOST_WAKE level
/// \param gpio the value of HOST_WAKE_GPIO
static void gps_geofence_wake_lock(int gpio)
{
    struct gps_geofence_wake *ac_data = &g_geofence_wake;
    ///we need to use wake_lock_timeout instead of wake_unlock
    wake_lock_timeout(&ac_data->wake_lock, 5*HZ);
}

static irqreturn_t gps_host_wake_isr(int irq, void *dev)
{
    struct gps_geofence_wake *ac_data = &g_geofence_wake;
    int gps_host_wake = ac_data->host_req_pin;
    char gpio_value = 0x00;

    printk("%s\n", __func__);

    gpio_value = gpio_get_value(gps_host_wake);

    // wake_lock
    gps_geofence_wake_lock(gpio_value);

    return IRQ_HANDLED;
}

/// initialize GPIO and IRQ
/// \param gpio the GPIO of HOST_WAKE
/// \return if SUCCESS, return the id of IRQ, if FAIL, return -EIO
static int gps_gpio_irq_init(int gpio)
{
    int ret = 0;
    int irq = 0;

    printk("[gps]%s\n", __func__);
    // 1. Set GPIO
    if ((gpio_request(gpio, "gps_host_wake")))
    {
        printk("[gps]Can't request HOST_REQ GPIO %d.It may be already registered in init.xyz.3rdparty.rc/init.xyz.rc\n", gpio);
        return -EIO;
    }
    gpio_export(gpio, false);
    gpio_direction_input(gpio);

    // 2. Set IRQ
    irq = gpio_to_irq(gpio);
    if (irq < 0)
    {
        printk("[gps]Could not get HOST_WAKE_GPIO = %d!, err = %d\n", gpio, irq);
        gpio_free(gpio);
        return -EIO;
    }

    ret = request_irq(irq, gps_host_wake_isr, IRQF_TRIGGER_RISING | IRQF_ONESHOT, "gps_host_wake", NULL);
    if (ret)
    {
        printk("[gps]Request_host wake irq failed.\n");
        gpio_free(gpio);
        return -EIO;
    }

    ret = irq_set_irq_wake(irq, 1);

    if (ret)
    {
        printk("[gps]Set_irq_wake failed.\n");
        gpio_free(gpio);
        free_irq(irq, NULL);
        return -EIO;
    }

    return irq;
}

/// cleanup GPIO and IRQ
static void gps_gpio_irq_cleanup(int gpio, int irq)
{
    pr_debug("[gps]%s\n", __func__);
    gpio_free(gpio);
    free_irq(irq, NULL);
}


static int k3_gps_bcm_probe(struct platform_device *pdev)
{
	GPS_BCM_INFO *gps_bcm;
	struct device *gps_power_dev = &pdev->dev;
	int ret = 0;
	struct device_node *np = gps_power_dev->of_node;

    int irq = 0;
    struct gps_geofence_wake *ac_data = &g_geofence_wake;

    printk(KERN_INFO "[GPS] start find gps_power\n");
	gps_bcm = kzalloc(sizeof(GPS_BCM_INFO), GFP_KERNEL);
	if (!gps_bcm) {
		dev_err(&pdev->dev, "[GPS] Alloc memory failed\n");
		return -ENOMEM;
	}


	if(of_property_read_u32(np, "huawei,gps_en", &gps_bcm->gpioid_en.gpio) < 0)		//gps_en
	{
		dev_err(&pdev->dev, "[GPS] of_property_read_u32 failed\n");
		goto err_free;
	}

	ret = gpio_request(gps_bcm->gpioid_en.gpio, "gps_enbale");
	if (ret < 0) {
		pr_err("[GPS] %s: gpio_direction_output %d  failed, ret:%d .\n", __func__, gps_bcm->gpioid_en.gpio ,ret);
		goto err_free;
	}

	gpio_export(gps_bcm->gpioid_en.gpio, false);
	ret	= gpio_direction_output( gps_bcm->gpioid_en.gpio , 0);
	printk(KERN_INFO "[GPS] OoO gpio_direction_output X \n");
	if (ret < 0) {
		printk("[GPS] %s: gpio_direction_output %d  failed, ret:%d .\n", __func__, gps_bcm->gpioid_en.gpio ,ret);
		goto err_free;
	}

	if (of_device_is_compatible(np, "huawei,gps_47531"))
	{
	    gps_bcm->is_hostwake = 1;
	}
	printk("[GPS] %s: of_device_is_compatible %d\n", __func__, gps_bcm->is_hostwake);
	if(gps_bcm->is_hostwake){
	gps_bcm->gpioid_hostwake.gpio = of_get_named_gpio(np, "huawei,gps_hostwake", 0);
   	if (gps_bcm->gpioid_hostwake.gpio<0)
   	{
		ret = -1;
		goto err_free;
     }
    // 1. Init GPIO and IRQ for HOST_WAKE
	printk("[gps]%s,gps_bcm->gpioid_hostwake.gpio=%d\n", __func__,gps_bcm->gpioid_hostwake.gpio);

    irq = gps_gpio_irq_init(gps_bcm->gpioid_hostwake.gpio);
    if (irq < 0)
    {
        return -EIO;
    }

    // 2. Register Driver
    memset(ac_data, 0, sizeof(struct gps_geofence_wake));

    // 2.1 Misc device setup
    ac_data->misc.minor = MISC_DYNAMIC_MINOR;
    ac_data->misc.name = HOST_WAKE_MODULE_NAME;
    ac_data->misc.fops = &gps_geofence_wake_fops;

    // 2.2 Information that be used later
    ac_data->irq = irq;
    ac_data->host_req_pin = gps_bcm->gpioid_hostwake.gpio;


    printk("[gps]misc register, name %s, irq %d, host req pin num %d\n", ac_data->misc.name, irq, ac_data->host_req_pin);
    // 2.3 Register misc driver
    if (0 != (ret = misc_register(&ac_data->misc)))
    {
        printk("[gps]cannot register gps geofence wake miscdev on minor=%d (%d)\n", MISC_DYNAMIC_MINOR, ret);
        return ret;
    }

    // 3. Init wake_lock
    wake_lock_init(&ac_data->wake_lock, WAKE_LOCK_SUSPEND, "gps_geofence_wakelock");
    }

printk(KERN_INFO "[GPS] finish gpio_direction_output gps_power\n");


	/* Set 32KC clock */
	gps_bcm->clk = devm_clk_get(gps_power_dev, NULL);
	printk(KERN_INFO "[GPS] clk is 0x%x\n", (int)gps_bcm->clk);
	if (IS_ERR(gps_bcm->clk)) {
		printk(KERN_INFO "[GPS] clk is error 0x%x\n", (int)gps_bcm->clk);
		ret = PTR_ERR(gps_bcm->clk);
		goto err_free_clk;
	}
	ret = clk_prepare_enable(gps_bcm->clk);
	if(ret)
	{
		printk(KERN_INFO "[GPS] clk enable is failed\n");
		goto err_free_clk;
	}
	printk(KERN_INFO "[GPS] clk is finish\n");


	kfree(gps_bcm);
	gps_bcm = NULL;
	return 0;

err_free_clk:
	clk_put(gps_bcm->clk);


err_free:
	kfree(gps_bcm);
	gps_bcm = NULL;
	return ret;
}


static int k3_gps_bcm_remove(struct platform_device *pdev)
{
	GPS_BCM_INFO *gps_bcm = platform_get_drvdata(pdev);
	struct gps_geofence_wake *ac_data = &g_geofence_wake;

	int ret = 0;
    printk("[gps]%s\n", __func__);
	dev_dbg(&pdev->dev, "k3_gps_bcm_remove +\n");

	if (!gps_bcm) {
		dev_err(&pdev->dev, "gps_bcm is NULL\n");
		return 0;
	}

	clk_disable(gps_bcm->clk);
	clk_put(gps_bcm->clk);

	gpio_free(gps_bcm->gpioid_en.gpio);
	//gpio_free(gps_bcm->gpioid_ret);
	//gpio_free(gps_bcm->gpioid_refclk); /* Add for E911 */

	kfree(gps_bcm);
	gps_bcm = NULL;
	platform_set_drvdata(pdev, NULL);

    /*clean geo*/
    // 1. Cleanup GPIO and IRQ
    if(gps_bcm->is_hostwake)
    {
	    gps_gpio_irq_cleanup(ac_data->host_req_pin, ac_data->irq);
        wake_lock_destroy(&ac_data->wake_lock);
	    // 2. Cleanup driver
	    if (0 != (ret = misc_deregister(&ac_data->misc)))
	    {
	        pr_err("[gps]cannot unregister gps geofence wake miscdev on minor=%d (%d)\n",MISC_DYNAMIC_MINOR, ret);
	        return ret;
	    }
    }
	dev_dbg(&pdev->dev, "k3_gps_bcm_remove -\n");



	return ret;
}

static void K3_gps_bcm_shutdown(struct platform_device *pdev)
{
	GPS_BCM_INFO *gps_bcm = platform_get_drvdata(pdev);
    printk("[%s] +\n", __func__);
	if (!gps_bcm) {
		dev_err(&pdev->dev, "gps_bcm is NULL\n");
		return;
	}

	//gpio_set_value(gps_bcm->gpioid_en.gpio, 0);
	//gpio_set_value(gps_bcm->gpioid_ret, 0);
	//gpio_set_value(gps_bcm->gpioid_refclk, 0);/* Add for E911 */
	gpio_direction_output(gps_bcm->gpioid_en.gpio, 0);

	clk_disable(gps_bcm->clk);
	clk_put(gps_bcm->clk);

	printk("[%s] -\n", __func__);
}


#ifdef CONFIG_PM
static int  k3_gps_bcm_suspend(struct platform_device *pdev, pm_message_t state)
{
	GPS_BCM_INFO *gps_bcm = platform_get_drvdata(pdev);
    printk("[%s] +\n", __func__);
	if (!gps_bcm) {
		dev_err(&pdev->dev, "gps_bcm is NULL\n");
		return 0;
	}
	gpio_set_value(gps_bcm->gpioid_en.gpio, 0);

	printk("[%s] -\n", __func__);
	return 0;
}
#else

#define k3_gps_bcm_suspend	NULL

#endif /* CONFIG_PM */

static const struct of_device_id gps_power_match_table[] = {
	{
		.compatible = DTS_COMP_GPS_POWER_NAME,   // compatible must match with which defined in dts
		.data = NULL,
	},
	{
	    .compatible = "huawei,gps_47531"
	},
};

MODULE_DEVICE_TABLE(of, gps_power_match_table);

static struct platform_driver k3_gps_bcm_driver = {
	.probe			= k3_gps_bcm_probe,
	.suspend		= k3_gps_bcm_suspend,
	.remove			= k3_gps_bcm_remove,
	.shutdown		= K3_gps_bcm_shutdown,
	.driver = {
		.name = "huawei,gps_power",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(gps_power_match_table), // dts required code
	},
};

static int __init k3_gps_bcm_init(void)
{
	return platform_driver_register(&k3_gps_bcm_driver);
}

static void __exit k3_gps_bcm_exit(void)
{
	platform_driver_unregister(&k3_gps_bcm_driver);
}


module_init(k3_gps_bcm_init);
module_exit(k3_gps_bcm_exit);

MODULE_AUTHOR("DRIVER_AUTHOR");
MODULE_DESCRIPTION("GPS Boardcom 47511 driver");
MODULE_LICENSE("GPL");
