#include "ai_ecmp_local_search.hpp"
#include "../utils/ai_ecmp_metrics.hpp"
#include <algorithm>
#include <random>
#include <chrono>
#include <vector>
#include <utility>
#include <iostream>

namespace ai_ecmp {

LocalSearch::LocalSearch(WORD32 dwMaxIterations, double exchangeCostFactor)
    : m_dwMaxIterations(dwMaxIterations)
    , m_exchangeCostFactor(exchangeCostFactor) {
}

std::unordered_map<WORD32, WORD32> LocalSearch::optimize(
    const std::unordered_map<WORD32, WORD32>& memberTable,
    const std::vector<WORD64>& memberCounts,
    const std::unordered_map<WORD32, WORD32>& portSpeeds) {
    
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 🚀 开始局部搜索优化，最大迭代次数: %u，交换代价因子: %.6f\n", 
                  m_dwMaxIterations, m_exchangeCostFactor);
    
    // 创建结果的副本
    std::unordered_map<WORD32, WORD32> result = memberTable;
    
    // 获取所有哈希索引
    std::vector<WORD32> hashIndices;
    for (const auto& entry : memberTable) {
        hashIndices.push_back(entry.first);
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 📊 成员表信息 - 哈希索引总数: %zu\n", hashIndices.size());
    
    // 如果没有足够的哈希索引用于交换，直接返回
    if (hashIndices.size() < 2) {
        XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] ⚠️ 哈希索引数量不足(%zu < 2)，无法进行交换优化\n", hashIndices.size());
        return result;
    }

    // ===== 新增：定义交换记录结构体 =====
    struct SwapRecord {
        WORD32 iteration;
        WORD32 hashIndex1;
        WORD32 hashIndex2;
        WORD32 portId1;
        WORD32 portId2;
        WORD64 count1;
        WORD64 count2;
        double improvement;
        double scoreAfter;
        double totalGapAfter;
    };
    
    // 存储所有成功的交换记录
    std::vector<SwapRecord> swapHistory;
    swapHistory.reserve(1000); // 预分配空间

    //固定随机数种子
    // std::mt19937 gen(123456789);
    
    // 初始化随机数生成器
    std::random_device randomDevice;
    std::mt19937 randomGenerator(randomDevice());
    std::uniform_int_distribution<size_t> indexDistribution(0, hashIndices.size() - 1);
    
    // 计算初始负载
    auto portLoads = utils::calculatePortLoads(result, memberCounts);
    auto originalEval = utils::calculateLoadBalanceMetrics(portLoads, portSpeeds);
    auto originalScore = utils::calculateBalanceScore(originalEval);

  
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 🎯 初始状态 - 总偏差: %.6f, 正偏差: %.6f, 负偏差: %.6f, 平衡得分: %.6f\n",
                  originalEval.totalGap, originalEval.upBoundGap, originalEval.lowBoundGap, 
                  originalScore);
    
    // 记录最佳解
    auto bestResult = result;
    auto bestPortLoads = portLoads;
    auto bestEval = originalEval;
    double bestScore = originalScore;
    
    // 实施局部搜索
    WORD32 dwIterations = 0;
    WORD32 dwConsecutiveFailures = 0;
    WORD32 dwSuccessfulSwaps = 0;
    WORD32 dwTotalSwapsAttempted = 0;
    constexpr WORD32 MAX_CONSECUTIVE_FAILURES = 100; // 连续失败次数上限
    
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 🔄 开始迭代优化（最大连续失败次数: %u）\n", MAX_CONSECUTIVE_FAILURES);
    
    // 实施局部搜索
    while (dwIterations < m_dwMaxIterations && dwConsecutiveFailures < MAX_CONSECUTIVE_FAILURES) {
        // 随机选择两个不同的哈希索引
        size_t idx1 = indexDistribution(randomGenerator);
        size_t idx2;
        do {
            idx2 = indexDistribution(randomGenerator);
        } while (idx1 == idx2 && hashIndices.size() > 1);
        
        WORD32 dwHashIndex1 = hashIndices[idx1];
        WORD32 dwHashIndex2 = hashIndices[idx2];
        
        dwTotalSwapsAttempted++;
        
        // 评估交换的改进量
        double improvement = utils::calculateSwapImprovement(
            result, memberCounts, portLoads, portSpeeds, dwHashIndex1, dwHashIndex2
        );
        
        // 考虑交换成本
        improvement -= m_exchangeCostFactor;
        
        // 如果有改进，执行交换
        if (improvement > 0) {
            // 获取当前端口分配
            WORD32 dwPortId1 = result[dwHashIndex1];
            WORD32 dwPortId2 = result[dwHashIndex2];
            
            // 获取流量计数
            WORD64 count1 = dwHashIndex1 < memberCounts.size() ? memberCounts[dwHashIndex1] : 0;
            WORD64 count2 = dwHashIndex2 < memberCounts.size() ? memberCounts[dwHashIndex2] : 0;
            
            XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 🔄 第%u次迭代 - 执行交换: [Hash%u->Port%u(流量:%llu)] <-> [Hash%u->Port%u(流量:%llu)], 改进量: +%.6f\n",
                          dwIterations + 1, dwHashIndex1, dwPortId1, count1, dwHashIndex2, dwPortId2, count2, improvement);
            
            // 更新端口负载
            portLoads[dwPortId1] -= count1;
            portLoads[dwPortId2] -= count2;
            
            // 交换端口分配
            result[dwHashIndex1] = dwPortId2;
            result[dwHashIndex2] = dwPortId1;
            
            portLoads[dwPortId1] += count2;
            portLoads[dwPortId2] += count1;
            
            // 评估新解
            auto newEval = utils::calculateLoadBalanceMetrics(portLoads, portSpeeds);
            double newScore = utils::calculateBalanceScore(newEval);
            
            dwSuccessfulSwaps++;
            
            // ===== 新增：记录交换信息 =====
            SwapRecord record = {
                dwIterations + 1,  // iteration
                dwHashIndex1,
                dwHashIndex2,
                dwPortId1,
                dwPortId2,
                count1,
                count2,
                improvement,
                newScore,
                newEval.totalGap
            };

            swapHistory.push_back(record);

            bestScore = newScore;
            dwConsecutiveFailures = 0;    //重置连续失败次数
            bestEval = newEval;
            bestResult = result;
            bestScore = newScore;
            
        } else {
            dwConsecutiveFailures++;
            
            // 每100次失败输出一次进度信息
            if (dwConsecutiveFailures % 100 == 0) {
                XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 🔍 第%u次迭代 - 连续失败: %u, 当前最佳得分: %.6f, 总尝试次数: %u\n",
                              dwIterations + 1, dwConsecutiveFailures, bestScore, dwTotalSwapsAttempted);
            }
        }
        
        dwIterations++;
        
        // 每1000次迭代输出一次统计信息
        if (dwIterations % 1000 == 0) {
            double successRate = dwTotalSwapsAttempted > 0 ? (double)dwSuccessfulSwaps / dwTotalSwapsAttempted * 100.0 : 0.0;
            XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 📈 迭代进度: %u/%u, 成功交换: %u/%u (%.1f%%), 最佳得分: %.6f\n",
                          dwIterations, m_dwMaxIterations, dwSuccessfulSwaps, dwTotalSwapsAttempted, successRate, bestScore);
        }
    }
    
    // 算法结束，输出完整统计信息
    double finalSuccessRate = dwTotalSwapsAttempted > 0 ? (double)dwSuccessfulSwaps / dwTotalSwapsAttempted * 100.0 : 0.0;
    double totalImprovement = bestScore - utils::calculateBalanceScore(originalEval);
    
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]  局部搜索完成!\n");
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]  执行统计:\n");
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 总迭代次数: %u/%u\n", dwIterations, m_dwMaxIterations);
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 尝试交换次数: %u\n", dwTotalSwapsAttempted);
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 成功交换次数: %u (成功率: %.1f%%)\n", dwSuccessfulSwaps, finalSuccessRate);
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 连续失败次数: %u\n", dwConsecutiveFailures);
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 终止原因: %s\n", 
                  dwIterations >= m_dwMaxIterations ? "达到最大迭代次数" : "达到最大连续失败次数");
    
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]  优化效果:\n");
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 初始得分: %.6f -> 最终得分: %.6f (改进: %.6f)\n",
                  utils::calculateBalanceScore(originalEval), bestScore, totalImprovement);
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 总偏差: %.6f -> %.6f\n", originalEval.totalGap, bestEval.totalGap);
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 正偏差: %.6f -> %.6f\n", originalEval.upBoundGap, bestEval.upBoundGap);
    XOS_SysLog(LOG_EMERGENCY, "[LocalSearch]   - 负偏差: %.6f -> %.6f\n", originalEval.lowBoundGap, bestEval.lowBoundGap);
    
    // ===== 新增：输出交换历史记录（前20个改进量最大的） =====
    if (!swapHistory.empty()) {
        // 按改进量降序排序
        std::sort(swapHistory.begin(), swapHistory.end(), 
                  [](const SwapRecord& a, const SwapRecord& b) {
                      return a.improvement > b.improvement;
                  });
        
        XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 🏆 Top 20 改进量最大的交换记录:\n");
        XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] %-4s %-8s %-10s %-10s %-12s %-12s %-12s %-12s %-10s\n", 
                      "排名", "迭代", "Hash1", "Hash2", "Port1->2", "Port2->1", "流量1", "流量2", "改进量");
        XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] %s\n", 
                      "------------------------------------------------------------------------------------------------");
        
        size_t displayCount = std::min(swapHistory.size(), size_t(20));
        for (size_t i = 0; i < displayCount; ++i) {
            const auto& record = swapHistory[i];
            XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] #%-3zu 第%-6u Hash%-6u Hash%-6u Port%-3u->%-3u Port%-3u->%-3u %-10llu %-10llu +%.6f\n",
                          i + 1,
                          record.iteration,
                          record.hashIndex1,
                          record.hashIndex2,
                          record.portId1,
                          record.portId2,
                          record.portId2,
                          record.portId1,
                          record.count1,
                          record.count2,
                          record.improvement);
        }
        
        XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] %s\n", 
                      "------------------------------------------------------------------------------------------------");
        XOS_SysLog(LOG_EMERGENCY, "[LocalSearch] 💡 总共记录了 %zu 次成功交换\n", swapHistory.size());
    }
    
    // 返回最佳解
    return bestResult;
}

} // namespace ai_ecmp
