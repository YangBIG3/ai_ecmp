#ifndef AI_ECMP_MANAGER_H
#define AI_ECMP_MANAGER_H

#include <unordered_map>
#include <memory>
#include <functional> 
#include "ai_ecmp_instance.hpp"


namespace ai_ecmp {

/**
 * ECMP管理器类，负责管理所有ECMP实例
 */
class CAISlbManagerSingleton {
public:
    /**
     * 获取单例实例
     * @return 单例实例引用
     */
    static CAISlbManagerSingleton& getManagerInstance();
    
    /**
     * 处理SG配置控制
     * @param bSwitchFlag 开关标志
     * @param pSgCfg SG配置信息
     * @return 操作结果码
     */
    WORD32 handleSgConfigCtrl(WORD16 bSwitchFlag, T_AI_ECMP_SG_CFG* pSgCfg);
    
    /**
     * 执行优化和调整过程
     * @param ecmpMsg 计数器统计消息
     * @return 总的操作结果码
     */
    WORD32 runOptimizationCycle(T_AI_ECMP_COUNTER_STATS_MSG& ecmpMsg);
    
    /**
     * 获取实例状态信息
     * @param dwSgId SG ID
     * @param pStatusInfo 状态信息输出指针
     * @return 操作结果码
     */
    WORD32 getInstanceStatus(WORD32 dwSgId, void* pStatusInfo);
    
    /**
     * 获取所有实例数量
     * @return 实例数量
     */
    WORD32 getInstanceCount();
    
    // ===== 新增：实例访问方法 =====
    /**
     * @brief 获取指定SG ID的实例指针
     * @param dwSgId SG ID
     * @return 实例指针，如果不存在则返回nullptr
     */
    EcmpInstance* getInstance(WORD32 dwSgId);
    
    /**
     * @brief 获取所有实例的引用（用于遍历）
     * @return 实例映射表的引用
     */
    const std::unordered_map<WORD32, std::unique_ptr<EcmpInstance>>& getAllInstances() const {
        return m_instances;
    }
    
    /**
     * @brief 对所有实例执行操作
     * @param func 要执行的函数
     */
    void forEachInstance(std::function<void(WORD32, EcmpInstance*)> func);
    
    /*
     * 记录SG配置详细信息
     * @param pSgCfg SG配置信息
     */
    void LogSgConfigDetails(const T_AI_ECMP_SG_CFG* pSgCfg);
    
private:
    // 单例模式，禁止外部创建实例
    CAISlbManagerSingleton() = default;
    ~CAISlbManagerSingleton() = default;
    CAISlbManagerSingleton(const CAISlbManagerSingleton&) = delete;
    CAISlbManagerSingleton& operator=(const CAISlbManagerSingleton&) = delete;
    
    // ECMP实例映射表
    std::unordered_map<WORD32, std::unique_ptr<EcmpInstance>> m_instances;
    
    // 调用SG权重修改接口
    WORD32 callSgWeightModifyCtrl(T_AI_ECMP_WEIGHT_MODIFY* pOpParam, T_AI_ECMP_SG_CFG* pSgCfg);
    
    // 调用SG下一跳修改接口
    WORD32 callSgItemNhopModifyCtrl(T_AI_ECMP_NHOP_MODIFY* pOpParam, T_AI_ECMP_SG_CFG* pSgCfg);
};

} // namespace ai_ecmp

#endif /* AI_ECMP_MANAGER_H */
