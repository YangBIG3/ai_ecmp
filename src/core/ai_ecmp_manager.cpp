#include "ai_ecmp_manager.hpp"
#include "ai_ecmp_error.h"
#include "ai_ecmp_instance.hpp"
#include "ai_ecmp_api.h"
#include <algorithm>
#include <memory>
#include <unordered_map>


namespace ai_ecmp {

CAISlbManagerSingleton& CAISlbManagerSingleton::getManagerInstance() {
    static CAISlbManagerSingleton s_instance;
    return s_instance;
}

WORD32 CAISlbManagerSingleton::handleSgConfigCtrl(WORD16 bSwitchFlag, T_AI_ECMP_SG_CFG* pSgCfg) {
    if (!pSgCfg) {
        return AI_ECMP_ERR_INVALID_PARAM; // 无效参数
    }
    
    WORD32 dwSgId = pSgCfg->dwSgId;
    
    if (bSwitchFlag == 1) {
        // 新增或更新SG配置
        auto it = m_instances.find(dwSgId);
        
        if (it == m_instances.end()) {
            // 创建新实例
            std::unique_ptr<EcmpInstance> pInstance(new EcmpInstance(*pSgCfg));
            m_instances[dwSgId] = std::move(pInstance);
            LogSgConfigDetails(pSgCfg);
            XOS_SysLog(LOG_EMERGENCY, "[AI ECMP]  %s : 创建了新的ECMP实例, SG ID: %u .\n", __FUNCTION__, dwSgId);
        } else {
            // 更新现有实例
            it->second->updateConfig(*pSgCfg);
            LogSgConfigDetails(pSgCfg);
            XOS_SysLog(LOG_EMERGENCY, "[AI ECMP]  %s : 更新了ECMP实例配置, SG ID: %u .\n", __FUNCTION__, dwSgId);
        }
    } else if (bSwitchFlag == 0) {
        // 删除SG配置
        auto it = m_instances.find(dwSgId);
        if (it != m_instances.end()) {
            m_instances.erase(it);
            XOS_SysLog(LOG_EMERGENCY, "[AI ECMP]  %s : 删除了ECMP实例, SG ID: %u .\n", __FUNCTION__, dwSgId);
        }
    } else {
        return AI_ECMP_ERR_CONFIG_INVALID; // 无效的开关标志
    }
    
    return AI_SUCCESS; // 成功
}

WORD32 CAISlbManagerSingleton::runOptimizationCycle(T_AI_ECMP_COUNTER_STATS_MSG ecmpMsg) {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] =====开始优化周期=====\n");
    WORD32 dwResult = AI_SUCCESS;

    // 获取所有实例
    WORD32 dwInstanceCount = getInstanceCount();
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] 当前实例数量: %u\n", dwInstanceCount);
    
    if (dwInstanceCount == 0) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] 没有ECMP实例，退出优化周期\n");
        return AI_ECMP_ERR_NO_INSTANCE;
    }
    
    // 遍历所有实例
    for (auto& instanceEntry : m_instances) {
        WORD32 dwSgId = instanceEntry.first;
        auto& pInstance = instanceEntry.second;

        // 更新计数器
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] 更新实例 %u 的计数器\n", dwSgId);
        if (!pInstance->updateCounters(ecmpMsg)) {
            XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 计数器更新失败\n", dwSgId);
            dwResult = AI_ECMP_ERR_COUNTER_READ;
            continue;
        }
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 计数器更新成功\n", dwSgId);
        
        // 执行优化
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] 开始执行实例 %u 的优化\n", dwSgId);
        if (pInstance->runOptimization()) {
            T_AI_ECMP_NHOP_MODIFY nhopModifyData = {0};
            T_AI_ECMP_STATUS status = pInstance->getStatus();
            
            XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 优化完成，状态: %d\n", dwSgId, status);
            
            // 根据状态执行不同操作
            if (status == AI_ECMP_EXPAND) {
                XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 需要扩容操作\n", dwSgId);
                if (pInstance->getExpandedNextHops(nhopModifyData)) {
                    XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 扩容配置生成成功，逻辑成员数: %u\n", 
                                 dwSgId, nhopModifyData.dwItemNum);
                    // 调用统一的下一跳修改接口
                    aiEcmpSendNhopModify(nhopModifyData);
                    XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 扩容操作下发完成\n", dwSgId);
                } else {
                    XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 扩容配置生成失败\n", dwSgId);
                    dwResult = AI_ECMP_ERR_EXPAND_FAILED;
                }
            } else if (status == AI_ECMP_ADJUST) {
                XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 需要调整下一跳\n", dwSgId);
                if (pInstance->getOptimizedNextHops(nhopModifyData)) {
                    XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 优化配置生成成功，项目数: %u\n", 
                                 dwSgId, nhopModifyData.dwItemNum);
                    // 调用统一的下一跳修改接口
                    aiEcmpSendNhopModify(nhopModifyData);
                    XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 下一跳调整执行完成\n", dwSgId);
                } else {
                    XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 优化配置生成失败\n", dwSgId);
                    dwResult = AI_ECMP_ERR_ADJUST_FAILED;
                }
            }
        } else {
            XOS_SysLog(LOG_EMERGENCY, "[ECMP] 实例 %u 不需要优化或优化失败\n", dwSgId);
        }
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] =====优化周期结束，结果: 0x%x=====\n", dwResult);
    return dwResult;
}



/**
 * @brief 打印 T_AI_ECMP_SG_CFG 结构体的详细内容
 * @param pSgCfg 指向要打印的配置结构体的指针
 */
void CAISlbManagerSingleton::LogSgConfigDetails(const T_AI_ECMP_SG_CFG* pSgCfg)
{
    // 安全检查，防止传入空指针
    if (!pSgCfg) {
        XOS_SysLog(LOG_EMERGENCY, "[AI ECMP] : Attempted to log a null SG Cfg.\n");
        return;
    }

    WORD32 dwSgId = pSgCfg->dwSgId;

    XOS_SysLog(LOG_EMERGENCY, "[AI ECMP] : --- Start Dumping SG Cfg (SG ID: %u) ---\n", dwSgId);
    
    // 打印 T_AI_ECMP_SG_CFG 的基本成员
    XOS_SysLog(LOG_EMERGENCY, "[AI ECMP] :   SG Cfg Base: SeqId=%d, FwdLagId=%d, ItemNum=%d, PortNum=%d, CounterBase=%d\n",
               pSgCfg->dwSeqId, pSgCfg->dwFwdLagId, pSgCfg->dwItemNum, pSgCfg->dwPortNum, pSgCfg->dwCounterBase);

    // 打印 T_AI_SG_ITEM_CFG 数组 (items)
    XOS_SysLog(LOG_EMERGENCY, "[AI ECMP] :   SG Cfg Items List:\n");
    for (WORD32 i = 0; i < FTM_TRUNK_MAX_HASH_NUM_15K; ++i)
    {
        if (pSgCfg->items[i].IsValid())
        {
            XOS_SysLog(LOG_EMERGENCY, "[AI ECMP] :     Item[%d]: PortId=%d, ItemOffset=%d\n",
                        i, pSgCfg->items[i].dwPortId, pSgCfg->items[i].dwItemOffset);
        }
    }

    // 打印 T_AI_SG_WEIGHT_CFG 数组 (ports)
    XOS_SysLog(LOG_EMERGENCY, "[AI ECMP] :   SG Cfg Ports List (Total Physical Ports: %d):\n", pSgCfg->dwPortNum);
    for (WORD32 i = 0; (i < pSgCfg->dwPortNum) && (i < FTM_LAG_MAX_MEM_NUM_15K); ++i)
    {
        if (pSgCfg->ports[i].IsValid())
        {
            XOS_SysLog(LOG_EMERGENCY, "[AI ECMP] :     Port[%d]: PortId=%d, Speed=%d, Weight=%d\n",
                        i, pSgCfg->ports[i].dwPortId, pSgCfg->ports[i].dwSpeed, pSgCfg->ports[i].dwWeight);
        }
    }
    XOS_SysLog(LOG_EMERGENCY, "[AI ECMP] : --- End Dumping SG Cfg (SG ID: %u) ---\n", dwSgId);
}



WORD32 CAISlbManagerSingleton::getInstanceStatus(WORD32 dwSgId, void* pStatusInfo) {
    auto it = m_instances.find(dwSgId);
    if (it == m_instances.end()) {
        return AI_ECMP_ERR_NOT_FOUND;
    }
    
    // TODO: 实现状态信息填充逻辑
    
    return AI_SUCCESS;
}

WORD32 CAISlbManagerSingleton::getInstanceCount() {
    return static_cast<WORD32>(m_instances.size());
}

// ===== 新增：实例访问方法实现 =====
EcmpInstance* CAISlbManagerSingleton::getInstance(WORD32 dwSgId) {
    auto it = m_instances.find(dwSgId);
    if (it != m_instances.end()) {
        return it->second.get();
    }
    return nullptr;
}

void CAISlbManagerSingleton::forEachInstance(std::function<void(WORD32, EcmpInstance*)> func) {
    for (auto& pair : m_instances) {
        func(pair.first, pair.second.get());
    }
}

WORD32 CAISlbManagerSingleton::callSgWeightModifyCtrl(T_AI_ECMP_WEIGHT_MODIFY* pOpParam, T_AI_ECMP_SG_CFG* pSgCfg) {
    // 此处应调用SG模块的接口
    
    return ftm_sgWeightModifyCtrl(pOpParam, pSgCfg);
}

WORD32 CAISlbManagerSingleton::callSgItemNhopModifyCtrl(T_AI_ECMP_NHOP_MODIFY* pOpParam, T_AI_ECMP_SG_CFG* pSgCfg) {
    // 此处应调用SG模块的接口
    
    return ftm_sgItemNhopModifyCtrl(pOpParam, pSgCfg);
}

// WORD32 CAISlbManagerSingleton::upsertSgConfig(T_AI_ECMP_SG_CFG* pSgCfg) {
//     WORD16 wSgId = pSgCfg->dwSgId;
    
//     // 查找是否已存在实例
//     auto it = m_instances.find(wSgId);
//     if (it == m_instances.end()) {
//         // 创建新实例
//         auto pInstance = std::make_unique<EcmpInstance>(pSgCfg);
//         m_instances[wSgId] = std::move(pInstance);
//         std::cout << "创建了新的ECMP实例，SG ID: " << wSgId << std::endl;
//     } else {
//         // 更新现有实例
//         it->second->setSgConfig(pSgCfg);
//         std::cout << "更新了ECMP实例配置，SG ID: " << wSgId << std::endl;
//     }

//     return 0;
// }

// WORD32 CAISlbManagerSingleton::deleteSgConfig(WORD16 wSgId) {
//     // 删除配置
//     auto it = m_instances.find(wSgId);
//     if (it != m_instances.end()) {
//         m_instances.erase(it);
//         std::cout << "删除了ECMP实例，SG ID: " << wSgId << std::endl;
//     }
    
//     return 0;
// }

} // namespace ai_ecmp
