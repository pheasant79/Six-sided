/**
 * @file menu.c
 * @brief 六面顶压机菜单系统实现
 */

/* 包含头文件 */
#include "main.h"
#include "menu.h"
#include "oled.h"
#include "key.h"
#include "pressure.h"
#include "temperature.h"
#include "motor.h"
#include "process_state.h"
#include "system_params.h"
#include "icon_def.h"
#include "oled_utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* 按键定义 */
#define KEY_UP      1
#define KEY_DOWN    2
#define KEY_LEFT    3
#define KEY_RIGHT   4
#define KEY_ENTER   5
#define KEY_BACK    6

/* 私有类型定义 */
typedef struct {
    uint32_t last_update;
    uint32_t prepare_time;
    ProcessState state;
    float feed_progress[3];
    float press_progress[2];
    uint8_t error_code;
} ProcessContext;

typedef struct {
    CalibrationState state;
    uint32_t step_count[3];  // x, y, z轴步数
    uint8_t is_calibrated;
    uint32_t last_update;
} CalibrationContext;

/* 私有变量定义 */
static uint8_t KeyNum;                    // 当前按键值
static SystemParams g_sys_params = {0};   // 系统参数
static ProcessParams g_process_params = {0}; // 工艺参数
static CalibrationContext g_calibration = {0}; // 校准状态
static ProcessContext g_process = {0};    // 流程状态
static SystemParams sys_params = {0};
static MotorParams motor_params[3] = {0};
static uint8_t current_menu = MENU_MAIN;
static uint8_t menu_select = 0;

/* 主菜单项定义 */
static const MenuItem main_menu_items[] = {
    {ICON_START,   "启动流程", menu_process},
    {ICON_SETTING, "参数设置", menu_parameter_setting},
    {ICON_SETTING, "电机控制", menu_motor_control},
    {ICON_SETTING, "调平校准", menu_calibration},
    {ICON_HISTORY, "历史日志", menu_history}
};

/* 系统检查菜单项 */
static const MenuItem check_menu_items[] = {
    {ICON_CHECK, "油压检查", NULL},
    {ICON_CHECK, "密封检查", NULL},
    {ICON_CHECK, "顶锤对中", NULL},
    {ICON_CHECK, "电控参数", NULL},
    {ICON_NEXT,  "开始运行", NULL}
};

/* 私有函数声明 */
static void display_menu_items(const MenuItem *items, uint8_t count, uint8_t select);
static void process_handle_prepare(ProcessContext *ctx);
static void process_handle_calibration(ProcessContext *ctx);
static void process_handle_feed(ProcessContext *ctx);
static void process_handle_press(ProcessContext *ctx);
static void update_calibration_status(void);
static void display_process_status(const ProcessContext *ctx);
static uint8_t handle_process_key(uint8_t key, ProcessContext *ctx);

/**
 * @brief 主菜单界面
 */
void menu1(void) {
    uint8_t select = 0;
    const uint8_t menu_count = sizeof(main_menu_items) / sizeof(main_menu_items[0]);
    
    while(1) {
        KeyNum = Key_GetNum();
        
        switch(KeyNum) {
            case KEY_UP:
                select = (select > 0) ? (select - 1) : (menu_count - 1);
                break;
            case KEY_DOWN:
                select = (select < (menu_count - 1)) ? (select + 1) : 0;
                break;
            case KEY_ENTER:
                if(main_menu_items[select].handler) {
                    main_menu_items[select].handler();
                }
                break;
            case KEY_BACK:
                return;
        }
        
        display_menu_items(main_menu_items, menu_count, select);
    }
}

/**
 * @brief 显示菜单项
 */
static void display_menu_items(const MenuItem *items, uint8_t count, uint8_t select) {
    OLED_Clear();
    OLED_ShowTitle("主菜单", 0);
    
    uint8_t start_idx = (select / 4) * 4;
    uint8_t end_idx = MIN(start_idx + 4, count);
    
    for(uint8_t i = start_idx; i < end_idx; i++) {
        OLED_ShowMenuItem(i - start_idx, items[i].text, i == select);
    }
    
    OLED_Update();
}

/**
 * @brief 工艺流程界面
 */
void menu_start_process(void) {
    // 初始化流程状态
    g_process.state = PROCESS_PREPARE;
    g_process.prepare_time = 5000;
    g_process.last_update = HAL_GetTick();
    memset(g_process.feed_progress, 0, sizeof(g_process.feed_progress));
    memset(g_process.press_progress, 0, sizeof(g_process.press_progress));
    g_process.error_code = 0;
    
    while(1) {
        KeyNum = Key_GetNum();
        uint32_t current_time = HAL_GetTick();
        
        // 处理按键
        if(handle_process_key(KeyNum, &g_process)) {
            return;
        }
        
        // 状态处理
        switch(g_process.state) {
            case PROCESS_PREPARE:
                process_handle_prepare(&g_process);
                break;
            case PROCESS_CALIBRATE:
                process_handle_calibration(&g_process);
                break;
            case PROCESS_FEED:
                process_handle_feed(&g_process);
                break;
            case PROCESS_PRESS1:
            case PROCESS_PRESS2:
                process_handle_press(&g_process);
                break;
            default:
                break;
        }
        
        // 显示状态
        display_process_status(&g_process);
    }
}

/**
 * @brief 处理准备阶段
 */
static void process_handle_prepare(ProcessContext *ctx) {
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed = current_time - ctx->last_update;
    
    if(elapsed >= ctx->prepare_time) {
        ctx->state = PROCESS_CALIBRATE;
        start_motor_calibration();
    }
}

/**
 * @brief 处理校准阶段
 */
static void process_handle_calibration(ProcessContext *ctx) {
    update_calibration_status();
    
    if(g_calibration.is_calibrated) {
        ctx->state = PROCESS_FEED;
    } else if(g_calibration.state == CALIBRATION_ERROR) {
        ctx->state = PROCESS_ERROR;
        ctx->error_code = ERROR_CALIBRATION_FAILED;
    }
}

/**
 * @brief 处理进料阶段
 */
static void process_handle_feed(ProcessContext *ctx) {
    static uint32_t last_update = 0;
    uint32_t current_time = HAL_GetTick();
    
    if(current_time - last_update >= 100) {
        // 更新进料进度
        for(int i = 0; i < 3; i++) {
            if(ctx->feed_progress[i] < 100.0f) {
                ctx->feed_progress[i] += 1.0f;
                // 控制对应电机
                Motor_Run(i);
            } else {
                Motor_Stop(i);
            }
        }
        
        // 检查是否完成
        if(ctx->feed_progress[0] >= 100.0f &&
           ctx->feed_progress[1] >= 100.0f &&
           ctx->feed_progress[2] >= 100.0f) {
            ctx->state = PROCESS_PRESS1;
            Motor_EmergencyStop(); // 停止所有电机
        }
        
        last_update = current_time;
    }
}

/**
 * @brief 处理压制阶段
 */
static void process_handle_press(ProcessContext *ctx) {
    static uint32_t last_update = 0;
    uint32_t current_time = HAL_GetTick();
    
    if(current_time - last_update >= 100) {
        uint8_t press_idx = (ctx->state == PROCESS_PRESS1) ? 0 : 1;
        
        if(ctx->press_progress[press_idx] < 100.0f) {
            ctx->press_progress[press_idx] += 0.5f;
            
            // 根据进度控制压力
            float target_pressure = (ctx->press_progress[press_idx] / 100.0f) * MAX_PRESSURE;
            // TODO: 实现压力控制逻辑
            
        } else if(ctx->state == PROCESS_PRESS1) {
            ctx->state = PROCESS_PRESS2;
        } else {
            ctx->state = PROCESS_COMPLETE;
        }
        
        last_update = current_time;
    }
}

/**
 * @brief 显示流程状态
 */
static void display_process_status(const ProcessContext *ctx) {
    OLED_Clear();
    
    // 显示标题
    const char *title;
    uint8_t highlight = 0;
    
    switch(ctx->state) {
        case PROCESS_PREPARE:
            title = "准备中...";
            break;
        case PROCESS_CALIBRATE:
            title = "校准中...";
            break;
        case PROCESS_FEED:
            title = "进料中...";
            break;
        case PROCESS_PRESS1:
            title = "一次加压";
            break;
        case PROCESS_PRESS2:
            title = "二次加压";
            break;
        case PROCESS_COMPLETE:
            title = "完成!";
            highlight = 1;
            break;
        case PROCESS_ERROR:
            title = "错误!";
            highlight = 1;
            break;
        default:
            title = "未知状态";
            break;
    }
    
    OLED_ShowTitle(title, highlight);
    
    // 显示进度
    switch(ctx->state) {
        case PROCESS_PREPARE:
            {
                uint32_t remain = (ctx->prepare_time - (HAL_GetTick() - ctx->last_update)) / 1000;
                OLED_Printf(2, 20, OLED_6X8, "启动倒计时: %ds", remain);
            }
            break;
            
        case PROCESS_CALIBRATE:
            OLED_Printf(2, 20, OLED_6X8, "X轴: %d", g_calibration.step_count[0]);
            OLED_Printf(2, 30, OLED_6X8, "Y轴: %d", g_calibration.step_count[1]);
            OLED_Printf(2, 40, OLED_6X8, "Z轴: %d", g_calibration.step_count[2]);
            break;
            
        case PROCESS_FEED:
            OLED_DrawValueBar(2, 20, 124, ctx->feed_progress[0], 100.0f);
            OLED_DrawValueBar(2, 32, 124, ctx->feed_progress[1], 100.0f);
            OLED_DrawValueBar(2, 44, 124, ctx->feed_progress[2], 100.0f);
            break;
            
        case PROCESS_PRESS1:
        case PROCESS_PRESS2:
            {
                uint8_t press_idx = (ctx->state == PROCESS_PRESS1) ? 0 : 1;
                OLED_DrawProgressBar(2, 30, 124, 12, (uint8_t)ctx->press_progress[press_idx]);
                
                // 显示当前压力和温度
                float current_pressure = Get_Pressure();
                float current_temp = Get_Temperature();
                OLED_Printf(2, 45, OLED_6X8, "P:%.1fGPa T:%d°C", 
                          current_pressure, (int)current_temp);
            }
            break;
            
        case PROCESS_ERROR:
            OLED_ShowStatus("发生错误!", 1);
            // 显示错误代码
            OLED_Printf(2, 30, OLED_6X8, "错误代码: 0x%02X", ctx->error_code);
            break;
            
        default:
            break;
    }
    
    OLED_Update();
}

/**
 * @brief 处理流程中的按键
 * @return 1表示退出流程，0表示继续
 */
static uint8_t handle_process_key(uint8_t key, ProcessContext *ctx) {
    if(key == KEY_BACK) {
        if(ctx->state != PROCESS_PREPARE && 
           ctx->state != PROCESS_COMPLETE && 
           ctx->state != PROCESS_ERROR) {
            // 显示确认对话框
            OLED_Clear();
            OLED_ShowTitle("确认", 0);
            OLED_ShowString(2, 20, "停止工艺?", OLED_6X8);
            OLED_ShowString(2, 32, "KEY1: 确认", OLED_6X8);
            OLED_ShowString(2, 44, "KEY2: 取消", OLED_6X8);
            OLED_Update();
            
            while(1) {
                uint8_t confirm_key = Key_GetNum();
                if(confirm_key == KEY_UP) {
                    // 停止所有电机
                    Motor_EmergencyStop();
                    return 1;
                } else if(confirm_key == KEY_DOWN) {
                    return 0;
                }
            }
        }
        return 1;
    }
    return 0;
}

/* 电机校准相关函数 */
void start_motor_calibration(void) {
    g_calibration.state = CALIBRATION_X_AXIS;
    memset(g_calibration.step_count, 0, sizeof(g_calibration.step_count));
    g_calibration.is_calibrated = 0;
    g_calibration.last_update = HAL_GetTick();
    
    // 设置电机方向和速度
    for(uint8_t i = 0; i < 3; i++) {
        Motor_SetDir(i, DIR_CW);
        Motor_SetSpeed(i, 100); // 较低速度进行校准
    }
}

void stop_motor_calibration(void) {
    g_calibration.state = CALIBRATION_ERROR;
    g_calibration.is_calibrated = 0;
    Motor_EmergencyStop();
}

static void update_calibration_status(void) {
    uint32_t current_time = HAL_GetTick();
    
    if(current_time - g_calibration.last_update >= 100) {
        switch(g_calibration.state) {
            case CALIBRATION_X_AXIS:
                if(g_calibration.step_count[0] < 1000) {
                    g_calibration.step_count[0] += 10;
                    Motor_Run(0);
                } else {
                    Motor_Stop(0);
                    g_calibration.state = CALIBRATION_Y_AXIS;
                }
                break;
                
            case CALIBRATION_Y_AXIS:
                if(g_calibration.step_count[1] < 1000) {
                    g_calibration.step_count[1] += 10;
                    Motor_Run(1);
                } else {
                    Motor_Stop(1);
                    g_calibration.state = CALIBRATION_Z_AXIS;
                }
                break;
                
            case CALIBRATION_Z_AXIS:
                if(g_calibration.step_count[2] < 1000) {
                    g_calibration.step_count[2] += 10;
                    Motor_Run(2);
                } else {
                    Motor_Stop(2);
                    g_calibration.state = CALIBRATION_COMPLETE;
                    g_calibration.is_calibrated = 1;
                }
                break;
                
            default:
                break;
        }
        
        g_calibration.last_update = current_time;
    }
}

uint8_t is_motor_calibrated(void) {
    return g_calibration.is_calibrated;
}

/* 系统设置相关函数 */
void toggle_night_mode(void) {
    g_sys_params.mode = (g_sys_params.mode == DISPLAY_MODE_NIGHT) ? 
                        DISPLAY_MODE_NORMAL : DISPLAY_MODE_NIGHT;
    OLED_SetBrightness(g_sys_params.mode == DISPLAY_MODE_NIGHT ? 32 : 255);
}

void update_system_state(void) {
    static uint32_t last_check = 0;
    uint32_t current_time = HAL_GetTick();
    
    if(current_time - last_check >= REFRESH_RATE_MS) {
        // 更新系统状态
        g_sys_params.pressure = Get_Pressure();
        g_sys_params.temperature = Get_Temperature();
        
        // 检查警报条件
        check_alert_conditions();
        
        last_check = current_time;
    }
}

void check_alert_conditions(void) {
    // 检查压力
    if(g_sys_params.pressure > MAX_PRESSURE) {
        g_sys_params.alert = ALERT_CRITICAL;
        g_sys_params.alert_count++;
        // 紧急停止
        Motor_EmergencyStop();
    } else if(g_sys_params.pressure > MAX_PRESSURE * 0.9f) {
        g_sys_params.alert = ALERT_WARNING;
    }
    
    // 检查温度
    if(g_sys_params.temperature > MAX_TEMPERATURE) {
        g_sys_params.alert = ALERT_CRITICAL;
        g_sys_params.alert_count++;
    } else if(g_sys_params.temperature > MAX_TEMPERATURE * 0.9f) {
        g_sys_params.alert = ALERT_WARNING;
    }
}

/**
 * @brief 显示菜单页面
 */
void display_menu_page(const MenuItem *items, uint8_t count, uint8_t select) {
    OLED_Clear();
    
    // 计算显示范围
    uint8_t start = (select / ROWS) * ROWS;
    uint8_t end = (start + ROWS > count) ? count : start + ROWS;
    
    // 显示菜单项
    for(uint8_t i = start; i < end; i++) {
        char line[17] = {0};  // 16字符宽度 + 结束符
        snprintf(line, sizeof(line), "%s %s", 
                items[i].icon,
                items[i].text);
        
        // 选中项显示反白
        OLED_ShowString(0, (i-start)*16, line, 
                       (i == select) ? OLED_HIGHLIGHT : OLED_NORMAL);
    }
    
    // 显示滚动条(如果需要)
    if(count > ROWS) {
        uint8_t bar_height = SCREEN_HEIGHT / count;
        uint8_t bar_pos = (select * SCREEN_HEIGHT) / count;
        OLED_DrawVLine(SCREEN_WIDTH-1, bar_pos, bar_height);
    }
    
    OLED_Update();
}

/**
 * @brief 显示数值进度条
 */
void display_value_bar(uint8_t row, float value, float max, const char *label) {
    char text[17];
    uint8_t progress = (uint8_t)((value * 100) / max);
    uint8_t width = (uint8_t)((SCREEN_WIDTH - 4) * progress / 100);
    
    // 显示标签和数值
    snprintf(text, sizeof(text), "%s: %.1f", label, value);
    OLED_ShowString(0, row*16, text, OLED_NORMAL);
    
    // 绘制进度条
    OLED_DrawRect(2, row*16+12, SCREEN_WIDTH-4, 4, 0);
    OLED_FillRect(2, row*16+12, width, 4, 1);
}

/**
 * @brief 显示状态行
 */
void display_status_line(uint8_t row, const char *label, const char *value) {
    char text[17];
    snprintf(text, sizeof(text), "%s: %s", label, value);
    OLED_ShowString(0, row*16, text, OLED_NORMAL);
}

/**
 * @brief 主菜单处理
 */
void menu_main(void) {
    const uint8_t menu_count = sizeof(main_menu_items)/sizeof(main_menu_items[0]);
    menu_select = 0;
    
    while(1) {
        uint8_t key = Key_GetNum();
        
        switch(key) {
            case KEY_UP:
                if(menu_select > 0) menu_select--;
                break;
                
            case KEY_DOWN:
                if(menu_select < menu_count-1) menu_select++;
                break;
                
            case KEY_ENTER:
                if(main_menu_items[menu_select].handler) {
                    main_menu_items[menu_select].handler();
                }
                break;
                
            case KEY_BACK:
                return;
        }
        
        display_menu_page(main_menu_items, menu_count, menu_select);
    }
}

/**
 * @brief 启动流程处理
 */
void menu_process(void) {
    uint8_t check_complete = 0;
    menu_select = 0;
    
    // 系统检查
    while(!check_complete) {
        uint8_t key = Key_GetNum();
        const uint8_t check_count = sizeof(check_menu_items)/sizeof(check_menu_items[0]);
        
        switch(key) {
            case KEY_UP:
                if(menu_select > 0) menu_select--;
                break;
                
            case KEY_DOWN:
                if(menu_select < check_count-1) menu_select++;
                break;
                
            case KEY_ENTER:
                if(menu_select == check_count-1) {
                    if(is_system_ready()) {
                        check_complete = 1;
                        sys_params.stage = STAGE_PRESS1;
                    }
                } else {
                    // 执行对应检查项
                    display_check_result(menu_select, 1);
                    sys_params.check_status |= (1 << menu_select);
                }
                break;
                
            case KEY_BACK:
                return;
        }
        
        display_menu_page(check_menu_items, check_count, menu_select);
    }
    
    // 开始加压加热流程
    menu_pressure_control();
}

/**
 * @brief 加压加热控制
 */
void menu_pressure_control(void) {
    while(1) {
        uint8_t key = Key_GetNum();
        
        // 更新当前状态
        sys_params.current_pressure = Get_Pressure();
        sys_params.current_temp = Get_Temperature();
        
        // 显示状态
        OLED_Clear();
        
        if(sys_params.stage == STAGE_PRESS1) {
            OLED_ShowTitle("一次加压", 0);
            display_value_bar(1, sys_params.current_pressure, 100, "压力");
            display_value_bar(2, sys_params.current_temp, MAX_TEMPERATURE, "温度");
        } else if(sys_params.stage == STAGE_PRESS2) {
            OLED_ShowTitle("二次加压", 0);
            display_value_bar(1, sys_params.current_pressure, 200, "压力");
            display_value_bar(2, sys_params.current_temp, MAX_TEMPERATURE, "温度");
        }
        
        // 显示操作提示
        OLED_ShowString(0, 48, "按确认继续", OLED_NORMAL);
        OLED_Update();
        
        // 检查是否达到目标
        if(is_pressure_stable() && is_temperature_stable()) {
            if(sys_params.stage == STAGE_PRESS1) {
                sys_params.stage = STAGE_PRESS2;
            } else if(sys_params.stage == STAGE_PRESS2) {
                sys_params.stage = STAGE_KEEP;
                menu_temp_keep();
                return;
            }
        }
        
        // 按键处理
        if(key == KEY_BACK) {
            if(show_confirm_dialog("停止工艺?")) {
                return;
            }
        }
        
        // 检查警报
        if(has_alarm()) {
            menu_alarm();
            return;
        }
    }
}

/**
 * @brief 电机控制界面
 */
void menu_motor_control(void) {
    uint8_t motor_select = 0;
    
    while(1) {
        uint8_t key = Key_GetNum();
        
        OLED_Clear();
        OLED_ShowTitle("电机控制", 0);
        
        // 显示三个电机状态
        for(uint8_t i = 0; i < 3; i++) {
            char text[17];
            snprintf(text, sizeof(text), "电机%c: %d step%s",
                    'A'+i, 
                    motor_params[i].current_step,
                    (i == motor_select) ? " ←" : "");
            OLED_ShowString(0, (i+1)*16, text, OLED_NORMAL);
        }
        
        // 显示操作提示
        OLED_ShowString(0, 48, "↑↓:选择 ←→:调整", OLED_NORMAL);
        OLED_Update();
        
        switch(key) {
            case KEY_UP:
                if(motor_select > 0) motor_select--;
                break;
                
            case KEY_DOWN:
                if(motor_select < 2) motor_select++;
                break;
                
            case KEY_LEFT:
                adjust_motor_step(motor_select, -10);
                break;
                
            case KEY_RIGHT:
                adjust_motor_step(motor_select, 10);
                break;
                
            case KEY_BACK:
                return;
        }
    }
}

/**
 * @brief 显示确认对话框
 * @return 1:确认 0:取消
 */
static uint8_t show_confirm_dialog(const char *msg) {
    OLED_Clear();
    OLED_ShowTitle("确认", 0);
    OLED_ShowString(0, 16, msg, OLED_NORMAL);
    OLED_ShowString(0, 32, "确认:↑  取消:↓", OLED_NORMAL);
    OLED_Update();
    
    while(1) {
        uint8_t key = Key_GetNum();
        if(key == KEY_UP) return 1;
        if(key == KEY_DOWN) return 0;
    }
}

/**
 * @brief 调整电机步数
 */
void adjust_motor_step(uint8_t motor_id, int32_t delta) {
    if(motor_id >= 3) return;
    
    int32_t new_step = motor_params[motor_id].current_step + delta;
    if(new_step < 0) new_step = 0;
    if(new_step > MAX_STEP) new_step = MAX_STEP;
    
    if(new_step != motor_params[motor_id].current_step) {
        motor_params[motor_id].target_step = new_step;
        if(delta > 0) {
            Motor_SetDir(motor_id, DIR_CW);
        } else {
            Motor_SetDir(motor_id, DIR_CCW);
        }
        Motor_Run(motor_id);
    }
}

/**
 * @brief 检查系统是否就绪
 */
uint8_t is_system_ready(void) {
    // 检查所有检查项是否完成
    return (sys_params.check_status == 0x0F);
}

/**
 * @brief 检查压力是否稳定
 */
uint8_t is_pressure_stable(void) {
    static float last_pressure = 0;
    static uint8_t stable_count = 0;
    
    float diff = sys_params.current_pressure - last_pressure;
    last_pressure = sys_params.current_pressure;
    
    if(fabs(diff) < 0.1f) {
        stable_count++;
        return (stable_count >= 10);
    } else {
        stable_count = 0;
        return 0;
    }
}

/**
 * @brief 检查温度是否稳定
 */
uint8_t is_temperature_stable(void) {
    static float last_temp = 0;
    static uint8_t stable_count = 0;
    
    float diff = sys_params.current_temp - last_temp;
    last_temp = sys_params.current_temp;
    
    if(fabs(diff) < 1.0f) {
        stable_count++;
        return (stable_count >= 10);
    } else {
        stable_count = 0;
        return 0;
    }
}

/**
 * @brief 检查是否有警报
 */
uint8_t has_alarm(void) {
    if(sys_params.current_pressure > MAX_PRESSURE ||
       sys_params.current_temp > MAX_TEMPERATURE) {
        return 1;
    }
    return 0;
}

/**
 * @brief 警报界面
 */
void menu_alarm(void) {
    OLED_Clear();
    OLED_ShowTitle(ICON_WARNING " 警报", 1);
    
    if(sys_params.current_pressure > MAX_PRESSURE) {
        OLED_ShowString(0, 16, "压力超限!", OLED_HIGHLIGHT);
        char text[17];
        snprintf(text, sizeof(text), "当前:%.1fkPa", sys_params.current_pressure);
        OLED_ShowString(0, 32, text, OLED_NORMAL);
    }
    
    if(sys_params.current_temp > MAX_TEMPERATURE) {
        OLED_ShowString(0, 16, "温度超限!", OLED_HIGHLIGHT);
        char text[17];
        snprintf(text, sizeof(text), "当前:%d°C", (int)sys_params.current_temp);
        OLED_ShowString(0, 32, text, OLED_NORMAL);
    }
    
    OLED_ShowString(0, 48, "按任意键返回", OLED_NORMAL);
    OLED_Update();
    
    // 等待按键
    while(!Key_GetNum());
} 