

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "hw_csi.h"

#define I2S(i) container_of(i, sensor_t, intf)

extern struct hw_csi_pad hw_csi_pad;
static hwsensor_vtbl_t s_imx179_vtbl;

extern void set_sensor_module_id(int index, unsigned char module_id);
int imx179_config(hwsensor_intf_t* si, void  *argp);

static bool is_poweron = false;

struct sensor_power_setting imx179_power_setting[] = {

    //MINIISP CORE 1.1V
    {
        .seq_type = SENSOR_SUSPEND,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //MINIISP DVDD 1.1V
    {
        .seq_type = SENSOR_RST2,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //MINIISP IOVDD 1.8V
    {
        .seq_type = SENSOR_IOVDD,
        .data = (void*)"common-iovdd",
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

	//SCAM IOVDD 1.8V
//	{
//		.seq_type = SENSOR_IOVDD,
//		.data = (void*)"common-iovdd",
//		.config_val = LDO_VOLTAGE_1P8V,
//		.sensor_index = SENSOR_INDEX_INVALID,
//		.delay = 1,
//	},

	//SCAM AVDD 2.85V
	{
		.seq_type = SENSOR_AVDD,
		.data = (void*)"main-sensor-avdd",
		.config_val = LDO_VOLTAGE_V2P85V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},

	//SCAM DVDD1.2V
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
};

static sensor_t s_imx179 =
{
    .intf = { .vtbl = &s_imx179_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(imx179_power_setting),
            .power_setting = imx179_power_setting,
     },
};

static const struct of_device_id
s_imx179_dt_match[] =
{
	{
        .compatible = "huawei,imx179",
        .data = &s_imx179.intf,
    },
	{
    },
};

MODULE_DEVICE_TABLE(of, s_imx179_dt_match);

static struct platform_driver
s_imx179_driver =
{
	.driver =
    {
		.name = "huawei,imx179",
		.owner = THIS_MODULE,
		.of_match_table = s_imx179_dt_match,
	},
};

char const*
imx179_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}

int
imx179_power_up(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	if(!is_poweron) {
		ret = hw_sensor_power_up(sensor);
		cam_notice("+++imx179 power on!+++");
		is_poweron = true;
	} else {
		cam_notice("+++not power on+++");
	}
	return ret;
}

int
imx179_power_down(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	if(is_poweron) {
		ret = hw_sensor_power_down(sensor);
		cam_notice("---imx179 power off!---");
		is_poweron = false;
	} else {
		cam_notice("---not power off---");
	}
	return ret;
}

int imx179_csi_enable(hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);

	ret = hw_csi_pad.hw_csi_enable(sensor->board_info->csi_index, sensor->board_info->csi_lane, sensor->board_info->csi_mipi_clk);
	return ret;
}

int imx179_csi_disable(hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	ret = hw_csi_pad.hw_csi_disable(sensor->board_info->csi_index);
	return ret;
}

static int
imx179_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = I2S(si);
//	hwsensor_board_info_t *board_info = sensor->board_info;
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;
//	int sensor_index = CAMERA_SENSOR_INVALID;
//	int ret = 0;
//	int camif_id = -1;

    cam_info("%s TODO.", __func__);
#if 0
	if(0 == board_info->gpios[FSIN].gpio) {
		cam_err("%s gpio type[FSIN] is not actived.", __func__);
		ret = -1;
		goto out;
	}

    ret = gpio_request(board_info->gpios[FSIN].gpio, "camif_id");
    if(ret < 0) {
		cam_err("failed to request gpio[%d]", board_info->gpios[FSIN].gpio);
		goto out;
    }
    ret = gpio_direction_input(board_info->gpios[FSIN].gpio);
    if(ret < 0) {
		cam_err("failed to control gpio[%d]", board_info->gpios[FSIN].gpio);
		goto out_gpio;
    }

    ret = gpio_get_value(board_info->gpios[FSIN].gpio);
    if(ret < 0) {
		cam_err("failed to get gpio[%d]", board_info->gpios[FSIN].gpio);
	goto out_gpio;
	} else {
		camif_id = ret;
		cam_notice("%s camif id = %d.", __func__, camif_id);
	}

	if (camif_id != board_info->camif_id) {
		cam_notice("%s camera[%s] module is not match.", __func__, board_info->name);
		board_info->sensor_index = CAMERA_SENSOR_INVALID;
		ret = -1;
    } else {
		cam_notice("%s camera[%s] match successfully.", __func__, board_info->name);
		sensor_index = board_info->sensor_index;
		set_sensor_module_id(sensor->board_info->sensor_index, 1);
		ret = 0;
	}

out_gpio:
	gpio_free(board_info->gpios[FSIN].gpio);
out:
    cdata->data = sensor_index;
    return ret;
#endif
    set_sensor_module_id(sensor->board_info->sensor_index, 1);
    cdata->data = sensor->board_info->sensor_index;

    hwsensor_writefile(sensor->board_info->sensor_index,
		sensor->board_info->name);
	return 0;
}

static hwsensor_vtbl_t
s_imx179_vtbl =
{
	.get_name = imx179_get_name,
	.config = imx179_config,
	.power_up = imx179_power_up,
	.power_down = imx179_power_down,
	.match_id = imx179_match_id,
	.csi_enable = imx179_csi_enable,
	.csi_disable = imx179_csi_disable,
	.match_id = imx179_match_id,
};

int
imx179_config(
        hwsensor_intf_t* si,
        void  *argp)
{
	struct sensor_cfg_data *data;

	int ret =0;
	data = (struct sensor_cfg_data *)argp;
	cam_debug("imx179 cfgtype = %d",data->cfgtype);
	switch(data->cfgtype){
		case SEN_CONFIG_POWER_ON:
			ret = si->vtbl->power_up(si);
			break;
		case SEN_CONFIG_POWER_OFF:
			ret = si->vtbl->power_down(si);
			break;
		case SEN_CONFIG_WRITE_REG:
			break;
		case SEN_CONFIG_READ_REG:
			break;
		case SEN_CONFIG_WRITE_REG_SETTINGS:
			break;
		case SEN_CONFIG_READ_REG_SETTINGS:
			break;
		case SEN_CONFIG_ENABLE_CSI:
			ret = si->vtbl->csi_enable(si);
			break;
		case SEN_CONFIG_DISABLE_CSI:
			ret = si->vtbl->csi_disable(si);
			break;
		case SEN_CONFIG_MATCH_ID:
			ret = si->vtbl->match_id(si,argp);
			break;
		default:
                cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
			break;
	}
	cam_debug("%s exit",__func__);
	return ret;
}

static int32_t
imx179_platform_probe(
        struct platform_device* pdev)
{
	int rc = 0;
	cam_debug("enter %s",__func__);

	if (pdev->dev.of_node) {
		rc = hw_sensor_get_dt_data(pdev, &s_imx179);
		if (rc < 0) {
			cam_err("%s failed line %d\n", __func__, __LINE__);
			goto imx179_sensor_probe_fail;
		}
	} else {
		cam_err("%s imx179 of_node is NULL.\n", __func__);
		goto imx179_sensor_probe_fail;
	}

	rc = hwsensor_register(pdev, &s_imx179.intf);
imx179_sensor_probe_fail:
	return rc;
}

static int __init
imx179_init_module(void)
{
	cam_info("Enter: %s", __func__);
    return platform_driver_probe(&s_imx179_driver,
            imx179_platform_probe);
}

static void __exit
imx179_exit_module(void)
{
    platform_driver_unregister(&s_imx179_driver);
}

module_init(imx179_init_module);
module_exit(imx179_exit_module);
MODULE_DESCRIPTION("imx179");
MODULE_LICENSE("GPL v2");

