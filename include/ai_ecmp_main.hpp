#ifndef AI_ECMP_MAIN_H
#define AI_ECMP_INSAI_ECMP_MAIN_HTANCE_H

#include "ftm_lag_external_release_define_15k.h"
#include "ai_ecmp_types.h"
#include "ai_qos_init.h"

#ifdef __cplusplus
extern "C" {
#endif
// 字节序转化
VOID convertByteOrderWfg(T_AI_SG_WEIGHT_CFG& weightCfg);
VOID convertByteOrderSfg(T_AI_ECMP_SG_CFG& sgCfg);
VOID convertByteOrderWeightMod(T_AI_ECMP_WEIGHT_MODIFY& weightMod);
VOID convertByteOrderNhopMod(T_AI_ECMP_NHOP_MODIFY& nhopMod);

// 对外接口
WORD32 aiEcmpCfgCallback(VOID * pArg, VOID * pMsgBody, WORD16 wMsgLen, VOID * pPData, BOOLEAN bSame);   //收到cfg后回调动作
WORD32 aiEcmpStateCallback(VOID * pArg, VOID * pMsgBody, WORD16 wMsgLen, VOID * pPData, BOOLEAN bSame);
// VOID aiEcmpSendWeightModify(const T_AI_ECMP_WEIGHT_MODIFY& input);  //发送权重调整信号给ECMP模块
VOID aiEcmpSendNhopModify(const T_AI_ECMP_NHOP_MODIFY& input);      //发生新的配置给ECMP模块

// TODO
// 读counter
VOID aiEcmpReadAction();  //100ms定时器执行，度counter
#ifdef __cplusplus
}
#endif

#endif /* AI_ECMP_MAIN_H */
// const WORD32 FTM_LAG_MAX_MEM_NUM_15K = 128;
// const WORD32 FTM_TRUNK_MAX_HASH_NUM_15K = 128;

// struct T_AI_SG_WEIGHT_CFG
// {    
//     WORD32 dwPortId = STD_DWORD_NULL;            /* 物理成员ifindex */     
//     WORD32 dwSpeed = 0;                          /* 物理成员实际带宽比，用于真实平衡度 */   
//     WORD32 dwWeight = 0;                         /* 物理成员散列后的权重(逻辑链路数) ，不是按照speed */   

//     void Print() const
//     {}  
// };

// struct T_AI_ECMP_SG_CFG
// {
//     WORD32 dwSgId = STD_DWORD_NULL;                          /* SG ifindex，平台下发的 */   
//     WORD32 dwSeqId = STD_DWORD_NULL;                         /* 通过版本号控制修改批次 */   
//     WORD32 dwFwdLagId = STD_DWORD_NULL;                      /* 微码Lag id，微码微码识别的SG转发索引 */
//     WORD32 dwItemNum = 0;                                    /* 散列后逻辑成员链路数 */
//     WORD32 dwPortNum = 0;                                    /* 实际物理成员链路数 */
//     WORD32 dwCounterBase = 0;                                /* 成员Counter Id基地址 */     
//     T_AI_SG_ITEM_CFG    items[FTM_TRUNK_MAX_HASH_NUM_15K];   /* 散列后逻辑成员数组 */  
//     T_AI_SG_WEIGHT_CFG  ports[FTM_TRUNK_MAX_HASH_NUM_15K];

//     T_AI_ECMP_SG_CFG(){};

//     void Print() const
//     {}  
// };


// struct T_AI_ECMP_WEIGHT_MODIFY
// {
//     WORD32 dwSgId = 0;                                   /* SG ifindex */
//     WORD32 dwSeqId = 0;                                  /* 通过版本号控制修改批次 */
//     WORD32 dwModifyNum = 0;                              /* 待修改的port id数量 */
//     WORD32 adwPortId[FTM_LAG_MAX_MEM_NUM_15K];           /* 物理成员ifindex */
//     WORD32 adwWeight[FTM_LAG_MAX_MEM_NUM_15K];           /* 修改权重(逻辑链路数) */

//     T_AI_ECMP_WEIGHT_MODIFY()
//     {
//         ufp_memset_s(adwPortId, sizeof(adwPortId), 0, sizeof(adwPortId));
//         ufp_memset_s(adwWeight, sizeof(adwWeight), 0, sizeof(adwWeight));
//     }

//     void Print() const
//     {}
// };

// struct T_AI_ECMP_NHOP_MODIFY
// {    
//     WORD32 dwSgId = 0;                                     /* SG ifindex */  
//     WORD32 dwSeqId = 0;                                    /* 通过版本号控制修改批次 */
//     WORD32 dwItemNum = 0;                                  /* 散列后逻辑成员链路数 */   
//     WORD32 adwLinkItem[FTM_TRUNK_MAX_HASH_NUM_15K];        /* 修改后散列后逻辑成员数组 */

//     T_AI_ECMP_NHOP_MODIFY(){};

//     void Print() const
//     {}
// };
