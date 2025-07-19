#ifndef AI_ECMP_TYPES_H
#define AI_ECMP_TYPES_H

#include "ai_common.h"


// #define AI_ECMP_MAX_ITEM_NUM 128
// #define AI_ECMP_MAX_PORT_NUM 128


const WORD32 FTM_LAG_MAX_MEM_NUM_15K = 128;
const WORD32 FTM_TRUNK_MAX_HASH_NUM_15K = 128;
const WORD32 AI_FCM_ECMP_MSG_ITEM_NUM = 128; 

#ifdef __cplusplus
extern "C" {
#endif

/* 基本类型定义 */
typedef unsigned short WORD16;
typedef unsigned int WORD32;
typedef unsigned long long WORD64;
typedef double DOUBLE;
typedef float FLOAT;
typedef bool BOOLEAN;

/* 状态枚举定义 */
typedef enum {
    AI_ECMP_INIT = 1,     /* 初始化状态 */
    AI_ECMP_WAIT,         /* 等待Counter信息 */
    AI_ECMP_ADJUST,       /* 调整中 */
    AI_ECMP_EVAL,         /* 评估中 */
    AI_ECMP_EXPAND,       /* 扩容中 */
    AI_ECMP_BALANCE,      /* 平衡状态 */
    AI_ECMP_FAIL          /* 调整失败 */
} T_AI_ECMP_STATUS;

/* ECMP逻辑成员信息 */
typedef struct {
    WORD32 dwPortId;         /* 逻辑成员对应的PortId */
    WORD32 dwItemOffset;    /*  逻辑成员索引偏移 */
} T_AI_SG_ITEM_CFG;

/* ECMP物理成员权重信息 */
typedef struct {
    WORD32 dwPortId;         /* 物理成员PortId */
    WORD32 dwSpeed;          /* 物理成员带宽 */
    WORD32 dwWeight;         /* 物理成员散列后的权重(逻辑链路数) */
} T_AI_SG_WEIGHT_CFG;

/* ECMP SG配置信息 */
typedef struct {
    WORD32 dwSgId;           /* SG id */
    WORD32 dwSeqId;            /* 版本号 */
    WORD32 dwFwdLagId;       /* 微码Lag id */
    WORD32 dwItemNum;        /* 散列后逻辑成员链路数 */
    WORD32 dwPortNum;        /* 实际物理成员链路数 */
    WORD32 dwCounterBase;    /* 成员Counter Id基地址 */
    T_AI_SG_ITEM_CFG items[FTM_TRUNK_MAX_HASH_NUM_15K];   /* 逻辑成员数组 */
    T_AI_SG_WEIGHT_CFG ports[FTM_LAG_MAX_MEM_NUM_15K];  /* 物理成员数组 */
} T_AI_ECMP_SG_CFG;

// /* 权重修改参数 */
// typedef struct {
//     WORD32 dwSgId;           /* SG id */
//     WORD32 dwSeqId;            /* 版本号 */
//     WORD32 dwModifyNum;      /* 待修改的port id数量 */
//     WORD32 adwPortId[FTM_LAG_MAX_MEM_NUM_15K];    /* 物理成员PortId */
//     WORD32 adwWeight[FTM_LAG_MAX_MEM_NUM_15K];    /* 修改权重(逻辑链路数) */
// } T_AI_ECMP_WEIGHT_MODIFY;

/* 下一跳修改参数 */
typedef struct {
    WORD32 dwSgId;           /* SG id */
    WORD32 dwSeqId;            /* 版本号 */
    WORD32 dwItemNum;        /* 散列后逻辑成员链路数 */
    WORD32 adwLinkItem[FTM_TRUNK_MAX_HASH_NUM_15K]; /* 修改后散列逻辑成员数组 */
} T_AI_ECMP_NHOP_MODIFY;

/* 评估指标 */
typedef struct {
    DOUBLE upBoundGap;      /* 正偏差 */
    DOUBLE lowBoundGap;     /* 负偏差 */
    DOUBLE totalGap;        /* 总体偏差 */
    DOUBLE avgGap;          /* 平均偏差 */
    DOUBLE balanceScore;    /* 平衡度得分 */
} T_AI_ECMP_EVAL;

typedef struct 
{
    WORD64 statCounter[AI_FCM_ECMP_MSG_ITEM_NUM];  /*当前支持智能队列数*/
}T_AI_ECMP_COUNTER_STATS_MSG;

/**
 * @brief 将ECMP状态枚举值转换为字符串
 * @param status ECMP状态
 * @return 状态对应的字符串
 */


#ifdef __cplusplus
}
#endif

#endif /* AI_ECMP_TYPES_H */