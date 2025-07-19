#include "ai_ecmp_printer.h"
#include "ai_ecmp_metrics.hpp"
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <set>

namespace ai_ecmp {

EcmpPrinter::EcmpPrinter(WORD32 sgId) 
    : m_sgId(sgId)
    , m_algorithmName("Unknown")
    , m_executionTime(0) {
}

void EcmpPrinter::setBeforeData(
    const std::unordered_map<WORD32, WORD32>& memberTable,
    const std::vector<WORD64>& memberCounts,
    const std::unordered_map<WORD32, WORD64>& portLoads,
    const std::unordered_map<WORD32, WORD32>& portSpeeds) {
    
    m_beforeMemberTable = memberTable;
    m_beforeMemberCounts = memberCounts;
    m_beforePortLoads = portLoads;
    m_beforePortSpeeds = portSpeeds;
}

void EcmpPrinter::setAfterData(
    const std::unordered_map<WORD32, WORD32>& memberTable,
    const std::vector<WORD64>& memberCounts,
    const std::unordered_map<WORD32, WORD64>& portLoads,
    const std::unordered_map<WORD32, WORD32>& portSpeeds) {
    
    m_afterMemberTable = memberTable;
    m_afterMemberCounts = memberCounts;
    m_afterPortLoads = portLoads;
    m_afterPortSpeeds = portSpeeds;
}

void EcmpPrinter::setExecutionTime(WORD32 executionTime) {
    m_executionTime = executionTime;
}

void EcmpPrinter::setAlgorithmName(const std::string& algorithmName) {
    m_algorithmName = algorithmName;
}

void EcmpPrinter::printMemberTable(
    const std::unordered_map<WORD32, WORD32>& memberTable,
    const char* title) {
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ========== %s ==========\n", m_sgId, title);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 成员表大小: %zu\n", m_sgId, memberTable.size());
    
    // 统计端口分布
    std::unordered_map<WORD32, WORD32> portDistribution;
    for (const auto& entry : memberTable) {
        portDistribution[entry.second]++;
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口分布统计:\n", m_sgId);
    for (const auto& portEntry : portDistribution) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   端口 %u: %u 个哈希索引\n", 
                  m_sgId, portEntry.first, portEntry.second);
    }
    
    // 详细成员表 (限制输出数量避免日志过多)
    if (memberTable.size() <= 20) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 详细成员映射:\n", m_sgId);
        std::vector<std::pair<WORD32, WORD32>> sortedMembers(memberTable.begin(), memberTable.end());
        std::sort(sortedMembers.begin(), sortedMembers.end());
        
        for (const auto& entry : sortedMembers) {
            XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   哈希[%u] -> 端口[%u]\n", 
                      m_sgId, entry.first, entry.second);
        }
    } else {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 成员表条目过多(%zu)，仅显示统计信息\n", 
                  m_sgId, memberTable.size());
    }
}

void EcmpPrinter::printLoadBalanceMetrics(
    const std::unordered_map<WORD32, WORD64>& portLoads,
    const std::unordered_map<WORD32, WORD32>& portSpeeds,
    const char* title) {
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ========== %s ==========\n", m_sgId, title);
    
    // 使用utils中的函数计算评估指标和端口利用率
    T_AI_ECMP_EVAL eval = utils::calculateLoadBalanceMetrics(portLoads, portSpeeds);
    auto portUtilization = utils::calculatePortUtilization(portLoads, portSpeeds);
    
    if (portUtilization.empty()) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 无有效的端口利用率数据\n", m_sgId);
        return;
    }
    
    // 计算基本统计
    std::vector<double> utilizations;
    for (const auto& entry : portUtilization) {
        utilizations.push_back(entry.second);
    }
    
    double minVal = *std::min_element(utilizations.begin(), utilizations.end());
    double avgVal = std::accumulate(utilizations.begin(), utilizations.end(), 0.0) / utilizations.size();
    double maxVal = *std::max_element(utilizations.begin(), utilizations.end());
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 负载统计:\n", m_sgId);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   最小值: %.6f    平均值: %.6f    最大值: %.6f\n", 
              m_sgId, minVal, avgVal, maxVal);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   负偏差: %.6f%% (%.6f)\n", 
              m_sgId, eval.lowBoundGap * 100, eval.lowBoundGap);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   正偏差: %.6f%% (%.6f)\n", 
              m_sgId, eval.upBoundGap * 100, eval.upBoundGap);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   总偏差: %.6f%% (%.6f)\n", 
              m_sgId, eval.totalGap * 100, eval.totalGap);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   平均偏差: %.6f%% (%.6f)\n", 
              m_sgId, eval.avgGap * 100, eval.avgGap);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   平衡得分: %.6f\n", 
              m_sgId, eval.balanceScore);
}

void EcmpPrinter::printPortDistributionComparison() {
    if (m_beforePortLoads.empty() || m_afterPortLoads.empty()) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 缺少优化前后数据，无法进行对比\n", m_sgId);
        return;
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ========== 端口分布对比 ==========\n", m_sgId);
    
    // 使用utils中的函数计算端口利用率
    auto beforeUtil = utils::calculatePortUtilization(m_beforePortLoads, m_beforePortSpeeds);
    auto afterUtil = utils::calculatePortUtilization(m_afterPortLoads, m_afterPortSpeeds);
    
    // 使用utils中的函数计算平均值
    double beforeAvg = utils::calculateAverageUtilization(beforeUtil);
    double afterAvg = utils::calculateAverageUtilization(afterUtil);
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口利用率对比:\n", m_sgId);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: %-15s %-20s %-25s\n", 
              m_sgId, "端口(ID,速率)", "利用率比例", "相对平均值比例");
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: %-15s %-20s %-25s\n", 
              m_sgId, "", "优化前 -> 优化后", "优化前 -> 优化后");
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: %s\n", m_sgId, "------------------------------------------------------------");
    
    // 获取所有端口并排序
    std::set<WORD32> allPorts;
    for (const auto& entry : beforeUtil) {
        allPorts.insert(entry.first);
    }
    for (const auto& entry : afterUtil) {
        allPorts.insert(entry.first);
    }
    
    // 使用auto和基于范围的for循环
    for (auto portId : allPorts) {
        double beforeVal = (beforeUtil.count(portId) > 0) ? beforeUtil[portId] : 0.0;
        double afterVal = (afterUtil.count(portId) > 0) ? afterUtil[portId] : 0.0;
        
        WORD32 speed = m_beforePortSpeeds.count(portId) ? m_beforePortSpeeds.at(portId) : 
                      (m_afterPortSpeeds.count(portId) ? m_afterPortSpeeds.at(portId) : 0);
        
        double beforeRelative = (beforeAvg > 0) ? (beforeVal / beforeAvg) : 0.0;
        double afterRelative = (afterAvg > 0) ? (afterVal / afterAvg) : 0.0;
        
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口(%u,%uG):     %.2f%% -> %.2f%%        %.2fx -> %.2fx\n",
                  m_sgId, portId, speed, beforeVal * 100, afterVal * 100, beforeRelative, afterRelative);
    }
}

void EcmpPrinter::printOptimizationSummary() {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ========== 优化总结 ==========\n", m_sgId);
    
    if (m_beforePortLoads.empty() || m_afterPortLoads.empty()) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 缺少完整的优化前后数据\n", m_sgId);
        return;
    }
    
    // 使用utils中的函数计算优化前后的指标
    T_AI_ECMP_EVAL beforeEval = utils::calculateLoadBalanceMetrics(m_beforePortLoads, m_beforePortSpeeds);
    T_AI_ECMP_EVAL afterEval = utils::calculateLoadBalanceMetrics(m_afterPortLoads, m_afterPortSpeeds);
    
    // ===== 修改：使用公共函数计算改进程度 =====
    double improvementPercent = utils::calculateImprovementPercentage(beforeEval, afterEval);
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 🔍 执行时间: %u 毫秒\n", m_sgId, m_executionTime);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 📈 总体改进: %.2f%%\n", m_sgId, improvementPercent);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 🧮 使用算法: %s\n", m_sgId, m_algorithmName.c_str());
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 📊 偏差对比:\n", m_sgId);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   正偏差: %.6f%% -> %.6f%%\n", 
              m_sgId, beforeEval.upBoundGap * 100, afterEval.upBoundGap * 100);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   负偏差: %.6f%% -> %.6f%%\n", 
              m_sgId, beforeEval.lowBoundGap * 100, afterEval.lowBoundGap * 100);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   总偏差: %.6f%% -> %.6f%%\n", 
              m_sgId, beforeEval.totalGap * 100, afterEval.totalGap * 100);
    
    // 判断优化效果
    if (improvementPercent > 10.0) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ✅ 优化效果显著\n", m_sgId);
    } else if (improvementPercent > 5.0) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ✅ 优化效果良好\n", m_sgId);
    } else if (improvementPercent > 1.0) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ⚠️ 优化效果轻微\n", m_sgId);
    } else if (improvementPercent > 0.0) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ⚠️ 优化效果微弱\n", m_sgId);
    } else {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: ❌ 未产生改进\n", m_sgId);
    }
}

void EcmpPrinter::printFullReport() {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: \n", m_sgId);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: %s\n", m_sgId, "============================================================");
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 📊 ECMP负载均衡优化报告\n", m_sgId);
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: %s\n", m_sgId, "============================================================");
    
    // 优化前数据
    if (!m_beforePortLoads.empty()) {
        printLoadBalanceMetrics(m_beforePortLoads, m_beforePortSpeeds, "优化前负载均衡指标");
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: \n", m_sgId);
    }
    
    // 优化后数据
    if (!m_afterPortLoads.empty()) {
        printLoadBalanceMetrics(m_afterPortLoads, m_afterPortSpeeds, "优化后负载均衡指标");
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: \n", m_sgId);
    }
    
    // 端口分布对比
    printPortDistributionComparison();
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: \n", m_sgId);
    
    // 优化总结
    printOptimizationSummary();
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: %s\n", m_sgId, "============================================================");
}

T_AI_ECMP_EVAL EcmpPrinter::calculateMetrics(
    const std::unordered_map<WORD32, WORD64>& portLoads,
    const std::unordered_map<WORD32, WORD32>& portSpeeds) {
    
    // 直接复用utils中的函数
    return utils::calculateLoadBalanceMetrics(portLoads, portSpeeds);
}

std::string EcmpPrinter::formatPercentageChange(double beforeVal, double afterVal) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << beforeVal << "% -> " << afterVal << "%";
    return oss.str();
}

} // namespace ai_ecmp
