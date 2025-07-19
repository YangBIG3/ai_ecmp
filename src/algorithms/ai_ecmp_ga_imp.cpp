#include "genetic_algorithm.h"
#include "../utils/ai_ecmp_metrics.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <numeric>

namespace ai_ecmp {

GeneticAlgorithm::GeneticAlgorithm(
    WORD32 population_size,
    WORD32 num_generations,
    double mutation_rate,
    double crossover_rate
)
    : m_populationSize(population_size)
    , m_numGenerations(num_generations)
    , m_mutationRate(mutation_rate)
    , m_crossoverRate(crossover_rate) {
}

std::unordered_map<WORD32, WORD32> GeneticAlgorithm::optimize(
    const std::unordered_map<WORD32, WORD32>& member_table,
    const std::vector<WORD64>& member_counts,
    const std::unordered_map<WORD32, WORD32>& port_speeds
) {
    // 如果成员表为空或只有一个元素，无法优化
    if (member_table.size() <= 1) {
        return member_table;
    }
    
    // 获取所有哈希索引
    std::vector<WORD32> hash_indices;
    for (const auto& entry : member_table) {
        hash_indices.push_back(entry.first);
    }
    
    // 初始化种群
    std::vector<Individual> population = initializePopulation(member_table, m_populationSize);
    
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 演化迭代
    for (WORD32 generation = 0; generation < m_numGenerations; ++generation) {
        // 评估所有个体的适应度
        std::vector<double> fitness_scores;
        for (const auto& individual : population) {
            double fitness = evaluateFitness(individual, member_table, member_counts, port_speeds);
            fitness_scores.push_back(fitness);
        }
        
        // 创建新一代种群
        std::vector<Individual> new_population;
        
        // 保留最优个体 (精英策略)
        auto max_it = std::max_element(fitness_scores.begin(), fitness_scores.end());
        size_t best_index = std::distance(fitness_scores.begin(), max_it);
        new_population.push_back(population[best_index]);
        
        // 生成新个体直到填满种群
        while (new_population.size() < m_populationSize) {
            // 选择父代
            auto [parent1, parent2] = selection(population, fitness_scores);
            
            // 交叉产生子代
            auto [child1, child2] = crossover(parent1, parent2, m_crossoverRate);
            
            // 变异
            child1 = mutate(child1, hash_indices, m_mutationRate);
            child2 = mutate(child2, hash_indices, m_mutationRate);
            
            // 添加到新种群
            new_population.push_back(child1);
            if (new_population.size() < m_populationSize) {
                new_population.push_back(child2);
            }
        }
        
        // 更新种群
        population = std::move(new_population);
    }
    
    // 评估最终种群中的所有个体
    std::vector<double> final_fitness;
    for (const auto& individual : population) {
        double fitness = evaluateFitness(individual, member_table, member_counts, port_speeds);
        final_fitness.push_back(fitness);
    }
    
    // 找出最优个体
    auto max_it = std::max_element(final_fitness.begin(), final_fitness.end());
    size_t best_index = std::distance(final_fitness.begin(), max_it);
    
    // 应用最优个体的交换操作
    auto best_table = applySwaps(member_table, population[best_index]);
    
    return best_table;
}

std::vector<GeneticAlgorithm::Individual> GeneticAlgorithm::initializePopulation(
    const std::unordered_map<WORD32, WORD32>& member_table,
    WORD32 population_size
) {
    std::vector<Individual> population;
    
    for (WORD32 i = 0; i < population_size; ++i) {
        population.push_back(createIndividual(member_table));
    }
    
    return population;
}

GeneticAlgorithm::Individual GeneticAlgorithm::createIndividual(
    const std::unordered_map<WORD32, WORD32>& member_table
) {
    // 创建一个随机个体，包含2-5个交换操作
    Individual individual;
    
    // 获取所有哈希索引
    std::vector<WORD32> hash_indices;
    for (const auto& entry : member_table) {
        hash_indices.push_back(entry.first);
    }
    
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> index_dist(0, hash_indices.size() - 1);
    std::uniform_int_distribution<size_t> count_dist(2, 5);
    
    // 随机确定交换操作数量
    size_t num_swaps = count_dist(gen);
    
    // 生成随机交换操作
    for (size_t i = 0; i < num_swaps; ++i) {
        size_t idx1 = index_dist(gen);
        size_t idx2;
        do {
            idx2 = index_dist(gen);
        } while (idx1 == idx2 && hash_indices.size() > 1);
        
        individual.emplace_back(hash_indices[idx1], hash_indices[idx2]);
    }
    
    return individual;
}

GeneticAlgorithm::Individual GeneticAlgorithm::mutate(
    const Individual& individual,
    const std::vector<WORD32>& hash_indices,
    double mutation_rate
) {
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<size_t> index_dist(0, hash_indices.size() - 1);
    
    // 复制原始个体
    Individual mutated = individual;
    
    // 基于概率决定变异方式
    double mutation_type = prob_dist(gen);
    
    // 增加新的随机交换操作
    if (mutation_type < mutation_rate) {
        size_t idx1 = index_dist(gen);
        size_t idx2;
        do {
            idx2 = index_dist(gen);
        } while (idx1 == idx2 && hash_indices.size() > 1);
        
        mutated.emplace_back(hash_indices[idx1], hash_indices[idx2]);
    }
    // 随机减少一个交换操作
    else if (mutation_type < mutation_rate + 0.2 && !mutated.empty()) {
        std::uniform_int_distribution<size_t> op_dist(0, mutated.size() - 1);
        size_t op_idx = op_dist(gen);
        mutated.erase(mutated.begin() + op_idx);
    }
    // 随机修改一个交换操作
    else if (!mutated.empty()) {
        std::uniform_int_distribution<size_t> op_dist(0, mutated.size() - 1);
        size_t op_idx = op_dist(gen);
        
        size_t idx1 = index_dist(gen);
        size_t idx2;
        do {
            idx2 = index_dist(gen);
        } while (idx1 == idx2 && hash_indices.size() > 1);
        
        mutated[op_idx] = std::make_pair(hash_indices[idx1], hash_indices[idx2]);
    }
    
    return mutated;
}

std::pair<GeneticAlgorithm::Individual, GeneticAlgorithm::Individual> GeneticAlgorithm::crossover(
    const Individual& parent1,
    const Individual& parent2,
    double crossover_rate
) {
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    
    // 创建子代
    Individual child1, child2;
    
    // 确定交叉点
    size_t min_length = std::min(parent1.size(), parent2.size());
    
    // 根据交叉率进行单点交叉
    for (size_t i = 0; i < min_length; ++i) {
        if (prob_dist(gen) < crossover_rate) {
            child1.push_back(parent1[i]);
            child2.push_back(parent2[i]);
        } else {
            child1.push_back(parent2[i]);
            child2.push_back(parent1[i]);
        }
    }
    
    // 复制剩余基因
    if (parent1.size() > min_length) {
        child1.insert(child1.end(), parent1.begin() + min_length, parent1.end());
    }
    
    if (parent2.size() > min_length) {
        child2.insert(child2.end(), parent2.begin() + min_length, parent2.end());
    }
    
    return {child1, child2};
}

std::pair<GeneticAlgorithm::Individual, GeneticAlgorithm::Individual> GeneticAlgorithm::selection(
    const std::vector<Individual>& population,
    const std::vector<double>& fitness_scores
) {
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 将所有适应度转换为正值
    double min_fitness = *std::min_element(fitness_scores.begin(), fitness_scores.end());
    double offset = std::abs(min_fitness) + 1.0; // 确保所有值都为正
    
    std::vector<double> positive_scores;
    for (double score : fitness_scores) {
        positive_scores.push_back(score + offset);
    }
    
    // 计算总适应度
    double total_fitness = std::accumulate(positive_scores.begin(), positive_scores.end(), 0.0);
    
    // 计算选择概率
    std::vector<double> selection_probs;
    for (double score : positive_scores) {
        selection_probs.push_back(score / total_fitness);
    }
    
    // 创建离散分布
    std::discrete_distribution<size_t> dist(selection_probs.begin(), selection_probs.end());
    
    // 选择两个父代
    size_t parent1_idx = dist(gen);
    size_t parent2_idx;
    do {
        parent2_idx = dist(gen);
    } while (parent1_idx == parent2_idx && population.size() > 1);
    
    return {population[parent1_idx], population[parent2_idx]};
}

double GeneticAlgorithm::evaluateFitness(
    const Individual& individual,
    const std::unordered_map<WORD32, WORD32>& original_table,
    const std::vector<WORD64>& member_counts,
    const std::unordered_map<WORD32, WORD32>& port_speeds
) {
    // 应用个体的交换操作
    auto modified_table = applySwaps(original_table, individual);
    
    // 计算端口负载
    auto port_loads = utils::calculatePortLoads(modified_table, member_counts);
    
    // 计算平衡指标
    auto eval = utils::calculateLoadBalanceMetrics(port_loads, port_speeds);
    
    // 计算适应度 (考虑交换操作数量作为代价)
    double exchange_cost_factor = 0.01; // 交换代价因子，可调整
    double fitness = utils::calculateBalanceScore(eval) - exchange_cost_factor * individual.size();
    
    return fitness;
}

std::unordered_map<WORD32, WORD32> GeneticAlgorithm::applySwaps(
    const std::unordered_map<WORD32, WORD32>& original_table,
    const Individual& individual
) {
    // 复制原始表
    auto result = original_table;
    
    // 应用所有交换操作
    for (const auto& swap_op : individual) {
        WORD32 hash_index1 = swap_op.first;
        WORD32 hash_index2 = swap_op.second;
        
        // 检查索引是否有效
        auto it1 = result.find(hash_index1);
        auto it2 = result.find(hash_index2);
        
        if (it1 != result.end() && it2 != result.end()) {
            // 交换端口分配
            std::swap(it1->second, it2->second);
        }
    }
    
    return result;
}

} // namespace ai_ecmp
