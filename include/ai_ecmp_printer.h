#ifndef AI_ECMP_PRINTER_H
#define AI_ECMP_PRINTER_H

#include "ai_ecmp_types.h"
#include <unordered_map>
#include <vector>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

namespace ai_ecmp {

/**
 * @brief ECMP优化结果打印工具类
 * 提供格式化打印ECMP优化前后数据对比的功能
 */
class EcmpPrinter {
public:
    /**
     * @brief 构造函数
     * @param sgId SG组ID
     */
    explicit EcmpPrinter(WORD32 sgId);
    
    /**
     * @brief 设置优化前数据
     * @param memberTable 成员表 (hash_index -> port_id)
     * @param memberCounts 成员计数表
     * @param portLoads 端口负载表 (port_id -> load_count)
     * @param portSpeeds 端口速率表 (port_id -> speed)
     */
    void setBeforeData(
        const std::unordered_map<WORD32, WORD32>& memberTable,
        const std::vector<WORD64>& memberCounts,
        const std::unordered_map<WORD32, WORD64>& portLoads,
        const std::unordered_map<WORD32, WORD32>& portSpeeds
    );
    
    /**
     * @brief 设置优化后数据
     * @param memberTable 优化后的成员表
     * @param memberCounts 成员计数表
     * @param portLoads 优化后的端口负载表
     * @param portSpeeds 端口速率表
     */
    void setAfterData(
        const std::unordered_map<WORD32, WORD32>& memberTable,
        const std::vector<WORD64>& memberCounts,
        const std::unordered_map<WORD32, WORD64>& portLoads,
        const std::unordered_map<WORD32, WORD32>& portSpeeds
    );
    
    /**
     * @brief 设置执行时间
     * @param executionTime 执行时间(毫秒)
     */
    void setExecutionTime(WORD32 executionTime);
    
    /**
     * @brief 设置算法名称
     * @param algorithmName 算法名称
     */
    void setAlgorithmName(const std::string& algorithmName);
    
    /**
     * @brief 打印成员表
     * @param memberTable 成员表
     * @param title 标题
     */
    void printMemberTable(
        const std::unordered_map<WORD32, WORD32>& memberTable,
        const char* title = "ECMP Member Table"
    );
    
    /**
     * @brief 打印负载均衡指标
     * @param portLoads 端口负载表
     * @param portSpeeds 端口速率表
     * @param title 标题
     */
    void printLoadBalanceMetrics(
        const std::unordered_map<WORD32, WORD64>& portLoads,
        const std::unordered_map<WORD32, WORD32>& portSpeeds,
        const char* title = "Load Balancing Metrics"
    );
    
    /**
     * @brief 打印端口分布对比表
     */
    void printPortDistributionComparison();
    
    /**
     * @brief 打印优化总结
     */
    void printOptimizationSummary();
    
    /**
     * @brief 打印完整的优化报告
     */
    void printFullReport();

private:
    WORD32 m_sgId;
    std::string m_algorithmName;
    WORD32 m_executionTime;
    
    // 优化前数据
    std::unordered_map<WORD32, WORD32> m_beforeMemberTable;
    std::vector<WORD64> m_beforeMemberCounts;
    std::unordered_map<WORD32, WORD64> m_beforePortLoads;
    std::unordered_map<WORD32, WORD32> m_beforePortSpeeds;
    
    // 优化后数据
    std::unordered_map<WORD32, WORD32> m_afterMemberTable;
    std::vector<WORD64> m_afterMemberCounts;
    std::unordered_map<WORD32, WORD64> m_afterPortLoads;
    std::unordered_map<WORD32, WORD32> m_afterPortSpeeds;
    
    /**
     * @brief 计算负载均衡评估指标
     * @param portLoads 端口负载表
     * @param portSpeeds 端口速率表
     * @return 评估结果
     */
    T_AI_ECMP_EVAL calculateMetrics(
        const std::unordered_map<WORD32, WORD64>& portLoads,
        const std::unordered_map<WORD32, WORD32>& portSpeeds
    );
    
    /**
     * @brief 计算端口利用率
     * @param portLoads 端口负载表
     * @param portSpeeds 端口速率表
     * @return 端口利用率表 (port_id -> utilization_ratio)
     */
    std::unordered_map<WORD32, double> calculatePortUtilization(
        const std::unordered_map<WORD32, WORD64>& portLoads,
        const std::unordered_map<WORD32, WORD32>& portSpeeds
    );
    
    /**
     * @brief 格式化百分比变化
     * @param beforeVal 优化前值
     * @param afterVal 优化后值
     * @return 格式化字符串
     */
    std::string formatPercentageChange(double beforeVal, double afterVal);
};

} // namespace ai_ecmp

#ifdef __cplusplus
}
#endif

#endif /* AI_ECMP_PRINTER_H */
