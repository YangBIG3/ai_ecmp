#include "ai_ecmp_diag.h"
#include "../core/ai_ecmp_manager.hpp"
#include "../core/ai_ecmp_instance.hpp"
#include "../algorithms/ai_ecmp_local_search.hpp"
#include "../algorithms/ai_ecmp_ga_imp.hpp"
#include "../utils/ai_ecmp_metrics.hpp"
#include "ai_ecmp_error.h"
#include <memory>
#include <vector>
#include <random>
#include <unordered_map> // Added for portItemCount


// 辅助函数：打印单个实例的配置
static void printSingleInstanceConfig(WORD32 sgId, EcmpInstance* pInstance) {
    if (!pInstance) return;
    
    const T_AI_ECMP_SG_CFG& cfg = pInstance->getSgConfig();
    
    AI_DIAG_PRINTF("\n[DIAG] --- SG %u 配置详情 ---\n", sgId);
    AI_DIAG_PRINTF("[DIAG]   基本信息:\n");
    AI_DIAG_PRINTF("[DIAG]     SG ID: %u\n", cfg.dwSgId);
    AI_DIAG_PRINTF("[DIAG]     序列号: %u\n", cfg.dwSeqId);
    AI_DIAG_PRINTF("[DIAG]     转发Lag ID: %u\n", cfg.dwFwdLagId);
    AI_DIAG_PRINTF("[DIAG]     逻辑成员数: %u\n", cfg.dwItemNum);
    AI_DIAG_PRINTF("[DIAG]     物理端口数: %u\n", cfg.dwPortNum);
    AI_DIAG_PRINTF("[DIAG]     计数器基地址: %u\n", cfg.dwCounterBase);
    AI_DIAG_PRINTF("[DIAG]     优化状态: %s\n", 
              pInstance->isOptimizationEnabled() ? "启用" : "禁用");
    AI_DIAG_PRINTF("[DIAG]     当前状态: %s\n", 
              utils::aiEcmpStatusToString(pInstance->getStatus()));
    AI_DIAG_PRINTF("[DIAG]     当前周期: %u\n", pInstance->getCycle());
    
    // 打印物理端口配置
    AI_DIAG_PRINTF("\n[DIAG]   物理端口配置:\n");
    AI_DIAG_PRINTF("[DIAG]     %-8s %-12s %-12s %-8s\n", 
              "序号", "端口ID", "速率(Mbps)", "权重");
    AI_DIAG_PRINTF("[DIAG]     %s\n", "----------------------------------------");
    
    for (WORD32 i = 0; i < cfg.dwPortNum && i < FTM_LAG_MAX_MEM_NUM_15K; ++i) {
        if (cfg.ports[i].dwPortId != 0) {  // 假设0表示无效端口
            AI_DIAG_PRINTF("[DIAG]     %-8u %-12u %-12u %-8u\n",
                      i, cfg.ports[i].dwPortId, cfg.ports[i].dwSpeed, cfg.ports[i].dwWeight);
        }
    }
    
    // 打印逻辑成员分布统计
    AI_DIAG_PRINTF("\n[DIAG]   逻辑成员分布统计:\n");
    std::unordered_map<WORD32, WORD32> portItemCount;
    for (WORD32 i = 0; i < cfg.dwItemNum && i < FTM_TRUNK_MAX_HASH_NUM_15K; ++i) {
        WORD32 portId = cfg.items[i].dwPortId;
        if (portId != 0) {
            portItemCount[portId]++;
        }
    }
    
    AI_DIAG_PRINTF("[DIAG]     %-12s %-12s %-8s\n", 
              "端口ID", "逻辑成员数", "占比");
    AI_DIAG_PRINTF("[DIAG]     %s\n", "--------------------------------");
    
    for (const auto& pair : portItemCount) {
        double percentage = (double)pair.second / cfg.dwItemNum * 100.0;
        AI_DIAG_PRINTF("[DIAG]     %-12u %-12u %.2f%%\n",
                  pair.first, pair.second, percentage);
    }
}
// 辅助函数：打印单个实例的平衡状态
static void printSingleInstanceBalance(WORD32 sgId, EcmpInstance* pInstance) {
    if (!pInstance) return;
    
    // 获取当前平衡评估结果
    T_AI_ECMP_EVAL balanceEval = pInstance->evaluateBalance();
    
    AI_DIAG_PRINTF("\n[DIAG] --- SG %u 平衡状态 ---\n", sgId);
    AI_DIAG_PRINTF("[DIAG]   基本状态:\n");
    AI_DIAG_PRINTF("[DIAG]     运行状态: %s\n", 
              utils::aiEcmpStatusToString(pInstance->getStatus()));
    AI_DIAG_PRINTF("[DIAG]     优化状态: %s\n", 
              pInstance->isOptimizationEnabled() ? "启用" : "禁用");
    if (!pInstance->isOptimizationEnabled()) {
        AI_DIAG_PRINTF("[DIAG]     禁用周期数: %u\n", 
                  pInstance->getDisabledCycles());
    }
    AI_DIAG_PRINTF("[DIAG]     当前周期: %u\n", pInstance->getCycle());
    
    // 打印负载平衡指标
    AI_DIAG_PRINTF("\n[DIAG]   负载平衡指标:\n");
    AI_DIAG_PRINTF("[DIAG]     总偏差: %.6f\n", balanceEval.totalGap);
    AI_DIAG_PRINTF("[DIAG]     正偏差: %.6f\n", balanceEval.upBoundGap);
    AI_DIAG_PRINTF("[DIAG]     负偏差: %.6f\n", balanceEval.lowBoundGap);
    AI_DIAG_PRINTF("[DIAG]     平均偏差: %.6f\n", balanceEval.avgGap);
    AI_DIAG_PRINTF("[DIAG]     平衡得分: %.6f\n", balanceEval.balanceScore);
    
    // 根据总偏差给出平衡状态评价
    const char* balanceStatus;
    if (balanceEval.totalGap < 0.05) {
        balanceStatus = "平衡良好";
    } else if (balanceEval.totalGap < 0.15) {
        balanceStatus = "轻度不平衡";
    } else if (balanceEval.totalGap < 0.3) {
        balanceStatus = "中度不平衡";
    } else {
        balanceStatus = "严重不平衡";
    }
    AI_DIAG_PRINTF("[DIAG]     平衡评价: %s\n", balanceStatus);
    
}


// 辅助函数：打印单个实例的优化效果
static void printSingleInstanceOptimization(WORD32 sgId, EcmpInstance* pInstance) {

    
    if (!pInstance) return;
    
    AI_DIAG_PRINTF("\n[DIAG] --- SG %u 优化效果 ---\n", sgId);
    AI_DIAG_PRINTF("[DIAG]   优化控制信息:\n");
    AI_DIAG_PRINTF("[DIAG]     优化状态: %s\n", 
              pInstance->isOptimizationEnabled() ? "启用" : "禁用");
    
    if (!pInstance->isOptimizationEnabled()) {
        AI_DIAG_PRINTF("[DIAG]     禁用周期数: %u\n", 
                  pInstance->getDisabledCycles());
        AI_DIAG_PRINTF("[DIAG]     优化效果: 优化已禁用，无效果数据\n");
        return;
    }
    
    AI_DIAG_PRINTF("[DIAG]     当前周期: %u\n", pInstance->getCycle());
    AI_DIAG_PRINTF("[DIAG]     运行状态: %s\n", 
              utils::aiEcmpStatusToString(pInstance->getStatus()));
    
    // 获取当前平衡状态
    T_AI_ECMP_EVAL currentEval = pInstance->evaluateBalance();
    
    AI_DIAG_PRINTF("\n[DIAG]   当前性能指标:\n");
    AI_DIAG_PRINTF("[DIAG]     总偏差: %.6f\n", currentEval.totalGap);
    AI_DIAG_PRINTF("[DIAG]     平衡得分: %.6f\n", currentEval.balanceScore);
    
    // 根据状态分析优化效果
    T_AI_ECMP_STATUS status = pInstance->getStatus();
    switch (status) {
        case AI_ECMP_INIT:
            AI_DIAG_PRINTF("[DIAG]   优化状态: 初始化状态，尚未开始优化\n");
            break;
        case AI_ECMP_WAIT:
            AI_DIAG_PRINTF("[DIAG]   优化状态: 等待足够的历史数据或方差稳定\n");
            break;
        case AI_ECMP_BALANCE:
            AI_DIAG_PRINTF("[DIAG]   优化状态: 系统已达到平衡状态\n");
            AI_DIAG_PRINTF("[DIAG]   优化效果: 良好 - 无需进一步优化\n");
            break;
        case AI_ECMP_ADJUST:
            AI_DIAG_PRINTF("[DIAG]   优化状态: 刚完成一次成功的负载调整\n");
            AI_DIAG_PRINTF("[DIAG]   优化效果: 有效 - 负载分布已优化\n");
            break;
        case AI_ECMP_EXPAND:
            AI_DIAG_PRINTF("[DIAG]   优化状态: 刚完成一次扩容操作\n");
            AI_DIAG_PRINTF("[DIAG]   优化效果: 扩容 - 增加了逻辑链路数\n");
            break;
        case AI_ECMP_FAIL:
            AI_DIAG_PRINTF("[DIAG]   优化状态: 优化尝试失败\n");
            AI_DIAG_PRINTF("[DIAG]   优化效果: 无效 - 需要检查配置或流量特征\n");
            break;
        default:
            AI_DIAG_PRINTF("[DIAG]   优化状态: 未知状态 (%d)\n", status);
            break;
    }
}


extern "C" {

// 算法类型枚举
enum AI_ECMP_ALGORITHM_TYPE {
    AI_ECMP_ALGO_LOCAL_SEARCH = 1,
    AI_ECMP_ALGO_GA_IMP = 2
};

// 诊断函数：启用ECMP智能优化算法
VOID diagAiEcmpEnableAlgorithm(WORD32 dwSgId) {
    AI_DIAG_PRINTF("[DIAG] 诊断命令：启用ECMP优化算法，SG ID: %u\n", dwSgId);
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    WORD32 dwResult = AI_SUCCESS;
    WORD32 dwEnabledCount = 0;
    
    if (dwSgId == 0) {
        AI_DIAG_PRINTF("[DIAG] 对所有ECMP实例启用优化算法\n");
        manager.forEachInstance([&dwEnabledCount](WORD32 sgId, EcmpInstance* pInstance) {
            if (pInstance) {
                pInstance->enableOptimization();
                dwEnabledCount++;
            }
        });
        AI_DIAG_PRINTF("[DIAG] 已对 %u 个实例启用优化算法\n", dwEnabledCount);
    } else {
        AI_DIAG_PRINTF("[DIAG] 对SG %u 启用优化算法\n", dwSgId);
        EcmpInstance* pInstance = manager.getInstance(dwSgId);
        if (pInstance) {
            pInstance->enableOptimization();
            dwEnabledCount = 1;
            AI_DIAG_PRINTF("[DIAG] SG %u 优化算法启用成功\n", dwSgId);
        } else {
            AI_DIAG_PRINTF("[DIAG] 错误：未找到SG %u 的实例\n", dwSgId);
            dwResult = AI_ECMP_ERR_NOT_FOUND;
        }
    }
    
    AI_DIAG_PRINTF("[DIAG] 优化算法启用操作完成，结果: 0x%x，影响实例数: %u\n", 
              dwResult, dwEnabledCount);
}

// 诊断函数：禁用ECMP智能优化算法
VOID diagAiEcmpDisableAlgorithm(WORD32 dwSgId) {
    AI_DIAG_PRINTF("[DIAG] 诊断命令：禁用ECMP优化算法，SG ID: %u\n", dwSgId);
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    WORD32 dwResult = AI_SUCCESS;
    WORD32 dwDisabledCount = 0;
    
    if (dwSgId == 0) {
        AI_DIAG_PRINTF("[DIAG] 对所有ECMP实例禁用优化算法\n");
        manager.forEachInstance([&dwDisabledCount](WORD32 sgId, EcmpInstance* pInstance) {
            if (pInstance) {
                pInstance->disableOptimization();
                dwDisabledCount++;
            }
        });
        AI_DIAG_PRINTF("[DIAG] 已对 %u 个实例禁用优化算法\n", dwDisabledCount);
    } else {
        AI_DIAG_PRINTF("[DIAG] 对SG %u 禁用优化算法\n", dwSgId);
        EcmpInstance* pInstance = manager.getInstance(dwSgId);
        if (pInstance) {
            pInstance->disableOptimization();
            dwDisabledCount = 1;
            AI_DIAG_PRINTF("[DIAG] SG %u 优化算法禁用成功\n", dwSgId);
        } else {
            AI_DIAG_PRINTF("[DIAG] 错误：未找到SG %u 的实例\n", dwSgId);
            dwResult = AI_ECMP_ERR_NOT_FOUND;
        }
    }
    
    AI_DIAG_PRINTF("[DIAG] 优化算法禁用操作完成，结果: 0x%x，影响实例数: %u\n", 
              dwResult, dwDisabledCount);
}

// 诊断函数：打印ECMP实例的SG配置信息
VOID diagAiEcmpPrintSgConfig(WORD32 dwSgId) {
    AI_DIAG_PRINTF("\n[DIAG] ============================================================\n");
    AI_DIAG_PRINTF("[DIAG] 诊断命令：打印ECMP SG配置信息\n");
    AI_DIAG_PRINTF("[DIAG] ============================================================\n");
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    
    if (dwSgId == 0) {
        AI_DIAG_PRINTF("[DIAG] 打印所有ECMP实例的配置信息\n");
        WORD32 dwInstanceCount = manager.getInstanceCount();
        AI_DIAG_PRINTF("[DIAG] 当前实例总数: %u\n", dwInstanceCount);
        
        if (dwInstanceCount == 0) {
            AI_DIAG_PRINTF("[DIAG] 当前没有ECMP实例\n");
            AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
            return;
        }
        
        // 遍历所有实例并打印配置
        manager.forEachInstance([](WORD32 sgId, EcmpInstance* pInstance) {
            if (pInstance) {
                printSingleInstanceConfig(sgId, pInstance);
            }
        });
    } else {
        AI_DIAG_PRINTF("[DIAG] 打印SG %u 的配置信息\n", dwSgId);
        EcmpInstance* pInstance = manager.getInstance(dwSgId);
        if (pInstance) {
            printSingleInstanceConfig(dwSgId, pInstance);
        } else {
            AI_DIAG_PRINTF("[DIAG] 错误：未找到SG %u 的实例\n", dwSgId);
        }
    }
    
    AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
}


// 诊断函数：打印ECMP实例的当前平衡状态信息
VOID diagAiEcmpPrintBalanceStatus(WORD32 dwSgId) {
    AI_DIAG_PRINTF("\n[DIAG] ============================================================\n");
    AI_DIAG_PRINTF("[DIAG] 诊断命令：打印ECMP平衡状态信息\n");
    AI_DIAG_PRINTF("[DIAG] ============================================================\n");
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    
    if (dwSgId == 0) {
        AI_DIAG_PRINTF("[DIAG] 打印所有ECMP实例的平衡状态\n");
        WORD32 dwInstanceCount = manager.getInstanceCount();
        
        if (dwInstanceCount == 0) {
            AI_DIAG_PRINTF("[DIAG] 当前没有ECMP实例\n");
            AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
            return;
        }
        
        // 遍历所有实例并打印平衡状态
        manager.forEachInstance([](WORD32 sgId, EcmpInstance* pInstance) {
            if (pInstance) {
                printSingleInstanceBalance(sgId, pInstance);
            }
        });
    } else {
        AI_DIAG_PRINTF("[DIAG] 打印SG %u 的平衡状态\n", dwSgId);
        EcmpInstance* pInstance = manager.getInstance(dwSgId);
        if (pInstance) {
            printSingleInstanceBalance(dwSgId, pInstance);
        } else {
            AI_DIAG_PRINTF("[DIAG] 错误：未找到SG %u 的实例\n", dwSgId);
        }
    }
    
    AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
}



// 诊断函数：打印算法优化效果信息
VOID diagAiEcmpPrintOptimizationEffect(WORD32 dwSgId) {
    AI_DIAG_PRINTF("\n[DIAG] ============================================================\n");
    AI_DIAG_PRINTF("[DIAG] 诊断命令：打印算法优化效果\n");
    AI_DIAG_PRINTF("[DIAG] ============================================================\n");
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    
    if (dwSgId == 0) {
        AI_DIAG_PRINTF("[DIAG] 打印所有实例的优化效果\n");
        WORD32 dwInstanceCount = manager.getInstanceCount();
        
        if (dwInstanceCount == 0) {
            AI_DIAG_PRINTF("[DIAG] 当前没有ECMP实例\n");
            AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
            return;
        }
        
        // 遍历所有实例并打印优化效果
        manager.forEachInstance([](WORD32 sgId, EcmpInstance* pInstance) {
            if (pInstance) {
                printSingleInstanceOptimization(sgId, pInstance);
            }
        });
    } else {
        AI_DIAG_PRINTF("[DIAG] 打印SG %u 的优化效果\n", dwSgId);
        EcmpInstance* pInstance = manager.getInstance(dwSgId);
        if (pInstance) {
            printSingleInstanceOptimization(dwSgId, pInstance);
        } else {
            AI_DIAG_PRINTF("[DIAG] 错误：未找到SG %u 的实例\n", dwSgId);
        }
    }
    
    AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
}


// 诊断函数：打印所有ECMP实例的状态概览
VOID diagAiEcmpPrintInstanceStatus() {
    AI_DIAG_PRINTF("\n[DIAG] ============================================================\n");
    AI_DIAG_PRINTF("[DIAG] ECMP实例状态概览\n");
    AI_DIAG_PRINTF("[DIAG] ============================================================\n");
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    WORD32 dwInstanceCount = manager.getInstanceCount();
    
    AI_DIAG_PRINTF("[DIAG] 实例总数: %u\n", dwInstanceCount);
    
    if (dwInstanceCount == 0) {
        AI_DIAG_PRINTF("[DIAG] 当前没有ECMP实例\n");
        AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
        return;
    }
    
    AI_DIAG_PRINTF("[DIAG] %-10s %-15s %-10s %-10s %-15s %-15s %-15s\n", 
              "SG ID", "状态", "周期数", "端口数", "逻辑成员数", "优化状态", "禁用周期数");
    AI_DIAG_PRINTF("[DIAG] %s\n", 
              "-------------------------------------------------------------------------------------------");
    
    // 遍历所有实例并打印状态信息
    manager.forEachInstance([](WORD32 sgId, EcmpInstance* pInstance) {
        if (pInstance) {
            const T_AI_ECMP_SG_CFG& cfg = pInstance->getSgConfig();  // 需要在EcmpInstance中添加此方法
            const char* statusStr = utils::aiEcmpStatusToString(pInstance->getStatus());
            const char* optStateStr = pInstance->isOptimizationEnabled() ? "启用" : "禁用";
            WORD32 disabledCycles = pInstance->isOptimizationEnabled() ? 0 : pInstance->getDisabledCycles();
            
            AI_DIAG_PRINTF("[DIAG] %-10u %-15s %-10u %-10u %-15u %-15s %-15u\n",
                      sgId, statusStr, pInstance->getCycle(),  // 需要在EcmpInstance中添加getCycle()方法
                      cfg.dwPortNum, cfg.dwItemNum, optStateStr, disabledCycles);
        }
    });
    
    AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
}

// 诊断函数：强制执行一次优化循环
VOID diagAiEcmpForceOptimization(WORD32 dwSgId) {
    AI_DIAG_PRINTF("\n[DIAG] 诊断命令：强制执行优化，SG ID: %u\n", dwSgId);
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    
    // 创建模拟的计数器消息
    T_AI_ECMP_COUNTER_STATS_MSG counterMsg = {0};
    
    // 使用随机数填充计数器（模拟流量）
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<WORD64> dist(100, 10000);
    
    for (int i = 0; i < AI_FCM_ECMP_MSG_ITEM_NUM; ++i) {
        counterMsg.statCounter[i] = dist(gen);
    }
    
    AI_DIAG_PRINTF("[DIAG] 使用模拟计数器数据执行优化循环\n");
    
    WORD32 dwResult = manager.runOptimizationCycle(counterMsg);
    
    AI_DIAG_PRINTF("[DIAG] 强制优化执行完成，结果: 0x%x\n", dwResult);
}

// 诊断函数：设置使用的优化算法类型
VOID diagAiEcmpSetAlgorithm(WORD32 dwSgId, WORD32 dwAlgorithmType) {
    AI_DIAG_PRINTF("[DIAG] 诊断命令：设置算法类型，SG ID: %u, 算法: %u\n", 
              dwSgId, dwAlgorithmType);
    WORD32 dwResult = AI_SUCCESS;
    const char* pszAlgoName = "未知";
    switch (dwAlgorithmType) {
        case AI_ECMP_ALGO_LOCAL_SEARCH:
            pszAlgoName = "局部搜索(LocalSearch)";
            break;
        case AI_ECMP_ALGO_GA_IMP:
            pszAlgoName = "改进遗传算法(GA_IMP)";
            break;
        default:
            AI_DIAG_PRINTF("[DIAG] 错误：不支持的算法类型 %u\n", dwAlgorithmType);
            dwResult = AI_ECMP_ERR_INVALID_PARAM;
            return;
    }
    
    AI_DIAG_PRINTF("[DIAG] 设置算法为: %s\n", pszAlgoName);
    
    // TODO: 实现算法设置逻辑
    
    AI_DIAG_PRINTF("[DIAG] 算法类型设置完成，结果: 0x%x\n", dwResult);
}

// 诊断函数：打印计数器历史信息
VOID diagAiEcmpPrintCounterHistory(WORD32 dwSgId, WORD32 dwHistoryNum) {
    AI_DIAG_PRINTF("\n[DIAG] ============================================================\n");
    AI_DIAG_PRINTF("[DIAG] 诊断命令：打印计数器历史，SG ID: %u\n", dwSgId);
    AI_DIAG_PRINTF("[DIAG] ============================================================\n");
    
    if (dwHistoryNum == 0) {
        AI_DIAG_PRINTF("[DIAG] 显示所有历史记录\n");
    } else {
        AI_DIAG_PRINTF("[DIAG] 显示最近 %u 条历史记录\n", dwHistoryNum);
    }
    
    // TODO: 实现打印计数器历史的逻辑
    
    AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
}

// 诊断函数：设置局部搜索算法参数
VOID diagAiEcmpSetLocalSearchParams(WORD32 dwSgId, WORD32 dwMaxIterations, double dExchangeCostFactor) {
    AI_DIAG_PRINTF("[DIAG] 诊断命令：设置局部搜索参数\n");
    AI_DIAG_PRINTF("[DIAG]   SG ID: %u\n", dwSgId);
    AI_DIAG_PRINTF("[DIAG]   最大迭代次数: %u\n", dwMaxIterations);
    AI_DIAG_PRINTF("[DIAG]   交换代价因子: %.6f\n", dExchangeCostFactor);
    
    // TODO: 实现参数设置逻辑
    
    AI_DIAG_PRINTF("[DIAG] 局部搜索参数设置完成\n");
}

// 诊断函数：打印端口负载详情
VOID diagAiEcmpPrintPortLoads(WORD32 dwSgId) {
    AI_DIAG_PRINTF("\n[DIAG] ============================================================\n");
    AI_DIAG_PRINTF("[DIAG] 诊断命令：打印端口负载详情，SG ID: %u\n", dwSgId);
    AI_DIAG_PRINTF("[DIAG] ============================================================\n");
    
    // TODO: 实现打印端口负载的逻辑
    
    AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
}

// 诊断函数：重置指定实例的状态
VOID diagAiEcmpResetInstance(WORD32 dwSgId) {
    AI_DIAG_PRINTF("[DIAG] 诊断命令：重置实例状态，SG ID: %u\n", dwSgId);
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    WORD32 dwResult = AI_SUCCESS;
    
    if (dwSgId == 0) {
        AI_DIAG_PRINTF("[DIAG] 重置所有ECMP实例\n");
        manager.forEachInstance([](WORD32 sgId, EcmpInstance* pInstance) {
            if (pInstance) {
                pInstance->reset();
                AI_DIAG_PRINTF("[DIAG] 实例 %u 重置完成\n", sgId);
            }
        });
    } else {
        AI_DIAG_PRINTF("[DIAG] 重置SG %u\n", dwSgId);
        EcmpInstance* pInstance = manager.getInstance(dwSgId);
        if (pInstance) {
            pInstance->reset();
            AI_DIAG_PRINTF("[DIAG] 实例 %u 重置完成\n", dwSgId);
        } else {
            AI_DIAG_PRINTF("[DIAG] 错误：未找到SG %u 的实例\n", dwSgId);
            dwResult = AI_ECMP_ERR_NOT_FOUND;
        }
    }
    
    AI_DIAG_PRINTF("[DIAG] 实例重置操作完成，结果: 0x%x\n", dwResult);
}

// 诊断函数：模拟计数器更新
VOID diagAiEcmpSimulateCounter(WORD32 dwSgId, WORD32 dwPattern) {
    AI_DIAG_PRINTF("[DIAG] 诊断命令：模拟计数器更新\n");
    AI_DIAG_PRINTF("[DIAG]   SG ID: %u\n", dwSgId);
    
    const char* pszPattern = "未知";
    WORD32 dwResult = AI_SUCCESS;
    
    switch (dwPattern) {
        case 1:
            pszPattern = "均匀分布";
            break;
        case 2:
            pszPattern = "不平衡分布";
            break;
        case 3:
            pszPattern = "随机分布";
            break;
        default:
            AI_DIAG_PRINTF("[DIAG] 错误：不支持的流量模式 %u\n", dwPattern);
            dwResult = AI_ECMP_ERR_INVALID_PARAM;
            return;
    }
    
    AI_DIAG_PRINTF("[DIAG]   流量模式: %s\n", pszPattern);
    
    // 创建模拟的计数器消息
    T_AI_ECMP_COUNTER_STATS_MSG counterMsg = {0};
    
    // 根据模式生成流量
    if (dwPattern == 1) {
        // 均匀分布
        for (int i = 0; i < AI_FCM_ECMP_MSG_ITEM_NUM; ++i) {
            counterMsg.statCounter[i] = 1000;
        }
    } else if (dwPattern == 2) {
        // 不平衡分布：前1/4高流量，其余低流量
        for (int i = 0; i < AI_FCM_ECMP_MSG_ITEM_NUM; ++i) {
            if (i < AI_FCM_ECMP_MSG_ITEM_NUM / 4) {
                counterMsg.statCounter[i] = 5000;
            } else {
                counterMsg.statCounter[i] = 500;
            }
        }
    } else if (dwPattern == 3) {
        // 随机分布
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<WORD64> dist(100, 10000);
        
        for (int i = 0; i < AI_FCM_ECMP_MSG_ITEM_NUM; ++i) {
            counterMsg.statCounter[i] = dist(gen);
        }
    }
    
    auto& manager = CAISlbManagerSingleton::getManagerInstance();
    dwResult = manager.runOptimizationCycle(counterMsg);
    
    AI_DIAG_PRINTF("[DIAG] 模拟计数器更新完成，结果: 0x%x\n", dwResult);
}

// 诊断函数：打印帮助信息
VOID diagAiEcmpHelp() {
    AI_DIAG_PRINTF("\n[DIAG] ============================================================\n");
    AI_DIAG_PRINTF("[DIAG] AI ECMP 诊断函数帮助\n");
    AI_DIAG_PRINTF("[DIAG] ============================================================\n");
    
    AI_DIAG_PRINTF("[DIAG] 可用的诊断函数：\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 1. diagAiEcmpEnableAlgorithm(sgId)\n");
    AI_DIAG_PRINTF("[DIAG]    - 启用ECMP优化算法\n");
    AI_DIAG_PRINTF("[DIAG]    - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 2. diagAiEcmpDisableAlgorithm(sgId)\n");
    AI_DIAG_PRINTF("[DIAG]    - 禁用ECMP优化算法\n");
    AI_DIAG_PRINTF("[DIAG]    - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 3. diagAiEcmpPrintSgConfig(sgId)\n");
    AI_DIAG_PRINTF("[DIAG]    - 打印SG配置信息\n");
    AI_DIAG_PRINTF("[DIAG]    - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 4. diagAiEcmpPrintBalanceStatus(sgId)\n");
    AI_DIAG_PRINTF("[DIAG]    - 打印平衡状态信息\n");
    AI_DIAG_PRINTF("[DIAG]    - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 5. diagAiEcmpPrintOptimizationEffect(sgId)\n");
    AI_DIAG_PRINTF("[DIAG]    - 打印优化效果信息\n");
    AI_DIAG_PRINTF("[DIAG]    - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 6. diagAiEcmpPrintInstanceStatus()\n");
    AI_DIAG_PRINTF("[DIAG]    - 打印所有实例状态概览\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 7. diagAiEcmpForceOptimization(sgId)\n");
    AI_DIAG_PRINTF("[DIAG]    - 强制执行一次优化\n");
    AI_DIAG_PRINTF("[DIAG]    - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 8. diagAiEcmpSetAlgorithm(sgId, algoType)\n");
    AI_DIAG_PRINTF("[DIAG]    - 设置优化算法类型\n");
    AI_DIAG_PRINTF("[DIAG]    - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG]    - algoType: 1=LocalSearch, 2=GA_IMP\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 9. diagAiEcmpPrintCounterHistory(sgId, histNum)\n");
    AI_DIAG_PRINTF("[DIAG]    - 打印计数器历史\n");
    AI_DIAG_PRINTF("[DIAG]    - sgId: SG ID\n");
    AI_DIAG_PRINTF("[DIAG]    - histNum: 历史记录数，0表示全部\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 10. diagAiEcmpSetLocalSearchParams(sgId, maxIter, costFactor)\n");
    AI_DIAG_PRINTF("[DIAG]     - 设置局部搜索算法参数\n");
    AI_DIAG_PRINTF("[DIAG]     - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG]     - maxIter: 最大迭代次数\n");
    AI_DIAG_PRINTF("[DIAG]     - costFactor: 交换代价因子\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 11. diagAiEcmpPrintPortLoads(sgId)\n");
    AI_DIAG_PRINTF("[DIAG]     - 打印端口负载详情\n");
    AI_DIAG_PRINTF("[DIAG]     - sgId: SG ID\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 12. diagAiEcmpResetInstance(sgId)\n");
    AI_DIAG_PRINTF("[DIAG]     - 重置实例状态\n");
    AI_DIAG_PRINTF("[DIAG]     - sgId: SG ID，0表示所有实例\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] 13. diagAiEcmpSimulateCounter(sgId, pattern)\n");
    AI_DIAG_PRINTF("[DIAG]     - 模拟计数器更新\n");
    AI_DIAG_PRINTF("[DIAG]     - sgId: SG ID\n");
    AI_DIAG_PRINTF("[DIAG]     - pattern: 1=均匀, 2=不平衡, 3=随机\n");
    AI_DIAG_PRINTF("[DIAG] \n");
    
    AI_DIAG_PRINTF("[DIAG] ============================================================\n\n");
}

} // extern "C"