#ifndef AI_ECMP_METRICS_HPP
#define AI_ECMP_METRICS_HPP

#include "ai_ecmp_types.h"
#include <unordered_map>
#include <vector>

namespace ai_ecmp {
namespace utils {

/**
 * @brief 将AI_ECMP状态枚举转换为字符串
 * @param status 状态枚举值
 * @return 状态字符串
 */
const char* aiEcmpStatusToString(T_AI_ECMP_STATUS status);

/**
 * @brief 根据成员表和计数器计算各端口负载
 * @param v_memberTable 成员表 (hash_index -> port_id)
 * @param v_memberCounts 成员计数表
 * @return 端口负载映射 (port_id -> load_count)
 */
std::unordered_map<WORD32, WORD64> calculatePortLoads(
    const std::unordered_map<WORD32, WORD32>& v_memberTable,
    const std::vector<WORD64>& v_memberCounts);

/**
 * @brief 计算端口利用率
 * @param v_portLoads 端口负载映射 (port_id -> load_count)
 * @param v_portSpeeds 端口速率映射 (port_id -> speed)
 * @return 端口利用率映射 (port_id -> utilization_ratio)
 */
std::unordered_map<WORD32, double> calculatePortUtilization(
    const std::unordered_map<WORD32, WORD64>& v_portLoads,
    const std::unordered_map<WORD32, WORD32>& v_portSpeeds);

/**
 * @brief 计算利用率平均值
 * @param v_portUtilization 端口利用率映射
 * @return 平均利用率
 */
double calculateAverageUtilization(
    const std::unordered_map<WORD32, double>& v_portUtilization);

/**
 * @brief 计算负载平衡指标
 * @param v_portLoads 端口负载映射 (port_id -> load_count)
 * @param v_portSpeeds 端口速率映射 (port_id -> speed)
 * @return 负载平衡评估结果
 */
T_AI_ECMP_EVAL calculateLoadBalanceMetrics(
    const std::unordered_map<WORD32, WORD64>& v_portLoads,
    const std::unordered_map<WORD32, WORD32>& v_portSpeeds);

/**
 * @brief 计算两个哈希索引交换后的负载改进量
 * @param v_memberTable 成员表
 * @param v_memberCounts 成员计数表
 * @param v_portLoads 端口负载表
 * @param v_portSpeeds 端口速率表
 * @param dwHashIndex1 要交换的第一个哈希索引
 * @param dwHashIndex2 要交换的第二个哈希索引
 * @return 交换后的平衡度改进量
 */
double calculateSwapImprovement(
    const std::unordered_map<WORD32, WORD32>& v_memberTable,
    const std::vector<WORD64>& v_memberCounts,
    const std::unordered_map<WORD32, WORD64>& v_portLoads,
    const std::unordered_map<WORD32, WORD32>& v_portSpeeds,
    WORD32 dwHashIndex1, WORD32 dwHashIndex2);

/**
 * @brief 根据评估结果计算平衡得分
 * @param v_eval 评估结果
 * @return 平衡得分 (越高越好)
 */
double calculateBalanceScore(const T_AI_ECMP_EVAL& v_eval);

/**
 * @brief 应用惩罚函数
 * @param value 输入值
 * @param threshold 阈值
 * @return 惩罚值
 */
double applyPenalty(double value, double threshold);

// ===== 新增：统计和方差计算工具函数 =====

/**
 * @brief 计算数值向量的平均值
 * @param values 数值向量
 * @return 平均值
 */
double calculateMean(const std::vector<double>& values);

/**
 * @brief 计算数值向量的标准差
 * @param values 数值向量
 * @param mean 平均值
 * @return 标准差
 */
double calculateStandardDeviation(const std::vector<double>& values, double mean);

/**
 * @brief 计算数值向量的变异系数
 * @param values 数值向量
 * @return 变异系数（标准差/平均值）
 */
double calculateVariationCoefficient(const std::vector<double>& values);

/**
 * @brief 计算计数器历史数据的整体变异系数
 * @param counterHistory 计数器历史数据（按周期存储）
 * @param memberCounts 当前成员计数表（用于获取size）
 * @return 整体变异系数
 */
double calculateCounterVarianceCoefficient(
    const std::vector<std::vector<WORD64>>& counterHistory,
    const std::vector<WORD64>& memberCounts);

/**
 * @brief 计算负载均衡改进百分比
 * @param beforeEval 优化前的评估结果
 * @param afterEval 优化后的评估结果
 * @return 改进百分比（负值表示恶化）
 */
double calculateImprovementPercentage(
    const T_AI_ECMP_EVAL& beforeEval,
    const T_AI_ECMP_EVAL& afterEval);

/**
 * @brief 评估优化效果是否达到最小改进阈值
 * @param beforeEval 优化前的评估结果
 * @param afterEval 优化后的评估结果
 * @param minImprovementThreshold 最小改进阈值（百分比，默认1%）
 * @return true表示改进达到阈值，false表示改进不足
 */


} // namespace utils
} // namespace ai_ecmp

#endif // AI_ECMP_METRICS_HPP
