#ifndef AI_ECMP_API_H
#define AI_ECMP_API_H

#include "ai_ecmp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SG配置信息同步接口
 * @param bSwitchFlag 开关标志，1-生效，0-删除
 * @param pSgCfg 指向SG配置信息的指针
 * @return 操作结果码
 * @author 张志宸10350479  @date 2025/05/13
 */
WORD32 AI_ECMP_SgConfigCtrl(WORD16 bSwitchFlag, T_AI_ECMP_SG_CFG* pSgCfg);

// /**
//  * @brief SG权重修改接口，为ftm接口，暂时写在这里
//  * @param pOpParam 指向权重修改参数的指针
//  * @param pSgCfg 指向修改后SG配置信息的指针
//  * @return 操作结果码
//  * @author 张志宸10350479  @date 2025/05/13
//  */
// WORD32 ftm_sgWeightModifyCtrl(T_AI_ECMP_WEIGHT_MODIFY* pOpParam, T_AI_ECMP_SG_CFG* pSgCfg);

// /**
//  * @brief SG逻辑成员port_id修改接口，为ftm接口，暂时写在这里
//  * @param pOpParam 指向下一跳修改参数的指针
//  * @param pSgCfg 指向修改后SG配置信息的指针
//  * @return 操作结果码
//  * @author 张志宸10350479  @date 2025/05/13
//  */
// WORD32 ftm_sgItemNhopModifyCtrl(T_AI_ECMP_NHOP_MODIFY* pOpParam, T_AI_ECMP_SG_CFG* pSgCfg);

/**
 * @brief 获取当前所有ECMP实例状态
 * @param pStatusBuffer 状态缓冲区
 * @param dwBufferSize 缓冲区大小
 * @return 实际状态数量
 * @author 张志宸10350479  @date 2025/05/13
 */
WORD32 ftm_aiEcmpGetAllStatus(void* pStatusBuffer, WORD32 dwBufferSize);


// WORD32 aiEcmpSendSynCfgToUfp(BYTE* pMsg, WORD16 wMsgLen);

#ifdef __cplusplus
}
#endif

#endif /* AI_ECMP_API_H */
