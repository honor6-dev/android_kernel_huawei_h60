/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2013 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/hw_log.h>
#include<linux/of.h>
#include <linux/huawei/dsm_pub.h>
#include <linux/platform_device.h>
#define DTS_AP_CP_TIMEOUT_NAME "ap_cp_timeout_log_dev"
#define HWLOG_TAG ap_cp_timeout
HWLOG_REGIST();
struct dsm_client *ap_cp_dclient = NULL;

static struct dsm_dev dsm_ap_cp = {
	.name = "dsm_apcp",
	.fops = NULL,
	.buff_size = 1024,
};

static struct attribute *ap_cp_attributes[] = {
	NULL
};

static const struct attribute_group ap_cp_attr_group = {
	.attrs = ap_cp_attributes,
};
static struct platform_device ap_cp_dev = {
    .name = "ap_cp_timeout",
};

int ap_cp_pf_remove(struct platform_device *dev)
{
    hwlog_info("called remove %s\n", __func__);
    return 0;
}
int ap_cp_pf_probe(struct platform_device *pdev)
{
    int ret = 0;

    hwlog_info("APCP__called probe %s\n", __func__);

    ret = platform_device_register(&ap_cp_dev);
    if (ret) {
        hwlog_err( "%s: platform_device_register failed, ret:%d.\n",
                __func__, ret);
        goto err_register_platform_device;
    }

    ret =  sysfs_create_group(&ap_cp_dev.dev.kobj, &ap_cp_attr_group);
    if (ret){
        hwlog_err("APCP_ %s rnic_adapter_pf_probe. \n", __func__);
        platform_device_unregister(&ap_cp_dev);
    }

err_register_platform_device:
    return ret;
}

static const struct of_device_id ap_cp_match_table[] = {
    {
        .compatible = DTS_AP_CP_TIMEOUT_NAME,
        .data = NULL,
    },
    {
    },
};
MODULE_DEVICE_TABLE(of, ap_cp_match_table);

static struct platform_driver ap_cp_driver = {
    .probe = ap_cp_pf_probe,
    .remove = ap_cp_pf_remove,

    .driver = {
        .name = "ap_cp_timeout_dev",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(ap_cp_match_table),
    },

};

static int __init ap_cp_timeout_module_init(void)
{
    int error = -1;

    error = platform_driver_register(&ap_cp_driver);
    if (error){
        hwlog_err("APCP__ %s ap_cp_init.\n", __func__);
        goto out;
    }

    if (!ap_cp_dclient) {
        ap_cp_dclient = dsm_register_client(&dsm_ap_cp);
    }

out:
	return error;
}

static void __exit ap_cp_timeout_module_exit(void)
{
    platform_driver_unregister(&ap_cp_driver);
}

late_initcall(ap_cp_timeout_module_init);
module_exit(ap_cp_timeout_module_exit);

MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei AP CP TIMOUT");
MODULE_LICENSE("GPL");
