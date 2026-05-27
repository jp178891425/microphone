#include "ui_page_Dsp.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "lvgl.h"

/*################################### 本地变量 ###################################*/
static DspObjsContext *ObjsCt = NULL;
static DspArgsContext ArgsCt;
static uint8_t args_initialized = 0;

// Panel对象数组，用于编码器控制，24个Panel
static lv_obj_t *panel_array[24];
static lv_obj_t *label_array[24];

// Slider对象数组 (3个slider: REV, VOL, Delay)
static lv_obj_t *slider_panel_array[3];  // slider的容器panel
static lv_obj_t *slider_array[3];        // slider控件

/*################################### 本地函数声明 ###################################*/

// 更新slider上方的数值显示
static void update_slider_value_labels(void)
{
    char buf[8];
    
    // 更新REV值显示
    sprintf(buf, "%d", ArgsCt.dsp_rev[ArgsCt.current_dsp]);
    lv_label_set_text(ObjsCt->ui_label21, buf);
    
    // 更新VOL值显示
    sprintf(buf, "%d", ArgsCt.dsp_vol[ArgsCt.current_dsp]);
    lv_label_set_text(ObjsCt->ui_label22, buf);
    
    // 更新Delay值显示（转换为毫秒，0-20 对应 0-2000ms）
    sprintf(buf, "%dms", ArgsCt.dsp_delay[ArgsCt.current_dsp] * 100);
    lv_label_set_text(ObjsCt->ui_label18, buf);
}

/*################################### 全局函数 ###################################*/

void ui_page_Dsp_init(lv_obj_t *screen)
{
    // 内存分配
    {
        ObjsCt = (DspObjsContext *)lv_mem_alloc(sizeof(DspObjsContext));
        memset(ObjsCt, 0, sizeof(DspObjsContext));
        if(!args_initialized)
        {
            uint8_t i;
            memset(&ArgsCt, 0, sizeof(DspArgsContext));
            ArgsCt.focus_target = 0;
            ArgsCt.edit_mode = 0;
            ArgsCt.current_dsp = 0;
            
            // 初始化所有DSP的参数为10（中间值）
            for(i = 0; i < 24; i++) {
                ArgsCt.dsp_rev[i] = 10;
                ArgsCt.dsp_vol[i] = 10;
                ArgsCt.dsp_delay[i] = 10;
            }
            args_initialized = 1;
        }
    }

    ObjsCt->Dsp_obj = lv_obj_create(screen);
    lv_obj_set_size(ObjsCt->Dsp_obj, 480, 320);
    lv_obj_set_style_bg_color(ObjsCt->Dsp_obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->Dsp_obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ObjsCt->Dsp_obj, LV_OBJ_FLAG_SCROLLABLE);

    // Panel9 (0, 1)
    ObjsCt->ui_panel9 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel9, 183);
    lv_obj_set_height(ObjsCt->ui_panel9, 19);
    lv_obj_set_x(ObjsCt->ui_panel9, 0);
    lv_obj_set_y(ObjsCt->ui_panel9, 1);
    lv_obj_clear_flag(ObjsCt->ui_panel9, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label23 = lv_label_create(ObjsCt->ui_panel9);
    lv_obj_set_width(ObjsCt->ui_label23, 183);
    lv_obj_set_height(ObjsCt->ui_label23, 19);
    lv_obj_set_x(ObjsCt->ui_label23, 0);
    lv_obj_set_y(ObjsCt->ui_label23, -2);
    lv_obj_set_align(ObjsCt->ui_label23, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label23, "  1:REV HALL1");
    lv_obj_set_style_text_color(ObjsCt->ui_label23, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label23, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label23, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label23, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel10 (0, 21)
    ObjsCt->ui_panel10 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel10, 183);
    lv_obj_set_height(ObjsCt->ui_panel10, 19);
    lv_obj_set_x(ObjsCt->ui_panel10, 0);
    lv_obj_set_y(ObjsCt->ui_panel10, 21);
    lv_obj_clear_flag(ObjsCt->ui_panel10, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label24 = lv_label_create(ObjsCt->ui_panel10);
    lv_obj_set_width(ObjsCt->ui_label24, 183);
    lv_obj_set_height(ObjsCt->ui_label24, 19);
    lv_obj_set_x(ObjsCt->ui_label24, 0);
    lv_obj_set_y(ObjsCt->ui_label24, -2);
    lv_obj_set_align(ObjsCt->ui_label24, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label24, "  2:REV HALL2");
    lv_obj_set_style_text_color(ObjsCt->ui_label24, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label24, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label24, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label24, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel11 (0, 41)
    ObjsCt->ui_panel11 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel11, 183);
    lv_obj_set_height(ObjsCt->ui_panel11, 19);
    lv_obj_set_x(ObjsCt->ui_panel11, 0);
    lv_obj_set_y(ObjsCt->ui_panel11, 41);
    lv_obj_clear_flag(ObjsCt->ui_panel11, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label25 = lv_label_create(ObjsCt->ui_panel11);
    lv_obj_set_width(ObjsCt->ui_label25, 183);
    lv_obj_set_height(ObjsCt->ui_label25, 19);
    lv_obj_set_x(ObjsCt->ui_label25, 0);
    lv_obj_set_y(ObjsCt->ui_label25, -2);
    lv_obj_set_align(ObjsCt->ui_label25, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label25, "  3:REV ROOM1");
    lv_obj_set_style_text_color(ObjsCt->ui_label25, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label25, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label25, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label25, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel12 (0, 61)
    ObjsCt->ui_panel12 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel12, 183);
    lv_obj_set_height(ObjsCt->ui_panel12, 19);
    lv_obj_set_x(ObjsCt->ui_panel12, 0);
    lv_obj_set_y(ObjsCt->ui_panel12, 61);
    lv_obj_clear_flag(ObjsCt->ui_panel12, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label26 = lv_label_create(ObjsCt->ui_panel12);
    lv_obj_set_width(ObjsCt->ui_label26, 183);
    lv_obj_set_height(ObjsCt->ui_label26, 19);
    lv_obj_set_x(ObjsCt->ui_label26, 0);
    lv_obj_set_y(ObjsCt->ui_label26, -2);
    lv_obj_set_align(ObjsCt->ui_label26, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label26, "  4:REV ROOM2");
    lv_obj_set_style_text_color(ObjsCt->ui_label26, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label26, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label26, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label26, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel13 (0, 81)
    ObjsCt->ui_panel13 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel13, 183);
    lv_obj_set_height(ObjsCt->ui_panel13, 19);
    lv_obj_set_x(ObjsCt->ui_panel13, 0);
    lv_obj_set_y(ObjsCt->ui_panel13, 81);
    lv_obj_clear_flag(ObjsCt->ui_panel13, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label27 = lv_label_create(ObjsCt->ui_panel13);
    lv_obj_set_width(ObjsCt->ui_label27, 183);
    lv_obj_set_height(ObjsCt->ui_label27, 19);
    lv_obj_set_x(ObjsCt->ui_label27, 0);
    lv_obj_set_y(ObjsCt->ui_label27, -2);
    lv_obj_set_align(ObjsCt->ui_label27, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label27, "  5:REV STAGE1");
    lv_obj_set_style_text_color(ObjsCt->ui_label27, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label27, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label27, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label27, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel14 (0, 101)
    ObjsCt->ui_panel14 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel14, 183);
    lv_obj_set_height(ObjsCt->ui_panel14, 19);
    lv_obj_set_x(ObjsCt->ui_panel14, 0);
    lv_obj_set_y(ObjsCt->ui_panel14, 101);
    lv_obj_clear_flag(ObjsCt->ui_panel14, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label28 = lv_label_create(ObjsCt->ui_panel14);
    lv_obj_set_width(ObjsCt->ui_label28, 183);
    lv_obj_set_height(ObjsCt->ui_label28, 19);
    lv_obj_set_x(ObjsCt->ui_label28, 0);
    lv_obj_set_y(ObjsCt->ui_label28, -2);
    lv_obj_set_align(ObjsCt->ui_label28, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label28, "  6:REV STAGE2");
    lv_obj_set_style_text_color(ObjsCt->ui_label28, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label28, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label28, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label28, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel15 (0, 121)
    ObjsCt->ui_panel15 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel15, 183);
    lv_obj_set_height(ObjsCt->ui_panel15, 19);
    lv_obj_set_x(ObjsCt->ui_panel15, 0);
    lv_obj_set_y(ObjsCt->ui_panel15, 121);
    lv_obj_clear_flag(ObjsCt->ui_panel15, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label29 = lv_label_create(ObjsCt->ui_panel15);
    lv_obj_set_width(ObjsCt->ui_label29, 183);
    lv_obj_set_height(ObjsCt->ui_label29, 19);
    lv_obj_set_x(ObjsCt->ui_label29, 0);
    lv_obj_set_y(ObjsCt->ui_label29, -2);
    lv_obj_set_align(ObjsCt->ui_label29, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label29, "  7:REV PLATE");
    lv_obj_set_style_text_color(ObjsCt->ui_label29, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label29, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label29, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label29, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel16 (0, 141)
    ObjsCt->ui_panel16 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel16, 183);
    lv_obj_set_height(ObjsCt->ui_panel16, 19);
    lv_obj_set_x(ObjsCt->ui_panel16, 0);
    lv_obj_set_y(ObjsCt->ui_panel16, 141);
    lv_obj_clear_flag(ObjsCt->ui_panel16, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label30 = lv_label_create(ObjsCt->ui_panel16);
    lv_obj_set_width(ObjsCt->ui_label30, 183);
    lv_obj_set_height(ObjsCt->ui_label30, 19);
    lv_obj_set_x(ObjsCt->ui_label30, 0);
    lv_obj_set_y(ObjsCt->ui_label30, -2);
    lv_obj_set_align(ObjsCt->ui_label30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label30, "  8:DRUM AMB");
    lv_obj_set_style_text_color(ObjsCt->ui_label30, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label30, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label30, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label30, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel17 (0, 161)
    ObjsCt->ui_panel17 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel17, 183);
    lv_obj_set_height(ObjsCt->ui_panel17, 19);
    lv_obj_set_x(ObjsCt->ui_panel17, 0);
    lv_obj_set_y(ObjsCt->ui_panel17, 161);
    lv_obj_clear_flag(ObjsCt->ui_panel17, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label31 = lv_label_create(ObjsCt->ui_panel17);
    lv_obj_set_width(ObjsCt->ui_label31, 183);
    lv_obj_set_height(ObjsCt->ui_label31, 19);
    lv_obj_set_x(ObjsCt->ui_label31, 0);
    lv_obj_set_y(ObjsCt->ui_label31, -2);
    lv_obj_set_align(ObjsCt->ui_label31, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label31, "  9:EARLY REF");
    lv_obj_set_style_text_color(ObjsCt->ui_label31, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label31, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label31, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label31, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel18 (0, 181)
    ObjsCt->ui_panel18 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel18, 183);
    lv_obj_set_height(ObjsCt->ui_panel18, 19);
    lv_obj_set_x(ObjsCt->ui_panel18, 0);
    lv_obj_set_y(ObjsCt->ui_panel18, 181);
    lv_obj_clear_flag(ObjsCt->ui_panel18, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label32 = lv_label_create(ObjsCt->ui_panel18);
    lv_obj_set_width(ObjsCt->ui_label32, 183);
    lv_obj_set_height(ObjsCt->ui_label32, 19);
    lv_obj_set_x(ObjsCt->ui_label32, 0);
    lv_obj_set_y(ObjsCt->ui_label32, -2);
    lv_obj_set_align(ObjsCt->ui_label32, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label32, "  10:GATE REVERB");
    lv_obj_set_style_text_color(ObjsCt->ui_label32, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label32, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label32, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label32, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel19 (0, 201)
    ObjsCt->ui_panel19 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel19, 183);
    lv_obj_set_height(ObjsCt->ui_panel19, 19);
    lv_obj_set_x(ObjsCt->ui_panel19, 0);
    lv_obj_set_y(ObjsCt->ui_panel19, 201);
    lv_obj_clear_flag(ObjsCt->ui_panel19, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label33 = lv_label_create(ObjsCt->ui_panel19);
    lv_obj_set_width(ObjsCt->ui_label33, 183);
    lv_obj_set_height(ObjsCt->ui_label33, 19);
    lv_obj_set_x(ObjsCt->ui_label33, 0);
    lv_obj_set_y(ObjsCt->ui_label33, -2);
    lv_obj_set_align(ObjsCt->ui_label33, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label33, "  11:SINGLE DELAY");
    lv_obj_set_style_text_color(ObjsCt->ui_label33, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label33, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label33, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label33, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel34 (0, 221)
    ObjsCt->ui_panel34 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel34, 183);
    lv_obj_set_height(ObjsCt->ui_panel34, 19);
    lv_obj_set_x(ObjsCt->ui_panel34, 0);
    lv_obj_set_y(ObjsCt->ui_panel34, 221);
    lv_obj_clear_flag(ObjsCt->ui_panel34, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label49 = lv_label_create(ObjsCt->ui_panel34);
    lv_obj_set_width(ObjsCt->ui_label49, 183);
    lv_obj_set_height(ObjsCt->ui_label49, 19);
    lv_obj_set_x(ObjsCt->ui_label49, 0);
    lv_obj_set_y(ObjsCt->ui_label49, -2);
    lv_obj_set_align(ObjsCt->ui_label49, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label49, "  12:DELAY");
    lv_obj_set_style_text_color(ObjsCt->ui_label49, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label49, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label49, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label49, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel20 (190, 1)
    ObjsCt->ui_panel20 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel20, 183);
    lv_obj_set_height(ObjsCt->ui_panel20, 19);
    lv_obj_set_x(ObjsCt->ui_panel20, 190);
    lv_obj_set_y(ObjsCt->ui_panel20, 1);
    lv_obj_clear_flag(ObjsCt->ui_panel20, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label34 = lv_label_create(ObjsCt->ui_panel20);
    lv_obj_set_width(ObjsCt->ui_label34, 183);
    lv_obj_set_height(ObjsCt->ui_label34, 19);
    lv_obj_set_x(ObjsCt->ui_label34, 0);
    lv_obj_set_y(ObjsCt->ui_label34, -2);
    lv_obj_set_align(ObjsCt->ui_label34, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label34, "  13:VOCAL ECHO");
    lv_obj_set_style_text_color(ObjsCt->ui_label34, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label34, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label34, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label34, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel21 (190, 21)
    ObjsCt->ui_panel21 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel21, 183);
    lv_obj_set_height(ObjsCt->ui_panel21, 19);
    lv_obj_set_x(ObjsCt->ui_panel21, 190);
    lv_obj_set_y(ObjsCt->ui_panel21, 21);
    lv_obj_clear_flag(ObjsCt->ui_panel21, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label39 = lv_label_create(ObjsCt->ui_panel21);
    lv_obj_set_width(ObjsCt->ui_label39, 183);
    lv_obj_set_height(ObjsCt->ui_label39, 19);
    lv_obj_set_x(ObjsCt->ui_label39, 0);
    lv_obj_set_y(ObjsCt->ui_label39, -2);
    lv_obj_set_align(ObjsCt->ui_label39, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label39, "  14:KTV");
    lv_obj_set_style_text_color(ObjsCt->ui_label39, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label39, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label39, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label39, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel22 (190, 41)
    ObjsCt->ui_panel22 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel22, 183);
    lv_obj_set_height(ObjsCt->ui_panel22, 19);
    lv_obj_set_x(ObjsCt->ui_panel22, 190);
    lv_obj_set_y(ObjsCt->ui_panel22, 41);
    lv_obj_clear_flag(ObjsCt->ui_panel22, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label40 = lv_label_create(ObjsCt->ui_panel22);
    lv_obj_set_width(ObjsCt->ui_label40, 183);
    lv_obj_set_height(ObjsCt->ui_label40, 19);
    lv_obj_set_x(ObjsCt->ui_label40, 0);
    lv_obj_set_y(ObjsCt->ui_label40, -2);
    lv_obj_set_align(ObjsCt->ui_label40, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label40, "  15:PHASER");
    lv_obj_set_style_text_color(ObjsCt->ui_label40, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label40, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label40, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label40, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel23 (190, 61)
    ObjsCt->ui_panel23 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel23, 183);
    lv_obj_set_height(ObjsCt->ui_panel23, 19);
    lv_obj_set_x(ObjsCt->ui_panel23, 190);
    lv_obj_set_y(ObjsCt->ui_panel23, 61);
    lv_obj_clear_flag(ObjsCt->ui_panel23, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label41 = lv_label_create(ObjsCt->ui_panel23);
    lv_obj_set_width(ObjsCt->ui_label41, 183);
    lv_obj_set_height(ObjsCt->ui_label41, 19);
    lv_obj_set_x(ObjsCt->ui_label41, 0);
    lv_obj_set_y(ObjsCt->ui_label41, -2);
    lv_obj_set_align(ObjsCt->ui_label41, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label41, "  16:FLANGER");
    lv_obj_set_style_text_color(ObjsCt->ui_label41, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label41, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label41, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label41, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel24 (190, 81)
    ObjsCt->ui_panel24 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel24, 183);
    lv_obj_set_height(ObjsCt->ui_panel24, 19);
    lv_obj_set_x(ObjsCt->ui_panel24, 190);
    lv_obj_set_y(ObjsCt->ui_panel24, 81);
    lv_obj_clear_flag(ObjsCt->ui_panel24, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label42 = lv_label_create(ObjsCt->ui_panel24);
    lv_obj_set_width(ObjsCt->ui_label42, 183);
    lv_obj_set_height(ObjsCt->ui_label42, 19);
    lv_obj_set_x(ObjsCt->ui_label42, 0);
    lv_obj_set_y(ObjsCt->ui_label42, -2);
    lv_obj_set_align(ObjsCt->ui_label42, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label42, "  17:CHORUS1");
    lv_obj_set_style_text_color(ObjsCt->ui_label42, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label42, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label42, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label42, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel25 (190, 101)
    ObjsCt->ui_panel25 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel25, 183);
    lv_obj_set_height(ObjsCt->ui_panel25, 19);
    lv_obj_set_x(ObjsCt->ui_panel25, 190);
    lv_obj_set_y(ObjsCt->ui_panel25, 101);
    lv_obj_clear_flag(ObjsCt->ui_panel25, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label43 = lv_label_create(ObjsCt->ui_panel25);
    lv_obj_set_width(ObjsCt->ui_label43, 183);
    lv_obj_set_height(ObjsCt->ui_label43, 19);
    lv_obj_set_x(ObjsCt->ui_label43, 0);
    lv_obj_set_y(ObjsCt->ui_label43, -2);
    lv_obj_set_align(ObjsCt->ui_label43, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label43, "  18:CHORUS2");
    lv_obj_set_style_text_color(ObjsCt->ui_label43, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label43, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label43, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label43, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel26 (190, 121)
    ObjsCt->ui_panel26 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel26, 183);
    lv_obj_set_height(ObjsCt->ui_panel26, 19);
    lv_obj_set_x(ObjsCt->ui_panel26, 190);
    lv_obj_set_y(ObjsCt->ui_panel26, 121);
    lv_obj_clear_flag(ObjsCt->ui_panel26, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label44 = lv_label_create(ObjsCt->ui_panel26);
    lv_obj_set_width(ObjsCt->ui_label44, 183);
    lv_obj_set_height(ObjsCt->ui_label44, 19);
    lv_obj_set_x(ObjsCt->ui_label44, 0);
    lv_obj_set_y(ObjsCt->ui_label44, -2);
    lv_obj_set_align(ObjsCt->ui_label44, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label44, "  19:PINGPONG DLY");
    lv_obj_set_style_text_color(ObjsCt->ui_label44, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label44, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label44, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label44, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel27 (190, 141)
    ObjsCt->ui_panel27 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel27, 183);
    lv_obj_set_height(ObjsCt->ui_panel27, 19);
    lv_obj_set_x(ObjsCt->ui_panel27, 190);
    lv_obj_set_y(ObjsCt->ui_panel27, 141);
    lv_obj_clear_flag(ObjsCt->ui_panel27, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label45 = lv_label_create(ObjsCt->ui_panel27);
    lv_obj_set_width(ObjsCt->ui_label45, 183);
    lv_obj_set_height(ObjsCt->ui_label45, 19);
    lv_obj_set_x(ObjsCt->ui_label45, 0);
    lv_obj_set_y(ObjsCt->ui_label45, -2);
    lv_obj_set_align(ObjsCt->ui_label45, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label45, "  20:TREMOLO");
    lv_obj_set_style_text_color(ObjsCt->ui_label45, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label45, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label45, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label45, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel30 (190, 161)
    ObjsCt->ui_panel30 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel30, 183);
    lv_obj_set_height(ObjsCt->ui_panel30, 19);
    lv_obj_set_x(ObjsCt->ui_panel30, 190);
    lv_obj_set_y(ObjsCt->ui_panel30, 161);
    lv_obj_clear_flag(ObjsCt->ui_panel30, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label46 = lv_label_create(ObjsCt->ui_panel30);
    lv_obj_set_width(ObjsCt->ui_label46, 183);
    lv_obj_set_height(ObjsCt->ui_label46, 19);
    lv_obj_set_x(ObjsCt->ui_label46, 0);
    lv_obj_set_y(ObjsCt->ui_label46, -2);
    lv_obj_set_align(ObjsCt->ui_label46, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label46, "  21:AUTO WAH");
    lv_obj_set_style_text_color(ObjsCt->ui_label46, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label46, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label46, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label46, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel31 (190, 181)
    ObjsCt->ui_panel31 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel31, 183);
    lv_obj_set_height(ObjsCt->ui_panel31, 19);
    lv_obj_set_x(ObjsCt->ui_panel31, 190);
    lv_obj_set_y(ObjsCt->ui_panel31, 181);
    lv_obj_clear_flag(ObjsCt->ui_panel31, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label47 = lv_label_create(ObjsCt->ui_panel31);
    lv_obj_set_width(ObjsCt->ui_label47, 183);
    lv_obj_set_height(ObjsCt->ui_label47, 19);
    lv_obj_set_x(ObjsCt->ui_label47, 0);
    lv_obj_set_y(ObjsCt->ui_label47, -2);
    lv_obj_set_align(ObjsCt->ui_label47, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label47, "  22:RADIO VOICE");
    lv_obj_set_style_text_color(ObjsCt->ui_label47, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label47, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label47, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label47, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel32 (190, 201)
    ObjsCt->ui_panel32 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel32, 183);
    lv_obj_set_height(ObjsCt->ui_panel32, 19);
    lv_obj_set_x(ObjsCt->ui_panel32, 190);
    lv_obj_set_y(ObjsCt->ui_panel32, 201);
    lv_obj_clear_flag(ObjsCt->ui_panel32, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label48 = lv_label_create(ObjsCt->ui_panel32);
    lv_obj_set_width(ObjsCt->ui_label48, 183);
    lv_obj_set_height(ObjsCt->ui_label48, 19);
    lv_obj_set_x(ObjsCt->ui_label48, 0);
    lv_obj_set_y(ObjsCt->ui_label48, -2);
    lv_obj_set_align(ObjsCt->ui_label48, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label48, "  23:DISTORTION");
    lv_obj_set_style_text_color(ObjsCt->ui_label48, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label48, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label48, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label48, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel35 (190, 221)
    ObjsCt->ui_panel35 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel35, 183);
    lv_obj_set_height(ObjsCt->ui_panel35, 19);
    lv_obj_set_x(ObjsCt->ui_panel35, 190);
    lv_obj_set_y(ObjsCt->ui_panel35, 221);
    lv_obj_clear_flag(ObjsCt->ui_panel35, LV_OBJ_FLAG_SCROLLABLE);

    ObjsCt->ui_label50 = lv_label_create(ObjsCt->ui_panel35);
    lv_obj_set_width(ObjsCt->ui_label50, 183);
    lv_obj_set_height(ObjsCt->ui_label50, 19);
    lv_obj_set_x(ObjsCt->ui_label50, 0);
    lv_obj_set_y(ObjsCt->ui_label50, -2);
    lv_obj_set_align(ObjsCt->ui_label50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label50, "  24:PITCH CHANGE");
    lv_obj_set_style_text_color(ObjsCt->ui_label50, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label50, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label50, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label50, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel33 - Slider容器 (3, 250)
    ObjsCt->ui_panel33 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel33, 371);
    lv_obj_set_height(ObjsCt->ui_panel33, 63);
    lv_obj_set_x(ObjsCt->ui_panel33, 3);
    lv_obj_set_y(ObjsCt->ui_panel33, 250);
    lv_obj_clear_flag(ObjsCt->ui_panel33, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_panel33, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_panel33, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_panel33, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_panel33, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_slider3 = lv_slider_create(ObjsCt->ui_panel33);
    lv_slider_set_value(ObjsCt->ui_slider3, 0, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_slider3) == LV_SLIDER_MODE_RANGE) 
        lv_slider_set_left_value(ObjsCt->ui_slider3, 0, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_slider3, 327);
    lv_obj_set_height(ObjsCt->ui_slider3, 10);
    lv_obj_set_x(ObjsCt->ui_slider3, 1);
    lv_obj_set_y(ObjsCt->ui_slider3, -2);
    lv_obj_set_align(ObjsCt->ui_slider3, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_slider3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_slider3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_slider3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ObjsCt->ui_slider3, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_slider3, lv_color_hex(0xB6EE0F), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_slider3, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ObjsCt->ui_slider3, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_slider3, lv_color_hex(0xF71313), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_slider3, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_slider3, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_slider3, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_slider3, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_slider3, 5, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_label17 = lv_label_create(ObjsCt->ui_panel33);
    lv_obj_set_width(ObjsCt->ui_label17, 43);
    lv_obj_set_height(ObjsCt->ui_label17, 20);
    lv_obj_set_x(ObjsCt->ui_label17, -163);
    lv_obj_set_y(ObjsCt->ui_label17, 18);
    lv_obj_set_align(ObjsCt->ui_label17, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label17, "0");
    lv_obj_set_style_text_color(ObjsCt->ui_label17, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label17, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label17, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label17, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_label18 = lv_label_create(ObjsCt->ui_panel33);
    lv_obj_set_width(ObjsCt->ui_label18, 70);
    lv_obj_set_height(ObjsCt->ui_label18, 20);
    lv_obj_set_x(ObjsCt->ui_label18, 147);
    lv_obj_set_y(ObjsCt->ui_label18, 18);
    lv_obj_set_align(ObjsCt->ui_label18, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label18, "2000ms");
    lv_obj_set_style_text_color(ObjsCt->ui_label18, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label18, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label18, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label18, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel36 - REV Slider容器 (165, -4)
    ObjsCt->ui_panel36 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel36, 40);
    lv_obj_set_height(ObjsCt->ui_panel36, 310);
    lv_obj_set_x(ObjsCt->ui_panel36, 165);
    lv_obj_set_y(ObjsCt->ui_panel36, -4);
    lv_obj_set_align(ObjsCt->ui_panel36, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_panel36, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_panel36, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_panel36, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_panel36, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_panel36, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_slider1 = lv_slider_create(ObjsCt->ui_panel36);
    lv_slider_set_value(ObjsCt->ui_slider1, 0, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_slider1) == LV_SLIDER_MODE_RANGE) 
        lv_slider_set_left_value(ObjsCt->ui_slider1, 0, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_slider1, 10);
    lv_obj_set_height(ObjsCt->ui_slider1, 220);
    lv_obj_set_align(ObjsCt->ui_slider1, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_slider1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_slider1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_slider1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ObjsCt->ui_slider1, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_slider1, lv_color_hex(0xB6EE0F), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_slider1, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ObjsCt->ui_slider1, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_slider1, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_slider1, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_slider1, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_slider1, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_label19 = lv_label_create(ObjsCt->ui_panel36);
    lv_obj_set_width(ObjsCt->ui_label19, 40);
    lv_obj_set_height(ObjsCt->ui_label19, 20);
    lv_obj_set_x(ObjsCt->ui_label19, 0);
    lv_obj_set_y(ObjsCt->ui_label19, 137);
    lv_obj_set_align(ObjsCt->ui_label19, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label19, "REV");
    lv_obj_set_style_text_color(ObjsCt->ui_label19, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label19, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label19, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label19, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_label21 = lv_label_create(ObjsCt->ui_panel36);
    lv_obj_set_width(ObjsCt->ui_label21, 40);
    lv_obj_set_height(ObjsCt->ui_label21, 20);
    lv_obj_set_x(ObjsCt->ui_label21, 1);
    lv_obj_set_y(ObjsCt->ui_label21, -129);
    lv_obj_set_align(ObjsCt->ui_label21, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label21, "20");
    lv_obj_set_style_text_color(ObjsCt->ui_label21, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label21, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label21, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label21, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Panel37 - VOL Slider容器 (213, -4)
    ObjsCt->ui_panel37 = lv_obj_create(ObjsCt->Dsp_obj);
    lv_obj_set_width(ObjsCt->ui_panel37, 40);
    lv_obj_set_height(ObjsCt->ui_panel37, 310);
    lv_obj_set_x(ObjsCt->ui_panel37, 213);
    lv_obj_set_y(ObjsCt->ui_panel37, -4);
    lv_obj_set_align(ObjsCt->ui_panel37, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_panel37, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_panel37, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_panel37, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_panel37, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_panel37, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_slider2 = lv_slider_create(ObjsCt->ui_panel37);
    lv_slider_set_value(ObjsCt->ui_slider2, 0, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_slider2) == LV_SLIDER_MODE_RANGE) 
        lv_slider_set_left_value(ObjsCt->ui_slider2, 0, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_slider2, 10);
    lv_obj_set_height(ObjsCt->ui_slider2, 220);
    lv_obj_set_align(ObjsCt->ui_slider2, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_slider2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_slider2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_slider2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ObjsCt->ui_slider2, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_slider2, lv_color_hex(0xB6EE0F), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_slider2, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ObjsCt->ui_slider2, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_slider2, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_slider2, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_slider2, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_slider2, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_label20 = lv_label_create(ObjsCt->ui_panel37);
    lv_obj_set_width(ObjsCt->ui_label20, 40);
    lv_obj_set_height(ObjsCt->ui_label20, 20);
    lv_obj_set_x(ObjsCt->ui_label20, -1);
    lv_obj_set_y(ObjsCt->ui_label20, 137);
    lv_obj_set_align(ObjsCt->ui_label20, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label20, "VOL");
    lv_obj_set_style_text_color(ObjsCt->ui_label20, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label20, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label20, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label20, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_label22 = lv_label_create(ObjsCt->ui_panel37);
    lv_obj_set_width(ObjsCt->ui_label22, 40);
    lv_obj_set_height(ObjsCt->ui_label22, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->ui_label22, -1);
    lv_obj_set_y(ObjsCt->ui_label22, -129);
    lv_obj_set_align(ObjsCt->ui_label22, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_label22, "20");
    lv_obj_set_style_text_color(ObjsCt->ui_label22, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_label22, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_label22, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_label22, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 初始化Panel数组，用于编码器控制（按顺序：左列从上到下，右列从上到下）
    panel_array[0] = ObjsCt->ui_panel9;
    panel_array[1] = ObjsCt->ui_panel10;
    panel_array[2] = ObjsCt->ui_panel11;
    panel_array[3] = ObjsCt->ui_panel12;
    panel_array[4] = ObjsCt->ui_panel13;
    panel_array[5] = ObjsCt->ui_panel14;
    panel_array[6] = ObjsCt->ui_panel15;
    panel_array[7] = ObjsCt->ui_panel16;
    panel_array[8] = ObjsCt->ui_panel17;
    panel_array[9] = ObjsCt->ui_panel18;
    panel_array[10] = ObjsCt->ui_panel19;
    panel_array[11] = ObjsCt->ui_panel34;
    panel_array[12] = ObjsCt->ui_panel20;
    panel_array[13] = ObjsCt->ui_panel21;
    panel_array[14] = ObjsCt->ui_panel22;
    panel_array[15] = ObjsCt->ui_panel23;
    panel_array[16] = ObjsCt->ui_panel24;
    panel_array[17] = ObjsCt->ui_panel25;
    panel_array[18] = ObjsCt->ui_panel26;
    panel_array[19] = ObjsCt->ui_panel27;
    panel_array[20] = ObjsCt->ui_panel30;
    panel_array[21] = ObjsCt->ui_panel31;
    panel_array[22] = ObjsCt->ui_panel32;
    panel_array[23] = ObjsCt->ui_panel35;

    label_array[0] = ObjsCt->ui_label23;
    label_array[1] = ObjsCt->ui_label24;
    label_array[2] = ObjsCt->ui_label25;
    label_array[3] = ObjsCt->ui_label26;
    label_array[4] = ObjsCt->ui_label27;
    label_array[5] = ObjsCt->ui_label28;
    label_array[6] = ObjsCt->ui_label29;
    label_array[7] = ObjsCt->ui_label30;
    label_array[8] = ObjsCt->ui_label31;
    label_array[9] = ObjsCt->ui_label32;
    label_array[10] = ObjsCt->ui_label33;
    label_array[11] = ObjsCt->ui_label49;
    label_array[12] = ObjsCt->ui_label34;
    label_array[13] = ObjsCt->ui_label39;
    label_array[14] = ObjsCt->ui_label40;
    label_array[15] = ObjsCt->ui_label41;
    label_array[16] = ObjsCt->ui_label42;
    label_array[17] = ObjsCt->ui_label43;
    label_array[18] = ObjsCt->ui_label44;
    label_array[19] = ObjsCt->ui_label45;
    label_array[20] = ObjsCt->ui_label46;
    label_array[21] = ObjsCt->ui_label47;
    label_array[22] = ObjsCt->ui_label48;
    label_array[23] = ObjsCt->ui_label50;

    // 批量设置所有Panel的统一样式
    {
        uint8_t i;
        for(i = 0; i < 24; i++) {
            lv_obj_set_style_radius(panel_array[i], 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_array[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_array[i], 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(panel_array[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // 初始化slider数组 (REV, VOL, Delay)
    slider_panel_array[0] = ObjsCt->ui_panel36;  // REV
    slider_panel_array[1] = ObjsCt->ui_panel37;  // VOL
    slider_panel_array[2] = ObjsCt->ui_panel33;  // Delay (2000ms)
    
    slider_array[0] = ObjsCt->ui_slider1;  // REV slider
    slider_array[1] = ObjsCt->ui_slider2;  // VOL slider
    slider_array[2] = ObjsCt->ui_slider3;  // Delay slider
    
    // 设置slider范围 0-20
    lv_slider_set_range(slider_array[0], 0, 20);
    lv_slider_set_range(slider_array[1], 0, 20);
    lv_slider_set_range(slider_array[2], 0, 20);

    // 设置初始焦点（第一个panel绿色边框）
    lv_obj_set_style_border_color(panel_array[0], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_page_Dsp_start(void)
{
    uint8_t i;
    
    if(ObjsCt == NULL) return;  // 安全检查
    
    ArgsCt.focus_target = 0;
    ArgsCt.edit_mode = 0;

    // 重置所有panel为白色边框，所有label为白色文字
    for(i = 0; i < 24; i++) {
        lv_obj_set_style_border_color(panel_array[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_array[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
    // 重置所有slider panel为白色边框
    for(i = 0; i < 3; i++) {
        lv_obj_set_style_border_color(slider_panel_array[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
    // 设置第一个panel为绿色焦点
    lv_obj_set_style_border_color(panel_array[0], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // 设置当前选中DSP（默认为DSP 0）的文字为橙色
    lv_obj_set_style_text_color(label_array[ArgsCt.current_dsp], lv_color_hex(0xFFA500), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // 更新slider显示当前DSP的值
    lv_slider_set_value(slider_array[0], ArgsCt.dsp_rev[ArgsCt.current_dsp], LV_ANIM_OFF);
    lv_slider_set_value(slider_array[1], ArgsCt.dsp_vol[ArgsCt.current_dsp], LV_ANIM_OFF);
    lv_slider_set_value(slider_array[2], ArgsCt.dsp_delay[ArgsCt.current_dsp], LV_ANIM_OFF);
    
    // 更新slider上方的数值显示
    update_slider_value_labels();
}

void ui_page_Dsp_exit(uint8_t del)
{
}

void ui_page_Dsp_clean(void)
{
    lv_mem_free(ObjsCt);
    ObjsCt = NULL;  // 防止野指针
}

void ui_page_Dsp_request(uint32_t delay)
{
    switch(ui_pmanagerCt.id_tar)
    {
        //当前界面
        case UI_PAGE_SEL_8:
            break;
        //删除后台
        default:
            ui_pmanager_switch(UI_PAGE_SEL_8, delay, true);
            break;
    }
}

// 编码器上键 - 焦点上移
void ui_page_Dsp_encoder_up(void)
{
    uint8_t i;
    if(ObjsCt == NULL) return;  // 安全检查
    
    // 如果在编辑模式，上键不移动焦点，而是增加slider值
    if(ArgsCt.edit_mode && ArgsCt.focus_target >= 24) {
        uint8_t slider_idx = ArgsCt.focus_target - 24;
        int16_t val = lv_slider_get_value(slider_array[slider_idx]);
        if(val < 20) {
            lv_slider_set_value(slider_array[slider_idx], val + 1, LV_ANIM_OFF);
            // 更新对应DSP的参数
            if(slider_idx == 0) ArgsCt.dsp_rev[ArgsCt.current_dsp] = val + 1;
            else if(slider_idx == 1) ArgsCt.dsp_vol[ArgsCt.current_dsp] = val + 1;
            else ArgsCt.dsp_delay[ArgsCt.current_dsp] = val + 1;
            // 更新数值显示
            update_slider_value_labels();
        }
        return;
    }
    
    // 清除当前焦点
    if(ArgsCt.focus_target < 24) {
        lv_obj_set_style_border_color(panel_array[ArgsCt.focus_target], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_border_color(slider_panel_array[ArgsCt.focus_target - 24], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // 焦点向上移动（循环）
    if(ArgsCt.focus_target > 0) {
        ArgsCt.focus_target--;
    } else {
        ArgsCt.focus_target = 26; // 回到最后一个slider
    }

    // 设置新焦点的绿色边框
    if(ArgsCt.focus_target < 24) {
        lv_obj_set_style_border_color(panel_array[ArgsCt.focus_target], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_border_color(slider_panel_array[ArgsCt.focus_target - 24], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// 编码器下键 - 焦点下移
void ui_page_Dsp_encoder_down(void)
{
    uint8_t i;
    if(ObjsCt == NULL) return;  // 安全检查
    
    // 如果在编辑模式，下键不移动焦点，而是减少slider值
    if(ArgsCt.edit_mode && ArgsCt.focus_target >= 24) {
        uint8_t slider_idx = ArgsCt.focus_target - 24;
        int16_t val = lv_slider_get_value(slider_array[slider_idx]);
        if(val > 0) {
            lv_slider_set_value(slider_array[slider_idx], val - 1, LV_ANIM_OFF);
            // 更新对应DSP的参数
            if(slider_idx == 0) ArgsCt.dsp_rev[ArgsCt.current_dsp] = val - 1;
            else if(slider_idx == 1) ArgsCt.dsp_vol[ArgsCt.current_dsp] = val - 1;
            else ArgsCt.dsp_delay[ArgsCt.current_dsp] = val - 1;
            // 更新数值显示
            update_slider_value_labels();
        }
        return;
    }
    
    // 清除当前焦点
    if(ArgsCt.focus_target < 24) {
        lv_obj_set_style_border_color(panel_array[ArgsCt.focus_target], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_border_color(slider_panel_array[ArgsCt.focus_target - 24], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // 焦点向下移动（循环）
    if(ArgsCt.focus_target < 26) {
        ArgsCt.focus_target++;
    } else {
        ArgsCt.focus_target = 0; // 回到第一个panel
    }

    // 设置新焦点的绿色边框
    if(ArgsCt.focus_target < 24) {
        lv_obj_set_style_border_color(panel_array[ArgsCt.focus_target], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_border_color(slider_panel_array[ArgsCt.focus_target - 24], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// 编码器OK键 - DSP选择/Slider编辑切换
void ui_page_Dsp_encoder_ok(void)
{
    if(ObjsCt == NULL) return;  // 安全检查
    
    // 如果在DSP panel上，选中该DSP并更新slider显示
    if(ArgsCt.focus_target < 24) {
        // 清除上一个选中DSP的橙色文字
        lv_obj_set_style_text_color(label_array[ArgsCt.current_dsp], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        
        // 选中新的DSP
        ArgsCt.current_dsp = ArgsCt.focus_target;
        
        // 设置新选中DSP的文字为橙色
        lv_obj_set_style_text_color(label_array[ArgsCt.current_dsp], lv_color_hex(0xFFA500), LV_PART_MAIN | LV_STATE_DEFAULT);
        
        // 更新三个slider显示该DSP的参数
        lv_slider_set_value(slider_array[0], ArgsCt.dsp_rev[ArgsCt.current_dsp], LV_ANIM_OFF);
        lv_slider_set_value(slider_array[1], ArgsCt.dsp_vol[ArgsCt.current_dsp], LV_ANIM_OFF);
        lv_slider_set_value(slider_array[2], ArgsCt.dsp_delay[ArgsCt.current_dsp], LV_ANIM_OFF);
        
        // 更新slider上方的数值显示
        update_slider_value_labels();
    }
    // 如果在slider上，切换编辑模式
    else {
        ArgsCt.edit_mode = !ArgsCt.edit_mode;
        uint8_t slider_idx = ArgsCt.focus_target - 24;
        
        if(ArgsCt.edit_mode) {
            // 进入编辑模式，slider panel边框变橙色
            lv_obj_set_style_border_color(slider_panel_array[slider_idx], lv_color_hex(0xFFA500), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            // 退出编辑模式，slider panel边框恢复绿色
            lv_obj_set_style_border_color(slider_panel_array[slider_idx], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}
