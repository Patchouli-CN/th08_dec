#pragma once

#include "inttypes.hpp"

#define COLOR_BLACK 0xff000000
#define COLOR_DARK_GREY 0xff404040  /* 菜单/未选中项的暗灰 */
#define COLOR_MEDIUM_GREY 0xff808080
#define COLOR_LIGHT_GREY 0xffa0a0a0
#define COLOR_WHITE 0xffffffff
#define COLOR_RED 0xffff4040       /* 失败/死亡提示红 */
#define COLOR_SOFT_RED 0xffd06060  /* 较柔和的红（标题界面选中项） */
#define COLOR_LIGHT_YELLOW 0xffffffd0
#define COLOR_YELLOW 0xffffff00
#define COLOR_SCORE_POPUP 0xffffc0a0 /* 得分弹字颜色（浅橙） */

#define COLOR_TEXT_WHITE 0xffffff
#define COLOR_TIME_ORBS_LIMIT 0xfffff0c0  /* 时之环达到临界值时的显示颜色（浅黄） */
#define COLOR_POWER_BAR_MAIN 0xe0e0e0ff   /* 灵力条主体颜色（ARGB） */
#define COLOR_POWER_BAR_EDGE 0x80e0e0ff   /* 灵力条边缘颜色（ARGB） */

#define COLOR_RGB_MASK 0x00FFFFFF
#define COLOR_ALPHA_MASK 0xFF000000
#define COLOR_RGB(color) ((color) & COLOR_RGB_MASK)
#define COLOR_ALPHA(color) (((color) & COLOR_ALPHA_MASK) >> 24)
#define COLOR_SET_ALPHA(color, alpha) (((alpha) << 24) | COLOR_RGB(color))
#define COLOR_SET_ALPHA2(color, alpha) (COLOR_RGB(color) | (((alpha) & 0xff) << 24))
#define COLOR_SET_ALPHA3(color, alpha) (COLOR_RGB(color) | ((alpha) << 24))
#define COLOR_COMBINE_ALPHA(color, alpha) (((alpha) & COLOR_ALPHA_MASK) | COLOR_RGB(color))

union ZunColor {
    u32 d3dColor;
    struct
    {
        u8 b;
        u8 g;
        u8 r;
        u8 a;
    };
};
