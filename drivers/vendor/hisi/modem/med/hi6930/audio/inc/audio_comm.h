/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : audio_comm.h
  版 本 号   : 初稿
  作    者   : C00137131
  生成日期   : 2012年7月5日
  最近修改   :
  功能描述   : audio_comm.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2012年7月5日
    作    者   : C00137131
    修改内容   : 创建文件

******************************************************************************/

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include  "audio.h"


#ifndef __AUDIO_COMM_H__
#define __AUDIO_COMM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif



/*****************************************************************************
  2 宏定义
*****************************************************************************/

/* 全局变量封装 */
#define AUDIO_COMM_GetMsgStatusPtr()    (&g_stAudioStatusDesc)
#define AUDIO_COMM_GetMsgFuncTbl()      (g_astAudioMsgFuncTable)

#define AUDIO_COMM_GetRtMsgStatusPtr()  (&g_stAudioRtStatusDesc)
#define AUDIO_COMM_GetRtMsgFuncTbl()    (g_astAudioRtMsgFuncTable)


/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/*****************************************************************************
  4 消息头定义
*****************************************************************************/


/*****************************************************************************
  5 消息定义
*****************************************************************************/


/*****************************************************************************
  6 STRUCT定义
*****************************************************************************/


/*****************************************************************************
  7 UNION定义
*****************************************************************************/


/*****************************************************************************
  8 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  9 全局变量声明
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

extern VOS_VOID AUDIO_COMM_FuncTableInit( VOS_VOID );
extern VOS_VOID AUDIO_COMM_Init( VOS_VOID );
extern VOS_UINT32 AUDIO_COMM_PidInit(enum VOS_INIT_PHASE_DEFINE enInitPhrase);
extern VOS_VOID AUDIO_COMM_PidProc(MsgBlock *pstOsaMsg);
extern VOS_VOID AUDIO_COMM_RtFuncTableInit( VOS_VOID );
extern VOS_UINT32 AUDIO_COMM_RtPidInit(enum VOS_INIT_PHASE_DEFINE enInitPhrase);
extern VOS_VOID AUDIO_COMM_RtPidProc(MsgBlock *pstOsaMsg);
extern VOS_UINT32 AUDIO_COMM_IsIdle(VOS_VOID);
extern VOS_VOID  AUDIO_COMM_CheckContext( VOS_VOID );



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of audio_comm.h */
