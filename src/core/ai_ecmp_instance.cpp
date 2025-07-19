#include "ai_ecmp_instance.hpp"
#include "../algorithms/ai_ecmp_local_search.hpp"
#include "ai_ecmp_types.h"
#include "ai_ecmp_printer.h"
#include <algorithm>
#include <numeric>
#include <cassert>
#include <random>
#include <chrono>  
#include "../utils/ai_ecmp_metrics.hpp"

namespace ai_ecmp {

EcmpInstance::EcmpInstance(const T_AI_ECMP_SG_CFG& sgConfig)
    : m_sgConfig(sgConfig)
    , m_status(AI_ECMP_INIT)
    , m_wCycle(0)
    , m_pPrinter(std::unique_ptr<EcmpPrinter>(new EcmpPrinter(sgConfig.dwSgId)))
    , m_lastExpandCycle(0)
    , m_adjustCyclesAfterExpansion(0)
    , m_consecutiveAdjustFailures(0)
    , m_inPostExpansionPeriod(false)
    , m_bOptimizationEnabled(true)      // 默认启用优化
    , m_dwDisabledCycles(0)              // 初始化禁用计数
{
    // 初始化内部数据结构
    convertConfig();
    // 初始化评估指标
    m_lastEval = {0.0, 0.0, 0.0, 0.0, 0.0};
}

void EcmpInstance::updateConfig(const T_AI_ECMP_SG_CFG& sgConfig) {
    m_sgConfig = sgConfig;
    convertConfig();
    // 清空计数器历史
    m_counterHistory.clear();
    m_wCycle = 0;
    // 重置扩容相关状态
    m_lastExpandCycle = 0;
    m_adjustCyclesAfterExpansion = 0;
    m_consecutiveAdjustFailures = 0;
    m_inPostExpansionPeriod = false;
}

void EcmpInstance::setAlgorithm(std::unique_ptr<AlgorithmBase>&& pAlgorithm) {
    m_pAlgorithm = std::move(pAlgorithm);
}

bool EcmpInstance::updateCounters(T_AI_ECMP_COUNTER_STATS_MSG& ecmpMsg) {
    // 从Counter获取接口读取当前的流量计数
    // 这里假设已经有了获取计数的接口
    
    // 保存当前计数器状态
    std::vector<WORD64> currentCounts(m_memberCounts.size());
    
    // 遍历所有成员索引
    for (size_t i = 0; i < m_memberCounts.size(); ++i) {
        // TODO: 这里需要实现实际的Counter读取逻辑
        // 假设已经有获取计数的函数: getCounterValue(baseId, offset)
        
        // m_memberCounts[i] = counterMsg.statCounter[i];
        // currentCounts[i] = m_memberCounts[i];

        // 临时用随机数代替
        static bool s_bRandSeeded = false;
        if (!s_bRandSeeded) {
            srand(time(nullptr));
            s_bRandSeeded = true;
        }
        m_memberCounts[i] += rand() % 100;
        currentCounts[i] = m_memberCounts[i];
    }
    
    // 添加到历史记录
    m_counterHistory.push_back(currentCounts);
    if (m_counterHistory.size() > 10) { // 保留最近10个周期的数据
        m_counterHistory.erase(m_counterHistory.begin());
    }
    
    // 计算当前负载分布
    calculateLoadMetrics();
    
    // 更新周期计数
    m_wCycle++;
    
    // 如果优化被禁用，也增加禁用计数
    if (!m_bOptimizationEnabled) {
        m_dwDisabledCycles++;
    }
    
    return true;
}

bool EcmpInstance::runOptimization() {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 开始执行优化，当前周期: %u\n", m_sgConfig.dwSgId, m_wCycle);
    
    // ===== 检查优化是否启用 =====
    if (!m_bOptimizationEnabled) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 优化算法已禁用（已禁用 %u 个周期），跳过优化流程\n", 
                  m_sgConfig.dwSgId, m_dwDisabledCycles);
        
        // 保持状态为当前状态，不改变
        // 注意：即使禁用优化，计数器历史仍然在updateCounters中更新，以保持数据连续性
        
        return false;
    }
    
    // 如果没有足够的历史数据，不执行优化
    if (m_counterHistory.size() < HISTORY_CYCLES_FOR_VARIANCE) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 计数器历史数据不足 (%zu < %u)，等待中\n", 
                      m_sgConfig.dwSgId, m_counterHistory.size(), HISTORY_CYCLES_FOR_VARIANCE);
        m_status = AI_ECMP_WAIT;
        return false;
    }
    
    // ===== 检查计数器数据方差稳定性 =====
    if (!isCounterVarianceStable()) {
        double varianceCoeff = utils::calculateCounterVarianceCoefficient(m_counterHistory, m_memberCounts);
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 计数器数据方差未稳定（变异系数: %.6f > %.6f），继续等待\n", 
                      m_sgConfig.dwSgId, varianceCoeff, VARIANCE_THRESHOLD);
        m_status = AI_ECMP_WAIT;
        return false;
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 计数器历史数据充足且方差稳定，开始评估平衡状态\n", m_sgConfig.dwSgId);
    
    // 评估当前平衡状态
    T_AI_ECMP_EVAL currentEval = evaluateBalance();
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 平衡评估结果 - 总偏差: %.6f, 上界偏差: %.6f, 下界偏差: %.6f, 平均偏差: %.6f, 平衡得分: %.6f\n",
                  m_sgConfig.dwSgId, currentEval.totalGap, currentEval.upBoundGap, 
                  currentEval.lowBoundGap, currentEval.avgGap, currentEval.balanceScore);
    
    // 记录优化前数据
    m_pPrinter->setBeforeData(m_ecmpMemberTable, m_memberCounts, m_portLoads, m_portSpeeds);
    m_pPrinter->printMemberTable(m_ecmpMemberTable, "优化前ECMP成员表");
    m_pPrinter->printLoadBalanceMetrics(m_portLoads, m_portSpeeds, "优化前负载均衡指标");
    
    // 如果平均偏差小于阈值，认为是平衡的
    if (currentEval.avgGap < 0.05) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 平均偏差 %.6f < 0.05，系统已平衡\n", 
                      m_sgConfig.dwSgId, currentEval.avgGap);
        m_status = AI_ECMP_BALANCE;
        // 重置扩容控制状态
        m_inPostExpansionPeriod = false;
        m_consecutiveAdjustFailures = 0;
        return false;
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 系统失衡，开始分析优化策略\n", m_sgConfig.dwSgId);
    
    // ===== 改进的扩容决策逻辑 =====
    bool shouldExpand = false;
    
    // 优先检查连续失败次数：如果达到阈值，无论是否在等待期都考虑扩容
    if (m_consecutiveAdjustFailures >= MAX_CONSECUTIVE_ADJUST_FAILURES) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 连续调优失败次数(%u)达到阈值(%u)，检查扩容需求\n", 
                      m_sgConfig.dwSgId, m_consecutiveAdjustFailures, MAX_CONSECUTIVE_ADJUST_FAILURES);
        
        bool bNeedExpansion = needExpansion();
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 扩容需求检查结果: %s\n", 
                      m_sgConfig.dwSgId, bNeedExpansion ? "需要扩容" : "不需要扩容");
        
        if (bNeedExpansion) {
            shouldExpand = true;
            // 如果之前在等待期，现在提前结束等待期
            if (m_inPostExpansionPeriod) {
                XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 连续失败达到阈值，提前结束扩容后等待期\n", m_sgConfig.dwSgId);
                m_inPostExpansionPeriod = false;
            }
        }
    }
    // 如果连续失败次数未达到阈值，且处于扩容后等待期，则强制执行算法优化
    else if (shouldSkipExpansionCheck()) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 处于扩容后等待期（已调优%u/%u周期），连续失败次数(%u)未达到阈值，继续算法优化\n", 
                      m_sgConfig.dwSgId, m_adjustCyclesAfterExpansion, CYCLES_AFTER_EXPANSION, m_consecutiveAdjustFailures);
        shouldExpand = false;
    }
    // 不在等待期且连续失败次数未达到阈值，则正常检查扩容需求但需要失败次数达到阈值才扩容
    else {
        bool bNeedExpansion = needExpansion();
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 扩容需求检查结果: %s，连续调优失败次数: %u\n", 
                      m_sgConfig.dwSgId, bNeedExpansion ? "需要扩容" : "不需要扩容", m_consecutiveAdjustFailures);
        
        // 需要扩容但失败次数未达到阈值，继续尝试算法优化
        if (bNeedExpansion) {
            XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 虽然需要扩容，但连续失败次数(%u)未达到阈值(%u)，继续尝试算法优化\n", 
                          m_sgConfig.dwSgId, m_consecutiveAdjustFailures, MAX_CONSECUTIVE_ADJUST_FAILURES);
        }
        shouldExpand = false;
    }
    
    if (shouldExpand) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 决定执行扩容操作\n", m_sgConfig.dwSgId);
        recordExpansionOperation();
        m_status = AI_ECMP_EXPAND;
        return true;
    }


    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 准备执行局部搜索优化\n", m_sgConfig.dwSgId);
    
    //设置为local search
    if (!m_pAlgorithm) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 创建局部搜索算法实例 (最大迭代: 10000, 交换成本: 0.1)\n", 
                      m_sgConfig.dwSgId);
        setAlgorithm(std::unique_ptr<AlgorithmBase>(new LocalSearch(10000, 0.1)));
    }
    if (!m_pAlgorithm) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 算法实例创建失败\n", m_sgConfig.dwSgId);
        recordAdjustmentResult(false);
        m_status = AI_ECMP_FAIL;
        return false;
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 执行局部搜索优化，当前成员表大小: %zu\n", 
                  m_sgConfig.dwSgId, m_ecmpMemberTable.size());
    
    // ========== 开始算法执行时间测量 ==========
    auto algorithmStartTime = std::chrono::high_resolution_clock::now();
    
    // 执行局部搜索优化
    auto optimizedTable = m_pAlgorithm->optimize(m_ecmpMemberTable, m_memberCounts, m_portSpeeds);
    
    // ========== 结束算法执行时间测量 ==========
    auto algorithmEndTime = std::chrono::high_resolution_clock::now();
    
    // 计算执行时间（毫秒）
    auto algorithmDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        algorithmEndTime - algorithmStartTime);
    WORD32 executionTimeMs = static_cast<WORD32>(algorithmDuration.count());
    
    // 计算执行时间（微秒，用于更精确的测量）
    auto algorithmDurationMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        algorithmEndTime - algorithmStartTime);
    WORD64 executionTimeMicros = static_cast<WORD64>(algorithmDurationMicros.count());
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 局部搜索优化完成，优化后成员表大小: %zu\n", 
                  m_sgConfig.dwSgId, optimizedTable.size());
    
    // 如果优化后的表与原表相同，不需要调整
    if (optimizedTable == m_ecmpMemberTable) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 优化后配置与原配置相同，无需调整\n", m_sgConfig.dwSgId);
        
        m_pPrinter->setExecutionTime(executionTimeMs);
        m_pPrinter->setAlgorithmName("LocalSearch");
        
        // 记录调优失败
        recordAdjustmentResult(false);
        
        // 打印简化报告（没有优化后数据）
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 📊 算法执行总结 - 耗时: %u ms, 结果: 无需调整\n", 
                  m_sgConfig.dwSgId, executionTimeMs);

        m_status = AI_ECMP_BALANCE;
        return false;
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 优化后配置有变化，准备评估优化效果\n", m_sgConfig.dwSgId);
    
    // ===== 评估优化前的平衡状态 =====
    T_AI_ECMP_EVAL beforeEval = currentEval;
    
    // 使用优化后的成员表临时计算端口负载
    std::unordered_map<WORD32, WORD64> tempPortLoads = 
        utils::calculatePortLoads(optimizedTable, m_memberCounts);
    
    // ===== 评估优化后的平衡状态（使用临时负载数据）=====
    T_AI_ECMP_EVAL afterEval = utils::calculateLoadBalanceMetrics(tempPortLoads, m_portSpeeds);
    
    // ===== 计算改进百分比并判断是否达到有效阈值 =====
    double improvementPercent = utils::calculateImprovementPercentage(beforeEval, afterEval);
    bool isEffective = isOptimizationEffective(beforeEval, afterEval, 1.0); // 1%阈值
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 优化效果评估 - 改进百分比: %.2f%%, 是否有效: %s\n", 
                  m_sgConfig.dwSgId, improvementPercent, isEffective ? "是" : "否");
    
    // ===== 如果改进不足，标记为调优失败，不进行下表，不更新成员表 =====
    if (!isEffective) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 算法优化改进不足(%.2f%% < 1%%)，标记为调优失败，保持原有配置不变\n", 
                      m_sgConfig.dwSgId, improvementPercent);
        
        m_pPrinter->setExecutionTime(executionTimeMs);
        m_pPrinter->setAlgorithmName("LocalSearch");
        
        // 记录调优失败
        recordAdjustmentResult(false);
        
        // 打印简化报告
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 📊 算法执行总结 - 耗时: %u ms, 改进: %.2f%%, 结果: 调优失败(改进不足)，配置未更新\n", 
                  m_sgConfig.dwSgId, executionTimeMs, improvementPercent);

        m_status = AI_ECMP_FAIL;
        return false;
    }
    
    // ===== 只有改进足够时，才真正更新成员表和负载指标 =====
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 算法优化改进足够(%.2f%% >= 1%%)，准备更新配置\n", 
                  m_sgConfig.dwSgId, improvementPercent);
    
    // 更新成员表
    m_ecmpMemberTable = std::move(optimizedTable);
    
    // 重新计算负载指标（使用新的成员表）
    calculateLoadMetrics();
    
    // ===== 记录优化后数据并打印报告 =====
    m_pPrinter->setAfterData(m_ecmpMemberTable, m_memberCounts, m_portLoads, m_portSpeeds);
    m_pPrinter->setAlgorithmName("LocalSearch");
    m_pPrinter->setExecutionTime(executionTimeMs);
    
    // 打印完整优化报告
    m_pPrinter->printFullReport();

    // 记录调优成功
    recordAdjustmentResult(true);

    m_status = AI_ECMP_ADJUST;
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 优化完成，改进%.2f%%，配置已更新，设置状态为调整模式\n", 
                  m_sgConfig.dwSgId, improvementPercent);
    
    return true;
}

bool EcmpInstance::getOptimizedNextHops(T_AI_ECMP_NHOP_MODIFY& nhopModifyData) {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 开始生成调整配置\n", m_sgConfig.dwSgId);
    if (m_status != AI_ECMP_ADJUST) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 当前状态 %s 不是调整状态，无法生成调整配置\n", 
                      m_sgConfig.dwSgId, utils::aiEcmpStatusToString(m_status));
        return false;
    }
    
    // 填充修改信息
    nhopModifyData.dwSgId = m_sgConfig.dwSgId;
    nhopModifyData.dwSeqId = m_sgConfig.dwSeqId;     // 每一次的修改seq号保持不变，仅由ftm确定seq号
    nhopModifyData.dwItemNum = static_cast<WORD32>(m_ecmpMemberTable.size());
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 填充基本信息 - SgId: %u, SeqId: %u, ItemNum: %u\n", 
              m_sgConfig.dwSgId, nhopModifyData.dwSgId, nhopModifyData.dwSeqId, nhopModifyData.dwItemNum);
    
    // 初始化数组
    for (WORD32 i = 0; i < FTM_TRUNK_MAX_HASH_NUM_15K; ++i) {
        nhopModifyData.adwLinkItem[i] = 0;
    }
    
    // 直接将m_ecmpMemberTable转换为T_AI_ECMP_NHOP_MODIFY形式
    // m_ecmpMemberTable的key是hash_index，value是port_id
    for (const auto& memberEntry : m_ecmpMemberTable) {
        WORD32 dwHashIndex = memberEntry.first;
        WORD32 dwPortId = memberEntry.second;
        
        if (dwHashIndex < FTM_TRUNK_MAX_HASH_NUM_15K) {
            nhopModifyData.adwLinkItem[dwHashIndex] = dwPortId;
        }
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 调整配置生成完成，等待硬件表更新后同步软件配置\n", 
              m_sgConfig.dwSgId);
    
    return true;
}

/**
* @brief 当前分配逻辑
* 遍历排序后的端口。
* 计算当前总的可用容量 available_capacity_total = MAX_TOTAL_LOGICAL_LINKS - sum_of_new_weights。
* 如果可用容量小于等于0，则停止分配。
* 对于每个端口，期望的增加量是其 currentWeight (目标是翻倍)。如果当前权重为0，则期望增加1。
* 实际可增加量 actual_increase_for_this_port 取期望增加量和总可用容量中的较小者。
* 更新该端口的 newWeight 和 sum_of_new_weights。
*/
bool EcmpInstance::getExpandedNextHops(T_AI_ECMP_NHOP_MODIFY& nhopModifyData) {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 开始生成扩容配置\n", m_sgConfig.dwSgId);
    
    if (m_status != AI_ECMP_EXPAND) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 当前状态 %d 不是扩容状态，无法生成扩容配置\n", 
                      m_sgConfig.dwSgId, m_status);
        return false;
    }

    const WORD32 dwMaxTotalLogicalLinks = FTM_TRUNK_MAX_HASH_NUM_15K; // 使用最大散列数
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 最大逻辑链路数: %u\n", m_sgConfig.dwSgId, dwMaxTotalLogicalLinks);

    // 填充基本信息
    nhopModifyData.dwSgId = m_sgConfig.dwSgId;
    nhopModifyData.dwSeqId = m_sgConfig.dwSeqId;
    
    if (m_sgConfig.dwPortNum == 0) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口数量为0，无需扩容\n", m_sgConfig.dwSgId);
        return true;
    }

    // 创建端口权重信息结构
    struct PortWeightInfo {
        WORD32 dwPortId;
        WORD32 dwCurrentWeight;
        WORD32 dwNewWeight;
    };

    std::vector<PortWeightInfo> portInfos;
    portInfos.reserve(m_sgConfig.dwPortNum);

    WORD32 dwCurrentTotalWeight = 0;
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 当前端口权重信息:\n", m_sgConfig.dwSgId);
    
    for (WORD16 i = 0; i < m_sgConfig.dwPortNum; ++i) {
        if (i < FTM_LAG_MAX_MEM_NUM_15K) {
            WORD32 dwPortId = m_sgConfig.ports[i].dwPortId;
            WORD32 dwWeight = m_sgConfig.ports[i].dwWeight;
            
            XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u:   端口 %u: 当前权重 %u\n", 
                          m_sgConfig.dwSgId, dwPortId, dwWeight);
            
            portInfos.push_back({
                dwPortId,
                dwWeight,
                dwWeight // newWeight 初始化为 currentWeight
            });
            dwCurrentTotalWeight += dwWeight;
        }
    }

    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 当前总权重: %u\n", m_sgConfig.dwSgId, dwCurrentTotalWeight);

    // 按当前权重升序排序，优先给权重小的端口增加链路
    std::sort(portInfos.begin(), portInfos.end(), [](const PortWeightInfo& a, const PortWeightInfo& b) {
        return a.dwCurrentWeight < b.dwCurrentWeight;
    });

    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口按权重排序完成，开始扩容分配\n", m_sgConfig.dwSgId);

    WORD32 dwSumOfNewWeights = dwCurrentTotalWeight; // 初始链路数

    // 扩容逻辑：为每个端口分配更多的逻辑链路
    for (WORD32 i = 0; i < portInfos.size(); ++i) {
        WORD32 dwAvailableCapacityTotal = dwMaxTotalLogicalLinks - dwSumOfNewWeights;
        
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 处理端口 %u，剩余容量: %u\n", 
                      m_sgConfig.dwSgId, portInfos[i].dwPortId, dwAvailableCapacityTotal);
        
        // 尝试将当前端口的权重翻倍，即增加量为其当前权重
        WORD32 dwDesiredIncrease = portInfos[i].dwCurrentWeight;
        if (dwDesiredIncrease == 0) {
            // 如果当前权重为0，则期望增加1
             dwDesiredIncrease = 1;
        }

        // ===== 修改：如果容量不足以满足期望增加量，直接扩容失败 =====
        if (dwAvailableCapacityTotal < dwDesiredIncrease) {
            XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 容量不足以满足端口 %u 的扩容需求（需要: %u, 可用: %u），扩容失败\n", 
                          m_sgConfig.dwSgId, portInfos[i].dwPortId, dwDesiredIncrease, dwAvailableCapacityTotal);
            return false;
        }

        WORD32 dwActualIncrease = dwDesiredIncrease; // 现在实际增加量就是期望增加量

        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口 %u 期望增加 %u，实际增加 %u\n", 
                      m_sgConfig.dwSgId, portInfos[i].dwPortId, dwDesiredIncrease, dwActualIncrease);

        portInfos[i].dwNewWeight += dwActualIncrease;
        dwSumOfNewWeights += dwActualIncrease;
        
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口 %u 新权重: %u -> %u\n", 
                      m_sgConfig.dwSgId, portInfos[i].dwPortId, 
                      portInfos[i].dwCurrentWeight, portInfos[i].dwNewWeight);
    }

    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 扩容后总权重: %u\n", m_sgConfig.dwSgId, dwSumOfNewWeights);

    // 构建扩容后的ECMP成员表用于下发（不更新内部状态）
    WORD32 dwCurrentIndex = 0;
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 开始生成扩容后的ECMP成员表\n", m_sgConfig.dwSgId);
    
    // 填充 T_AI_ECMP_NHOP_MODIFY 结构
    nhopModifyData.dwItemNum = dwSumOfNewWeights;
    
    // 初始化数组
    for (WORD32 i = 0; i < FTM_TRUNK_MAX_HASH_NUM_15K; ++i) {
        nhopModifyData.adwLinkItem[i] = 0;
    }
    
    // 为每个端口按照新权重分配索引到下发结构中
    for (const auto& portInfo : portInfos) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 为端口 %u 分配 %u 个索引 (起始索引: %u)\n", 
                      m_sgConfig.dwSgId, portInfo.dwPortId, portInfo.dwNewWeight, dwCurrentIndex);
        
        for (WORD32 j = 0; j < portInfo.dwNewWeight; ++j) {
            if (dwCurrentIndex < FTM_TRUNK_MAX_HASH_NUM_15K) {
                nhopModifyData.adwLinkItem[dwCurrentIndex] = portInfo.dwPortId;
                dwCurrentIndex++;
            } else {
                XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 达到最大索引限制\n", m_sgConfig.dwSgId);
                break;
            }
        }
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 扩容配置生成完成，总索引数: %u，等待硬件表更新后同步软件配置\n", 
                  m_sgConfig.dwSgId, dwCurrentIndex);
    
    return true;
}

T_AI_ECMP_EVAL EcmpInstance::evaluateBalance() {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 开始平衡状态评估\n", m_sgConfig.dwSgId);
    
    // 使用utils中的函数计算端口负载
    std::unordered_map<WORD32, WORD64> portLoads = 
        ai_ecmp::utils::calculatePortLoads(m_ecmpMemberTable, m_memberCounts);
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 计算得到 %zu 个端口的负载数据\n", 
              m_sgConfig.dwSgId, portLoads.size());
    
    // 使用utils中的函数计算负载平衡指标
    T_AI_ECMP_EVAL evalResult = 
        ai_ecmp::utils::calculateLoadBalanceMetrics(portLoads, m_portSpeeds);
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 平衡评估完成 - 总偏差: %.6f, 上界偏差: %.6f, 下界偏差: %.6f, 平均偏差: %.6f, 平衡得分: %.6f\n",
              m_sgConfig.dwSgId, evalResult.totalGap, evalResult.upBoundGap, 
              evalResult.lowBoundGap, evalResult.avgGap, evalResult.balanceScore);
    
    // 保存评估结果
    m_lastEval = evalResult;
    
    return evalResult;
}

WORD32 EcmpInstance::getSgId() const {
    return m_sgConfig.dwSgId;
}

T_AI_ECMP_STATUS EcmpInstance::getStatus() const {
    return m_status;
}

void EcmpInstance::reset() {
    m_wCycle = 0;
    m_status = AI_ECMP_INIT;
    m_counterHistory.clear();
    // 重置扩容控制状态
    m_lastExpandCycle = 0;
    m_adjustCyclesAfterExpansion = 0;
    m_consecutiveAdjustFailures = 0;
    m_inPostExpansionPeriod = false;
    // 重置优化控制状态（保持当前的启用/禁用状态）
    m_dwDisabledCycles = 0;
}

void EcmpInstance::convertConfig() {
    // 清空现有映射
    m_ecmpMemberTable.clear();
    m_portSpeeds.clear();
    
    // 初始化成员计数表
    if (m_sgConfig.dwItemNum >= 0) {
         m_memberCounts.assign(m_sgConfig.dwItemNum, 0);
    } else {
        m_memberCounts.clear();
    }
    
    // 填充成员表
    for (WORD16 i = 0; i < m_sgConfig.dwItemNum; ++i) {
        if (i < AI_ECMP_MAX_ITEM_NUM) {
            WORD32 dwOffset = m_sgConfig.items[i].dwItemOffset;
            WORD32 dwPortId = m_sgConfig.items[i].dwPortId;
            m_ecmpMemberTable[dwOffset] = dwPortId;
        }
    }
    
    // 填充端口速率表
    for (WORD16 i = 0; i < m_sgConfig.dwPortNum; ++i) {
        if (i < AI_ECMP_MAX_PORT_NUM) {
            WORD32 dwPortId = m_sgConfig.ports[i].dwPortId;
            WORD32 dwSpeed = m_sgConfig.ports[i].dwSpeed;
            m_portSpeeds[dwPortId] = dwSpeed;
        }
    }
}

void EcmpInstance::calculateLoadMetrics() {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 开始计算负载指标\n", m_sgConfig.dwSgId);
    
    // 使用utils中的函数计算端口负载，替换原有的重复代码
    m_portLoads = ai_ecmp::utils::calculatePortLoads(m_ecmpMemberTable, m_memberCounts);
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 负载指标计算完成，共 %zu 个端口\n", 
              m_sgConfig.dwSgId, m_portLoads.size());
    
    // 打印每个端口的负载详情
    // for (const auto& entry : m_portLoads) {
    //     XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口 %u 负载: %llu\n", 
    //               m_sgConfig.dwSgId, entry.first, entry.second);
    // }
}

bool EcmpInstance::needExpansion() {
    // TODO：如果上一次优化结果总偏差超过某个阈值且当前权重较小，考虑扩容
    // TODO：如果检查到当前没有调整空间，则直接扩容
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 开始扩容需求分析\n", m_sgConfig.dwSgId);
    
    constexpr double EXPANSION_THRESHOLD = 0.2;
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 扩容阈值: %.2f\n", m_sgConfig.dwSgId, EXPANSION_THRESHOLD);

    // 检查是否有端口权重小于2
    bool bHasLowWeightPort = false;
    for(WORD16 i = 0; i < m_sgConfig.dwPortNum; ++i) {
        if (i < AI_ECMP_MAX_PORT_NUM) {
            WORD32 dwPortId = m_sgConfig.ports[i].dwPortId;
            WORD32 dwWeight = m_sgConfig.ports[i].dwWeight;
            
            XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口 %u 权重: %u\n", 
                          m_sgConfig.dwSgId, dwPortId, dwWeight);
            
            if(dwWeight < 2) {
                XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口 %u 权重 %u < 2，触发扩容条件\n", 
                              m_sgConfig.dwSgId, dwPortId, dwWeight);
                bHasLowWeightPort = true;
            }
        }
    }
    
    if (bHasLowWeightPort) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 存在低权重端口，需要扩容\n", m_sgConfig.dwSgId);
        return true;
    }
    
    // 检查总偏差是否超过阈值
    if (m_lastEval.totalGap > EXPANSION_THRESHOLD) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 总偏差 %.6f > %.2f，超过扩容阈值，需要扩容\n", 
                      m_sgConfig.dwSgId, m_lastEval.totalGap, EXPANSION_THRESHOLD);
        return true;
    }
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 不需要扩容\n", m_sgConfig.dwSgId);
    return false;
}

bool EcmpInstance::hasAdjustmentSpace() {
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 开始调整空间分析\n", m_sgConfig.dwSgId);
    
    // 如果只有一个端口，没有调整空间
    if (m_sgConfig.dwPortNum <= 1) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 端口数量 %u <= 1，无调整空间\n", 
                      m_sgConfig.dwSgId, m_sgConfig.dwPortNum);
        return false;
    }
    
    // 如果所有端口速率相同且每个端口只有一个逻辑成员，没有调整空间
    bool bAllSameSpeed = true;
    if (m_sgConfig.dwPortNum > 0 && 0 < AI_ECMP_MAX_PORT_NUM) {
        WORD32 dwFirstSpeed = m_sgConfig.ports[0].dwSpeed;
        
        for (WORD16 i = 1; i < m_sgConfig.dwPortNum; ++i) {
            if (i < AI_ECMP_MAX_PORT_NUM) {
                if (m_sgConfig.ports[i].dwSpeed != dwFirstSpeed) {
                    bAllSameSpeed = false;
                    break;
                }
            }
        }
    } 

    if (bAllSameSpeed && (m_sgConfig.dwItemNum == m_sgConfig.dwPortNum)) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 所有端口速率相同且每端口仅一个逻辑成员，无调整空间\n", 
                      m_sgConfig.dwSgId);
        return false;
    }
    
    return true;
}

// ===== 新增的私有方法实现 =====

bool EcmpInstance::shouldSkipExpansionCheck() {
    // 如果不在扩容后等待期，不需要跳过
    if (!m_inPostExpansionPeriod) {
        return false;
    }
    
    // 检查是否还在等待期内
    if (m_adjustCyclesAfterExpansion < CYCLES_AFTER_EXPANSION) {
        return true;
    }
    
    // 等待期结束，退出等待期状态
    m_inPostExpansionPeriod = false;
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 扩容后等待期结束，重新启用扩容检查\n", m_sgConfig.dwSgId);
    return false;
}

void EcmpInstance::recordExpansionOperation() {
    m_lastExpandCycle = m_wCycle;
    m_adjustCyclesAfterExpansion = 0;
    m_consecutiveAdjustFailures = 0;
    m_inPostExpansionPeriod = true;
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 记录扩容操作，周期: %u，进入%u周期等待期\n", 
                  m_sgConfig.dwSgId, m_wCycle, CYCLES_AFTER_EXPANSION);
}

void EcmpInstance::recordAdjustmentResult(bool success) {
    if (m_inPostExpansionPeriod) {
        m_adjustCyclesAfterExpansion++;
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 扩容后调优周期计数: %u/%u\n", 
                      m_sgConfig.dwSgId, m_adjustCyclesAfterExpansion, CYCLES_AFTER_EXPANSION);
    }
    
    if (success) {
        // 调优成功，重置失败计数
        m_consecutiveAdjustFailures = 0;
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 调优成功，重置连续失败计数\n", m_sgConfig.dwSgId);
    } else {
        // 调优失败，增加失败计数
        m_consecutiveAdjustFailures++;
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 调优失败，连续失败次数: %u\n", 
                      m_sgConfig.dwSgId, m_consecutiveAdjustFailures);
    }
}

// ===== 新增：实现方差稳定性检查方法 =====
bool EcmpInstance::isCounterVarianceStable() {
    if (m_counterHistory.size() < HISTORY_CYCLES_FOR_VARIANCE) {
        return false;
    }
    
    double varianceCoeff = utils::calculateCounterVarianceCoefficient(m_counterHistory, m_memberCounts);
    bool isStable = varianceCoeff <= VARIANCE_THRESHOLD;
    
    XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 方差稳定性检查 - 变异系数: %.6f, 阈值: %.6f, 结果: %s\n", 
                  m_sgConfig.dwSgId, varianceCoeff, VARIANCE_THRESHOLD, 
                  isStable ? "稳定" : "不稳定");
    
    return isStable;
}

/**
 * @brief 评估优化效果是否达到最小改进阈值
 * @param beforeEval 优化前的评估结果
 * @param afterEval 优化后的评估结果
 * @param minImprovementThreshold 最小改进阈值（百分比，默认1%）
 * @return true表示改进达到阈值，false表示改进不足
 */
bool EcmpInstance::isOptimizationEffective(
    const T_AI_ECMP_EVAL& beforeEval,
    const T_AI_ECMP_EVAL& afterEval,
    double minImprovementThreshold) {
    
    double improvementPercent = utils::calculateImprovementPercentage(beforeEval, afterEval);
    return improvementPercent >= minImprovementThreshold;
}

// ===== 新增：优化控制方法实现 =====
void EcmpInstance::enableOptimization() {
    if (!m_bOptimizationEnabled) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 启用优化算法，之前已禁用 %u 个周期\n", 
                  m_sgConfig.dwSgId, m_dwDisabledCycles);
        m_bOptimizationEnabled = true;
        m_dwDisabledCycles = 0;
    } else {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 优化算法已经是启用状态\n", m_sgConfig.dwSgId);
    }
}

void EcmpInstance::disableOptimization() {
    if (m_bOptimizationEnabled) {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 禁用优化算法\n", m_sgConfig.dwSgId);
        m_bOptimizationEnabled = false;
        m_dwDisabledCycles = 0;  // 重置禁用计数
    } else {
        XOS_SysLog(LOG_EMERGENCY, "[ECMP] SG %u: 优化算法已经是禁用状态\n", m_sgConfig.dwSgId);
    }
}
} // namespace ai_ecmp

