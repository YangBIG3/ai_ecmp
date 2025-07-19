#include "ai_ecmp_metrics.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace ai_ecmp {
namespace utils {

const char* aiEcmpStatusToString(T_AI_ECMP_STATUS status) {
    switch (status) {
        case AI_ECMP_INIT:    return "INIT";
        case AI_ECMP_WAIT:    return "WAIT";
        case AI_ECMP_ADJUST:  return "ADJUST";
        case AI_ECMP_EVAL:    return "EVAL";
        case AI_ECMP_EXPAND:  return "EXPAND";
        case AI_ECMP_BALANCE: return "BALANCE";
        case AI_ECMP_FAIL:    return "FAIL";
        default:              return "UNKNOWN";
    }
}

std::unordered_map<WORD32, WORD64> calculatePortLoads(
    const std::unordered_map<WORD32, WORD32>& v_memberTable,
    const std::vector<WORD64>& v_memberCounts) {
    
    std::unordered_map<WORD32, WORD64> portLoads;
    
    // 遍历所有哈希索引
    for (const auto& entry : v_memberTable) {
        WORD32 dwHashIndex = entry.first;
        WORD32 dwPortId = entry.second;

        // 确保索引在有效范围内
        if (dwHashIndex < v_memberCounts.size()) {
            portLoads[dwPortId] += v_memberCounts[dwHashIndex];
        }
    }
    
    return portLoads;
}

std::unordered_map<WORD32, double> calculatePortUtilization(
    const std::unordered_map<WORD32, WORD64>& v_portLoads,
    const std::unordered_map<WORD32, WORD32>& v_portSpeeds) {
    
    std::unordered_map<WORD32, double> utilization;
    
    for (const auto& loadEntry : v_portLoads) {
        WORD32 portId = loadEntry.first;
        WORD64 load = loadEntry.second;
        
        auto speedIt = v_portSpeeds.find(portId);
        if (speedIt != v_portSpeeds.end() && speedIt->second > 0) {
            utilization[portId] = static_cast<double>(load) / speedIt->second;
        }
    }
    
    return utilization;
}

double calculateAverageUtilization(
    const std::unordered_map<WORD32, double>& v_portUtilization) {
    
    if (v_portUtilization.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (const auto& entry : v_portUtilization) {
        sum += entry.second;
    }
    
    return sum / v_portUtilization.size();
}

T_AI_ECMP_EVAL calculateLoadBalanceMetrics(
    const std::unordered_map<WORD32, WORD64>& v_portLoads,
    const std::unordered_map<WORD32, WORD32>& v_portSpeeds) {
    
    T_AI_ECMP_EVAL eval = {0};
    
    // 如果没有端口负载数据，直接返回默认值
    if (v_portLoads.empty() || v_portSpeeds.empty()) {
        return eval;
    }
    
    // 计算归一化负载 (考虑端口速率)
    std::vector<double> normalizedLoads;
    for (const auto& entry : v_portLoads) {
        WORD32 dwPortId = entry.first;
        WORD64 load = entry.second;
        
        // 寻找对应端口的速率
        auto speedIt = v_portSpeeds.find(dwPortId);
        if (speedIt != v_portSpeeds.end() && speedIt->second > 0) {
            WORD32 dwSpeed = speedIt->second;
            normalizedLoads.push_back(static_cast<double>(load) / dwSpeed);
        }
    }
    
    // 如果没有有效的归一化负载，直接返回默认值
    if (normalizedLoads.empty()) {
        return eval;
    }
    
    // 计算平均负载
    double avgLoad = std::accumulate(normalizedLoads.begin(), normalizedLoads.end(), 0.0) / normalizedLoads.size();
    
    // 找出最大和最小负载
    double minLoad = *std::min_element(normalizedLoads.begin(), normalizedLoads.end());
    double maxLoad = *std::max_element(normalizedLoads.begin(), normalizedLoads.end());
    
    // 计算正负偏差
    if (avgLoad > 0) {
        eval.upBoundGap = (maxLoad - avgLoad) / avgLoad;
        eval.lowBoundGap = (avgLoad - minLoad) / avgLoad;
        eval.totalGap = eval.upBoundGap + eval.lowBoundGap;
    }
    
    // 计算平均偏差
    double sumAbsDev = 0.0;
    for (double loadValue : normalizedLoads) {
        sumAbsDev += std::abs(loadValue - avgLoad);
    }
    
    if (avgLoad > 0) {
        eval.avgGap = sumAbsDev / normalizedLoads.size() / avgLoad;
    }
    
    // 计算平衡度得分 (越大越好)
    eval.balanceScore = -eval.totalGap;
    
    return eval;
}

double calculateSwapImprovement(
    const std::unordered_map<WORD32, WORD32>& v_memberTable,
    const std::vector<WORD64>& v_memberCounts,
    const std::unordered_map<WORD32, WORD64>& v_portLoads,
    const std::unordered_map<WORD32, WORD32>& v_portSpeeds,
    WORD32 dwHashIndex1, WORD32 dwHashIndex2) {
    
    // 检查索引是否有效
    auto it1 = v_memberTable.find(dwHashIndex1);
    auto it2 = v_memberTable.find(dwHashIndex2);
    
    if (it1 == v_memberTable.end() || it2 == v_memberTable.end() ||
        dwHashIndex1 >= v_memberCounts.size() || dwHashIndex2 >= v_memberCounts.size()) {
        return 0.0;  // 无效索引，没有改进
    }
    
    WORD32 dwPortId1 = it1->second;
    WORD32 dwPortId2 = it2->second;
    
    // 如果交换的是同一个端口，没有改进
    if (dwPortId1 == dwPortId2) {
        return 0.0;
    }
    
    // 获取流量计数
    WORD64 count1 = v_memberCounts[dwHashIndex1];
    WORD64 count2 = v_memberCounts[dwHashIndex2];
    
    // 计算交换前的端口负载
    std::unordered_map<WORD32, WORD64> originalLoads = v_portLoads;
    
    // 计算交换后的端口负载
    std::unordered_map<WORD32, WORD64> newLoads = v_portLoads;
    
    // 移除原始流量
    newLoads[dwPortId1] -= count1;
    newLoads[dwPortId2] -= count2;
    
    // 添加交换后的流量
    newLoads[dwPortId1] += count2;
    newLoads[dwPortId2] += count1;
    
    // 计算交换前后的平衡度
    T_AI_ECMP_EVAL originalEval = calculateLoadBalanceMetrics(originalLoads, v_portSpeeds);
    T_AI_ECMP_EVAL newEval = calculateLoadBalanceMetrics(newLoads, v_portSpeeds);
    
    // 返回改进量 (新平衡度 - 原平衡度)
    return calculateBalanceScore(newEval) - calculateBalanceScore(originalEval);
}

double calculateBalanceScore(const T_AI_ECMP_EVAL& v_eval) {
    // 加权计算平衡度得分，考虑正负偏差和平均偏差
    // 权重可以根据实际需求调整
    double posWeight = 1.0;
    double negWeight = 1.0;
    double avgWeight = 0.5;
    
    // 对偏差应用惩罚函数
    double posPenalty = applyPenalty(v_eval.upBoundGap, 0.1);
    double negPenalty = applyPenalty(v_eval.lowBoundGap, 0.1);
    
    // 计算得分 (负值，越接近0越好)
    // return -(posWeight * (v_eval.upBoundGap + posPenalty) + 
    //          negWeight * (v_eval.lowBoundGap + negPenalty) + 
    //          avgWeight * v_eval.avgGap);

    return -(posWeight * v_eval.upBoundGap + negWeight * v_eval.lowBoundGap);
}

double applyPenalty(double value, double threshold) {
    // 当值超过阈值时，应用二次惩罚
    if (value > threshold) {
        return (value - threshold) * (value - threshold);
    }
    return 0.0;
}

// ===== 新增：统计和方差计算工具函数实现 =====

double calculateMean(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double value : values) {
        sum += value;
    }
    
    return sum / values.size();
}

double calculateStandardDeviation(const std::vector<double>& values, double mean) {
    if (values.size() <= 1) {
        return 0.0;
    }
    
    double variance = 0.0;
    for (double value : values) {
        double diff = value - mean;
        variance += diff * diff;
    }
    
    variance /= (values.size() - 1);  // 样本标准差
    return std::sqrt(variance);
}

double calculateVariationCoefficient(const std::vector<double>& values) {
    if (values.empty()) {
        return 1.0; // 返回高变异系数，表示不稳定
    }
    
    double mean = calculateMean(values);
    if (mean <= 0) {
        return 1.0; // 返回高变异系数，表示不稳定
    }
    
    double stdDev = calculateStandardDeviation(values, mean);
    return stdDev / mean;
}

double calculateCounterVarianceCoefficient(
    const std::vector<std::vector<WORD64>>& counterHistory,
    const std::vector<WORD64>& memberCounts) {
    
    if (counterHistory.size() < 2) {
        return 1.0; // 返回高方差，表示不稳定
    }
    
    // 计算每个哈希索引在历史周期中的变异系数，然后取平均值
    std::vector<double> allVarianceCoeffs;
    
    size_t memberCount = memberCounts.size();
    for (size_t hashIndex = 0; hashIndex < memberCount; ++hashIndex) {
        std::vector<double> memberValues;
        
        // 收集该哈希索引在所有历史周期中的计数值
        for (const auto& historyCounts : counterHistory) {
            if (hashIndex < historyCounts.size()) {
                memberValues.push_back(static_cast<double>(historyCounts[hashIndex]));
            }
        }
        
        if (memberValues.size() >= 2) {
            double varianceCoeff = calculateVariationCoefficient(memberValues);
            allVarianceCoeffs.push_back(varianceCoeff);
        }
    }
    
    // 返回所有哈希索引变异系数的平均值
    if (allVarianceCoeffs.empty()) {
        return 1.0; // 返回高方差，表示不稳定
    }
    
    return calculateMean(allVarianceCoeffs);
}

// 注意：isCounterVarianceStable 方法已移动到 EcmpInstance 类中，这里不再实现

// ===== 新增：改进百分比计算函数实现 =====

/**
 * @brief 计算负载均衡改进百分比
 * @param beforeEval 优化前的评估结果
 * @param afterEval 优化后的评估结果
 * @return 改进百分比（负值表示恶化）
 */
double calculateImprovementPercentage(
    const T_AI_ECMP_EVAL& beforeEval,
    const T_AI_ECMP_EVAL& afterEval) {
    
    // 计算优化前后的总体偏差（正偏差 + 负偏差）
    double totalBiasBefore = beforeEval.upBoundGap + beforeEval.lowBoundGap;
    double totalBiasAfter = afterEval.upBoundGap + afterEval.lowBoundGap;
    
    // 如果优化前没有偏差，无法计算改进百分比
    if (totalBiasBefore <= 0) {
        return 0.0;
    }
    
    // 计算改进百分比：(前-后)/前 * 100
    double improvementPercent = ((totalBiasBefore - totalBiasAfter) / totalBiasBefore) * 100.0;
    
    return improvementPercent;
}



} // namespace utils
} // namespace ai_ecmp
