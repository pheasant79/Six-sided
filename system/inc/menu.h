/**
 * @file menu.h
 * @brief 六面顶压机菜单系统头文件
 */
#ifndef __MENU_H__
#define __MENU_H__

#include "main.h"
#include "motor.h"
#include "oled.h"
#include "process_state.h"
#include "system_params.h"
#include "icon_def.h"

/* UI配置常量 */
#define SCREEN_WIDTH  128    // OLED屏幕宽度
#define SCREEN_HEIGHT 64     // OLED屏幕高度
#define CHAR_WIDTH    16     // 字符宽度
#define CHAR_HEIGHT   8      // 字符高度
#define ROWS          4      // 显示行数
#define COLS          8      // 显示列数

/* 辅助宏定义 */
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

/* 系统参数范围定义 */
#define MAX_PRESSURE     200.0f  // 最大压力(kPa)
#define MIN_PRESSURE     0.0f    // 最小压力(kPa)
#define MAX_TEMPERATURE  200     // 最大温度(°C)
#define MIN_TEMPERATURE  0       // 最小温度(°C)
#define MAX_TIME        3600     // 最大时间(秒)
#define MAX_STEP        1000     // 最大步数

/* 图标定义 */
#define ICON_START      "⯈"     // 启动/选择图标
#define ICON_CHECK      "✔"     // 检查通过图标
#define ICON_HEAT       "🔥"    // 加热图标
#define ICON_KEEP       "♨"     // 保温图标
#define ICON_COOL       "❄"     // 冷却图标
#define ICON_SETTING    "⚙"     // 设置图标
#define ICON_HISTORY    "📜"    // 历史图标
#define ICON_WARNING    "⚠"     // 警告图标
#define ICON_UP         "⬆"     // 向上图标
#define ICON_DOWN       "⬇"     // 向下图标
#define ICON_BACK       "←"     // 返回图标
#define ICON_NEXT       "→"     // 下一步图标

/* 菜单状态枚举 */
typedef enum {
    MENU_MAIN,         // 主菜单
    MENU_PROCESS,      // 启动流程
    MENU_CHECK,        // 系统检查
    MENU_PRESSURE,     // 加压加热
    MENU_KEEP,         // 保温保压
    MENU_COOL,         // 卸压冷却
    MENU_PARAM,        // 参数设置
    MENU_MOTOR,        // 电机控制
    MENU_CALIBRATE,    // 调平校准
    MENU_HISTORY,      // 历史日志
    MENU_ALARM         // 报警界面
} MenuState;

/* 系统检查项枚举 */
typedef enum {
    CHECK_PRESSURE,    // 油压检查
    CHECK_SEAL,        // 密封检查
    CHECK_ALIGN,       // 顶锤对中
    CHECK_PARAM,       // 电控参数
    CHECK_ALL          // 全部检查项
} CheckItem;

/* 工艺阶段枚举 */
typedef enum {
    STAGE_IDLE,        // 空闲
    STAGE_CHECK,       // 检查
    STAGE_PRESS1,      // 一次加压
    STAGE_PRESS2,      // 二次加压
    STAGE_KEEP,        // 保温保压
    STAGE_COOL,        // 卸压冷却
    STAGE_COMPLETE     // 完成
} ProcessStage;

/* 系统参数结构体 */
typedef struct {
    float target_pressure;    // 目标压力
    float current_pressure;   // 当前压力
    float target_temp;        // 目标温度
    float current_temp;       // 当前温度
    uint32_t target_time;     // 目标时间
    uint32_t current_time;    // 当前时间
    uint8_t check_status;     // 检查状态位图
    ProcessStage stage;       // 工艺阶段
} SystemParams;

/* 电机参数结构体 */
typedef struct {
    uint32_t target_step;     // 目标步数
    uint32_t current_step;    // 当前步数
    uint8_t is_running;       // 运行状态
    uint8_t is_calibrated;    // 校准状态
} MotorParams;

/* 历史记录结构体 */
typedef struct {
    uint32_t timestamp;       // 时间戳
    float pressure;           // 压力值
    float temperature;        // 温度值
    uint32_t duration;        // 持续时间
    uint8_t status;          // 完成状态
} HistoryRecord;

/* 菜单项结构体 */
typedef struct {
    IconType icon;           // 图标类型
    const char *text;        // 显示文本
    void (*action)(void);    // 处理函数
} MenuItem;

/* 工艺参数结构体 */
typedef struct {
    float target_pressure;    // 目标压力
    float target_temp;        // 目标温度
    uint32_t keep_time;       // 保持时间
    uint32_t cool_time;       // 冷却时间
    uint8_t stage_complete;   // 阶段完成标志
} ProcessParams;

/* 函数声明 */
// 菜单处理函数
void menu_main(void);                    // 主菜单
void menu_process(void);                 // 启动流程
void menu_system_check(void);            // 系统检查
void menu_pressure_control(void);         // 加压加热
void menu_temp_keep(void);               // 保温保压
void menu_cool_down(void);               // 卸压冷却
void menu_parameter_setting(void);        // 参数设置
void menu_motor_control(void);           // 电机控制
void menu_calibration(void);             // 调平校准
void menu_history(void);                 // 历史日志
void menu_alarm(void);                   // 报警界面

// 显示函数
void display_menu_page(const MenuItem *items, uint8_t count, uint8_t select);
void display_value_bar(uint8_t row, float value, float max, const char *label);
void display_status_line(uint8_t row, const char *label, const char *value);
void display_progress(uint8_t row, uint8_t progress);
void display_motor_status(uint8_t motor_id, uint32_t step);
void display_check_result(CheckItem item, uint8_t passed);
void display_alarm(const char *msg, uint8_t level);

// 控制函数
uint8_t start_system_check(void);
uint8_t start_pressure_control(float target);
uint8_t start_temperature_control(float target);
uint8_t start_motor_calibration(void);
void save_history_record(void);
void handle_alarm_condition(void);

// 参数操作函数
void set_target_pressure(float pressure);
void set_target_temperature(float temp);
void set_target_time(uint32_t seconds);
void adjust_motor_step(uint8_t motor_id, int32_t delta);

// 状态查询函数
uint8_t is_system_ready(void);
uint8_t is_pressure_stable(void);
uint8_t is_temperature_stable(void);
uint8_t is_motor_calibrated(void);
uint8_t has_alarm(void);

/* OLED显示函数声明 */
void OLED_ShowTitle(const char *title, uint8_t highlight);
void OLED_ShowMenuItem(uint8_t row, const char *text, uint8_t selected);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t mode);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLED_DrawVLine(uint8_t x, uint8_t y, uint8_t h);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_Printf(uint8_t x, uint8_t y, uint8_t font, const char *fmt, ...);
void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t progress);
void OLED_DrawValueBar(uint8_t x, uint8_t y, uint8_t w, float value, float max);
void OLED_ShowStatus(const char *status, uint8_t highlight);
void OLED_SetBrightness(uint8_t brightness);

/* 菜单函数声明 */
void menu1(void);
void menu_start_process(void);
void menu_settings(void);
void menu3_SystemConfig(void);
void menu4_SystemInfo(void);

#endif /* __MENU_H__ */ 