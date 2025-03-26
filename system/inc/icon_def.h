/**
 * @file icon_def.h
 * @brief 图标定义
 */
#ifndef __ICON_DEF_H__
#define __ICON_DEF_H__

/* 图标枚举 */
typedef enum {
    ICON_START = 0x01,    // ⯈ 启动/选择图标
    ICON_CHECK,           // ✔ 检查通过图标
    ICON_HEAT,            // 🔥 加热图标
    ICON_KEEP,            // ♨ 保温图标
    ICON_COOL,            // ❄ 冷却图标
    ICON_SETTING,         // ⚙ 设置图标
    ICON_HISTORY,         // 📜 历史图标
    ICON_WARNING,         // ⚠ 警告图标
    ICON_UP,              // ⬆ 向上图标
    ICON_DOWN,            // ⬇ 向下图标
    ICON_BACK,            // ← 返回图标
    ICON_NEXT            // → 下一步图标
} IconType;

/* 图标字符定义 */
#define ICON_START_CHAR      "⯈"
#define ICON_CHECK_CHAR      "✔"
#define ICON_HEAT_CHAR       "🔥"
#define ICON_KEEP_CHAR       "♨"
#define ICON_COOL_CHAR       "❄"
#define ICON_SETTING_CHAR    "⚙"
#define ICON_HISTORY_CHAR    "📜"
#define ICON_WARNING_CHAR    "⚠"
#define ICON_UP_CHAR         "⬆"
#define ICON_DOWN_CHAR       "⬇"
#define ICON_BACK_CHAR       "←"
#define ICON_NEXT_CHAR       "→"

#endif /* __ICON_DEF_H__ */ 