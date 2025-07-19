#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

#include "algorithm_base.h"
#include <vector>
#include <utility>

namespace smart_ecmp {

/**
 * 改进的遗传算法实现
 */
class GeneticAlgorithm : public AlgorithmBase {
public:
    /**
     * 构造函数
     * @param population_size 种群大小
     * @param num_generations 演化代数
     * @param mutation_rate 变异率
     * @param crossover_rate 交叉率
     */
    GeneticAlgorithm(
        WORD32 population_size = 200,
        WORD32 num_generations = 50,
        double mutation_rate = 0.3,
        double crossover_rate = 0.7
    );
    
    /**
     * 运行算法优化
     * @param member_table 成员表 (hash_index -> portId)
     * @param member_counts 成员计数表
     * @param port_speeds 端口速率表
     * @return 优化后的成员表
     */
    std::unordered_map<WORD32, WORD32> optimize(
        const std::unordered_map<WORD32, WORD32>& member_table,
        const std::vector<WORD64>& member_counts,
        const std::unordered_map<WORD32, WORD32>& port_speeds
    ) override;
    
private:
    WORD32 m_populationSize;
    WORD32 m_numGenerations;
    double m_mutationRate;
    double m_crossoverRate;
    
    // 定义个体类型 (交换操作列表)
    using SwapOperation = std::pair<WORD32, WORD32>; // (hash_index1, hash_index2)
    using Individual = std::vector<SwapOperation>;
    
    // 初始化种群
    std::vector<Individual> initializePopulation(
        const std::unordered_map<WORD32, WORD32>& member_table,
        WORD32 population_size
    );
    
    // 创建随机个体
    Individual createIndividual(const std::unordered_map<WORD32, WORD32>& member_table);
    
    // 变异操作
    Individual mutate(
        const Individual& individual,
        const std::vector<WORD32>& hash_indices,
        double mutation_rate
    );
    
    // 交叉操作
    std::pair<Individual, Individual> crossover(
        const Individual& parent1,
        const Individual& parent2,
        double crossover_rate
    );
    
    // 选择操作
    std::pair<Individual, Individual> selection(
        const std::vector<Individual>& population,
        const std::vector<double>& fitness_scores
    );
    
    // 评估个体适应度
    double evaluateFitness(
        const Individual& individual,
        const std::unordered_map<WORD32, WORD32>& original_table,
        const std::vector<WORD64>& member_counts,
        const std::unordered_map<WORD32, WORD32>& port_speeds
    );
    
    // 应用个体的交换操作到成员表
    std::unordered_map<WORD32, WORD32> applySwaps(
        const std::unordered_map<WORD32, WORD32>& original_table,
        const Individual& individual
    );
};

} // namespace smart_ecmp

#endif /* GENETIC_ALGORITHM_H */
