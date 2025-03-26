/**
 * @file process_state.h
 * @brief 工艺流程状态定义
 */
#ifndef __PROCESS_STATE_H__
#define __PROCESS_STATE_H__

/* 工艺流程状态枚举 */
typedef enum {
    PROCESS_IDLE,        // 空闲状态
    PROCESS_PREPARE,     // 准备阶段
    PROCESS_CALIBRATE,   // 校准阶段
    PROCESS_FEED,        // 进料阶段
    PROCESS_PRESS1,      // 一次加压
    PROCESS_PRESS2,      // 二次加压
    PROCESS_KEEP,        // 保温保压
    PROCESS_COOL,        // 冷却阶段
    PROCESS_COMPLETE,    // 完成状态
    PROCESS_ERROR       // 错误状态
} ProcessState;

/* 校准状态枚举 */
typedef enum {
    CALIBRATION_IDLE,    // 空闲状态
    CALIBRATION_X_AXIS,  // X轴校准
    CALIBRATION_Y_AXIS,  // Y轴校准
    CALIBRATION_Z_AXIS,  // Z轴校准
    CALIBRATION_COMPLETE,// 校准完成
    CALIBRATION_ERROR    // 校准错误
} CalibrationState;

/* 错误代码定义 */
#define ERROR_NONE                  0x00
#define ERROR_CALIBRATION_FAILED    0x01
#define ERROR_PRESSURE_LIMIT        0x02
#define ERROR_TEMPERATURE_LIMIT     0x03
#define ERROR_MOTOR_FAULT          0x04
#define ERROR_SENSOR_FAULT         0x05

#endif /* __PROCESS_STATE_H__ */ 