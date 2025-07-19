#include "ai_ecmp_main.hpp"
#include "ai_ecmp_api.h"
#include "ai_qos_public.h"
extern WORD32 AI_ECMP_SgConfigCtrl(WORD16 switchFlag, T_AI_ECMP_SG_CFG* p_sg_cfg);

VOID convertByteOrderWfg(T_AI_SG_WEIGHT_CFG& weightCfg)
{
    weightCfg.dwPortId = XOS_INVERT_WORD32(weightCfg.dwPortId);
    weightCfg.dwSpeed = XOS_INVERT_WORD32(weightCfg.dwSpeed);
    weightCfg.dwWeight = XOS_INVERT_WORD32(weightCfg.dwWeight);
}

VOID convertByteOrderSfg(T_AI_ECMP_SG_CFG& sgCfg)
{
    sgCfg.dwSgId = XOS_INVERT_WORD32(sgCfg.dwSgId);
    sgCfg.dwSeqId = XOS_INVERT_WORD32(sgCfg.dwSeqId);
    sgCfg.dwFwdLagId = XOS_INVERT_WORD32(sgCfg.dwFwdLagId);
    sgCfg.dwItemNum = XOS_INVERT_WORD32(sgCfg.dwItemNum);
    sgCfg.dwPortNum = XOS_INVERT_WORD32(sgCfg.dwPortNum);
    sgCfg.dwCounterBase = XOS_INVERT_WORD32(sgCfg.dwCounterBase);

    // 因为items[]没有结构定义，我们不能转换其内容

    for (size_t i = 0; i < FTM_TRUNK_MAX_HASH_NUM_15K; ++i)
    {
        convertByteOrderWfg(sgCfg.ports[i]);
    }
}

VOID convertByteOrderWeightMod(T_AI_ECMP_WEIGHT_MODIFY& weightMod)
{
    weightMod.dwSgId = XOS_INVERT_WORD32(weightMod.dwSgId);
    weightMod.dwSeqId = XOS_INVERT_WORD32(weightMod.dwSeqId);
    weightMod.dwModifyNum = XOS_INVERT_WORD32(weightMod.dwModifyNum);

    for (size_t i = 0; i < FTM_LAG_MAX_MEM_NUM_15K; ++i)
    {
        weightMod.adwPortId[i] = XOS_INVERT_WORD32(weightMod.adwPortId[i]);
        weightMod.adwWeight[i] = XOS_INVERT_WORD32(weightMod.adwWeight[i]);
    }
}

VOID convertByteOrderNhopMod(T_AI_ECMP_NHOP_MODIFY& nhopMod)
{
    nhopMod.dwSgId = XOS_INVERT_WORD32(nhopMod.dwSgId);
    nhopMod.dwSeqId = XOS_INVERT_WORD32(nhopMod.dwSeqId);
    nhopMod.dwItemNum = XOS_INVERT_WORD32(nhopMod.dwItemNum);

    for (size_t i = 0; i < FTM_TRUNK_MAX_HASH_NUM_15K; ++i)
    {
        nhopMod.adwLinkItem[i] = XOS_INVERT_WORD32(nhopMod.adwLinkItem[i]);
    }
}

static inline void LogEcmpSgCfg(const T_AI_ECMP_SG_CFG& cfg)
{
    /* 整体信息 */
    XOS_SysLog(LOG_EMERGENCY,
               "[AILP] T_AI_ECMP_SG_CFG("
               "dwSgId:%d, dwSeqId:%d, dwFwdLagId:%d, "
               "dwItemNum:%d, dwPortNum:%d, dwCounterBase:%d)\n",
               cfg.dwSgId, cfg.dwSeqId, cfg.dwFwdLagId,
               cfg.dwItemNum, cfg.dwPortNum, cfg.dwCounterBase);

    /* 逐个逻辑成员（hash item） */
    for (WORD32 i = 0; i < FTM_TRUNK_MAX_HASH_NUM_15K; ++i)
    {
        const auto& item = cfg.items[i];
        if (!item.IsValid())          /* 跳过空槽 */
            continue;

        XOS_SysLog(LOG_EMERGENCY,
                   "[AILP]   ITEM[%03d] PortId:%d  ItemOffset:%d\n",
                   i, item.dwPortId, item.dwItemOffset);
    }

    /* 逐个物理端口（成员） */
    for (WORD32 i = 0;
         i < cfg.dwPortNum && i < FTM_LAG_MAX_MEM_NUM_15K;
         ++i)
    {
        const auto& port = cfg.ports[i];
        if (!port.IsValid())
            continue;

        XOS_SysLog(LOG_EMERGENCY,
                   "[AILP]   PORT[%03d] PortId:%d  Speed:%d  Weight:%d\n",
                   i, port.dwPortId, port.dwSpeed, port.dwWeight);
    }
}
WORD32 aiEcmpCfgCallback(VOID * pArg, VOID * pMsgBody, WORD16 wMsgLen, VOID * pPData, BOOLEAN bSame)
{
    XOS_SysLog(LOG_EMERGENCY,"[AILP] %s \n" , __FUNCTION__);
    WORD32 dwRet = AI_SUCCESS;

    XOS_SysLog(LOG_EMERGENCY,"[AILP] aiEcmpCfgCallback %s \n" , __FUNCTION__);
    //aiInputErrorInfo("[AI_QOS] <aiEcmpCfgCallback> entry");
    if (NULL == pMsgBody)
    {
        //aiInputErrorInfo("[AI_QOS] <aiEcmpCfgCallback> pMsgBody Null");
        return AI_FAILURE;
    }

    // // 检查消息长度是否正确
    const size_t expectedSize = sizeof(T_AI_ECMP_SG_CFG);
    if (wMsgLen != expectedSize)
    {
        //aiInputErrorInfo("[AI_QOS] <aiEcmpCfgCallback> Invalid message length");
        return AI_FAILURE;
    }

    // 如果 bSame 为 FALSE，进行字节序转换
    T_AI_ECMP_SG_CFG& ecmpMsg = *reinterpret_cast<T_AI_ECMP_SG_CFG*>(pMsgBody);
    if (FALSE == bSame)
    {
        convertByteOrderSfg(ecmpMsg);
    }
    XOS_SysLog(LOG_EMERGENCY,"[AILP] aiEcmpCfgCallback begin\n" );
    LogEcmpSgCfg(ecmpMsg);
    // AI_ECMP_SgConfigCtrl(0,&ecmpMsg);

    auto& aiSlbManagerSingleton = CAISlbManagerSingleton::getManagerInstance();
    aiSlbManagerSingleton.handleSgConfigCtrl(1, &ecmpMsg);  //更新配置

    // ecmp实例针对当前传过来的处理数据
    // todo 
    return dwRet;
}


WORD32 aiEcmpStateCallback(VOID * pArg, VOID * pMsgBody, WORD16 wMsgLen, VOID * pPData, BOOLEAN bSame)
{
    XOS_SysLog(LOG_EMERGENCY, "[AILP] %s entered \n", __FUNCTION__);

    WORD32 dwRet = AI_SUCCESS;

    // 检查 pMsgBody 是否为空
    if (NULL == pMsgBody)
    {
        XOS_SysLog(LOG_EMERGENCY, "[AILP] %s: pMsgBody is NULL\n", __FUNCTION__);
        return AI_FAILURE;
    }

    // 检查消息长度是否符合预期
    const size_t expectedSize = sizeof(T_AI_ECMP_COUNTER_STATS_MSG);
    if (wMsgLen != expectedSize)
    {
        XOS_SysLog(LOG_EMERGENCY, "[AILP] %s: Invalid message length, expected %zu but got %d\n", __FUNCTION__, expectedSize, wMsgLen);
        return AI_FAILURE;
    }

    // 打印当前消息体的开始
    XOS_SysLog(LOG_EMERGENCY, "[AILP] %s: Message body is valid. Proceeding with byte order conversion if needed.\n", __FUNCTION__);

    // 获取并转换字节顺序（如果需要）
    T_AI_ECMP_COUNTER_STATS_MSG& ecmpMsg = *reinterpret_cast<T_AI_ECMP_COUNTER_STATS_MSG*>(pMsgBody);
    if (FALSE == bSame)
    {
        XOS_SysLog(LOG_EMERGENCY, "[AILP] %s: Byte order conversion is required.\n", __FUNCTION__);
        //convertByteOrderSfg(ecmpMsg);
    }
    else
    {
        XOS_SysLog(LOG_EMERGENCY, "[AILP] %s: No byte order conversion needed.\n", __FUNCTION__);
    }

    // 打印转换后的消息内容（假设我们可以直接打印所有数据，具体内容可以根据结构体定义调整）
    XOS_SysLog(LOG_EMERGENCY, "[AILP] %s: ecmpMsg counter results:\n", __FUNCTION__);
    for (int i = 0; i < 128; ++i)
    {
        XOS_SysLog(LOG_EMERGENCY, "[AILP] ecmpMsg.counterResults[%d]: 0x%llX\n", i, ecmpMsg.statCounter[i]);
    }

    // ecmp实例针对当前传过来的处理数据（后续处理的地方，可以根据需求继续）
    // todo: 对 ecmpMsg 进行处理操作

    auto& aiSlbManagerSingleton = CAISlbManagerSingleton::getManagerInstance();
    aiSlbManagerSingleton.runOptimizationCycle(ecmpMsg);  //更新计数
    XOS_SysLog(LOG_EMERGENCY, "[AILP] %s: Processing ECMP message data...\n", __FUNCTION__);

    return dwRet;
}



VOID aiEcmpSendWeightModify(const T_AI_ECMP_WEIGHT_MODIFY& input)
{
    const WORD32 buffer_size = 4096; //开辟足够的空间存储检测信息
    BYTE buffer[buffer_size] = {0};
    
    T_AI_ECMP_WEIGHT_MODIFY temp = input;  // 创建副本，避免修改原始数据
    convertByteOrderWeightMod(temp);

    if (sizeof(temp) > buffer_size)
    {
        // 错误处理：结构体太大
        return;
    }

    MEMCPY_S(buffer, buffer_size, &temp, sizeof(temp));
    aiEcmpSendWeightChangeToUfp(buffer, buffer_size);
}

VOID aiEcmpSendNhopModify(const T_AI_ECMP_NHOP_MODIFY& input)
{
    const WORD32 buffer_size = 4096; //开辟足够的空间存储检测信息
    BYTE buffer[buffer_size] = {0};
    
    T_AI_ECMP_NHOP_MODIFY temp = input;
    convertByteOrderNhopMod(temp);

    if (sizeof(temp) > buffer_size)
    {
        // 错误处理：结构体太大
        return;
    }

    MEMCPY_S(buffer, buffer_size, &temp, sizeof(temp));

    aiEcmpSendSynCfgToUfp(buffer, buffer_size);
}



VOID aiEcmpReadAction()
{
   // todo 执行动作
   // runOptimizationCycle
   // for instance 
   //   du counter
   //   算平衡度
   //   runOptimizationCycle()
   //   dilever() 
}    


WORD32 aiEcmpReadTimePro(VOID *pArg)
{
    aiEcmpReadAction();
    //业务处理
    return AI_SUCCESS;
}
