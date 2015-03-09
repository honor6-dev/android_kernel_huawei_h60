/* Copyright (c) 2008-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/

#include "k3_fb.h"
#include "k3_mipi_dsi.h"
#include <linux/lcd_tuning.h>

#include <linux/of.h>
#include <linux/log_jank.h>
#include <linux/huawei/hisi_adc.h>



#define DTS_COMP_VIRTUAL_PANEL "hisilicon,mipi_virtual_panel"
static struct k3_fb_panel_data mipi_virtual_panel_data;
static int hkadc_buf = 0;

#define GPIO_VSP	"gpio_lcd_vsp"
#define GPIO_VSN	"gpio_lcd_vsn"


struct ldo_ops {
	void(*k3_vsp)(bool en);
	void(*k3_vsn)(bool en);
	void(*k3_vddio)(bool en);
	void(*k3_vci)(bool en);
};

struct voltage_node {
	unsigned int vsn_gpio;
	unsigned int vsp_gpio;
};

static struct voltage_node volt = {0};
static struct ldo_ops ldo_en;

static struct gpio_desc virtual_lcd_gpio_request_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_VSP, &volt.vsp_gpio, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_VSN, &volt.vsn_gpio, 0},
};

static ssize_t k3_hkadc_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", hkadc_buf*4);
}

static ssize_t k3_hkadc_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	int channel = 0;

	ret = sscanf(buf, "%u", &channel);
	if(ret <= 0) {
		K3_FB_ERR("Sscanf return invalid, ret = %d\n", ret);
		return count;
	}

	hkadc_buf = hisi_adc_get_value(channel);
	K3_FB_INFO("channel[%d] value is %d\n", channel, hkadc_buf);
	return count;
}

static void k3_vsp_enable(bool en)
{
	K3_FB_INFO("vsp enable(%d)\n", en);

	if(en)
		gpio_direction_output(volt.vsp_gpio, 1);
	else
		gpio_direction_output(volt.vsp_gpio, 0);
}

static void k3_vsn_enable(bool en)
{
	K3_FB_INFO("vsn enable(%d)\n", en);

	if(en)
		gpio_direction_output(volt.vsn_gpio, 1);
	else
		gpio_direction_output(volt.vsn_gpio, 0);
}

static void k3_vddio_enable(bool en)
{
	char __iomem *vbuck5_en;
	char __iomem *vbuck5_set;
	char __iomem *lsw50_en;

	unsigned long vbuck5_en_phy = 0xfff34298;
	unsigned long vbuck5_set_phy = 0xfff3429c;
	unsigned long lsw50_phy = 0xfff342B0;

	vbuck5_en = (char __iomem*)ioremap(vbuck5_en_phy, (size_t)4);
	vbuck5_set = (char __iomem*)ioremap(vbuck5_set_phy, (size_t)4);
	lsw50_en = (char __iomem*)ioremap(lsw50_phy, (size_t)4);

	K3_FB_INFO("VLSW50 & VBUCK5 enable(%d)\n", en);

	if(en) {
		/*VBUCK5 ON*/
		set_reg(vbuck5_en, 1, 1, 1);
		set_reg(vbuck5_set, 3, 3, 0);
		/*VLSW50 ON*/
		set_reg(lsw50_en, 1, 1, 4);
	} else {
		/*VLSW50 OFF*/
		set_reg(lsw50_en, 0, 1, 4);
	}
}

static void k3_vci_enable(bool en)
{
	char __iomem *ldo17;
	unsigned long ldo17_phy = 0xfff3424C;

	ldo17 = (char __iomem *)ioremap(ldo17_phy, (size_t)4);

	K3_FB_INFO("ldo17 enable(%d)\n", en);

	if (en) {
		set_reg(ldo17, 5, 3, 0);
		set_reg(ldo17, 1, 1, 5);
	} else
		set_reg(ldo17, 0, 1, 5);
}

static ssize_t k3_voltage_enable_store(struct device*dev,
	struct device_attribute *attr, const char*buf, size_t count)
{
	char command[10] = {0};

	if (!sscanf(buf, "%s", command)) {
		K3_FB_INFO("bad command(%s)\n", command);
		return count;
	}

	K3_FB_INFO("command(%s)\n", command);
	if (!strncmp("vsp:", command, strlen("vsp:"))) {
		if('0' == command[strlen("vsp:")])
			ldo_en.k3_vsp(false);
		else
			ldo_en.k3_vsp(true);
	}

	if (!strncmp("vsn:", command, strlen("vsn:"))) {
		if('0' == command[strlen("vsn:")])
			ldo_en.k3_vsn(false);
		else
			ldo_en.k3_vsn(true);
	}

	if (!strncmp("vddio:", command, strlen("vddio:"))) {
		if('0' == command[strlen("vddio:")])
			ldo_en.k3_vddio(false);
		else
			ldo_en.k3_vddio(true);
	}

	if (!strncmp("vci:", command, strlen("vci:"))) {
		if('0' == command[strlen("vci:")])
			ldo_en.k3_vci(false);
		else
			ldo_en.k3_vci(true);
	}

	return count;
}

static struct device_attribute k3_lcd_class_attrs[] = {
	__ATTR(hkadc_debug, S_IRUSR|S_IWUSR, k3_hkadc_debug_show, k3_hkadc_debug_store),
	__ATTR(voltage_enable, S_IWUSR, NULL, k3_voltage_enable_store),
	__ATTR_NULL,
};

static int mipi_virtual_panel_set_fastboot(struct platform_device *pdev)
{
	return 0;
}

static int mipi_virtual_panel_on(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;
	struct k3_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL);
	k3fd = platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	K3_FB_INFO("fb%d, +!\n", k3fd->index);

	pinfo = &(k3fd->panel_info);

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		K3_FB_INFO("power on\n");
		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		K3_FB_INFO("mipi lp send sequence\n");
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		K3_FB_INFO("mipi hs send sequence\n");
	} else
		K3_FB_ERR("failed to init lcd!\n");

	if (k3fd->panel_info.bl_set_type & BL_SET_BY_PWM)
		K3_FB_INFO("Set backlight by pwm\n");
	else if (k3fd->panel_info.bl_set_type & BL_SET_BY_BLPWM)
		K3_FB_INFO("Set backlight by blpwm\n");
	else if (k3fd->panel_info.bl_set_type & BL_SET_BY_MIPI)
		K3_FB_INFO("Set backlight by mipi\n");
	else
		K3_FB_ERR("No such bl_set_type!\n");

	K3_FB_INFO("fb%d, -!\n", k3fd->index);

	return 0;
}

static int mipi_virtual_panel_off(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	K3_FB_INFO("fb%d!\n", k3fd->index);

	return 0;
}

static int mipi_virtual_panel_remove(struct platform_device *pdev)
{
	return 0;
}

static int mipi_virtual_panel_set_backlight(struct platform_device *pdev)
{
	struct k3_fb_data_type *k3fd = NULL;

	BUG_ON(pdev == NULL);
	k3fd = platform_get_drvdata(pdev);
	BUG_ON(k3fd == NULL);

	K3_FB_INFO("fb%d!\n", k3fd->index);

	return 0;
}

static struct k3_panel_info mipi_virtual_panel_info = {0};
static struct k3_fb_panel_data mipi_virtual_panel_data = {
	.panel_info = &mipi_virtual_panel_info,
	.set_fastboot = mipi_virtual_panel_set_fastboot,
	.on = mipi_virtual_panel_on,
	.off = mipi_virtual_panel_off,
	.remove = mipi_virtual_panel_remove,
	.set_backlight = mipi_virtual_panel_set_backlight,
};

static int mipi_virtual_panel_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct k3_panel_info *pinfo = NULL;
	struct class *k3_lcd_class;
	struct platform_device *this_dev = NULL;
	struct device_node *np = NULL;

	K3_FB_INFO("+.\n");

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_VIRTUAL_PANEL);
	if(!np) {
		K3_FB_ERR("NOT FOUND device node %s!\n", DTS_COMP_VIRTUAL_PANEL);
		ret = -EINVAL;
		goto err_return;
	}

	pdev->id = 1;
	/* init lcd panel info */
	pinfo = mipi_virtual_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct k3_panel_info));
	pinfo->xres = 1080;
	pinfo->yres = 1920;
	pinfo->width  = 76;  //mm
	pinfo->height = 135; //mm
	pinfo->type = PANEL_MIPI_CMD;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;

	pinfo->bl_min = 1;
	pinfo->bl_max = 255;
	pinfo->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF;

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->dirty_region_updt_support = 0;

	pinfo->sbl_support = 0;
	pinfo->smart_bl.strength_limit = 128;
	pinfo->smart_bl.calibration_a = 30;
	pinfo->smart_bl.calibration_b = 95;
	pinfo->smart_bl.calibration_c = 5;
	pinfo->smart_bl.calibration_d = 1;
	pinfo->smart_bl.t_filter_control = 5;
	pinfo->smart_bl.backlight_min = 480;
	pinfo->smart_bl.backlight_max = 4096;
	pinfo->smart_bl.backlight_scale = 0xff;
	pinfo->smart_bl.ambient_light_min = 14;
	pinfo->smart_bl.filter_a = 1738;
	pinfo->smart_bl.filter_b = 6;
	pinfo->smart_bl.logo_left = 0;
	pinfo->smart_bl.logo_top = 0;

	pinfo->ifbc_type = IFBC_TYPE_NON;

	pinfo->ldi.h_back_porch = 23;
	pinfo->ldi.h_front_porch = 50;
	pinfo->ldi.h_pulse_width = 20;
	pinfo->ldi.v_back_porch = 12;
	pinfo->ldi.v_front_porch = 14;
	pinfo->ldi.v_pulse_width = 4;

	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.dsi_bit_clk = 480;

	pinfo->pxl_clk_rate = 150*1000000;

	volt.vsp_gpio = of_get_named_gpio(np, "gpios", 0);  /*gpio_5_1, gpio_041*/
	volt.vsn_gpio = of_get_named_gpio(np, "gpios", 1);  /*gpio_5_2, gpio_042*/

	ldo_en.k3_vsp = k3_vsp_enable;
	ldo_en.k3_vsn = k3_vsn_enable;
	ldo_en.k3_vci = k3_vci_enable;
	ldo_en.k3_vddio = k3_vddio_enable;

	/* lcd gpio request */
	gpio_cmds_tx(virtual_lcd_gpio_request_cmds, \
		ARRAY_SIZE(virtual_lcd_gpio_request_cmds));

	/* alloc panel device data */
	ret = platform_device_add_data(pdev, &mipi_virtual_panel_data,
		sizeof(struct k3_fb_panel_data));
	if (ret) {
		K3_FB_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}

	this_dev = k3_fb_add_device(pdev);

	k3_lcd_class = class_create(THIS_MODULE, "k3_lcd");
	if (IS_ERR(k3_lcd_class)) {
		K3_FB_ERR("k3_lcd class create failed\n");
		return 0;
	}
	k3_lcd_class->dev_attrs = k3_lcd_class_attrs;

	if (IS_ERR(device_create(k3_lcd_class, &pdev->dev, 0, NULL, "lcd_info"))) {
		K3_FB_ERR("lcd_info create failed\n");
		class_destroy(k3_lcd_class);
		return 0;
	}

	K3_FB_INFO("-.\n");

	return 0;

err_device_put:
	platform_device_put(pdev);
err_return:
	return ret;
}

static const struct of_device_id k3_panel_match_table[] = {
	{
		.compatible = DTS_COMP_VIRTUAL_PANEL,
		.data = NULL,
	},
};
MODULE_DEVICE_TABLE(of, k3_panel_match_table);


static struct platform_driver this_driver = {
	.probe = mipi_virtual_panel_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_virtual_panel",
		.of_match_table = of_match_ptr(k3_panel_match_table),
	},
};

static int __init mipi_virtual_panel_init(void)
{
	int ret = 0;

	if (read_lcd_type() != UNKNOWN_LCD) {
		K3_FB_INFO("lcd type is not UNKNOWN_LCD, return!\n");
		return ret;
	}

	ret = platform_driver_register(&this_driver);
	if (ret) {
		K3_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_virtual_panel_init);
