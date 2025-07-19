#ifndef AI_ECMP_LOCAL_SEARCH_HPP
#define AI_ECMP_LOCAL_SEARCH_HPP

#include "ai_ecmp_algorithm_base.hpp"

namespace ai_ecmp {

/**
 * 局部搜索算法实现
 */
class LocalSearch : public AlgorithmBase {
public:
    /**
     * 构造函数
     * @param dwMaxIterations 最大迭代次数
     * @param exchangeCostFactor 交换代价因子
     */
    LocalSearch(WORD32 dwMaxIterations = 10000, double exchangeCostFactor = 0.0);
    
    /**
     * 运行算法优化
     * @param memberTable 成员表 (hash_index -> portId)
     * @param memberCounts 成员计数表
     * @param portSpeeds 端口速率表
     * @return 优化后的成员表
     */
    std::unordered_map<WORD32,WORD32> optimize(
        const std::unordered_map<WORD32, WORD32>& memberTable,
        const std::vector<WORD64>& memberCounts,
        const std::unordered_map<WORD32, WORD32>& portSpeeds) override;
    
private:
    WORD32 m_dwMaxIterations; // 最大迭代次数
    double m_exchangeCostFactor; // 交换代价因子
    
    // 评估交换操作的改进
    double evaluateSwap(
        const std::unordered_map<WORD32, WORD32>& memberTable,
        const std::vector<WORD64>& memberCounts,
        std::unordered_map<WORD32, WORD64>& portLoads,
        WORD32 dwHashIndex1, WORD32 dwHashIndex2);
};

} // namespace ai_ecmp

#endif /* AI_ECMP_LOCAL_SEARCH_HPP */
