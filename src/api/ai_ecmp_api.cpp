#include "ai_ecmp_api.h"
#include "../core/ai_ecmp_manager.hpp"

using namespace ai_ecmp;

extern "C" {

WORD32 AI_ECMP_SgConfigCtrl(WORD16 bSwitchFlag, T_AI_ECMP_SG_CFG* pSgCfg) {
    // 调用ECMP管理器处理配置控制
    return CAISlbManagerSingleton::getManagerInstance().handleSgConfigCtrl(bSwitchFlag, pSgCfg);
}

// WORD32 ftm_sgWeightModifyCtrl(T_AI_ECMP_WEIGHT_MODIFY* pOpParam, T_AI_ECMP_SG_CFG* pSgCfg) {
//     // 此函数应由UFP L1(SG)模块实现，这里仅作示例
//     // 在实际项目中，此函数将由外部模块提供
    
//     // 假设权重修改成功
//     // TODO: 实现真实的权重修改逻辑
    
//     // 更新 pSgCfg...
    
//     return 0; // 成功
// }

// WORD32 ftm_sgItemNhopModifyCtrl(T_AI_ECMP_NHOP_MODIFY* pOpParam, T_AI_ECMP_SG_CFG* pSgCfg) {
//     // 此函数应由UFP L1(SG)模块实现，这里仅作示例
//     // 在实际项目中，此函数将由外部模块提供
    
//     // 假设下一跳修改成功
//     // TODO: 实现真实的下一跳修改逻辑
    
//     // 更新 pSgCfg...
    
//     return 0; // 成功
// }

WORD32 ftm_aiEcmpGetAllStatus(void* pStatusBuffer, WORD32 dwBufferSize) {
    // 获取所有实例状态信息
    // TODO: 实现状态信息获取逻辑
    
    return EcmpManager::getManagerInstance().getInstanceCount();
}

} // extern "C"
