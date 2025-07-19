
/*****************************************************************
*File Name  : ai_common.h
*Desciption : AI通用定义
*Version    : V1.0
*Record     ：2021-11-30
*
*
******************************************************************/

#ifndef    AI_COMMON_H
#define    AI_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include "ai_error.h"
#include "ai_diag.h"
#include "ai_common_ex.h"

#define    AI_EPS                   (1e-6)
#define    AI_NA                    (0)
#define    AI_READ(X)               (1)
#define    AI_ANNOTATION(X)         (0)
#define    AI_STR_ARR_NAME_LEN      (255)

#define    AI_ASSERT(exp)           XOS_ASSERT(exp)

/* 用于周期表示 */
#define AI_PERIOD_TEN          (10)
#define AI_PERIOD_QUARTER      (15)
#define AI_PERIOD_HALF_HOUR    (30)
#define AI_PERIOD_HOUR         (60)

/* 用于时间表示 */
#define AI_TIMER_PERIOD_1MS         (1)                             //定时器基准周期1ms
#define AI_TIMER_PERIOD_10MS        (10)                            //定时器基准周期10ms
#define AI_TIMER_PERIOD_S           (1000)                          //定时器基准周期1s
#define AI_TIMER_PERIOD_MIN         (60 * 1000)        //定时器基准周期1分钟

/*内存申请，释放*/
#define    AI_GetUB(_size,usrID)  XOS_GetUB_EX((_size),(usrID))
#define    AI_RetUB(_pucBuf)      XOS_RetUB(_pucBuf)

/* 兼容老代码，确保是POD类型才可以使用AI_MEMSET和AI_DATA_INIT，建议直接使用 XOS_MEMSET_S */
#define    AI_MEMSET(_addr_,_value_,_len_)    XOS_MEMSET_S((_addr_),(_len_),(_value_),(_len_))   
#define    AI_DATA_INIT(_data_)               XOS_MEMSET_S(&(_data_),sizeof(_data_),0,sizeof(_data_)) 
#define    AI_CONT_STR(_key_,_value_,_str_,_form_)     ( (_key_)==(_value_)? (_str_): (_form_) )   

#define AI_VALUE_BYTE_MAX       (0xFF)
#define AI_VALUE_WORD16_MAX     (0xFFFF)
#define AI_VALUE_WORD32_MAX     (0xFFFFFFFF)
#define AI_VALUE_WORD64_MAX     (0xFFFFFFFFFFFFFFFF)

#define AI_VALUE2BYTE(val)  (BYTE)((val) & 0xFF)
#define AI_VALUE2WORD(val)  (WORD16)((val) & 0xFFFF)
#define AI_VALUE2DWORD(val) (WORD32)((val) & 0xFFFFFFFF)
#define AI_VALUE2WORD64(val) (WORD64)((val) & 0xFFFFFFFFFFFFFFFF)

/*<compare MAX>*/
#define AI_STATS_MATH_MAX(_v1_,_v2_)    ((_v1_)>(_v2_) ? (_v1_):(_v2_))
#define AI_STATS_MATH_AVG(_v1_,_v2_)    (((_v1_)+(_v2_))/2)

/* Started by CodeFlow, REQ, ATYP */
/* Started by AICoder, pid:368b8bae2adb466895c3e0a7acb31cd4 */
#define AI_FLOAT_EQ(a, b) (fabs((a) - (b)) < AI_EPS)        // 比较两个浮点数是否相等
#define AI_FLOAT_NEQ(a, b) (fabs((a) - (b)) >= AI_EPS)      // 比较两个浮点数是否不相等
#define AI_FLOAT_GT(a, b) ((a) > (b) + AI_EPS)              // 比较两个浮点数 a 是否大于 b
#define AI_FLOAT_LT(a, b) ((a) < (b) - AI_EPS)              // 比较两个浮点数 a 是否小于 b
#define AI_FLOAT_GE(a, b) ((a) >= (b) - AI_EPS)             // 比较两个浮点数 a 是否大于等于 b
#define AI_FLOAT_LE(a, b) ((a) <= (b) + AI_EPS)             // 比较两个浮点数 a 是否小于等于 b
/* Ended by AICoder, pid:368b8bae2adb466895c3e0a7acb31cd4 */
/* Ended by CodeFlow, REQ, ATYP */

/* Started by AICoder, pid:5ec80tc32braad6141420ae9b089fa268ad4a98a */
/**
 * @brief 宏定义，用于确保计数器在指定范围内。
 *
 * 该宏会检查传入的计数器值，如果其值超过 maxNum，
 * 则将其取模以保持在范围内。同时，确保 maxNum 不为零。
 *
 * @param counter 需要检查和更新的计数器变量。
 * @param maxNum 计数器的最大值。
 */
#define AI_UPDATE_COUNTER(counter, maxNum) \
do { \
    if ((maxNum) != 0) \
    { \
        if ((counter) >= (maxNum)) \
        { \
            (counter) %= (maxNum); \
        } \
    } \
    else \
    { \
        /* 错误处理：maxNum 不能为零 */ \
        AI_TRACE_ERROR("\n Return %x ! \n", maxNum); \
    } \
} while (0)
/* Ended by AICoder, pid:5ec80tc32braad6141420ae9b089fa268ad4a98a */

/*<compare the values>*/
#define AI_CMP_VALUE(_v1_,_v2_)\
do{\
     if((_v1_) != (_v2_) )\
     {\
        return AI_FALSE;\
     }\
 }while(0)

/**< AI_CHECK_RC_GOTO_LABEL 检查rc返回类型,跳转到label,WITH_ASSERT*/
#define AI_CHECK_RC_GOTO_LABEL(rc,label)\
 do{\
     if(AI_SUCCESS != rc)\
     {\
        AI_TRACE_ERROR("\n Return %x ! \n", rc );\
        AI_ASSERT(0);\
        goto label;\
     }\
 }while(0)

 /**< AI_CHECK_XOS_RET: 对 tulip 的函数返回值做检查,WITH_ASSERT */
#define AI_CHECK_XOS_RET(XosRet)  \
            do{ \
                if (XOS_SUCCESS != (XosRet)) \
                { \
                    AI_TRACE_ERROR("\n Ret: 0x%x \n", XosRet); \
                    AI_ASSERT(0); \
                    return(XosRet); \
                } \
            }while(0)

#define AI_ENTRY_POINT_CHECK(pArg_In,pCall)\
do{\
    if(NULL == pArg_In)\
    {\
        AI_TRACE_ERROR("\n AIRP---parament of %s---->%s is NULL!\n", pCall,#pArg_In);\
        AI_ASSERT(0);\
        return AI_FAILURE;\
    }\
}while(0)

/*根据返回值进行判断*/
#define AI_ENTRY_CHECK_RC(rc, call, becall)\
do{\
    if(AI_SUCCESS != rc)\
    {\
        AI_TRACE_ERROR("\n AIRP: %s Call %s Fail![ErrorCode:%d]\n", call, becall, rc);\
        return rc;\
    }\
}while(0)

 /**< AI_CHECK_RC 检查rc返回类型，WITH_ASSERT*/
#define AI_CHECK_RC(rc)\
 do{\
     if(AI_SUCCESS != rc)\
     {\
        AI_TRACE_ERROR("\n error Return %x ! \n", rc );\
        AI_ASSERT(0);\
        return rc;\
     }\
 }while(0)

#define AI_CHECK_NO_RETURN(rc)\
do {\
    if(AI_SUCCESS != rc)\
    {\
        AI_TRACE_ERROR("\n error Return %x ! \n", rc );\
        AI_ASSERT(0);\
        return;\
    }\
} while(0)
typedef enum
{
   AI_NO_READY,
   AI_READY_OK
   
}AI_READY_STATE;

typedef enum
{
   AI_INVALID,
   AI_VALID
   
}AI_VALID_STATE;

/*<===定义使用状态===>*/
typedef enum
{
   AI_UNUSE,
   AI_INUSE

}AI_USE_STATE;

/*<===定义激活状态===>*/
typedef enum
{
   AI_DISABLE,
   AI_ENABLE   

}AI_ENABLE_STATE;

/*<===定义虚拟节能开关状态===>*/
typedef enum 
{
    AI_VIRTUAL_SWITCH_NOT_SUPPORT,
    AI_VIRTUAL_SWITCH_CLOSE,
    AI_VIRTUAL_SWITCH_OPEN,
}AI_VIRTUAL_SWITCH_STATE;

/*<===定义BOOL状态===>*/
typedef enum
{
   AI_FALSE,
   AI_TRUE 
     
}AI_BOOL;

/*<===定义函数返回状态===>*/
typedef enum
{
   AI_SUCCESS,
   AI_FAILURE 
     
}AI_RETURN_STATUS;

typedef enum
{
   AI_OFF,
   AI_ON 
     
}AI_LOCK_STATUS;

VOID diagAiErrorInfoShow();

VOID diagAiFilteredErrorInfoShow(const char *filter) ;

VOID diagAiErrorInfoClear();


VOID aiInputErrorInfo(const char *error_type);

VOID diagErrorInfoFreeMemory();

#define AI_IPV4_LEN             4
#define AI_MAC_LEN              6
#define AI_IPV6_LEN             16

static inline void ai_dword_add(WORD32 *dwBaseValue, WORD32 dwValue)
{
    if (*dwBaseValue <= 0xFFFFFFFF - dwValue) {
        *dwBaseValue += dwValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *dwBaseValue = *dwBaseValue - (0xFFFFFFFF - dwValue) - 1; /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_dword_cut(WORD32 *dwBaseValue, WORD32 dwValue)
{
    if (*dwBaseValue >= dwValue) {
        *dwBaseValue -= dwValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *dwBaseValue = 0xFFFFFFFF - (dwValue - *dwBaseValue) + 1; /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_dword_add_set_inite(WORD32 *dwBaseValue, WORD32 dwValue, WORD32 dwIniteValue)
{
    if (*dwBaseValue <= 0xFFFFFFFF - dwValue) {
        *dwBaseValue += dwValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *dwBaseValue = dwIniteValue; /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_dword_cut_set_inite(WORD32 *dwBaseValue, WORD32 dwValue, WORD32 dwIniteValue)
{
    if (*dwBaseValue >= dwValue) {
        *dwBaseValue -= dwValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *dwBaseValue = dwIniteValue; /* 将自动翻转转换为控制翻转 */
    }
}

static inline WORD32 ai_dword_add2(WORD32 dwValue1, WORD32 dwValue2)
{
    if (dwValue1 <= 0xFFFFFFFF - dwValue2) {
        return (dwValue1 + dwValue2);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        return(dwValue1 - (0xFFFFFFFF - dwValue2) - 1); /* 将自动翻转转换为控制翻转 */
    }
}

static inline WORD32 ai_dword_cut2(WORD32 dwValue1, WORD32 dwValue2)
{
    if (dwValue1 >=  dwValue2) {
        return (dwValue1 - dwValue2);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        return(0xFFFFFFFF - (dwValue2 - dwValue1) + 1); /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_word64_add(WORD64 *dwBaseValue, WORD64 dwValue)
{
    if (*dwBaseValue <= 0xFFFFFFFFFFFFFFFF - dwValue) {
        *dwBaseValue += dwValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *dwBaseValue = *dwBaseValue - (0xFFFFFFFFFFFFFFFF - dwValue) - 1; /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_word64_cut(WORD64 *dwBaseValue, WORD64 dwValue)
{
    if (*dwBaseValue >=  dwValue) {
        *dwBaseValue -= dwValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
       *dwBaseValue = 0xFFFFFFFFFFFFFFFF - (dwValue - *dwBaseValue) + 1;  /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_word_add(WORD16 *wBaseValue, WORD16 wValue)
{
    if (*wBaseValue <= 0xFFFF - wValue) {
        *wBaseValue += wValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *wBaseValue = *wBaseValue - (0xFFFF - wValue) - 1; /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_word_cut(WORD16 *wBaseValue, WORD16 wValue)
{
    if (*wBaseValue >= wValue) {
        *wBaseValue -= wValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *wBaseValue = 0xFFFF - (wValue - *wBaseValue) + 1;  /* 将自动翻转转换为控制翻转 */
    }
}

static inline WORD16 ai_word_add2(WORD16 wValue1, WORD16 wValue2)
{
    if (wValue1 <= 0xFFFF - wValue2) {
        return (wValue1 + wValue2);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        return(wValue1 - ((WORD16)(0xFFFF) - wValue2) - (WORD16)1); /* 将自动翻转转换为控制翻转 */
    }
}

static inline WORD16 ai_word_cut2(WORD16 wValue1, WORD16 wValue2)
{
    if (wValue1 >= wValue2) {
        return (wValue1 - wValue2);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        return((WORD16)(0xFFFF) - (wValue2 - wValue1) + (WORD16)1); /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_byte_add(BYTE *wBaseValue, BYTE wValue)
{
    if (*wBaseValue <= 0xFF - wValue) {
        *wBaseValue += wValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *wBaseValue = *wBaseValue - (0xFF - wValue) - 1; /* 将自动翻转转换为控制翻转 */
    }
}

static inline void ai_byte_cut(BYTE *wBaseValue, BYTE wValue)
{
    if (*wBaseValue >= wValue) {
        *wBaseValue -= wValue;
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        *wBaseValue = 0xFF - (wValue - *wBaseValue) + 1; /* 将自动翻转转换为控制翻转 */
    }
}

static inline BYTE ai_byte_add2(BYTE wBaseValue, BYTE wValue)
{
    if (wBaseValue <= 0xFF - wValue) {
        return (wBaseValue+wValue);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
       //return ((BYTE)(wBaseValue&0XFF) - ((BYTE)(0xFF) - wValue) - (BYTE)1); /* 将自动翻转转换为控制翻转 */
        return AI_VALUE2BYTE(AI_VALUE2BYTE(wBaseValue) - ((BYTE)(0xFF) - AI_VALUE2BYTE(wValue)) - (BYTE)1); /* 将自动翻转转换为控制翻转 */
    }
}

static inline BYTE ai_byte_cut2(BYTE wBaseValue, BYTE wValue)
{
    if (wBaseValue >= wValue) {
        return (wBaseValue-wValue);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
       //return ((BYTE)(wBaseValue&0XFF) - ((BYTE)(0xFF) - wValue) - (BYTE)1); /* 将自动翻转转换为控制翻转 */
        return AI_VALUE2BYTE((BYTE)(0xFF)- (AI_VALUE2BYTE(wValue) - AI_VALUE2BYTE(wBaseValue)) + (BYTE)1); /* 将自动翻转转换为控制翻转 */
    }
}

static inline BYTE ai_word16_cut2(BYTE wValue1, BYTE wValue2)
{
    if (wValue1 >= wValue2) {
        return (wValue1 - wValue2);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        return((BYTE)(0xFF) - ( wValue2 - (BYTE)(wValue1 &0XFF)) + (BYTE)1); /* 将自动翻转转换为控制翻转 */
    }
}

static inline WORD64 ai_word64_add2(WORD64 wValue1, WORD64 wValue2)
{
    if (wValue1 <= 0xFFFFFFFFFFFFFFFF - wValue2) {
        return (wValue1 + wValue2);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        return(wValue1 - ((WORD64)(0xFFFFFFFFFFFFFFFF) - wValue2) - (WORD64)1); /* 将自动翻转转换为控制翻转 */
    }
}

static inline WORD64 ai_word64_cut2(WORD64 wValue1, WORD64 wValue2)
{
    if (wValue1 >= wValue2) {
        return (wValue1 - wValue2);
    }
    else {
        /*
            A+B = A-(-B)
                = A-(MAX+1-B)
                = A-(MAX-B)-1
        */
        return((WORD64)(0xFFFFFFFFFFFFFFFF) - (wValue2 - wValue1) + (WORD64)1); /* 将自动翻转转换为控制翻转 */
    }
}

#ifdef __cplusplus
}
#endif
#endif
