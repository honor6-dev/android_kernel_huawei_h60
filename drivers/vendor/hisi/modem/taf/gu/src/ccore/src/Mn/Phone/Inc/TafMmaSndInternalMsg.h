
#ifndef _TAF_MMA_SND_INTERNAL_MSG_H_
#define _TAF_MMA_SND_INTERNAL_MSG_H_

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "TafMmaCtx.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define TAF_MMA_INVALID_INTERNAL_MSG_ID                 (0xFFFF)                /* 消息ID的无效值 */

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


enum TAF_MMA_INTERNAL_MSG_ID_ENUM
{
    MMA_MMA_INTER_POWER_INIT                = 0x4000,                       /*_H2ASN_MsgChoice TAF_MMA_INTER_POWER_INIT_REQ_STRU */
    
    MMA_MMA_INTER_USIM_STATUS_CHANGE_IND,                                   /*_H2ASN_MsgChoice TAF_MMA_INTER_USIM_STATUS_CHANGE_IND_STRU */

    MMA_MMA_PHONE_MODE_RSLT_IND,                                            /*_H2ASN_MsgChoice TAF_MMA_PHONE_MODE_RSLT_IND_STRU */
    
    MMA_MMA_SIM_LOCK_STATUS_CHANGE_IND,                                     /*_H2ASN_MsgChoice TAF_MMA_SIM_LOCK_STATUS_CHANGE_IND_STRU */

    MMA_MMA_INTER_MSG_BUTT
};
typedef VOS_UINT32 TAF_MMA_INTERNAL_MSG_ID_ENUM_UINT32;



enum TAF_MMA_PHONE_MODE_RESULT_ENUM
{
    TAF_MMA_PHONE_MODE_RESULT_SWITCH_ON_SUCC,
    
    TAF_MMA_PHONE_MODE_RESULT_SWITCH_ON_FAIL,

    TAF_MMA_PHONE_MODE_RESULT_POWER_OFF_SUCC,

    TAF_MMA_PHONE_MODE_RESULT_POWER_OFF_FAIL,

    TAF_MMA_PHONE_MODE_RESULT_BUTT,

};
typedef VOS_UINT8 TAF_MMA_PHONE_MODE_RESULT_ENUM_UINT8;

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


typedef struct
{
    MSG_HEADER_STRU                     MsgHeader;                        /* 消息头 *//*_H2ASN_Skip*/
}TAF_MMA_INTER_POWER_INIT_REQ_STRU;
typedef struct
{
    MSG_HEADER_STRU                     MsgHeader;                        /* 消息头 *//*_H2ASN_Skip*/
}TAF_MMA_INTER_USIM_STATUS_CHANGE_IND_STRU;
typedef struct
{
    MSG_HEADER_STRU                     MsgHeader;                        /* 消息头 *//*_H2ASN_Skip*/
}TAF_MMA_SIM_LOCK_STATUS_CHANGE_IND_STRU;
typedef struct
{
    MSG_HEADER_STRU                                         MsgHeader;                        /* 消息头 *//*_H2ASN_Skip*/
    TAF_MMA_PHONE_MODE_RESULT_ENUM_UINT8                    enRslt;
    VOS_UINT8                                               aucRsv[3];
}TAF_MMA_PHONE_MODE_RSLT_IND_STRU;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
/*****************************************************************************
  H2ASN顶级消息结构定义
*****************************************************************************/
typedef struct
{
    TAF_MMA_INTERNAL_MSG_ID_ENUM_UINT32 enMsgId;
    
    VOS_UINT8                           aucMsgBlock[4];
    /***************************************************************************
        _H2ASN_MsgChoice_When_Comment          TAF_MMA_INTERNAL_MSG_ID_ENUM_UINT32
    ****************************************************************************/
}TAF_MMA_SND_INTERNAL_MSG_DATA;
/*_H2ASN_Length UINT32*/


typedef struct
{
    VOS_MSG_HEADER
    TAF_MMA_SND_INTERNAL_MSG_DATA       stMsgData;
}TafMmaSndInternalMsg_MSG;


/*****************************************************************************
  10 函数声明
*****************************************************************************/
TAF_MMA_INTERNAL_MSG_BUF_STRU* TAF_MMA_GetNextInternalMsg( VOS_VOID );

TAF_MMA_INTERNAL_MSG_BUF_STRU *TAF_MMA_GetIntMsgSendBuf(
    VOS_UINT32                          ulLen
);

VOS_UINT32  TAF_MMA_SndInternalMsg(
    VOS_VOID                           *pSndMsg
);


VOS_VOID TAF_MMA_SndPhoneModeRsltInd(
    TAF_MMA_PHONE_MODE_RESULT_ENUM_UINT8                    enRslt
);

VOS_VOID TAF_MMA_SndInterUsimChangeInd(VOS_VOID);

VOS_VOID TAF_MMA_SndInterPowerInitReq(VOS_VOID);

VOS_VOID TAF_MMA_SndSimlocakStatusChangeInd(VOS_VOID);

#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif



