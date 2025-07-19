#ifndef AI_ECMP_ERROR_H
#define AI_ECMP_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/* 智能ECMP错误码定义 */
typedef enum {
    AI_ECMP_SUCCESS                = 0,    /* 操作成功 */
    AI_ECMP_ERR_INVALID_PARAM      = 1001, /* 无效参数 */
    AI_ECMP_ERR_NO_MEMORY          = 1002, /* 内存分配失败 */
    AI_ECMP_ERR_NOT_FOUND          = 1003, /* 未找到指定实例 */
    AI_ECMP_ERR_ALREADY_EXIST      = 1004, /* 实例已存在 */
    AI_ECMP_ERR_CONFIG_INVALID     = 1005, /* 配置无效 */
    AI_ECMP_ERR_NO_SPACE           = 1006, /* 没有调整空间 */
    AI_ECMP_ERR_COUNTER_READ       = 1007, /* 读取计数器失败 */
    AI_ECMP_ERR_OPTIMIZE_FAILED    = 1008, /* 优化算法失败 */
    AI_ECMP_ERR_EXPAND_FAILED      = 1009, /* 扩容失败 */
    AI_ECMP_ERR_ADJUST_FAILED      = 1010, /* 调整失败 */
    AI_ECMP_ERR_SG_OPERATION       = 1011, /* SG操作失败 */
    AI_ECMP_ERR_NO_INSTANCE        = 1012, /* 没有ECMP实例 */
    AI_ECMP_ERR_INTERNAL           = 1999  /* 内部错误 */
} T_AI_ECMP_ERROR_CODE;

/* 错误信息结构体 */
// typedef struct {
//     WORD32 errorCode;              /* 错误码 */
//     char errorMessage[128];        /* 错误信息 */
//     WORD32 sgId;                   /* 相关的SG ID */
//     WORD32 additionalInfo;         /* 附加信息 */
// } T_AI_ECMP_ERROR_INFO;


#ifdef __cplusplus
}
#endif

#endif /* AI_ECMP_ERROR_H */
