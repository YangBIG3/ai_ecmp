#ifndef AI_ECMP_DIAG_H
#define AI_ECMP_DIAG_H

#include "ai_ecmp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 诊断函数：启用ECMP智能优化算法
 * @param dwSgId SG ID，0表示对所有实例生效
 */
VOID diagAiEcmpEnableAlgorithm(WORD32 dwSgId);

/**
 * @brief 诊断函数：禁用ECMP智能优化算法
 * @param dwSgId SG ID，0表示对所有实例生效
 */
VOID diagAiEcmpDisableAlgorithm(WORD32 dwSgId);

/**
 * @brief 诊断函数：打印ECMP实例的SG配置信息
 * @param dwSgId SG ID，0表示打印所有实例
 */
VOID diagAiEcmpPrintSgConfig(WORD32 dwSgId);

/**
 * @brief 诊断函数：打印ECMP实例的当前平衡状态信息
 * @param dwSgId SG ID，0表示打印所有实例
 */
VOID diagAiEcmpPrintBalanceStatus(WORD32 dwSgId);

/**
 * @brief 诊断函数：打印算法优化效果信息
 * @param dwSgId SG ID，0表示打印所有实例
 */
VOID diagAiEcmpPrintOptimizationEffect(WORD32 dwSgId);

/**
 * @brief 诊断函数：打印所有ECMP实例的状态概览
 */
VOID diagAiEcmpPrintInstanceStatus();

/**
 * @brief 诊断函数：强制执行一次优化循环
 * @param dwSgId SG ID，0表示对所有实例执行
 */
VOID diagAiEcmpForceOptimization(WORD32 dwSgId);

/**
 * @brief 诊断函数：设置使用的优化算法类型
 * @param dwSgId SG ID，0表示对所有实例生效
 * @param dwAlgorithmType 算法类型: 1-LocalSearch, 2-GA_IMP
 */
VOID diagAiEcmpSetAlgorithm(WORD32 dwSgId, WORD32 dwAlgorithmType);

/**
 * @brief 诊断函数：打印计数器历史信息
 * @param dwSgId SG ID
 * @param dwHistoryNum 显示的历史记录数量，0表示全部
 */
VOID diagAiEcmpPrintCounterHistory(WORD32 dwSgId, WORD32 dwHistoryNum);

/**
 * @brief 诊断函数：设置局部搜索算法参数
 * @param dwSgId SG ID，0表示对所有实例生效
 * @param dwMaxIterations 最大迭代次数
 * @param dExchangeCostFactor 交换代价因子
 */
VOID diagAiEcmpSetLocalSearchParams(WORD32 dwSgId, WORD32 dwMaxIterations, double dExchangeCostFactor);

/**
 * @brief 诊断函数：打印端口负载详情
 * @param dwSgId SG ID
 */
VOID diagAiEcmpPrintPortLoads(WORD32 dwSgId);

/**
 * @brief 诊断函数：重置指定实例的状态
 * @param dwSgId SG ID，0表示重置所有实例
 */
VOID diagAiEcmpResetInstance(WORD32 dwSgId);

/**
 * @brief 诊断函数：模拟计数器更新
 * @param dwSgId SG ID
 * @param dwPattern 流量模式: 1-均匀, 2-不平衡, 3-随机
 */
VOID diagAiEcmpSimulateCounter(WORD32 dwSgId, WORD32 dwPattern);

/**
 * @brief 诊断函数：打印帮助信息
 */
VOID diagAiEcmpHelp();

#ifdef __cplusplus
}
#endif

#endif /* AI_ECMP_DIAG_H */
