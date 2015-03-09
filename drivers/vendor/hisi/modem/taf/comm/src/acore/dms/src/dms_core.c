

/*****************************************************************************
  1 头文件包含
*****************************************************************************/

#include "vos.h"
#include "msp_errno.h"
#include <dms.h>
#include "dms_core.h"
#include "SOCPInterface.h"
#include "PsLib.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/

/*lint -e767 -e960*/
#define THIS_FILE_ID PS_FILE_ID_DMS_CORE_C
/*lint +e767 +e960*/


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

DMS_MAIN_INFO                           g_stDmsMainInfo = {0};
VOS_UINT32                              g_usbPlugFlag = 0;

VOS_UINT32                              g_ulPcuiRxSem;
VOS_UINT32                              g_ulCtrlRxSem;

#if (VOS_OS_VER == VOS_WIN32)
static const struct file_operations     g_stPortCfgOps;
#else
static const struct file_operations     g_stPortCfgOps =
{
    .owner      = THIS_MODULE,
    .write      = DMS_WritePortCfgFile,
    .read       = DMS_ReadPortCfgFile,
};
#endif

DMS_NLK_ENTITY_STRU                     g_stDmsNlkEntity = {0};

#if (VOS_OS_VER == VOS_WIN32)
static struct netlink_kernel_cfg        g_stDmsNlkCfg;
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static struct netlink_kernel_cfg        g_stDmsNlkCfg =
{
    .input      = DMS_NLK_Input,
};
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0) */
#endif /* VOS_OS_VER == VOS_WIN32 */


/*****************************************************************************
  3 外部函数声明
*****************************************************************************/

extern VOS_VOID At_MsgProc(MsgBlock* pMsg);
extern VOS_UINT32 At_PidInit(enum VOS_INIT_PHASE_DEFINE enPhase);


/*****************************************************************************
  4 函数实现
*****************************************************************************/

/*****************************************************************************
 函 数 名  : initDmsHdlcInit
 功能描述  : 初始化物理通道与SOCP的对应关系
 输入参数  :

 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月72日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_VOID initDmsHdlcInit(VOS_VOID)
{
#if (RAT_MODE != RAT_GU)
    /*DIAG通道 */
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_TCP_20248].ucChanStat =0 ;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_TCP_20248].ucHdlcFlag = 1;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_TCP_20248].ulCodeDesChanId = SOCP_CODER_DST_LOM_IND;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_TCP_20248].ulDecodeSrcChanId =SOCP_DECODER_SRC_LOM ;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_TCP_20248].ulRecivBufLen = 0;

    /*DIAG与AT混传的通道 */
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM1_DIAG_CTRL].ucChanStat = 0;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM1_DIAG_CTRL].ucHdlcFlag = 1;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM1_DIAG_CTRL].ulCodeDesChanId = SOCP_CODER_DST_LOM_CNF;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM1_DIAG_CTRL].ulDecodeSrcChanId = SOCP_DECODER_SRC_LOM;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM1_DIAG_CTRL].ulRecivBufLen = 0;

    /*诊断上报专用 */
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM2_DIAG_APP].ucChanStat = 0;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM2_DIAG_APP].ucHdlcFlag = 1;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM2_DIAG_APP].ulCodeDesChanId = SOCP_CODER_DST_LOM_IND;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM2_DIAG_APP].ulDecodeSrcChanId = 0;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM2_DIAG_APP].ulRecivBufLen = 0;
#endif
}

VOS_VOID initDmsMainInfo(VOS_VOID)
{
    VOS_UINT32 ulport;

    VOS_MemSet(g_ast_dsm_debug_info_table, 0, sizeof(g_ast_dsm_debug_info_table));

    /*VOS_MemSet(&g_stDmsMainInfo, 0, sizeof(DMS_MAIN_INFO)); */

    g_stDmsMainInfo.ucDmsVcom1SleepFlag = TRUE;
    g_stDmsMainInfo.ucDmsVcomATSleepFlag = TRUE;
    g_stDmsMainInfo.ucDmsVcomUartSleepFlag = TRUE;
    g_stDmsMainInfo.pfnRdDtaCallback   = NULL;
    g_stDmsMainInfo.pfnConnectCallBack = NULL;

    /*初始化所有端口的handle*/
    for(ulport = 0; ulport < EN_DMS_BEARER_LAST; ulport++)
    {
        g_stDmsMainInfo.stPhyProperty[ulport].slPortHandle = UDI_INVALID_HANDLE;
    }


    /*最终逻辑通道使能的通道属性*/
#if (VOS_WIN32 == VOS_OS_VER)
    g_stDmsMainInfo.stLogicPhy[EN_DMS_CHANNEL_DIAG].aenPhyChan = EN_DMS_BEARER_TCP_20248;
#else
    g_stDmsMainInfo.stLogicPhy[EN_DMS_CHANNEL_DIAG].aenPhyChan =EN_DMS_BEARER_USB_COM1_DIAG_CTRL;
#endif

    /*物理通道默认属性，存在多个物理通道有能力对应一个逻辑通道*/
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_TCP_20248].aenLogicChan = EN_DMS_CHANNEL_DIAG;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM1_DIAG_CTRL].aenLogicChan = EN_DMS_CHANNEL_DIAG;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM2_DIAG_APP].aenLogicChan  = EN_DMS_CHANNEL_DIAG;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM4_AT].aenLogicChan  = EN_DMS_CHANNEL_AT;
    g_stDmsMainInfo.stPhyProperty[EN_DMS_BEARER_USB_COM_CTRL].aenLogicChan  = EN_DMS_CHANNEL_AT;

    g_stDmsMainInfo.bPortCfgFlg     = FALSE;
    g_stDmsMainInfo.bPortOpenFlg    = FALSE;
    g_stDmsMainInfo.ulPortCfgValue  = DMS_TEST_MODE;

    /*初始化物理通道与SOCP通道的对应关系*/
    initDmsHdlcInit();

    /*初始化AT通道使用的静态内存*/
    Dms_StaticBufInit();

    /* 创建文件 */
    DMS_InitPorCfgFile();
}


/*****************************************************************************
 函 数 名  : dmsGetConnStaFun
 功能描述  : 获取通道连接处理函数
 输入参数  :

 输出参数  :
 返 回 值  : VOS_NULL/回调函数
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

DMS_CONNECT_STA_PFN dmsGetConnStaFun(VOS_VOID)
{
    DMS_MAIN_INFO * pstMainInfo = dmsGetMainInfo();

    if (pstMainInfo == NULL)
    {
        return NULL;
    }
    else
    {
        return pstMainInfo->pfnConnectCallBack;
    }
}

/*****************************************************************************
 函 数 名  : dms_SetConnectStaCallback
 功能描述  : 通道连接事件处理注册函数
 输入参数  : pfnReg: 回调函数指针

 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_VOID dms_SetConnectStaCallback(DMS_CONNECT_STA_PFN pfnReg)
{
    DMS_MAIN_INFO * pstMainInfo = dmsGetMainInfo();

    if (NULL == pfnReg)
    {
        return ;
    }

    pstMainInfo->pfnConnectCallBack = pfnReg;

    return ;
}

/*****************************************************************************
 函 数 名  : dmsGetReadFun
 功能描述  : 获取AT读回调函数
 输入参数  :
 输出参数  :
 返 回 值  : 回调函数指针
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

DMS_READ_DATA_PFN dmsGetReadFun(VOS_VOID)
{
    return g_stDmsMainInfo.pfnRdDtaCallback;
}

/*****************************************************************************
 函 数 名  : dmsGetMainInfo
 功能描述  : 获取DMS全局变量指针
 输入参数  :
 输出参数  :
 返 回 值  : 全局变量指针
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

DMS_MAIN_INFO * dmsGetMainInfo(VOS_VOID)
{
    return &g_stDmsMainInfo;
}

/*****************************************************************************
函 数 名  : dmsgetPhyBearProperty
功能描述  : 获取物理通道数据结构指针
输入参数  :
输出参数  :
返 回 值  : 结构体指针
调用函数  :
被调函数  :
修改历史  :
    1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function

*****************************************************************************/

 DMS_PHY_BEAR_PROPERTY_STRU* dmsgetPhyBearProperty(VOS_VOID)
{
    DMS_MAIN_INFO * pstMainInfo = NULL;

    pstMainInfo = dmsGetMainInfo();

    return pstMainInfo->stPhyProperty;
}


/*****************************************************************************
 函 数 名  : dms_Init
 功能描述  : DMS模块初始化函数
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_VOID dms_Init(VOS_VOID)
{
    initDmsMainInfo();
    if (VOS_OK != VOS_SmCCreate( "PCUIRX", 0, VOS_SEMA4_FIFO, &g_ulPcuiRxSem))
    {
        vos_printf("dms_Init: creat pcui rx sem fail!\n");;
    }

    if (VOS_OK != VOS_SmCCreate( "CTRLRX", 0, VOS_SEMA4_FIFO, &g_ulCtrlRxSem))
    {
        vos_printf("dms_Init: creat ctrl rx sem fail!\n");;
    }
    BSP_USB_RegUdiEnableCB(dms_UsbEnableEvtProc);

    BSP_USB_RegUdiDisableCB(dms_UsbDisableEvtProc);

    return ;
}


VOS_VOID dms_UsbDisableEvtProc(VOS_VOID)
{
    VOS_UINT32                          ulRet;

    g_usbPlugFlag = 0;

    DMS_DEBUG_SDM_FUN(EN_SDM_DMS_DISABLE,0, 0, 0);

    /* nv值为1表示有at sever，非1表示无at sever，无at sever关闭pcui和ctrl口 */
    if (TRUE == g_stDmsMainInfo.bPortOpenFlg)
    {
        /*关闭AT PCUI通道*/
        ulRet = dmsVcomAtPcuiClose();
        if(ERR_MSP_SUCCESS!=ulRet)
        {
            DMS_DEBUG_SDM_FUN(EN_SDM_DMS_DISABLE_ERR,0, 0, 3);
        }

        /*关闭AT CTRL通道*/
        ulRet = dmsVcomCtrlClose();
        if(ERR_MSP_SUCCESS!=ulRet)
        {
            DMS_DEBUG_SDM_FUN(EN_SDM_DMS_DISABLE_ERR,0, 0, 5);
        }
    }

    /*关闭NDIS TRCL通道*/
    ulRet =  dms_NcmClose();
    if(ERR_MSP_SUCCESS!=ulRet)
    {
        DMS_DEBUG_SDM_FUN(EN_SDM_DMS_DISABLE_ERR,0, 0, 4);
    }

    return ;
}



VOS_VOID dms_UsbEnableEvtProc(VOS_VOID)
{
    VOS_UINT32                          ulRet;
    NVE_INFO_S                          stAtServerNv;
    VOS_INT32                           lReadNvRet;

    VOS_MemSet(&stAtServerNv, 0, sizeof(NVE_INFO_S));
    VOS_MemCpy(stAtServerNv.nv_name, "ATSERV", sizeof("ATSERV"));
    stAtServerNv.nv_number      = NVE_AT_SERVER_INDEX;
    stAtServerNv.nv_operation   = NVE_READ_OPERATE;
    stAtServerNv.valid_size     = 1;

    g_usbPlugFlag = 1;

    DMS_DEBUG_SDM_FUN(EN_SDM_DMS_INIT,0, 0, 0);

    /* NVE只读取一次，读取后不再读取 */
    if (FALSE == g_stDmsMainInfo.bPortCfgFlg)
    {
        lReadNvRet = DRV_NVE_ACCESS(&stAtServerNv);
        g_dms_debug_atserv_nv_info.lOperatRet       = lReadNvRet;
        g_dms_debug_atserv_nv_info.ulNvValue        = stAtServerNv.nv_data[0];

        g_stDmsMainInfo.bPortCfgFlg                 = TRUE;

        /* nv值为1表示有at sever，非1表示无at sever，无at sever打开pcui和ctrl口 */
        if(1 != stAtServerNv.nv_data[0] || ERR_MSP_SUCCESS != lReadNvRet)
        {
            g_stDmsMainInfo.ulPortCfgValue = DMS_TEST_MODE;
        }
        else
        {
            g_stDmsMainInfo.ulPortCfgValue = DMS_NORMAL_MODE;
        }
    }

    if (DMS_TEST_MODE == g_stDmsMainInfo.ulPortCfgValue)
    {
        g_stDmsMainInfo.bPortOpenFlg = TRUE;

        /*打开 AT PCUI 通道*/
        ulRet = dmsVcomAtPcuiOpen();
        if(ERR_MSP_SUCCESS!=ulRet)
        {
            DMS_DEBUG_SDM_FUN(EN_SDM_DMS_INIT_ERR,0, 0, 3);
        }

        /*打开 AT CTRL 通道*/
        ulRet = dmsVcomCtrolOpen();
        if(ERR_MSP_SUCCESS!=ulRet)
        {
            DMS_DEBUG_SDM_FUN(EN_SDM_DMS_INIT_ERR,0, 0, 5);
        }
    }

    /*打开 NDIS CTRL 通道*/
    ulRet =  dms_NcmCfg();
    if(ERR_MSP_SUCCESS!=ulRet)
    {
        DMS_DEBUG_SDM_FUN(EN_SDM_DMS_INIT_ERR,0, 0, 4);
    }

    g_ulNdisCfgFlag = 1;

    return ;

}

/*****************************************************************************
 函 数 名  : dms_UdiRead
 功能描述  : udi_read封装
 输入参数  : enCOM: 通道ID
             ucRedBuf: 内存buf
             ulToReadSize:需要读取长度
             pulRealySize:实际读取长度
 输出参数  :
 返 回 值  : ERR_MSP_FAILURE/ERR_MSP_SUCCESS
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_UINT32 dms_UdiRead(DMS_PHY_BEAR_ENUM enCOM,VOS_UINT8 * ucRedBuf, VOS_UINT32 ulToReadSize,  VOS_UINT32 * pulRealySize)
{
    VOS_UINT32 ret = ERR_MSP_SUCCESS;
    VOS_INT32  slReadlen = 0;

    if (UDI_INVALID_HANDLE != g_stDmsMainInfo.stPhyProperty[enCOM].slPortHandle)
    {
        DMS_DEBUG_SDM_FUN(EN_SDM_DMS_UDI_READ_START, (VOS_UINT32)slReadlen, (VOS_UINT32)ulToReadSize, (VOS_UINT32)ucRedBuf);

        slReadlen = udi_read(g_stDmsMainInfo.stPhyProperty[enCOM].slPortHandle,ucRedBuf,ulToReadSize);

        DMS_DEBUG_SDM_FUN(EN_SDM_DMS_UDI_READ_END, (VOS_UINT32)slReadlen, (VOS_UINT32)ulToReadSize, (VOS_UINT32)ucRedBuf);

        /*串口设备失败的返回值并非都是-1*/
        if (slReadlen < 0 )
        {
            * pulRealySize = 0;
            ret = ERR_MSP_FAILURE;
        }
        else
        {
            * pulRealySize = (VOS_UINT32)slReadlen;
        }
    }
    else
    {
        ret = ERR_MSP_FAILURE;
    }
    return ret;
}


/*****************************************************************************
 函 数 名  : dms_GetPortHandle
 功能描述  : 判断通道handle是否有效
 输入参数  : enChan: 通道ID

 输出参数  :
 返 回 值  : FALSE/TRUE
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_BOOL dms_GetPortHandle(DMS_PHY_BEAR_ENUM enChan)
{
    if (UDI_INVALID_HANDLE == g_stDmsMainInfo.stPhyProperty[enChan].slPortHandle)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


/*****************************************************************************
 函 数 名  : dmsAtPcuiTaskSetSleepFlag
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_VOID dmsAtPcuiTaskSetSleepFlag(VOS_BOOL ucSleepFlag)
{
    DMS_MAIN_INFO* pstMainInfo = dmsGetMainInfo();

    pstMainInfo->ucDmsVcomATSleepFlag = ucSleepFlag;
    return ;
}

/*****************************************************************************
 函 数 名  : dmsAtPcuiTaskGetSleepFlag
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_BOOL dmsAtPcuiTaskGetSleepFlag(VOS_VOID)
{
    DMS_MAIN_INFO* pstMainInfo = dmsGetMainInfo();

    return  pstMainInfo->ucDmsVcomATSleepFlag ;

}

/*****************************************************************************
 函 数 名  : dmsAtCtrlTaskSetSleepFlag
 功能描述  :
 输入参数  :

 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_VOID dmsAtCtrlTaskSetSleepFlag(VOS_BOOL ucSleepFlag)
{
    DMS_MAIN_INFO* pstMainInfo = dmsGetMainInfo();

    pstMainInfo->ucDmsVcomUartSleepFlag = ucSleepFlag;

    return ;
}

/*****************************************************************************
 函 数 名  : dmsAtCtrlTaskGetSleepFlag
 功能描述  :
 输入参数  :

 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/

VOS_BOOL dmsAtCtrlTaskGetSleepFlag(VOS_VOID)
{
    DMS_MAIN_INFO* pstMainInfo = dmsGetMainInfo();

    return  pstMainInfo->ucDmsVcomUartSleepFlag ;

}

/*****************************************************************************
 函 数 名  : DMS_DsFidInit
 功能描述  : dms FID 初始化函数
 输入参数  :

 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/
VOS_UINT32 DMS_DsFidInit(enum VOS_INIT_PHASE_DEFINE ip)
{
    VOS_UINT32 ulRelVal = 0;

    switch (ip)
    {
    case VOS_IP_LOAD_CONFIG:

        dms_Init();

        ulRelVal = VOS_RegisterPIDInfo(WUEPS_PID_AT, (Init_Fun_Type) At_PidInit, (Msg_Fun_Type) At_MsgProc);

        if (ulRelVal != VOS_OK)
        {
            return VOS_ERR;
        }

        ulRelVal = VOS_RegisterTaskPrio(MSP_APP_DS_FID, DMS_APP_DS_TASK_PRIORITY);
        if (ulRelVal != VOS_OK)
        {
            return VOS_ERR;
        }

        ulRelVal = VOS_RegisterSelfTask(MSP_APP_DS_FID, (VOS_TASK_ENTRY_TYPE)dms_AtPcuiTask, VOS_PRIORITY_M2, 8196);    /*lint !e64 */
        if (VOS_NULL_BYTE  == ulRelVal)
        {
            return VOS_ERR;
        }

        /*CTRL自处理任务注册 */
        ulRelVal = VOS_RegisterSelfTask(MSP_APP_DS_FID, (VOS_TASK_ENTRY_TYPE)dms_VcomCtrlAtTask, VOS_PRIORITY_M2,8196);    /*lint !e64 */
        if (VOS_NULL_BYTE  == ulRelVal)
        {
            return VOS_ERR;
        }

        break;

    default:
        break;
    }

    if(ulRelVal == 0)
    {

    }
    return VOS_OK;
}

/*****************************************************************************
 函 数 名  : DMS_InitPorCfgFile
 功能描述  : OnDemand虚拟文件读实现
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 成功或失败

 修改历史      :
  1.日    期   : 2013年10月25日
    作    者   : z6057
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 DMS_InitPorCfgFile(VOS_VOID)
{
/*lint -e960 */
    if(VOS_NULL_PTR == proc_create("portcfg", DMS_VFILE_CRT_LEVEL, VOS_NULL_PTR, &g_stPortCfgOps))
    {
        LogPrint("\r\n DMS_InitPorCfgFile: proc_create Return NULL \r\n");

        return VOS_ERR;
    }
/*lint +e960 */

    return VOS_OK;
}

/*****************************************************************************
 函 数 名  : DMS_ReadPortCfgFile
 功能描述  : PortCfg虚拟文件读实现
 输入参数  : file --- 文件句柄
             buf  --- 用户空间
             ppos --- 文件偏移，参数未使用
 输出参数  : 无
 返 回 值  : 成功或失败

 修改历史      :
  1.日    期   : 2013年10月25日
    作    者   : z6057
    修改内容   : 新生成函数

*****************************************************************************/
ssize_t DMS_ReadPortCfgFile(
    struct file                        *file,
    char __user                        *buf,
    size_t                              len,
    loff_t                             *ppos
)
{
    VOS_CHAR                            acModeTemp[DMS_PORTCFG_FILE_LEN];
    VOS_INT32                           lRlst;
    VOS_UINT32                          ulLength;
    if (*ppos > 0)
    {
        return 0;
    }

    VOS_MemSet(acModeTemp, 0x00, DMS_PORTCFG_FILE_LEN);

    VOS_sprintf((VOS_CHAR *)acModeTemp, "%d", g_stDmsMainInfo.ulPortCfgValue);

    ulLength        = VOS_StrLen(acModeTemp);
    len             = PS_MIN(len, ulLength);

    /*拷贝内核空间数据到用户空间上面*/
    lRlst           = copy_to_user(buf,(VOS_VOID *)acModeTemp, len);

    if (lRlst < 0)
    {
        return -EPERM;
    }

    *ppos += (loff_t)len;

    return (ssize_t)len;
}

/*****************************************************************************
 函 数 名  : DMS_WritePortCfgFile
 功能描述  : PortCfg虚拟文件写实现
 输入参数  : file ----- 文件句柄
             buf  ----- 用户空间数据
             lLength -- 用户数据长度
             ppos - ----文件偏移，参数未使用
 输出参数  : 无
 返 回 值  : 成功或失败

 修改历史      :
  1.日    期   : 2013年10月25日
    作    者   : z6057
    修改内容   : 新生成函数

*****************************************************************************/
ssize_t DMS_WritePortCfgFile(
    struct file                        *file,
    const char __user                  *buf,
    size_t                              len,
    loff_t                             *ppos
)
{
    VOS_CHAR                            acModeTemp[DMS_PORTCFG_FILE_LEN];
    VOS_UINT32                          ulRlst;
    VOS_UINT32                          ulStrLen;
    VOS_UINT32                          i;
    VOS_UINT32                          ulValue;

    ulValue = 0;
    VOS_MemSet(acModeTemp, 0x00, DMS_PORTCFG_FILE_LEN);

    if (len >= DMS_PORTCFG_FILE_LEN)
    {
        return -ENOSPC;
    }

    /*拷贝用户空间数据到内核空间上面*/
    ulRlst = copy_from_user((VOS_VOID *)acModeTemp, (VOS_VOID *)buf, len);
    if (ulRlst > 0)
    {
        return -EFAULT;
    }

    acModeTemp[len] = '\0';

    ulStrLen = VOS_StrLen(acModeTemp);

    for ( i = 0; i < ulStrLen; i++ )
    {
        if ( (acModeTemp[i] >= '0') && (acModeTemp[i] <= '9') )
        {
            ulValue = (ulValue * 10) + (acModeTemp[i] - '0');
        }
    }

    g_stDmsMainInfo.ulPortCfgValue  = ulValue;

    /* 如果已经写过这个文件，则以写的值为准，后续不需要再读NVE */
    g_stDmsMainInfo.bPortCfgFlg     = TRUE;

    return (ssize_t)len;
}


VOS_UINT32 DMS_RegOmChanDataReadCB(
    DMS_OM_CHAN_ENUM_UINT32             enChan,
    DMS_OM_CHAN_DATA_READ_CB_FUNC       pFunc
)
{
    DMS_NLK_OM_CHAN_PROPERTY_STRU      *pstOmChanProp = VOS_NULL_PTR;

    /* 检查通道号和函数指针 */
    if ((enChan >= DMS_OM_CHAN_BUTT) || (VOS_NULL_PTR == pFunc))
    {
        printk(KERN_ERR "[%s][LINE: %d] Invalid channel %d.\n",
            __func__, __LINE__, (VOS_INT)enChan);
        return VOS_ERR;
    }

    /* 设置通道数据回调函数 */
    pstOmChanProp = DMS_GET_NLK_OM_CHAN_PROP(enChan);
    pstOmChanProp->pDataFunc = pFunc;

    return VOS_OK;
}


VOS_UINT32 DMS_RegOmChanEventCB(
    DMS_OM_CHAN_ENUM_UINT32             enChan,
    DMS_OM_CHAN_EVENT_CB_FUNC           pFunc
)
{
    DMS_NLK_OM_CHAN_PROPERTY_STRU      *pstOmChanProp = VOS_NULL_PTR;

    /* 检查通道号 */
    if ((enChan >= DMS_OM_CHAN_BUTT) || (VOS_NULL_PTR == pFunc))
    {
        printk(KERN_ERR "[%s][LINE: %d] Invalid channel %d.\n",
            __func__, __LINE__, (VOS_INT)enChan);
        return VOS_ERR;
    }

    /* 设置通道事件回调函数 */
    pstOmChanProp = DMS_GET_NLK_OM_CHAN_PROP(enChan);
    pstOmChanProp->pEvtFunc = pFunc;

    return VOS_OK;
}


VOS_UINT32 DMS_WriteOmData(
    DMS_OM_CHAN_ENUM_UINT32             enChan,
    VOS_UINT8                          *pucData,
    VOS_UINT32                          ulLength
)
{
    VOS_UINT8                          *pucMem = VOS_NULL_PTR;
    VOS_UINT32                          ulMemNum;
    VOS_UINT32                          ulLastMemSize;
    VOS_UINT32                          ulCnt;

    DMS_DBG_NLK_DL_TOTAL_PKT_NUM(1);

    /* 检查通道 */
    if (enChan >= DMS_OM_CHAN_BUTT)
    {
        DMS_DBG_NLK_DL_ERR_CHAN_PKT_NUM(1);
        return VOS_ERR;
    }

    /* 检查数据 */
    if ((VOS_NULL_PTR == pucData) || (0 == ulLength))
    {
        DMS_DBG_NLK_DL_ERR_PARA_PKT_NUM(1);
        return VOS_ERR;
    }

    DMS_DBG_NLK_DL_NORM_CHAN_PKT_NUM(enChan, 1);

    /* 对数据分块, 避免一次发送过多数据 */
    pucMem        = pucData;
    ulMemNum      = ulLength / DMS_GET_NLK_DATA_SIZE();
    ulLastMemSize = ulLength % DMS_GET_NLK_DATA_SIZE();

    /* 发送固定大小数据块 */
    for (ulCnt = 0; ulCnt < ulMemNum; ulCnt++)
    {
        DMS_NLK_Send(DMS_GET_NLK_PHY_BEAR(enChan), DMS_GET_NLK_MSG_TYPE(enChan), pucMem, DMS_GET_NLK_DATA_SIZE());
        pucMem += DMS_GET_NLK_DATA_SIZE();
    }

    /* 发送最后一块数据块 */
    if (0 != ulLastMemSize)
    {
        DMS_NLK_Send(DMS_GET_NLK_PHY_BEAR(enChan), DMS_GET_NLK_MSG_TYPE(enChan), pucMem, ulLastMemSize);
    }

    return VOS_OK;
}
VOS_VOID DMS_NLK_InitEntity(VOS_VOID)
{
    DMS_NLK_ENTITY_STRU                *pstNlkEntity = VOS_NULL_PTR;

    pstNlkEntity = DMS_GET_NLK_ENTITY();

    /* netlink socket */
    pstNlkEntity->pstSock    = VOS_NULL_PTR;

    /* netlink 消息数据块大小 */
    pstNlkEntity->ulDataSize = DMS_NLK_DEFUALT_DATA_SIZE;

    /* netlink 物理承载进程号 */
    pstNlkEntity->astPhyBearProp[DMS_NLK_PHY_BEAR_LTE].lPid     = DMS_NLK_INVALID_PID;
    pstNlkEntity->astPhyBearProp[DMS_NLK_PHY_BEAR_GU].lPid      = DMS_NLK_INVALID_PID;

    /* netlink 逻辑通道属性(LTE CLTR) */
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_LTE_CTRL].pDataFunc = VOS_NULL_PTR;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_LTE_CTRL].pEvtFunc  = VOS_NULL_PTR;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_LTE_CTRL].enPhyBear = DMS_NLK_PHY_BEAR_LTE;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_LTE_CTRL].enMsgType = DMS_NLK_MSG_TYPE_LTE_CTRL;

    /* netlink 逻辑通道属性(LTE DATA) */
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_LTE_DATA].pDataFunc = VOS_NULL_PTR;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_LTE_DATA].pEvtFunc  = VOS_NULL_PTR;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_LTE_DATA].enPhyBear = DMS_NLK_PHY_BEAR_LTE;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_LTE_DATA].enMsgType = DMS_NLK_MSG_TYPE_LTE_DATA;

    /* netlink 逻辑通道属性(GU DATA) */
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_GU_DATA].pDataFunc  = VOS_NULL_PTR;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_GU_DATA].pEvtFunc   = VOS_NULL_PTR;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_GU_DATA].enPhyBear  = DMS_NLK_PHY_BEAR_GU;
    pstNlkEntity->astOmChanProp[DMS_OM_CHAN_GU_DATA].enMsgType  = DMS_NLK_MSG_TYPE_GU_DATA;

    return;
}


VOS_UINT32 DMS_NLK_CfgOpen(
    struct nlmsghdr                    *pstNlkHdr,
    DMS_NLK_PHY_BEAR_ENUM_UINT32        enPhyBear
)
{
    DMS_NLK_PHY_BEAR_PROPERTY_STRU     *pstPhyBearProp = VOS_NULL_PTR;
    DMS_OM_CHAN_EVENT_CB_FUNC           pEvtFunc       = VOS_NULL_PTR;
    DMS_OM_CHAN_ENUM_UINT32             enChan;

    /* 检查承载号 */
    if (enPhyBear >= DMS_NLK_PHY_BEAR_BUTT)
    {
        printk("[%s][LINE: %d] Invalid PHY bearer %d.\n",
            __func__, __LINE__, (VOS_INT)enPhyBear);
        return VOS_ERR;
    }

    /* 设置承载PID */
    pstPhyBearProp = DMS_GET_NLK_PHY_BEAR_PROP(enPhyBear);
    pstPhyBearProp->lPid = pstNlkHdr->nlmsg_pid;

    /* 遍历所有与该承载关联的通道 */
    for (enChan = 0; enChan < DMS_OM_CHAN_BUTT; enChan++)
    {
        pEvtFunc = DMS_GET_NLK_OM_CHAN_EVT_CB_FUNC(enChan);

        /* 通知物理承载相同的逻辑通道使用者通道打开 */
        if ((enPhyBear == DMS_GET_NLK_PHY_BEAR(enChan)) && (VOS_NULL_PTR != pEvtFunc))
        {
            pEvtFunc(enChan, DMS_CHAN_EVT_OPEN);
        }
    }

    return VOS_OK;
}


VOS_UINT32 DMS_NLK_CfgClose(
    struct nlmsghdr                    *pstNlkHdr,
    DMS_NLK_PHY_BEAR_ENUM_UINT32        enBear
)
{
    DMS_NLK_PHY_BEAR_PROPERTY_STRU     *pstPhyBearProp = VOS_NULL_PTR;
    DMS_OM_CHAN_EVENT_CB_FUNC           pEvtFunc       = VOS_NULL_PTR;
    DMS_OM_CHAN_ENUM_UINT32             enChan;

    /* 检查承载号 */
    if (enBear >= DMS_NLK_PHY_BEAR_BUTT)
    {
        printk("[%s][LINE: %d] Invalid PHY bearer %d.\n",
            __func__, __LINE__, (VOS_INT)enBear);
        return VOS_ERR;
    }

    /* 设置承载PID */
    pstPhyBearProp = DMS_GET_NLK_PHY_BEAR_PROP(enBear);
    pstPhyBearProp->lPid = DMS_NLK_INVALID_PID;

    /* 遍历所有与该承载关联的通道 */
    for (enChan = 0; enChan < DMS_OM_CHAN_BUTT; enChan++)
    {
        pEvtFunc = DMS_GET_NLK_OM_CHAN_EVT_CB_FUNC(enChan);

        /* 通知物理承载相同的逻辑通道使用者通道关闭 */
        if ((enBear == DMS_GET_NLK_PHY_BEAR(enChan)) && (VOS_NULL_PTR != pEvtFunc))
        {
            pEvtFunc(enChan, DMS_CHAN_EVT_CLOSE);
        }
    }

    return VOS_OK;
}


VOS_VOID DMS_NLK_ProcLteCfgMsg(struct nlmsghdr *pstNlkHdr)
{
    DMS_NLK_CFG_STRU                   *pstMsg = VOS_NULL_PTR;

    pstMsg = nlmsg_data(pstNlkHdr);

    switch (pstMsg->enCfg)
    {
        case DMS_NLK_CFG_TYPE_OPEN:
            (VOS_VOID)DMS_NLK_CfgOpen(pstNlkHdr, DMS_NLK_PHY_BEAR_LTE);
            DMS_DBG_NLK_UL_SEND_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
            break;

        case DMS_NLK_CFG_TYPE_CLOSE:
            (VOS_VOID)DMS_NLK_CfgClose(pstNlkHdr, DMS_NLK_PHY_BEAR_LTE);
            DMS_DBG_NLK_UL_SEND_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
            break;

        default:
            DMS_DBG_NLK_UL_FREE_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
            break;
    }

    return;
}


VOS_VOID DMS_NLK_ProcGuCfgMsg(struct nlmsghdr *pstNlkHdr)
{
    DMS_NLK_CFG_STRU                   *pstMsg = VOS_NULL_PTR;

    pstMsg = nlmsg_data(pstNlkHdr);

    switch (pstMsg->enCfg)
    {
        case DMS_NLK_CFG_TYPE_OPEN:
            (VOS_VOID)DMS_NLK_CfgOpen(pstNlkHdr, DMS_NLK_PHY_BEAR_GU);
            DMS_DBG_NLK_UL_SEND_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
            break;

        case DMS_NLK_CFG_TYPE_CLOSE:
            (VOS_VOID)DMS_NLK_CfgClose(pstNlkHdr, DMS_NLK_PHY_BEAR_GU);
            DMS_DBG_NLK_UL_SEND_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
            break;

        default:
            DMS_DBG_NLK_UL_FREE_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
            break;
    }

    return;
}


VOS_VOID DMS_NLK_ProcLteCtrlMsg(struct nlmsghdr *pstNlkHdr)
{
    DMS_OM_CHAN_DATA_READ_CB_FUNC       pDataFunc  = VOS_NULL_PTR;
    DMS_NLK_PAYLOAD_STRU               *pstPayload = VOS_NULL_PTR;

    /* 获取通道注册的回调 */
    pDataFunc = DMS_GET_NLK_OM_CHAN_DATA_CB_FUNC(DMS_OM_CHAN_LTE_CTRL);
    if (VOS_NULL_PTR != pDataFunc)
    {
        /* 透传数据内容 */
        pstPayload = nlmsg_data(pstNlkHdr);
        (VOS_VOID)pDataFunc(DMS_OM_CHAN_LTE_CTRL, pstPayload->aucData, pstPayload->ulLength);
        DMS_DBG_NLK_UL_SEND_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
    }
    else
    {
        DMS_DBG_NLK_UL_FREE_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
    }

    return;
}


VOS_VOID DMS_NLK_ProcLteDataMsg(struct nlmsghdr *pstNlkHdr)
{
    DMS_OM_CHAN_DATA_READ_CB_FUNC       pDataFunc  = VOS_NULL_PTR;
    DMS_NLK_PAYLOAD_STRU               *pstPayload = VOS_NULL_PTR;

    /* 获取通道注册的回调 */
    pDataFunc = DMS_GET_NLK_OM_CHAN_DATA_CB_FUNC(DMS_OM_CHAN_LTE_DATA);
    if (VOS_NULL_PTR != pDataFunc)
    {
        /* 透传数据内容 */
        pstPayload = nlmsg_data(pstNlkHdr);
        (VOS_VOID)pDataFunc(DMS_OM_CHAN_LTE_DATA, pstPayload->aucData, pstPayload->ulLength);
        DMS_DBG_NLK_UL_SEND_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
    }
    else
    {
        DMS_DBG_NLK_UL_FREE_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
    }

    return;
}


VOS_VOID DMS_NLK_ProcGuDataMsg(struct nlmsghdr *pstNlkHdr)
{
    DMS_OM_CHAN_DATA_READ_CB_FUNC       pDataFunc = VOS_NULL_PTR;
    DMS_NLK_PAYLOAD_STRU               *pstPayload = VOS_NULL_PTR;

    /* 获取通道注册的回调 */
    pDataFunc = DMS_GET_NLK_OM_CHAN_DATA_CB_FUNC(DMS_OM_CHAN_GU_DATA);
    if (VOS_NULL_PTR != pDataFunc)
    {
        /* 透传数据内容 */
        pstPayload = nlmsg_data(pstNlkHdr);
        (VOS_VOID)pDataFunc(DMS_OM_CHAN_GU_DATA, pstPayload->aucData, pstPayload->ulLength);
        DMS_DBG_NLK_UL_SEND_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
    }
    else
    {
        DMS_DBG_NLK_UL_FREE_MSG_NUM(pstNlkHdr->nlmsg_type, 1);
    }

    return;
}


VOS_INT DMS_NLK_Send(
    DMS_NLK_PHY_BEAR_ENUM_UINT32        enPhyBear,
    DMS_NLK_MSG_TYPE_ENUM_UINT32        enMsgType,
    VOS_UINT8                          *pucData,
    VOS_UINT32                          ulLength
)
{
    struct sk_buff                     *pstSkb      = VOS_NULL_PTR;
    struct nlmsghdr                    *pstNlkHdr   = VOS_NULL_PTR;
    DMS_NLK_PAYLOAD_STRU               *pstPlayload = VOS_NULL_PTR;
    VOS_UINT                            ulMsgSize;
    VOS_UINT                            ulPayloadSize;
    VOS_INT                             lRet;

    DMS_DBG_NLK_DL_TOTAL_MSG_NUM(1);

    /* 检查 netlink socket */
    if (VOS_NULL_PTR == DMS_GET_NLK_SOCK())
    {
        DMS_DBG_NLK_DL_ERR_SOCK_MSG_NUM(1);
        return -EIO;
    }

    /* 检查物理承载状态 */
    if (DMS_NLK_INVALID_PID == DMS_GET_NLK_PHY_PID(enPhyBear))
    {
        DMS_DBG_NLK_DL_ERR_PID_MSG_NUM(1);
        return -EINVAL;
    }

    /* 申请 netlink 消息 */
    ulPayloadSize = sizeof(DMS_NLK_PAYLOAD_STRU) + ulLength;
    ulMsgSize = NLMSG_SPACE(ulPayloadSize);

    pstSkb = nlmsg_new(ulPayloadSize, GFP_ATOMIC);
    if (VOS_NULL_PTR == pstSkb)
    {
        DMS_DBG_NLK_DL_ALLOC_MSG_FAIL_NUM(1);
        return -ENOBUFS;
    }

    /* 填充 netlink 消息头 */
    /* Use "ulMsgSize - sizeof(*pstNlkHdr)" here (incluing align pads) */
    pstNlkHdr = nlmsg_put(pstSkb, 0, 0, (VOS_INT)enMsgType,
                    (VOS_INT)(ulMsgSize - sizeof(struct nlmsghdr)), 0);
    if (VOS_NULL_PTR == pstNlkHdr)
    {
        kfree_skb(pstSkb);
        DMS_DBG_NLK_DL_PUT_MSG_FAIL_NUM(1);
        return -EMSGSIZE;
    }

    /* 填充 netlink 消息接收PID */
    /*lint -e545*/
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) || (VOS_OS_VER == VOS_WIN32))
    NETLINK_CB(pstSkb).portid = DMS_GET_NLK_PHY_PID(enPhyBear);
#else
    NETLINK_CB(pstSkb).pid = DMS_GET_NLK_PHY_PID(enPhyBear);
#endif
    NETLINK_CB(pstSkb).dst_group = 0;
    /*lint +e545*/

    /* 填充 netlink 消息内容 */
    pstPlayload = nlmsg_data(pstNlkHdr);
    pstPlayload->ulLength = ulLength;
    memcpy(pstPlayload->aucData, pucData, ulLength);

    /* 发送 netlink 消息 */
    /*lint -e545*/
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) || (VOS_OS_VER == VOS_WIN32))
    lRet = netlink_unicast(DMS_GET_NLK_SOCK(), pstSkb, NETLINK_CB(pstSkb).portid, 0);
#else
    lRet = netlink_unicast(DMS_GET_NLK_SOCK(), pstSkb, NETLINK_CB(pstSkb).pid, 0);
#endif
    /*lint +e545*/
    if (lRet < 0)
    {
        DMS_DBG_NLK_DL_UNICAST_MSG_FAIL_NUM(1);
        return lRet;
    }

    DMS_DBG_NLK_DL_UNICAST_MSG_SUCC_NUM(1);
    return 0;
}


VOS_VOID DMS_NLK_Input(struct sk_buff *pstSkb)
{
    struct nlmsghdr                    *pstNlkHdr = VOS_NULL_PTR;

    DMS_DBG_NLK_UL_TOTAL_MSG_NUM(1);

    /* 获取 netlink 消息 */
    pstNlkHdr = nlmsg_hdr(pstSkb);

    /* 检查 netlink 消息是否合法 */
    if (!NLMSG_OK(pstNlkHdr, pstSkb->len))
    {
        DMS_DBG_NLK_UL_ERR_MSG_NUM(1);
        return;
    }

    /* 处理 netlink 消息 */
    switch (pstNlkHdr->nlmsg_type)
    {
        case DMS_NLK_MSG_TYPE_LTE_CFG:
            DMS_NLK_ProcLteCfgMsg(pstNlkHdr);
            break;

        case DMS_NLK_MSG_TYPE_LTE_CTRL:
            DMS_NLK_ProcLteCtrlMsg(pstNlkHdr);
            break;

        case DMS_NLK_MSG_TYPE_LTE_DATA:
            DMS_NLK_ProcLteDataMsg(pstNlkHdr);
            break;

        case DMS_NLK_MSG_TYPE_GU_CFG:
            DMS_NLK_ProcGuCfgMsg(pstNlkHdr);
            break;

        case DMS_NLK_MSG_TYPE_GU_DATA:
            DMS_NLK_ProcGuDataMsg(pstNlkHdr);
            break;

        default:
            DMS_DBG_NLK_UL_UNKNOWN_MSG_NUM(1);
            break;
    }

    return;
}
VOS_INT __init DMS_NLK_Init(VOS_VOID)
{
    struct sock                        *pstSock      = VOS_NULL_PTR;
    DMS_NLK_ENTITY_STRU                *pstNlkEntity = VOS_NULL_PTR;

    /* 初始化 netlink 实体 */
    DMS_NLK_InitEntity();

    /* 在内核态创建一个 netlink socket */
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) || (VOS_OS_VER == VOS_WIN32))
    pstSock = netlink_kernel_create(&init_net, NETLINK_HW_LOGCAT, &g_stDmsNlkCfg);
#else
    pstSock = netlink_kernel_create(&init_net, NETLINK_HW_LOGCAT, 0,
                            DMS_NLK_Input, NULL, THIS_MODULE);
#endif
    if (VOS_NULL_PTR == pstSock)
    {
        printk(KERN_ERR "[%s][LINE: %d] Fail to create netlink socket.\n",
            __func__, __LINE__);
        DMS_DBG_NLK_CREATE_SOCK_FAIL_NUM(1);
        return -ENOMEM;
    }

    /* 保存 socket */
    pstNlkEntity = DMS_GET_NLK_ENTITY();
    pstNlkEntity->pstSock = pstSock;

    return 0;
}
VOS_VOID __exit DMS_NLK_Exit(VOS_VOID)
{
    /* 释放 netlink socket */
    netlink_kernel_release(DMS_GET_NLK_SOCK());
    DMS_NLK_InitEntity();
    return;
}

#ifndef _lint
/* This function is called on driver initialization and exit */
module_init(DMS_NLK_Init);
module_exit(DMS_NLK_Exit);
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

