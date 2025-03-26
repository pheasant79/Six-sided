/**
 * @file system_params.h
 * @brief 系统参数定义
 */
#ifndef __SYSTEM_PARAMS_H__
#define __SYSTEM_PARAMS_H__

#include <stdint.h>

/* 系统参数范围 */
#define MAX_PRESSURE        1500    // 最大压力(kPa)
#define MIN_PRESSURE        0       // 最小压力
#define MAX_TEMPERATURE    200     // 最大温度(°C)
#define MIN_TEMPERATURE    0       // 最小温度
#define MAX_TIME          3600    // 最大时间(s)
#define REFRESH_RATE_MS   100     // 刷新率(ms)

/* 显示模式 */
typedef enum {
    DISPLAY_MODE_NORMAL = 0,   // 正常模式
    DISPLAY_MODE_NIGHT = 1     // 夜间模式
} DisplayMode;

/* 系统参数结构体 */
typedef struct {
    float pressure;            // 当前压力
    float temperature;         // 当前温度
    uint32_t runtime;         // 运行时间
    uint8_t alert;            // 警报状态
    uint16_t alert_count;     // 警报计数
    DisplayMode mode;         // 显示模式
} SystemParams;

/* 工艺参数结构体 */
typedef struct {
    float target_pressure;    // 目标压力
    float target_temp;        // 目标温度
    uint32_t keep_time;       // 保持时间
    uint32_t cool_time;       // 冷却时间
    uint8_t stage_complete;   // 阶段完成标志
} ProcessParams;

#endif /* __SYSTEM_PARAMS_H__ */ 