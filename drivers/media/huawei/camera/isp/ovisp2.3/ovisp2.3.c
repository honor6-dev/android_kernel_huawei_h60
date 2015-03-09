


#include <linux/compiler.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/huawei/camera.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-core.h>
#include <linux/pm_qos.h>

#include "hwisp_intf.h"
#include "cam_log.h"
#include "hw_isp_io.h"
#include "isp_ops.h"
#include <linux/huawei/dsm_pub.h>

static struct pm_qos_request qos_request_ddr;
static bool camera_poweron = false;
static bool is_camera_suspend = false;
extern bool camera_isp_open;

#define DDR_BLOCK_PROFILE	8000

struct dsm_client_ops ops1={
	.poll_state = NULL,
	.dump_func = NULL,
};

struct dsm_dev dev_ovisp23 = {
	.name = "dsm_ovisp23",
	.fops = &ops1,
	.buff_size = 30000,
};

struct dsm_client *client_ovisp23;

typedef enum _tag_ovisp23_constants
{
    OVISP23_MAX_STREAMS                         =   9,
} ovisp23_constants_t;
/*
typedef struct _tag_ovisp23_stream_res
{
    hwisp_stream_resource_t                     base;
} ovisp23_stream_res_t;
*/
static uint32_t buf_count = 0;
extern isp_hw_data_t isp_hw_data;
extern u8 *isp_firmware_addr;
extern u32 isp_firmware_size;
typedef struct _tag_ovisp23
{
	struct platform_device* pdev;
	hwisp_intf_t                                intf;
	hwisp_notify_intf_t*                        notify;
	//ovisp23_stream_res_t                        stream_res[OVISP23_MAX_STREAMS];
	atomic_t opened;
	struct list_head                            streams;
    char const*                                 name;
} ovisp23_t;

#define I2OV(i) container_of(i, ovisp23_t, intf)
#define Notify2OV(i) container_of(i, ovisp23_t, notify)
#define I2STM(i) container_of(i, hwisp_stream_t, intf)

bool is_ovisp23_poweron(void)
{
        return camera_poweron;
}

static int ovisp23_create_stream(hwisp_intf_t* i,struct video_device* vdev,hwisp_stream_info_t* info);
static int
ovisp23_stream_start(
        hwisp_intf_t* i,
        hwisp_stream_intf_t* info);

static int
ovisp23_stream_stop(
        hwisp_intf_t* i,
        hwisp_stream_intf_t* info);

char const*
ovisp23_get_name(
        hwisp_intf_t* i)
{
    ovisp23_t* ov = NULL;
    ov = I2OV(i);
    return ov->name;
}

static void k3_isp_fix_ddrfreq(unsigned int ddr_bandwidth)
{
	qos_request_ddr.pm_qos_class = 0;
	pm_qos_add_request(&qos_request_ddr, PM_QOS_MEMORY_THROUGHPUT, ddr_bandwidth);
	return;
}

static void k3_isp_release_ddrfreq(void)
{
	pm_qos_remove_request(&qos_request_ddr);
	return;
}

static int
ovisp23_power_on(
        hwisp_intf_t* i)
{
	int rc =0;
	ovisp23_t* ov = NULL;
	ov = I2OV(i);
	if (!atomic_read((&ov->opened))){
		rc = hw_isp_init(&ov->pdev->dev);
		if (rc) {
		cam_err("%s failed to init isp.\n", __func__);
		return rc;
		}

		rc = hw_isp_poweron();
		if(rc) {
		cam_err("%s failed to power on isp.\n", __func__);
		return rc;
		}
		k3_isp_fix_ddrfreq(DDR_BLOCK_PROFILE);
	}
	else{
		cam_notice("%s isp has been opened.\n", __func__);
	}
        atomic_add(1,&ov->opened);
        camera_poweron = true;
        return 0;
}

static int
ovisp23_power_off(
        hwisp_intf_t* i)
{
	int rc = 0;
	ovisp23_t* ov = NULL;
	ov = I2OV(i);

	if (atomic_dec_and_test(&(ov->opened))) {
		rc = hw_isp_poweroff();
		if (rc) {
			cam_err("failed to poweroff isp!");
			return rc;
		}

		rc = hw_isp_deinit(&ov->pdev->dev);
		if (rc) {
			cam_err("failed to deinit isp!");
			return rc;
		}
		k3_isp_release_ddrfreq();
	} else {
		cam_err("camera dev has been closed!");
	}
	camera_poweron = false;
    return 0;
}

static int
ovisp23_reset(
        hwisp_intf_t* i)
{
    ovisp23_t* ov = NULL;
	ov = I2OV(i);
    return 0;
}
/*
static int
ovisp23_load_firmware(
        hwisp_intf_t* i)
{
    ovisp23_t* ov = NULL;
	ov = I2OV(i);
    return 0;
}

static int
ovisp23_init_reg(
        hwisp_intf_t* i)
{
    ovisp23_t* ov = NULL;
	ov = I2OV(i);
    return 0;
}
*/
/*
static hwisp_stream_resource_t*
ovisp23_allocate_stream_resource(
        hwisp_intf_t* i,
        hwisp_stream_info_t* si)
{
    // TODO: just let testing code go
    ovisp23_t* ov = I2OV(i);
    return (hwisp_stream_resource_t*)&ov->stream_res[0];
}

static int
ovisp23_release_stream_resource(
        hwisp_intf_t* i,
        hwisp_stream_resource_t* sr)
{
    ovisp23_t* ov = NULL;
	ov = I2OV(i);
    return 0;
}
*/
static int ovisp23_read_reg(struct isp_cfg_reg *data)
{
	switch(data->val_bits)
	{
		case 8:
			data->val = ISP_GETREG8(data->reg);
			break;
		case 16:
			data->val = ISP_GETREG16(data->reg);
			break;
		case 32:
			data->val = ISP_GETREG32(data->reg);
			break;
		default:
			data->val = 0xffffffff;
			cam_err("%s wrong isp reg bits length = %d,set val to 0xffffffff",__func__,data->val_bits);
			return -1;
			break;
	}
	cam_debug("reg = 0x%x bits = %d val = 0x%x",data->reg,data->val_bits,data->val);
	// TODO support mask read
	return 0;
}

static int ovisp23_write_reg(struct isp_cfg_reg *data)
{
	cam_debug("reg = 0x%x bits = %d val = 0x%x",data->reg,data->val_bits,data->val);
	switch(data->val_bits)
	{
		case 8:
			ISP_SETREG8(data->reg,data->val);
			break;
		case 16:
			ISP_SETREG16(data->reg,data->val);
			break;
		case 32:
			ISP_SETREG32(data->reg,data->val);
			break;
		default:
			data->val = 0xffffffff;
			cam_err("%s wrong isp reg bits length = %d,set val to 0xffffffff",__func__,data->val_bits);
			return -1;
			break;
	}
	// TODO support mask write
	return 0;
}

static int ovisp23_read_reg_setting(struct isp_cfg_reg_array *data)
{
	int ret = 0;
	uint32_t data_length = sizeof(struct isp_cfg_reg)*data->length;
	uint32_t i;
	struct isp_cfg_reg *settings = NULL;

	settings = (struct isp_cfg_reg *)kzalloc(data_length, GFP_KERNEL);

	if(settings == NULL)
	{
		cam_err("allocate setting memory fail %s", __func__);
		return -ENOMEM;
	}
	if(copy_from_user((void *)settings, (void __user *) data->reg_array, data_length)){
		cam_err("%s copy_from_user error.", __func__);
		ret = -EFAULT;
		goto out;
	}

	if(data->length == 0){
		cam_info("setting length is zero");
		ret = -1;
		goto out;
	}

	for(i=0;i<data->length;i++){

		switch(settings[i].val_bits)
		{
			case 8:
				settings[i].val = ISP_GETREG8(settings[i].reg);
				break;
			case 16:
				settings[i].val = ISP_GETREG16(settings[i].reg);
				break;
			case 32:
				settings[i].val = ISP_GETREG32(settings[i].reg);
				break;
			default:
				settings[i].val = 0xffffffff;
				cam_err("%s wrong isp reg bits length = %d,set val to 0xffffffff",__func__,settings[i].val_bits);
				break;
		}
		// TODO support mask read
	}

	if (copy_to_user((void __user *)data->reg_array,
		settings, data_length)) {
		cam_err("%s copy_to_user error.\n", __func__);
		ret = -EFAULT;
		goto out;
	}
out:
	kfree(settings);
	return ret;
}

static int ovisp23_write_reg_setting(struct isp_cfg_reg_array *data)
{
	int ret = 0;
	uint32_t data_length = sizeof(struct isp_cfg_reg)*data->length;
       uint32_t i;
	 uint32_t temp =0;
	struct isp_cfg_reg *settings = NULL;

	settings = (struct isp_cfg_reg *)kzalloc(data_length, GFP_KERNEL);

	if(settings == NULL)
	{
		cam_err("allocate setting memory fail %s", __func__);
		return -ENOMEM;
	}
	if(copy_from_user((void *)settings, (void __user *) data->reg_array, data_length)){
		cam_err("%s copy_from_user error.", __func__);
		ret = -EFAULT;
		goto out;
	}

	if(data->length == 0){
		cam_info("setting length is zero");
		ret = -1;
		goto out;
	}

	for(i=0;i<data->length;i++){

		switch(settings[i].val_bits)
		{
			case 8:
				if(settings[i].mask!=0){
					temp = ISP_GETREG8(settings[i].reg)&settings[i].mask;
					ISP_SETREG8(settings[i].reg,(settings[i].val|temp));
				}else
					ISP_SETREG8(settings[i].reg,settings[i].val);
				break;
			case 16:
				if(settings[i].mask!=0){
					temp = ISP_GETREG16(settings[i].reg);
					temp = temp&settings[i].mask;
					ISP_SETREG16(settings[i].reg,(settings[i].val|temp));
				}else
					ISP_SETREG16(settings[i].reg,settings[i].val);
				break;
			case 32:
				if(settings[i].mask!=0){
					temp = ISP_GETREG32(settings[i].reg);
					temp = temp&settings[i].mask;
					ISP_SETREG32(settings[i].reg,(settings[i].val|temp));
				}else
					ISP_SETREG32(settings[i].reg,settings[i].val);
				break;
			default:
				settings[i].val = 0xffffffff;
				cam_err("%s wrong isp reg bits length = %d,set val to 0xffffffff",__func__,settings[i].val_bits);
				break;
		}
		// TODO support mask read
	}

out:
	kfree(settings);
	return ret;
}

static int ovisp23_config(hwisp_intf_t* i,void* cfg)
{
	int ret = 0;
	ovisp23_t* ov = NULL;
	struct isp_cfg_data *pcfg = NULL;

	pcfg = (struct isp_cfg_data *)cfg;
	ov = I2OV(i);

	switch(pcfg->cfgtype){
		case CONFIG_POWER_ON:
			if (!camera_poweron) {
				ret = ovisp23_power_on(i);
				camera_poweron = true;
			}
			break;
		case CONFIG_POWER_OFF:
			if (camera_poweron) {
				ret = ovisp23_power_off(i);
				camera_poweron = false;
			}
			break;
		case CONFIG_READ_REG:
			ret= ovisp23_read_reg(&pcfg->reg_s.reg);
			break;
		case CONFIG_WRITE_REG:
			ret= ovisp23_write_reg(&pcfg->reg_s.reg);
			break;
		case CONFIG_READ_REG_SETTINGS:
			ret= ovisp23_read_reg_setting(&pcfg->reg_s.reg_settings);
			break;
		case CONFIG_WRITE_REG_SETTINGS:
			ret= ovisp23_write_reg_setting(&pcfg->reg_s.reg_settings);
			break;
		case CONFIG_BUFFER_TO_ISP:
           	ret = isp_cmd_update_buffer_cmd(pcfg->data);
		    	break;
		case CONFIG_REPROCESS_CMD:
			ret = isp_reprocess_cmdset(&pcfg->reg_s.reprocess_params);
			break;
		default:
			break;
	}
    return ret;
}

static hwisp_vtbl_t
s_vtbl_ovisp23 =
{
    .get_name = ovisp23_get_name,
    .power_on = ovisp23_power_on,
    .power_off = ovisp23_power_off,
    .reset = ovisp23_reset,
//    .load_firmware = ovisp23_load_firmware,
//   .init_reg = ovisp23_init_reg,
    .config = ovisp23_config,
    .create_stream = ovisp23_create_stream,
    .stream_start = ovisp23_stream_start,
    .stream_stop = ovisp23_stream_stop,
};


static ovisp23_t
s_ovisp23 =
{
    .intf = { .vtbl = &s_vtbl_ovisp23, },
    .name = "ovisp2.3",
};

static const struct of_device_id
s_ovisp23_dt_match[] =
{
	{
        .compatible = "huawei,huawei_isp",
        .data = &s_ovisp23.intf,
    },
	{
    },
};

#define PIPELINE(id) (id)
void ovisp23_notify_sof( uint32_t id)
{
    hwisp_event_t isp_ev;
    isp_ev.kind = HWISP_SOF;
    isp_ev.data.sof.pipeline = PIPELINE(id);
    hwisp_notify_intf_sof(s_ovisp23.notify,&isp_ev);
}

void ovisp23_notify_eof(uint32_t id)
{
    hwisp_event_t isp_ev;
    isp_ev.kind = HWISP_EOF;
    isp_ev.data.eof.pipeline = PIPELINE(id);
   hwisp_notify_intf_eof(s_ovisp23.notify,&isp_ev);
}

void ovisp23_notify_cmd_ready(uint32_t cmd, uint32_t result)
{
    hwisp_event_t isp_ev;
    isp_ev.kind = HWISP_READY;
    isp_ev.data.ready.cmd = cmd;
    isp_ev.data.ready.result = result;
    cam_debug("%s cmd = %d result = %d",__func__,cmd,result);
    hwisp_notify_intf_cmd_ready(s_ovisp23.notify,&isp_ev);
}

static int
ovisp23_create_stream(
        hwisp_intf_t* i,
        struct video_device* vdev,
        hwisp_stream_info_t* info)
{
	ovisp23_t* ov = I2OV(i);
	hwisp_stream_t* stm = NULL;
	bool found = false;
	uint32_t attach_port, port;

	if (info->portid == ISP_PORT_VC0) {
		/* better to use spinlock to iterate list */
		for (port = ISP_PORT_WRITE0; port <= ISP_PORT_WRITE5; port++) {
			list_for_each_entry(stm, &ov->streams, node) {
				if (stm->port.id == port) {
					found  = true;
					break;
				}
			}

			if (found) { break; }
		}
		attach_port = (found) ? port : ISP_PORT_VC0;
		hw_isp_meta_data_ctrl(META_CTRL_ENABLE, attach_port);
	}

    return hwisp_create_stream(vdev, i, info, &ov->streams);
}

static int
ovisp23_stream_start(
        hwisp_intf_t* isp_intf,
        hwisp_stream_intf_t* stm_intf)
{
    ovisp23_t* ov = NULL;
    ov = I2OV(isp_intf);
    buf_count = 0;
    return 0;
}

static int
ovisp23_stream_stop(
        hwisp_intf_t* isp_intf,
        hwisp_stream_intf_t* stm_intf)
{
    ovisp23_t* ov = NULL;
    hwisp_stream_t* stm = I2STM(stm_intf);
    ov = I2OV(isp_intf);

	if (stm->port.id == ISP_PORT_VC0)
		hw_isp_meta_data_ctrl(META_CTRL_DISABLE, ISP_PORT_VC0);

    if(stm->port.use_phy_memory == 0){
    	if( ((stm->port.id)% 2) == 0){
		ISP_SETREG8(REG_PORT_READY(stm->port.id), ISP_GETREG8(REG_PORT_READY(stm->port.id)) & 0xfe);
	}else{
		ISP_SETREG8(REG_PORT_READY(stm->port.id), ISP_GETREG8(REG_PORT_READY(stm->port.id)) & 0xfb);
	}
        cam_debug("%s %d",__func__,stm->port.id);
    }
   memset(&isp_hw_data.mac_array[stm->port.id], 0, sizeof(struct ovisp32_mac));

   cam_debug("%s %d",__func__,stm->port.id);
    return 0;
}

hwisp_buf_t* ovisp23_get_buf_from_readyq(isp_port_e port)
{
    struct list_head* pos = NULL;
    hwisp_stream_t* stm = NULL;
    hwisp_buf_t* buf = NULL;
    list_for_each(pos, &s_ovisp23.streams) {
        hwisp_stream_intf_t* intf = hwisp_stream_get_by_node(pos);
        if (!intf) {
            continue;
        }
        stm = I2STM(intf);
/*    stm can't be null
        if(!stm)
        {
            cam_err("stm = NULL");
            continue;
        }*/
        if(stm->port.id != port){
            continue;
        }
        /*
           if(port == 0 && buf_count%2 ==1){
           HWCAM_CFG_INFO("no more empty buf in readyq!");
           return NULL;
           }
           if(port == 1 && buf_count%2 ==0){
           return NULL;
           }
           buf_count++;
           */
        //if(port !=0)
        //return NULL;
        buf = hwisp_stream_intf_get_buf(intf);
        if (!buf) {
            HWCAM_CFG_INFO("no more empty buf in readyq for stream[%d %d]!",stm->stream_type,stm->port.id);
            return NULL;
        }
        buf->info.port.pix_format = stm->port.pix_format;
        buf->info.port.use_phy_memory = stm->port.use_phy_memory;
        cam_debug("%s buf phyadd = %lx",__func__,buf->info.y_addr_phy);
        return buf;
    }
    cam_err("There is no stream match the port %d",port);
    return NULL;
}


int ovisp23_put_buf_to_doneq(isp_port_e port,hwisp_buf_t* buf)
{
    struct list_head* pos = NULL;
    hwisp_stream_t* stm;
    list_for_each(pos, &s_ovisp23.streams) {
        hwisp_stream_intf_t* intf = hwisp_stream_get_by_node(pos);
        if (!intf) {
            continue;
        }

        stm = I2STM(intf);
/*	stm can't be null
        if(!stm)
        {
            cam_err("stm = NULL");
            continue;
        }*/
        if(stm->port.id != port){
            continue;
        }
        if (buf) {
            hwisp_stream_intf_buf_done(intf, buf);
            cam_debug("%s buf phyadd = %lx",__func__,buf->info.y_addr_phy);
        return 0;
        }
    }
    return 0;
}

int ovisp23_put_buf_to_idelq(isp_port_e port,hwisp_buf_t* buf)
{
    struct list_head* pos = NULL;
    hwisp_stream_t* stm;
    list_for_each(pos, &s_ovisp23.streams) {
        hwisp_stream_intf_t* intf = hwisp_stream_get_by_node(pos);
        if (!intf) {
            continue;
        }
        stm = I2STM(intf);
/*	stm can't be null
        if(!stm)
        {
            cam_err("stm = NULL");
            continue;
        }*/
        if(stm->port.id != port){
            continue;
        }
        if (buf) {
            hwisp_stream_intf_put_buf(intf, buf);
            cam_debug("%s buf phyadd = %lx",__func__,buf->info.y_addr_phy);
            return 0;
        }
    }
    return 0;
}

hwisp_stream_t* ovisp23_get_stream(isp_port_e port)
{
    struct list_head* pos = NULL;
    hwisp_stream_t* stm = NULL;
	bool found = false;

    list_for_each(pos, &s_ovisp23.streams) {
        stm = container_of(pos, hwisp_stream_t, node);
		if (stm->port.id == port) {
			found  = true;
			break;
		}
    }

    return (found ? stm : NULL);
}

static int hwcamera_suspend(struct platform_device *pdev, pm_message_t state)
{
	int rc = 0;
	cam_notice("%s+\n",__func__);
	HWCAM_CFG_INFO("Camera suspend");
	if (camera_poweron) {
		rc = hw_isp_poweroff();
		camera_poweron = false;
              is_camera_suspend = true;
		k3_isp_release_ddrfreq();
	}

	cam_notice("%s-\n",__func__);
	return rc;
}


static int hwcamera_resume(struct platform_device *pdev)
{
	int rc = 0;
	cam_notice("%s+\n",__func__);
	HWCAM_CFG_INFO("Camera resume");
	if (is_camera_suspend && !camera_poweron) {
		k3_isp_fix_ddrfreq(DDR_BLOCK_PROFILE);
		hw_isp_poweron();
		camera_poweron = true;
		is_camera_suspend = false;
	}
	cam_notice("%s-\n",__func__);
	return rc;
}
MODULE_DEVICE_TABLE(of, s_ovisp23_dt_match);

static struct platform_driver
s_ovisp23_driver =
{
	.suspend = hwcamera_suspend,
	.resume = hwcamera_resume,
	.driver =
    {
		.name = "huawei,huawei_isp",
		.owner = THIS_MODULE,
		.of_match_table = s_ovisp23_dt_match,
	},
};

static int32_t
ovisp23_platform_probe(
        struct platform_device* pdev)
{
   int32_t ret;
	INIT_LIST_HEAD(&s_ovisp23.streams);
   ret = hwisp_register(pdev,&s_ovisp23.intf, &s_ovisp23.notify);
   if(ret == 0){
	s_ovisp23.pdev = pdev;
	atomic_set(&s_ovisp23.opened, 0);
	ret = hw_alloc_firmware_memory();
	if (ret < 0) {
		ret = -ENOMEM;
		cam_err("%s: camera init alloc memory for firmware fail ret=%d.\n", __func__, ret);
	}
	if(client_ovisp23 == NULL){
        client_ovisp23 = dsm_register_client(&dev_ovisp23);
    }
   }
   else
   	cam_err("ret = %d\n",ret);
   return ret;
}

static int __init
ovisp23_init_module(void)
{
    return platform_driver_probe(&s_ovisp23_driver,
            ovisp23_platform_probe);
}

static void __exit
ovisp23_exit_module(void)
{
    hw_free_firmware_memory();
    platform_driver_unregister(&s_ovisp23_driver);
}

module_init(ovisp23_init_module);
module_exit(ovisp23_exit_module);
MODULE_DESCRIPTION("ovisp23");
MODULE_LICENSE("GPL v2");

