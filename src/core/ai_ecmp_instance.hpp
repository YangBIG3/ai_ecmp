#ifndef AI_ECMP_INSTANCE_H
#define AI_ECMP_INSTANCE_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include "ai_ecmp_types.h"
#include "ai_ecmp_algorithm_base.hpp"
#include "ai_ecmp_printer.h"


namespace ai_ecmp {

// Forward declaration for AlgorithmBase if its header isn't included
class AlgorithmBase;

/**
 * ECMP实例类，维护单个SG配置的状态和参数
 */
class EcmpInstance {
public:

    /**
     * @brief 设置当前使用的算法
     * @param pAlgorithm 算法指针
     */
    void setAlgorithm(std::unique_ptr<AlgorithmBase>&& pAlgorithm);
    
    /**
     * 构造函数
     * @param sgCfg SG配置信息
     */
    explicit EcmpInstance(const T_AI_ECMP_SG_CFG& sgCfg);
    
    /**
     * 更新配置
     * @param sgCfg 新的SG配置
     */
    void updateConfig(const T_AI_ECMP_SG_CFG& sgCfg);
    
    /**
     * 更新计数器信息
     * @return 是否更新成功
     */
    bool updateCounters(T_AI_ECMP_COUNTER_STATS_MSG& counterMsg);
    
    /**
     * 执行优化
     * @return 是否需要调整
     */
    bool runOptimization();
    
    /**
     * 获取优化后的下一跳修改
     * @param nhopModify 输出参数，存储修改信息
     * @return 是否有修改
     */
    bool getOptimizedNextHops(T_AI_ECMP_NHOP_MODIFY& nhopModify);
    
    /**
     * 获取扩容后的下一跳修改
     * @param nhopModify 输出参数，存储修改信息
     * @return 是否需要扩容
     */
    bool getExpandedNextHops(T_AI_ECMP_NHOP_MODIFY& nhopModify);
    
    /**
     * 评估当前负载均衡状态
     * @return 评估结果
     */
    T_AI_ECMP_EVAL evaluateBalance();
    
    /**
     * 获取SG ID
     * @return SG ID
     */
    WORD32 getSgId() const;
    
    /**
     * 获取当前状态
     * @return 状态枚举值
     */
    T_AI_ECMP_STATUS getStatus() const;
    
    /**
     * 重置状态
     */
    void reset();
    
    // ===== 新增：优化控制相关方法 =====
    /**
     * @brief 启用优化算法
     */
    void enableOptimization();
    
    /**
     * @brief 禁用优化算法
     */
    void disableOptimization();
    
    /**
     * @brief 检查优化算法是否启用
     * @return true表示启用，false表示禁用
     */
    bool isOptimizationEnabled() const { return m_bOptimizationEnabled; }
    
    /**
     * @brief 获取禁用期间的周期计数
     * @return 禁用的周期数
     */
    WORD32 getDisabledCycles() const { return m_dwDisabledCycles; }
    
    /**
     * @brief 获取当前周期数
     * @return 当前周期数
     */
    WORD16 getCycle() const { return m_wCycle; }
    
    /**
     * @brief 获取SG配置引用
     * @return SG配置的常量引用
     */
    const T_AI_ECMP_SG_CFG& getSgConfig() const { return m_sgConfig; }
    
private:
    // 扩容后等待调优的周期数
    static constexpr WORD16 CYCLES_AFTER_EXPANSION = 3;
    
    // 连续调优失败的最大次数，超过此次数才考虑扩容
    static constexpr WORD16 MAX_CONSECUTIVE_ADJUST_FAILURES = 2;
    
    // ===== 新增：方差稳定性控制常量 =====
    // 历史周期数量（用于方差计算）
    static constexpr WORD16 HISTORY_CYCLES_FOR_VARIANCE = 5;
    
    // 方差稳定阈值（当方差小于此值时认为稳定）
    static constexpr double VARIANCE_THRESHOLD = 0.05;  // 5%的变异系数阈值

    //选用算法
    std::unique_ptr<AlgorithmBase> m_pAlgorithm;
    // SG配置
    T_AI_ECMP_SG_CFG m_sgConfig;
    
    // 逻辑成员映射表 (hash_index -> portId)
    std::unordered_map<WORD32, WORD32> m_ecmpMemberTable;
    
    // 成员计数表 (hash_index -> count)
    std::vector<WORD64> m_memberCounts;
    
    // 端口负载表 (portId -> load)
    std::unordered_map<WORD32, WORD64> m_portLoads;
    
    // 端口速率表 (portId -> speed)
    std::unordered_map<WORD32, WORD32> m_portSpeeds;
    
    // 多周期计数器历史
    std::vector<std::vector<WORD64>> m_counterHistory;
    
    // 上一次评估结果
    T_AI_ECMP_EVAL m_lastEval;
    
    // 当前状态
    T_AI_ECMP_STATUS m_status;

    // 监测周期数
    WORD16 m_wCycle;

    // 优化结果打印器
    std::unique_ptr<EcmpPrinter> m_pPrinter;

    // ===== 扩容控制相关变量 =====
    // 上次扩容的周期数
    WORD16 m_lastExpandCycle;
    
    // 扩容后已进行调优的周期数
    WORD16 m_adjustCyclesAfterExpansion;
    
    // 连续调优失败的次数
    WORD16 m_consecutiveAdjustFailures;
    
    // 是否处于扩容后的等待期
    bool m_inPostExpansionPeriod;
    
    // ===== 新增：优化控制相关变量 =====
    // 优化算法启用标志（默认启用）
    bool m_bOptimizationEnabled;
    
    // 禁用期间的周期计数
    WORD32 m_dwDisabledCycles;

    
    // 计算负载分布指标
    void calculateLoadMetrics();
    
    // 将配置转换为内部数据结构
    void convertConfig();
    
    // 检查是否需要扩容
    bool needExpansion();
    
    // 检查是否有调整空间
    bool hasAdjustmentSpace();
    
    // 检查是否应该跳过扩容检查（扩容后等待期）
    bool shouldSkipExpansionCheck();
    
    // 记录扩容操作
    void recordExpansionOperation();
    
    // 记录调优操作结果
    void recordAdjustmentResult(bool success);

    // ===== 方差稳定性检查相关方法 =====
    
    /**
     * @brief 检查计数器数据方差是否稳定
     * @return true表示方差稳定，可以进行优化；false表示需要继续等待
     */
    bool isCounterVarianceStable();

    //评估优化效果是否达到最小改进阈值
    bool isOptimizationEffective(
        const T_AI_ECMP_EVAL& beforeEval,
        const T_AI_ECMP_EVAL& afterEval,
        double minImprovementThreshold = 1.0);

};

} // namespace ai_ecmp

#endif /* AI_ECMP_INSTANCE_H */
