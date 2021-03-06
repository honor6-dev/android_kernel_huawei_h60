
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "CmmcaMain.h"
#include "CmmcaCtx.h"
#include "CmmcaMeasReselMgmt.h"
#include "CmmcaPktMgmt.h"
#include "CmmcaRegMgmt.h"
#include "CbpaPsInterface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define    THIS_FILE_ID        PS_FILE_ID_CMMCA_MAIN_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if (FEATURE_ON == FEATURE_CL_INTERWORK)
/* 根据Cmmc码流中Cmd找对应的处理函数 */
const CMMCA_CBPCA_MSG_PROC_STRU         g_astCmmcaMsgProcCbpcaTab[] =
{
    /* Packet Data */
    {ID_CMMCA_MMC_RAT_PDN_TABLE_SETUP_REQ,                  CMMCA_PKT_RcvCmmcSetPdnTabReq},
    {ID_CMMCA_MMC_RAT_DEFAULT_BEARER_REQ,                   CMMCA_PKT_RcvCmmcDefaultBearConnReq},
    {ID_CMMCA_MMC_RAT_BEARER_DISCONNECT_REQ,                CMMCA_PKT_RcvCmmcBearDiscReq},

    /* reg相关消息，发给cmmca */
    {ID_CMMCA_MMC_RAT_RAT_MODE_REQ,                         CMMCA_REG_RcvRatModeReq},
    {ID_CMMCA_MMC_RAT_POWERUP_REQ,                          CMMCA_REG_RcvPowerUpReq},
    {ID_CMMCA_MMC_RAT_POWERDOWN_REQ,                        CMMCA_REG_RcvPowerDownReq},
    {ID_CMMCA_MMC_RAT_MCC_SEARCH_REQ,                       CMMCA_REG_RcvMccSearchReq},
    {ID_CMMCA_MMC_RAT_ACQ_REQ,                              CMMCA_REG_RcvRatAcqReq},
    {ID_CMMCA_MMC_RAT_PS_REG_REQ,                           CMMCA_REG_RcvPsRegReq},
    {ID_CMMCA_MMC_RAT_CELL_INFO_PS_REG_REQ,                 CMMCA_REG_RcvCellInfoPsRegReq},
    {ID_CMMCA_MMC_RAT_NO_SERVICE_REQ,                       CMMCA_REG_RcvNoServiceReq},
    {ID_CMMCA_MMC_RAT_DATACONNECTION_CMD_REQ,               CMMCA_REG_RcvPktDataCmdReq},
    {ID_CMMCA_MMC_RAT_BEARER_DETACH_REQ,                    CMMCA_REG_RcvBearDetachReq},

    /* Meas and Resel:直接透传给ERRC或者MMC */
    {ID_CMMCA_MMC_RAT_IRAT_MEAS_IND,                        CMMCA_RcvCmmcMeasReselReq},
    {ID_CMMCA_MMC_RAT_IRAT_SYSTEM_CHG_RSP,                  CMMCA_RcvCmmcSystemChgRsp},
    {ID_CMMCA_MMC_RAT_IRAT_SIB8_MEAS_IND,                   CMMCA_RcvCmmcMeasReselReq},
    {ID_CMMCA_MMC_RAT_RAT_MEAS_CMD,                         CMMCA_RcvCmmcMeasReselReq},
    {ID_CMMCA_MMC_RAT_RAT_RESELMEAS_CMD,                    CMMCA_RcvCmmcMeasReselReq},
    {ID_CMMCA_MMC_RAT_STOP_IRAT_RATMEAS_CMD,                CMMCA_RcvCmmcMeasReselReq},

    {ID_CMMCA_MMC_RAT_IRAT_RESELMEAS_IND,                   CMMCA_RcvCmmcMeasReselReq},
    {ID_CMMCA_MMC_RAT_SET_IDLEMODEMOBILITYINFO,             CMMCA_RcvCmmcMeasReselReq},

    /* 透传给MTC */
    {ID_CMMCA_MMC_RAT_OTHER_RAT_INFO_IND,                   CMMCA_RcvCmmcOtherRatInfoInd}
};


/* g_astCmmcaMsgProcCbpcaTab的Item个数 */
const VOS_UINT32 g_ulCmmcaMsgProcCbpcaTabSize  = sizeof(g_astCmmcaMsgProcCbpcaTab) / sizeof(g_astCmmcaMsgProcCbpcaTab[0]);


/* 根据TimerId找对应的处理函数 */
const CMMCA_TIMER_MSG_PROC_STRU         g_astCmmcaMsgProcTimerTab[] =
{
    {TI_CMMCA_WAIT_IPV6_RA_INFO,                            CMMCA_PKT_RcvTiWaitIpv6RaExp}
};

/* g_astCmmcaMsgProcTimerTab的Item个数 */
const VOS_UINT32 g_ulCmmcaMsgProcTimerTabSize  = sizeof(g_astCmmcaMsgProcTimerTab) / sizeof(g_astCmmcaMsgProcTimerTab[0]);

/*****************************************************************************
  3 函数定义
*****************************************************************************/

VOS_VOID CMMCA_RcvCbpcaMsg(struct MsgCB *pstMsg)
{
    /* 定义消息头指针*/
    CBPCA_DATA_IND_MSG_STRU            *pstDataInd = VOS_NULL_PTR;
    CMMCA_MMC_RAT_CMD_DATA_STRU        *pstCmdData = VOS_NULL_PTR;
    CMMCA_CBPCA_MSG_PROC_FUNC           pProcMsgFunc = VOS_NULL_PTR;
    VOS_UINT32                          ulIndex;

    /* 获取消息头指针*/
    pstDataInd = (CBPCA_DATA_IND_MSG_STRU *)pstMsg;

    if (CBPCA_CMMCA_DATA_IND != pstDataInd->enMsgType)
    {
        CMMCA_ERROR_LOG("CMMCA_RcvCbpcaMsg: not data ind");

        return;
    }

    pstCmdData = (CMMCA_MMC_RAT_CMD_DATA_STRU *)(pstDataInd->aucData);

    /* 查表g_astCmmcaMsgProcCbpcaTab找pstCmdData->enCmdId对应的处理函数，参数类型为CMMCA_MMC_RAT_CMD_DATA_STRU */
    for (ulIndex = 0; ulIndex < g_ulCmmcaMsgProcCbpcaTabSize; ulIndex++)
    {
        if (g_astCmmcaMsgProcCbpcaTab[ulIndex].ulCmdId == pstCmdData->enCmdId)
        {
            /* Cmd Id 匹配 */
            pProcMsgFunc = g_astCmmcaMsgProcCbpcaTab[ulIndex].pProcMsgFunc;

            break;
        }
    }

    /* 如果处理函数存在则调用 */
    if ( VOS_NULL_PTR != pProcMsgFunc )
    {
        pProcMsgFunc(pstDataInd);
    }
    else
    {
        CMMCA_ERROR_LOG("CMMCA_RcvCbpcaMsg: proc msg func is null");
    }

    return;
}
VOS_VOID CMMCA_RcvTafMsg(struct MsgCB *pstMsg)
{
    /* 定义消息头指针*/
    MSG_HEADER_STRU                     *pstHeader = VOS_NULL_PTR;

    /* 获取消息头指针*/
    pstHeader = (MSG_HEADER_STRU *)pstMsg;

    /* APS消息 */
    if(MN_CALLBACK_PS_CALL == pstHeader->ulMsgName)
    {
        CMMCA_PKT_RcvTafPsEvt((TAF_PS_EVT_STRU*)pstMsg);
    }

    return;
}
VOS_VOID CMMCA_RcvErrcMsg(struct MsgCB *pstMsg)
{
    LRRC_CMMCA_CDMA_MSG_IRAT_MEAS_REQ_STRU  *pstIratMeasReq = VOS_NULL_PTR;
    CMMCA_MMC_RAT_CMD_ID_ENUM_UINT16         enCmdIdFromMsgId;
    CMMCA_MMC_RAT_CMD_ID_ENUM_UINT16         enCmdIdInData;
    VOS_UINT32                               ulRslt;

    enCmdIdFromMsgId = ID_CMMCA_RAT_MMC_CMD_MAX;

    /* ERRC回复给CMMCA的消息pstMsg,其结构体为LRRC_CMMCA_CDMA_MSG_IRAT_MEAS_REQ_STRU */
    pstIratMeasReq         = (LRRC_CMMCA_CDMA_MSG_IRAT_MEAS_REQ_STRU *)pstMsg;

    ulRslt = CMMCA_GetCmdIdFromErrcMsgId(pstIratMeasReq->enMsgId, &enCmdIdFromMsgId);

    if (VOS_OK != ulRslt)
    {
        CMMCA_ERROR_LOG("CMMCA_RcvErrcMsg: Not find Cmd Id ");
        return;
    }

    enCmdIdInData = *((CMMCA_MMC_RAT_CMD_ID_ENUM_UINT16 *)(pstIratMeasReq->aucMsgData));

    /* 检查根据MsgId计算出来的CmdId跟Errc实际传来的CmdId是否一致 */
    if (enCmdIdFromMsgId != enCmdIdInData)
    {
        CMMCA_ERROR_LOG("CMMCA_RcvErrcMsg: MsgId and CmdId don't match ");
        return;
    }

    /* 调用CBPCA模块接口将数据发送到CBPCA */
    ulRslt = CBPCA_SndDataToCbpca(WUEPS_PID_CMMCA,
                                  CMMCA_CBPCA_DATA_REQ,
                                  (VOS_UINT8 *)(pstIratMeasReq->aucMsgData),
                                  pstIratMeasReq->ulLen);

    if (VOS_OK != ulRslt)
    {
        CMMCA_ERROR_LOG("CMMCA_RcvErrcMsg: send Msg to Cbpca fail");
    }

    return;
}
VOS_VOID CMMCA_RcvTimerMsg(struct MsgCB *pstMsg)
{
    /* 定义消息头指针*/
    REL_TIMER_MSG                      *pstTiMsg = VOS_NULL_PTR;
    CMMCA_TIMER_MSG_PROC_FUNC           pProcMsgFunc = VOS_NULL_PTR;
    VOS_UINT32                          ulIndex;

    /* 获取消息头指针*/
    pstTiMsg = (REL_TIMER_MSG *)pstMsg;

    /* 停止定时器 */
    CMMCA_StopTimer(WUEPS_PID_CMMCA, pstTiMsg->ulName);

    /* 查表g_astCmmcaMsgProcTimerTab */
    for (ulIndex = 0; ulIndex < g_ulCmmcaMsgProcTimerTabSize; ulIndex++)
    {
        if (g_astCmmcaMsgProcTimerTab[ulIndex].enTimerId == pstTiMsg->ulName)
        {
            /* Timer Id 匹配 */
            pProcMsgFunc = g_astCmmcaMsgProcTimerTab[ulIndex].pProcMsgFunc;

            break;
        }
    }

    /* 如果处理函数存在则调用 */
    if ( VOS_NULL_PTR != pProcMsgFunc )
    {
        pProcMsgFunc(pstTiMsg);
    }
    else
    {
        CMMCA_ERROR_LOG("CMMCA_RcvTimerMsg: proc msg func is null");
    }


    return;
}


VOS_VOID CMMCA_ProcMsg(MsgBlock* pMsg)
{
    if (VOS_NULL_PTR == pMsg)
    {
        return;
    }

    switch (pMsg->ulSenderPid)
    {
        case WUEPS_PID_CBPCA:
            CMMCA_RcvCbpcaMsg(pMsg);
            break;

        case WUEPS_PID_TAF:
            CMMCA_RcvTafMsg(pMsg);
            break;

        case PS_PID_ERRC:
            CMMCA_RcvErrcMsg(pMsg);
            break;

        case VOS_PID_TIMER:
            CMMCA_RcvTimerMsg(pMsg);
            break;

        case WUEPS_PID_MMA:
            CMMCA_REG_RcvMmaMsg(pMsg);
            break;
        default:
            CMMCA_WARNING_LOG("CMMCA_PidMsgProc: Abnormal PID.");
            break;

    }

    return;
}

#endif


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif





