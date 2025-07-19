#ifndef AI_ECMP_ALGORITHM_BASE_H
#define AI_ECMP_ALGORITHM_BASE_H

#include <unordered_map>
#include <vector>
#include "ai_ecmp_types.h"

namespace ai_ecmp {

/**
 * 算法基类，定义通用接口
 */
class AlgorithmBase {
public:
    virtual ~AlgorithmBase() = default;
    
    /**
     * 运行算法优化
     * @param memberTable 成员表 (hash_index -> portId)
     * @param memberCounts 成员计数表(hash_index -> count)
     * @param portSpeeds 端口速率表(portId -> speed)
     * @return 优化后的成员表(hash_index -> portId) 
     */
    virtual std::unordered_map<WORD32, WORD32> optimize(
        const std::unordered_map<WORD32, WORD32>& memberTable,
        const std::vector<WORD64>& memberCounts,
        const std::unordered_map<WORD32, WORD32>& portSpeeds) = 0;
    
    /** 
     * 计算负载分布指标
     * @param memberTable 成员表(hash_index -> portId)
     * @param memberCounts 成员计数表(hash_index -> count)
     * @param portSpeeds 端口速率表(portId -> speed)
     * @return 评估结果
     */
    T_AI_ECMP_EVAL evaluateBalance(
        const std::unordered_map<WORD32, WORD32>& memberTable,
        const std::vector<WORD64>& memberCounts,
        const std::unordered_map<WORD32, WORD32>& portSpeeds);
    
protected:
    /**
     * 计算端口负载
     * @param memberTable 成员表(hash_index -> portId) 
     * @param memberCounts 成员计数表(hash_index -> count)
     * @return 端口负载表(portId -> load)
     */
    std::unordered_map<WORD32, WORD64> calculatePortLoads(
        const std::unordered_map<WORD32, WORD32>& memberTable,
        const std::vector<WORD64>& memberCounts);
};

} // namespace ai_ecmp

#endif /* AI_ECMP_ALGORITHM_BASE_H */
