#include "ui_page_Spec.h"
#include "string.h"
#include "ui_pmanager.h"
#include "my_i2c_driver.h"
#include "i2c_interface.h"
#include "ui_assets.h"

static SpecObjsContext *ObjsCt;
static SpecArgsContext ArgsCt;
static uint8_t args_initialized;

#define NUM_BARS 12
#define LEVEL_MAX 19
static uint8_t levels[NUM_BARS];
static int16_t last_fg_h[NUM_BARS]; // 缓存上一次高度，避免值未变时重复重绘

#define SIM_MAX_LEVEL 18 // 模拟电平上限（调试用，联调时移除或改为实际范围）

// 等级→高度查找表（预先计算，避免循环内乘除法）
static const uint8_t fg_h_lut[LEVEL_MAX] = {
    0, 11, 22, 33, 44, 55, 66, 77, 88, 99, 111, 122, 133, 144, 155, 166, 177, 188, 199
};




static void timer_cb(lv_timer_t *t)
{
    (void)t;
    if(!ObjsCt) return;

    // --- 模拟电平数据（已禁用，改由I2C从机读取）---
    // static uint8_t sl[NUM_BARS] = {0, 3, 6, 9, 2, 5, 8, 11, 1, 4, 7, 10};
    // static int8_t  sd[NUM_BARS] = {1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  1};
    // for(int i = 0; i < NUM_BARS; i++) {
    //     sl[i] += sd[i];
    //     if(sl[i] >= SIM_MAX_LEVEL) sd[i] = -1;
    //     if(sl[i] == 0)            sd[i] =  1;
    //     levels[i] = sl[i];
    // }

    // --- 从I2C从机读取MIC1/MIC2电平 ---
    // 从机0x22 -> MIC1，从机0x23 -> MIC2
    // RecvBuf[0]的值直接就是电平等级
    if(I2C_MasterReceiveBuffer(0x22, MIC1_Buf, PACKET_LEN, 5000) == ERROR_OK) {
        uint8_t lv = MIC1_Buf[0];
        if(lv >= LEVEL_MAX) lv = LEVEL_MAX - 1; // 越界保护
        levels[0] = lv; // MIC1
    }
    if(I2C_MasterReceiveBuffer(0x23, MIC2_Buf, PACKET_LEN, 5000) == ERROR_OK) {
        uint8_t lv = MIC2_Buf[0];
        if(lv >= LEVEL_MAX) lv = LEVEL_MAX - 1; // 越界保护
        levels[1] = lv; // MIC2
    }
    if(I2C_MasterReceiveBuffer(0x24, Aux_Buf, PACKET_LEN, 5000) == ERROR_OK) {
        uint8_t lv_l = Aux_Buf[0];
        uint8_t lv_r = Aux_Buf[1];
        if(lv_l >= LEVEL_MAX) lv_l = LEVEL_MAX - 1; // 越界保护
        levels[2] = lv_l; // Aux
        if(lv_r >= LEVEL_MAX) lv_r = LEVEL_MAX - 1; // 越界保护
        levels[3] = lv_r; // Aux
    }
    if(I2C_MasterReceiveBuffer(0x25, MP3_BT_BUF, PACKET_LEN, 5000) == ERROR_OK) {
        uint8_t lv_l = MP3_BT_BUF[0];
        uint8_t lv_r = MP3_BT_BUF[1];
        if(lv_l >= LEVEL_MAX) lv_l = LEVEL_MAX - 1; // 越界保护
        levels[4] = lv_l; // MP3/BT
        if(lv_r >= LEVEL_MAX) lv_r = LEVEL_MAX - 1; // 越界保护
        levels[5] = lv_r; // MP3/BT
    }
    if(I2C_MasterReceiveBuffer(0x26, PC_Buf, PACKET_LEN, 5000) == ERROR_OK) {
        uint8_t lv_l = PC_Buf[0];
        uint8_t lv_r = PC_Buf[1];
        if(lv_l >= LEVEL_MAX) lv_l = LEVEL_MAX - 1; // 越界保护
        levels[6] = lv_l; // PC/OTG
        if(lv_r >= LEVEL_MAX) lv_r = LEVEL_MAX - 1; // 越界保护
        levels[7] = lv_r; // PC/OTG
    }

    // --- 更新12路电平条前景高度 ---
    lv_obj_t * const masks[NUM_BARS] = {
        ObjsCt->mic1_bar_fg_mask,
        ObjsCt->mic2_bar_fg_mask,
        ObjsCt->vol34_bar_left_fg_mask,
        ObjsCt->vol34_bar_right_fg_mask,
        ObjsCt->usb_bt_bar_left_fg_mask,
        ObjsCt->usb_bt_bar_right_fg_mask,
        ObjsCt->pc_otg_bar_left_fg_mask,
        ObjsCt->pc_otg_bar_right_fg_mask,
        ObjsCt->main_bar_left_fg_mask,
        ObjsCt->main_bar_right_fg_mask,
        ObjsCt->pfl_bar_left_fg_mask,
        ObjsCt->pfl_bar_right_fg_mask,
    };
    for(int i = 0; i < NUM_BARS; i++) {
        if(masks[i]) {
            uint8_t fg_h = fg_h_lut[levels[i]];
            if(fg_h != last_fg_h[i]) {
                last_fg_h[i] = fg_h;
                lv_obj_set_height(masks[i], fg_h);
            }
        }
    }

    if (ArgsCt.mic1_vol_slider_value != MIC1_Buf[2])
    {
        ArgsCt.mic1_vol_slider_value = MIC1_Buf[2];
        lv_slider_set_value(ObjsCt->mic1_slider, ArgsCt.mic1_vol_slider_value, LV_ANIM_ON);
    }
    if (ArgsCt.mic2_vol_slider_value != MIC2_Buf[2])
    {
        ArgsCt.mic2_vol_slider_value = MIC2_Buf[2];
        lv_slider_set_value(ObjsCt->mic2_slider, ArgsCt.mic2_vol_slider_value, LV_ANIM_ON);
    }
    if (ArgsCt.vol34_vol_slider_value != Aux_Buf[2])
    {
        ArgsCt.vol34_vol_slider_value = Aux_Buf[2];
        lv_slider_set_value(ObjsCt->vol34_slider, ArgsCt.vol34_vol_slider_value, LV_ANIM_ON);
    }
    if (ArgsCt.usb_bt_vol_slider_value != MP3_BT_BUF[2])
    {
        ArgsCt.usb_bt_vol_slider_value = MP3_BT_BUF[2];
        lv_slider_set_value(ObjsCt->usb_bt_slider, ArgsCt.usb_bt_vol_slider_value, LV_ANIM_ON);
    }
    if (ArgsCt.pc_otg_vol_slider_value != PC_Buf[2])
    {
        ArgsCt.pc_otg_vol_slider_value = PC_Buf[2];
        lv_slider_set_value(ObjsCt->pc_otg_slider, ArgsCt.pc_otg_vol_slider_value, LV_ANIM_ON);
    }
    
}

void ui_page_Spec_init(lv_obj_t *screen)
{
    ObjsCt = (SpecObjsContext *)lv_mem_alloc(sizeof(SpecObjsContext));
    memset(ObjsCt, 0, sizeof(SpecObjsContext));
    if(!args_initialized)
    {
        memset(&ArgsCt, 0, sizeof(MainOutArgsContext));
        ArgsCt.focus_target = 0;
        ArgsCt.edit_mode = 0;    
        args_initialized = 1;
    }

    ObjsCt->Spec_obj = lv_obj_create(screen);
    lv_obj_set_size(ObjsCt->Spec_obj, 480, 320);
    lv_obj_set_style_bg_color(ObjsCt->Spec_obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->Spec_obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->Spec_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ObjsCt->Spec_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ObjsCt->Spec_obj, LV_OBJ_FLAG_SCROLLABLE);

    // create 12 meter panels based on ui_Screen2 absolute positions
    ObjsCt->mic1_meter_group = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->mic1_meter_group, 55);
    lv_obj_set_height(ObjsCt->mic1_meter_group, 220);
    lv_obj_set_x(ObjsCt->mic1_meter_group, -211);
    lv_obj_set_y(ObjsCt->mic1_meter_group, -11);
    lv_obj_set_align(ObjsCt->mic1_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->mic1_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->mic1_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic1_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic1_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic1_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ObjsCt->mic1_meter_group, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    ObjsCt->mic1_slider_panel = lv_obj_create(ObjsCt->mic1_meter_group);  // 移入PFL meter group
    lv_obj_set_width(ObjsCt->mic1_slider_panel, 11);
    lv_obj_set_height(ObjsCt->mic1_slider_panel, 220);
    lv_obj_set_x(ObjsCt->mic1_slider_panel, -21);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->mic1_slider_panel, 0);
    lv_obj_set_align(ObjsCt->mic1_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->mic1_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->mic1_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->mic1_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->mic1_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic1_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->mic1_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->mic1_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->mic1_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->mic1_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->mic1_slider = lv_slider_create(ObjsCt->mic1_slider_panel);
    lv_slider_set_range(ObjsCt->mic1_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->mic1_slider, ArgsCt.mic1_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->mic1_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->mic1_slider, ArgsCt.mic1_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->mic1_slider, 7);
    lv_obj_set_height(ObjsCt->mic1_slider, 205);
    lv_obj_set_x(ObjsCt->mic1_slider, 0);
    lv_obj_set_y(ObjsCt->mic1_slider, -3);
    lv_obj_set_align(ObjsCt->mic1_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->mic1_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic1_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic1_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->mic1_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->mic1_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic1_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->mic1_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->mic1_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->mic1_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->mic1_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->mic1_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic1_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic1_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->mic1_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->mic1_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic1_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->mic1_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic1_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic1_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->mic1_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->mic1_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic1_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->mic1_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->mic1_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->mic1_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->mic1_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->mic1_label_plus10 = lv_label_create(ObjsCt->mic1_meter_group);
    lv_obj_set_width(ObjsCt->mic1_label_plus10, 30);
    lv_obj_set_height(ObjsCt->mic1_label_plus10, 15);
    lv_obj_set_x(ObjsCt->mic1_label_plus10, 1);
    lv_obj_set_y(ObjsCt->mic1_label_plus10, -99);
    lv_obj_set_align(ObjsCt->mic1_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic1_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->mic1_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic1_label_plus6 = lv_label_create(ObjsCt->mic1_meter_group);
    lv_obj_set_width(ObjsCt->mic1_label_plus6, 30);
    lv_obj_set_height(ObjsCt->mic1_label_plus6, 15);
    lv_obj_set_x(ObjsCt->mic1_label_plus6, 1);
    lv_obj_set_y(ObjsCt->mic1_label_plus6, -69);
    lv_obj_set_align(ObjsCt->mic1_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic1_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->mic1_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic1_label_0db = lv_label_create(ObjsCt->mic1_meter_group);
    lv_obj_set_width(ObjsCt->mic1_label_0db, 30);
    lv_obj_set_height(ObjsCt->mic1_label_0db, 15);
    lv_obj_set_x(ObjsCt->mic1_label_0db, 1);
    lv_obj_set_y(ObjsCt->mic1_label_0db, -37);
    lv_obj_set_align(ObjsCt->mic1_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic1_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->mic1_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic1_label_minus10 = lv_label_create(ObjsCt->mic1_meter_group);
    lv_obj_set_width(ObjsCt->mic1_label_minus10, 30);
    lv_obj_set_height(ObjsCt->mic1_label_minus10, 15);
    lv_obj_set_x(ObjsCt->mic1_label_minus10, 1);
    lv_obj_set_y(ObjsCt->mic1_label_minus10, 0);
    lv_obj_set_align(ObjsCt->mic1_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic1_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->mic1_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic1_label_minus30 = lv_label_create(ObjsCt->mic1_meter_group);
    lv_obj_set_width(ObjsCt->mic1_label_minus30, 30);
    lv_obj_set_height(ObjsCt->mic1_label_minus30, 15);
    lv_obj_set_x(ObjsCt->mic1_label_minus30, 1);
    lv_obj_set_y(ObjsCt->mic1_label_minus30, 47);
    lv_obj_set_align(ObjsCt->mic1_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic1_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->mic1_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic1_label_minus50 = lv_label_create(ObjsCt->mic1_meter_group);
    lv_obj_set_width(ObjsCt->mic1_label_minus50, 30);
    lv_obj_set_height(ObjsCt->mic1_label_minus50, 15);
    lv_obj_set_x(ObjsCt->mic1_label_minus50, 1);
    lv_obj_set_y(ObjsCt->mic1_label_minus50, 98);
    lv_obj_set_align(ObjsCt->mic1_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic1_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->mic1_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->mic1_bar = lv_img_create(ObjsCt->mic1_meter_group);
    lv_img_set_src(ObjsCt->mic1_bar, &level1_bg);
    lv_obj_set_width(ObjsCt->mic1_bar, 18);
    lv_obj_set_height(ObjsCt->mic1_bar, 211);
    lv_obj_set_x(ObjsCt->mic1_bar, 18);
    lv_obj_set_y(ObjsCt->mic1_bar, 0);
    lv_obj_set_align(ObjsCt->mic1_bar, LV_ALIGN_CENTER);

    ObjsCt->mic1_bar_fg_mask = lv_obj_create(ObjsCt->mic1_meter_group);
    lv_obj_set_width(ObjsCt->mic1_bar_fg_mask, 18);
    lv_obj_set_height(ObjsCt->mic1_bar_fg_mask, 0);
    lv_obj_set_x(ObjsCt->mic1_bar_fg_mask, 18);
    lv_obj_set_y(ObjsCt->mic1_bar_fg_mask, -4);
    lv_obj_set_align(ObjsCt->mic1_bar_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->mic1_bar_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->mic1_bar_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic1_bar_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->mic1_bar_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->mic1_bar_fg = lv_img_create(ObjsCt->mic1_bar_fg_mask);
    lv_img_set_src(ObjsCt->mic1_bar_fg, &level_1);
    lv_obj_set_width(ObjsCt->mic1_bar_fg, 18);
    lv_obj_set_height(ObjsCt->mic1_bar_fg, 211);
    lv_obj_align(ObjsCt->mic1_bar_fg, LV_ALIGN_BOTTOM_MID, 0, 0);

    ObjsCt->mic1_title_box = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->mic1_title_box, 51);
    lv_obj_set_height(ObjsCt->mic1_title_box, 50);
    lv_obj_set_x(ObjsCt->mic1_title_box, -206);
    lv_obj_set_y(ObjsCt->mic1_title_box, 126);
    lv_obj_set_align(ObjsCt->mic1_title_box, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->mic1_title_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->mic1_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic1_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic1_title_box, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic1_title_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ObjsCt->mic1_title_box, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(ObjsCt->mic1_title_box);
    ObjsCt->mic1_title_lr = lv_label_create(ObjsCt->mic1_title_box);
    lv_obj_set_width(ObjsCt->mic1_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->mic1_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->mic1_title_lr, -1);
    lv_obj_set_y(ObjsCt->mic1_title_lr, -10);
    lv_obj_set_align(ObjsCt->mic1_title_lr, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic1_title_lr, "L+R");
    lv_obj_set_style_text_font(ObjsCt->mic1_title_lr, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic1_title_name = lv_label_create(ObjsCt->mic1_title_box);
    lv_obj_set_width(ObjsCt->mic1_title_name, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->mic1_title_name, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->mic1_title_name, -2);
    lv_obj_set_y(ObjsCt->mic1_title_name, 9);
    lv_obj_set_align(ObjsCt->mic1_title_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic1_title_name, "MIC1");
    lv_obj_set_style_text_font(ObjsCt->mic1_title_name, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->mic2_meter_group = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->mic2_meter_group, 55);
    lv_obj_set_height(ObjsCt->mic2_meter_group, 220);
    lv_obj_set_x(ObjsCt->mic2_meter_group, -155);
    lv_obj_set_y(ObjsCt->mic2_meter_group, -11);
    lv_obj_set_align(ObjsCt->mic2_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->mic2_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->mic2_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic2_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic2_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic2_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ObjsCt->mic2_meter_group, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    ObjsCt->mic2_slider_panel = lv_obj_create(ObjsCt->mic2_meter_group);  // 移入PFL meter group
    lv_obj_set_width(ObjsCt->mic2_slider_panel, 11);
    lv_obj_set_height(ObjsCt->mic2_slider_panel, 220);
    lv_obj_set_x(ObjsCt->mic2_slider_panel, -21);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->mic2_slider_panel, 0);
    lv_obj_set_align(ObjsCt->mic2_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->mic2_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->mic2_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->mic2_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->mic2_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic2_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->mic2_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->mic2_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->mic2_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->mic2_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->mic2_slider = lv_slider_create(ObjsCt->mic2_slider_panel);
    lv_slider_set_range(ObjsCt->mic2_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->mic2_slider, ArgsCt.mic2_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->mic2_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->mic2_slider, ArgsCt.mic2_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->mic2_slider, 7);
    lv_obj_set_height(ObjsCt->mic2_slider, 205);
    lv_obj_set_x(ObjsCt->mic2_slider, 0);
    lv_obj_set_y(ObjsCt->mic2_slider, -3);
    lv_obj_set_align(ObjsCt->mic2_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->mic2_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic2_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic2_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->mic2_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->mic2_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic2_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->mic2_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->mic2_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->mic2_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->mic2_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->mic2_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic2_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic2_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->mic2_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->mic2_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic2_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->mic2_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic2_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic2_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->mic2_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->mic2_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic2_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->mic2_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->mic2_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->mic2_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->mic2_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->mic2_label_plus10 = lv_label_create(ObjsCt->mic2_meter_group);
    lv_obj_set_width(ObjsCt->mic2_label_plus10, 30);
    lv_obj_set_height(ObjsCt->mic2_label_plus10, 15);
    lv_obj_set_x(ObjsCt->mic2_label_plus10, 1);
    lv_obj_set_y(ObjsCt->mic2_label_plus10, -99);
    lv_obj_set_align(ObjsCt->mic2_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic2_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->mic2_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic2_label_plus6 = lv_label_create(ObjsCt->mic2_meter_group);
    lv_obj_set_width(ObjsCt->mic2_label_plus6, 30);
    lv_obj_set_height(ObjsCt->mic2_label_plus6, 15);
    lv_obj_set_x(ObjsCt->mic2_label_plus6, 1);
    lv_obj_set_y(ObjsCt->mic2_label_plus6, -69);
    lv_obj_set_align(ObjsCt->mic2_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic2_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->mic2_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic2_label_0db = lv_label_create(ObjsCt->mic2_meter_group);
    lv_obj_set_width(ObjsCt->mic2_label_0db, 30);
    lv_obj_set_height(ObjsCt->mic2_label_0db, 15);
    lv_obj_set_x(ObjsCt->mic2_label_0db, 1);
    lv_obj_set_y(ObjsCt->mic2_label_0db, -37);
    lv_obj_set_align(ObjsCt->mic2_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic2_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->mic2_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic2_label_minus10 = lv_label_create(ObjsCt->mic2_meter_group);
    lv_obj_set_width(ObjsCt->mic2_label_minus10, 30);
    lv_obj_set_height(ObjsCt->mic2_label_minus10, 15);
    lv_obj_set_x(ObjsCt->mic2_label_minus10, 1);
    lv_obj_set_y(ObjsCt->mic2_label_minus10, 0);
    lv_obj_set_align(ObjsCt->mic2_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic2_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->mic2_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic2_label_minus30 = lv_label_create(ObjsCt->mic2_meter_group);
    lv_obj_set_width(ObjsCt->mic2_label_minus30, 30);
    lv_obj_set_height(ObjsCt->mic2_label_minus30, 15);
    lv_obj_set_x(ObjsCt->mic2_label_minus30, 1);
    lv_obj_set_y(ObjsCt->mic2_label_minus30, 47);
    lv_obj_set_align(ObjsCt->mic2_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic2_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->mic2_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic2_label_minus50 = lv_label_create(ObjsCt->mic2_meter_group);
    lv_obj_set_width(ObjsCt->mic2_label_minus50, 30);
    lv_obj_set_height(ObjsCt->mic2_label_minus50, 15);
    lv_obj_set_x(ObjsCt->mic2_label_minus50, 1);
    lv_obj_set_y(ObjsCt->mic2_label_minus50, 98);
    lv_obj_set_align(ObjsCt->mic2_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic2_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->mic2_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->mic2_bar = lv_img_create(ObjsCt->mic2_meter_group);
    lv_img_set_src(ObjsCt->mic2_bar, &level1_bg);
    lv_obj_set_width(ObjsCt->mic2_bar, 18);
    lv_obj_set_height(ObjsCt->mic2_bar, 211);
    lv_obj_set_x(ObjsCt->mic2_bar, 18);
    lv_obj_set_y(ObjsCt->mic2_bar, 0);
    lv_obj_set_align(ObjsCt->mic2_bar, LV_ALIGN_CENTER);

    ObjsCt->mic2_bar_fg_mask = lv_obj_create(ObjsCt->mic2_meter_group);
    lv_obj_set_width(ObjsCt->mic2_bar_fg_mask, 18);
    lv_obj_set_height(ObjsCt->mic2_bar_fg_mask, 0);
    lv_obj_set_x(ObjsCt->mic2_bar_fg_mask, 18);
    lv_obj_set_y(ObjsCt->mic2_bar_fg_mask, -4);
    lv_obj_set_align(ObjsCt->mic2_bar_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->mic2_bar_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->mic2_bar_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic2_bar_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->mic2_bar_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->mic2_bar_fg = lv_img_create(ObjsCt->mic2_bar_fg_mask);
    lv_img_set_src(ObjsCt->mic2_bar_fg, &level_1);
    lv_obj_set_width(ObjsCt->mic2_bar_fg, 18);
    lv_obj_set_height(ObjsCt->mic2_bar_fg, 211);
    lv_obj_align(ObjsCt->mic2_bar_fg, LV_ALIGN_BOTTOM_MID, 0, 0);

    ObjsCt->mic2_title_box = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->mic2_title_box, 52);
    lv_obj_set_height(ObjsCt->mic2_title_box, 50);
    lv_obj_set_x(ObjsCt->mic2_title_box, -152);
    lv_obj_set_y(ObjsCt->mic2_title_box, 126);
    lv_obj_set_align(ObjsCt->mic2_title_box, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->mic2_title_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->mic2_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->mic2_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->mic2_title_box, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->mic2_title_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ObjsCt->mic2_title_box, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(ObjsCt->mic2_title_box);
    ObjsCt->mic2_title_lr = lv_label_create(ObjsCt->mic2_title_box);
    lv_obj_set_width(ObjsCt->mic2_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->mic2_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->mic2_title_lr, -1);
    lv_obj_set_y(ObjsCt->mic2_title_lr, -10);
    lv_obj_set_align(ObjsCt->mic2_title_lr, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic2_title_lr, "L+R");
    lv_obj_set_style_text_font(ObjsCt->mic2_title_lr, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->mic2_title_name = lv_label_create(ObjsCt->mic2_title_box);
    lv_obj_set_width(ObjsCt->mic2_title_name, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->mic2_title_name, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->mic2_title_name, -2);
    lv_obj_set_y(ObjsCt->mic2_title_name, 9);
    lv_obj_set_align(ObjsCt->mic2_title_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->mic2_title_name, "MIC2");
    lv_obj_set_style_text_font(ObjsCt->mic2_title_name, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->vol34_meter_group = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->vol34_meter_group, 71);
    lv_obj_set_height(ObjsCt->vol34_meter_group, 220);
    lv_obj_set_x(ObjsCt->vol34_meter_group, -91);
    lv_obj_set_y(ObjsCt->vol34_meter_group, -11);
    lv_obj_set_align(ObjsCt->vol34_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->vol34_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->vol34_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->vol34_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->vol34_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->vol34_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->vol34_slider_panel = lv_obj_create(ObjsCt->vol34_meter_group);  // 移入PFL meter group
    lv_obj_set_width(ObjsCt->vol34_slider_panel, 11);
    lv_obj_set_height(ObjsCt->vol34_slider_panel, 220);
    lv_obj_set_x(ObjsCt->vol34_slider_panel, -28);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->vol34_slider_panel, 0);
    lv_obj_set_align(ObjsCt->vol34_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->vol34_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->vol34_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->vol34_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->vol34_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->vol34_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->vol34_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->vol34_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->vol34_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->vol34_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->vol34_slider = lv_slider_create(ObjsCt->vol34_slider_panel);
    lv_slider_set_range(ObjsCt->vol34_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->vol34_slider, ArgsCt.vol34_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->vol34_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->vol34_slider, ArgsCt.vol34_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->vol34_slider, 7);
    lv_obj_set_height(ObjsCt->vol34_slider, 205);
    lv_obj_set_x(ObjsCt->vol34_slider, 0);
    lv_obj_set_y(ObjsCt->vol34_slider, -3);
    lv_obj_set_align(ObjsCt->vol34_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->vol34_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->vol34_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->vol34_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->vol34_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->vol34_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->vol34_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->vol34_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->vol34_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->vol34_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->vol34_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->vol34_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->vol34_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->vol34_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->vol34_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->vol34_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->vol34_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->vol34_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->vol34_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->vol34_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->vol34_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->vol34_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->vol34_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->vol34_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->vol34_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->vol34_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->vol34_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->vol34_bar_left = lv_img_create(ObjsCt->vol34_meter_group);
    lv_img_set_src(ObjsCt->vol34_bar_left, &level2_bg);
    lv_obj_set_width(ObjsCt->vol34_bar_left, 15);
    lv_obj_set_height(ObjsCt->vol34_bar_left, 211);
    lv_obj_set_x(ObjsCt->vol34_bar_left, -12);
    lv_obj_set_y(ObjsCt->vol34_bar_left, 0);
    lv_obj_set_align(ObjsCt->vol34_bar_left, LV_ALIGN_CENTER);

    ObjsCt->vol34_bar_left_fg_mask = lv_obj_create(ObjsCt->vol34_meter_group);
    lv_obj_set_width(ObjsCt->vol34_bar_left_fg_mask, 15);
    lv_obj_set_height(ObjsCt->vol34_bar_left_fg_mask, 0);
    lv_obj_set_x(ObjsCt->vol34_bar_left_fg_mask, -12);
    lv_obj_set_y(ObjsCt->vol34_bar_left_fg_mask, -4);
    lv_obj_set_align(ObjsCt->vol34_bar_left_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->vol34_bar_left_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->vol34_bar_left_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->vol34_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->vol34_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->vol34_bar_left_fg = lv_img_create(ObjsCt->vol34_bar_left_fg_mask);
    lv_img_set_src(ObjsCt->vol34_bar_left_fg, &level_2);
    lv_obj_set_width(ObjsCt->vol34_bar_left_fg, 15);
    lv_obj_set_height(ObjsCt->vol34_bar_left_fg, 211);
    lv_obj_align(ObjsCt->vol34_bar_left_fg, LV_ALIGN_BOTTOM_MID, 0, 0);
    ObjsCt->vol34_label_plus10 = lv_label_create(ObjsCt->vol34_meter_group);
    lv_obj_set_width(ObjsCt->vol34_label_plus10, 30);
    lv_obj_set_height(ObjsCt->vol34_label_plus10, 15);
    lv_obj_set_x(ObjsCt->vol34_label_plus10, 12);
    lv_obj_set_y(ObjsCt->vol34_label_plus10, -98);
    lv_obj_set_align(ObjsCt->vol34_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->vol34_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->vol34_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->vol34_label_plus6 = lv_label_create(ObjsCt->vol34_meter_group);
    lv_obj_set_width(ObjsCt->vol34_label_plus6, 30);
    lv_obj_set_height(ObjsCt->vol34_label_plus6, 15);
    lv_obj_set_x(ObjsCt->vol34_label_plus6, 12);
    lv_obj_set_y(ObjsCt->vol34_label_plus6, -68);
    lv_obj_set_align(ObjsCt->vol34_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->vol34_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->vol34_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->vol34_label_0db = lv_label_create(ObjsCt->vol34_meter_group);
    lv_obj_set_width(ObjsCt->vol34_label_0db, 30);
    lv_obj_set_height(ObjsCt->vol34_label_0db, 15);
    lv_obj_set_x(ObjsCt->vol34_label_0db, 12);
    lv_obj_set_y(ObjsCt->vol34_label_0db, -37);
    lv_obj_set_align(ObjsCt->vol34_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->vol34_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->vol34_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->vol34_label_minus10 = lv_label_create(ObjsCt->vol34_meter_group);
    lv_obj_set_width(ObjsCt->vol34_label_minus10, 30);
    lv_obj_set_height(ObjsCt->vol34_label_minus10, 15);
    lv_obj_set_x(ObjsCt->vol34_label_minus10, 12);
    lv_obj_set_y(ObjsCt->vol34_label_minus10, 1);
    lv_obj_set_align(ObjsCt->vol34_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->vol34_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->vol34_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->vol34_label_minus30 = lv_label_create(ObjsCt->vol34_meter_group);
    lv_obj_set_width(ObjsCt->vol34_label_minus30, 30);
    lv_obj_set_height(ObjsCt->vol34_label_minus30, 15);
    lv_obj_set_x(ObjsCt->vol34_label_minus30, 12);
    lv_obj_set_y(ObjsCt->vol34_label_minus30, 48);
    lv_obj_set_align(ObjsCt->vol34_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->vol34_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->vol34_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->vol34_label_minus50 = lv_label_create(ObjsCt->vol34_meter_group);
    lv_obj_set_width(ObjsCt->vol34_label_minus50, 30);
    lv_obj_set_height(ObjsCt->vol34_label_minus50, 15);
    lv_obj_set_x(ObjsCt->vol34_label_minus50, 12);
    lv_obj_set_y(ObjsCt->vol34_label_minus50, 99);
    lv_obj_set_align(ObjsCt->vol34_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->vol34_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->vol34_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->vol34_bar_right = lv_img_create(ObjsCt->vol34_meter_group);
    lv_img_set_src(ObjsCt->vol34_bar_right, &level2_bg);
    lv_obj_set_width(ObjsCt->vol34_bar_right, 15);
    lv_obj_set_height(ObjsCt->vol34_bar_right, 211);
    lv_obj_set_x(ObjsCt->vol34_bar_right, 27);
    lv_obj_set_y(ObjsCt->vol34_bar_right, 0);
    lv_obj_set_align(ObjsCt->vol34_bar_right, LV_ALIGN_CENTER);

    ObjsCt->vol34_bar_right_fg_mask = lv_obj_create(ObjsCt->vol34_meter_group);
    lv_obj_set_width(ObjsCt->vol34_bar_right_fg_mask, 15);
    lv_obj_set_height(ObjsCt->vol34_bar_right_fg_mask, 0);
    lv_obj_set_x(ObjsCt->vol34_bar_right_fg_mask, 27);
    lv_obj_set_y(ObjsCt->vol34_bar_right_fg_mask, -4);
    lv_obj_set_align(ObjsCt->vol34_bar_right_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->vol34_bar_right_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->vol34_bar_right_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->vol34_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->vol34_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->vol34_bar_right_fg = lv_img_create(ObjsCt->vol34_bar_right_fg_mask);
    lv_img_set_src(ObjsCt->vol34_bar_right_fg, &level_2);
    lv_obj_set_width(ObjsCt->vol34_bar_right_fg, 15);
    lv_obj_set_height(ObjsCt->vol34_bar_right_fg, 211);
    lv_obj_align(ObjsCt->vol34_bar_right_fg, LV_ALIGN_BOTTOM_MID, 0, 0);

    ObjsCt->vol34_title_box = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->vol34_title_box, 56);
    lv_obj_set_height(ObjsCt->vol34_title_box, 50);
    lv_obj_set_x(ObjsCt->vol34_title_box, -83);
    lv_obj_set_y(ObjsCt->vol34_title_box, 127);
    lv_obj_set_align(ObjsCt->vol34_title_box, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->vol34_title_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->vol34_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->vol34_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->vol34_title_box, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->vol34_title_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ObjsCt->vol34_title_box, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(ObjsCt->vol34_title_box);
    ObjsCt->vol34_title_lr = lv_label_create(ObjsCt->vol34_title_box);
    lv_obj_set_width(ObjsCt->vol34_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->vol34_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->vol34_title_lr, -1);
    lv_obj_set_y(ObjsCt->vol34_title_lr, -10);
    lv_obj_set_align(ObjsCt->vol34_title_lr, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->vol34_title_lr, "L       R");
    lv_obj_set_style_text_font(ObjsCt->vol34_title_lr, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->vol34_title_name = lv_label_create(ObjsCt->vol34_title_box);
    lv_obj_set_width(ObjsCt->vol34_title_name, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->vol34_title_name, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->vol34_title_name, -2);
    lv_obj_set_y(ObjsCt->vol34_title_name, 9);
    lv_obj_set_align(ObjsCt->vol34_title_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->vol34_title_name, "3/4VOL");
    lv_obj_set_style_text_font(ObjsCt->vol34_title_name, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->usb_bt_meter_group = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->usb_bt_meter_group, 71);
    lv_obj_set_height(ObjsCt->usb_bt_meter_group, 220);
    lv_obj_set_x(ObjsCt->usb_bt_meter_group, -19);
    lv_obj_set_y(ObjsCt->usb_bt_meter_group, -11);
    lv_obj_set_align(ObjsCt->usb_bt_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->usb_bt_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->usb_bt_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->usb_bt_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->usb_bt_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->usb_bt_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->usb_bt_slider_panel = lv_obj_create(ObjsCt->usb_bt_meter_group);  // 移入PFL meter group
    lv_obj_set_width(ObjsCt->usb_bt_slider_panel, 11);
    lv_obj_set_height(ObjsCt->usb_bt_slider_panel, 220);
    lv_obj_set_x(ObjsCt->usb_bt_slider_panel, -28);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->usb_bt_slider_panel, 0);
    lv_obj_set_align(ObjsCt->usb_bt_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->usb_bt_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->usb_bt_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->usb_bt_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->usb_bt_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->usb_bt_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->usb_bt_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->usb_bt_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->usb_bt_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->usb_bt_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->usb_bt_slider = lv_slider_create(ObjsCt->usb_bt_slider_panel);
    lv_slider_set_range(ObjsCt->usb_bt_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->usb_bt_slider, ArgsCt.usb_bt_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->usb_bt_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->usb_bt_slider, ArgsCt.usb_bt_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->usb_bt_slider, 7);
    lv_obj_set_height(ObjsCt->usb_bt_slider, 205);
    lv_obj_set_x(ObjsCt->usb_bt_slider, 0);
    lv_obj_set_y(ObjsCt->usb_bt_slider, -3);
    lv_obj_set_align(ObjsCt->usb_bt_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->usb_bt_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->usb_bt_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->usb_bt_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->usb_bt_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->usb_bt_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->usb_bt_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->usb_bt_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->usb_bt_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->usb_bt_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->usb_bt_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->usb_bt_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->usb_bt_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->usb_bt_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->usb_bt_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->usb_bt_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->usb_bt_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->usb_bt_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->usb_bt_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->usb_bt_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->usb_bt_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->usb_bt_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->usb_bt_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->usb_bt_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->usb_bt_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->usb_bt_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->usb_bt_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->usb_bt_bar_left = lv_img_create(ObjsCt->usb_bt_meter_group);
    lv_img_set_src(ObjsCt->usb_bt_bar_left, &level2_bg);
    lv_obj_set_width(ObjsCt->usb_bt_bar_left, 15);
    lv_obj_set_height(ObjsCt->usb_bt_bar_left, 211);
    lv_obj_set_x(ObjsCt->usb_bt_bar_left, -12);
    lv_obj_set_y(ObjsCt->usb_bt_bar_left, 0);
    lv_obj_set_align(ObjsCt->usb_bt_bar_left, LV_ALIGN_CENTER);

    ObjsCt->usb_bt_bar_left_fg_mask = lv_obj_create(ObjsCt->usb_bt_meter_group);
    lv_obj_set_width(ObjsCt->usb_bt_bar_left_fg_mask, 15);
    lv_obj_set_height(ObjsCt->usb_bt_bar_left_fg_mask, 0);
    lv_obj_set_x(ObjsCt->usb_bt_bar_left_fg_mask, -12);
    lv_obj_set_y(ObjsCt->usb_bt_bar_left_fg_mask, -4);
    lv_obj_set_align(ObjsCt->usb_bt_bar_left_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->usb_bt_bar_left_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->usb_bt_bar_left_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->usb_bt_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->usb_bt_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->usb_bt_bar_left_fg = lv_img_create(ObjsCt->usb_bt_bar_left_fg_mask);
    lv_img_set_src(ObjsCt->usb_bt_bar_left_fg, &level_2);
    lv_obj_set_width(ObjsCt->usb_bt_bar_left_fg, 15);
    lv_obj_set_height(ObjsCt->usb_bt_bar_left_fg, 211);
    lv_obj_align(ObjsCt->usb_bt_bar_left_fg, LV_ALIGN_BOTTOM_MID, 0, 0);
    ObjsCt->usb_bt_label_plus10 = lv_label_create(ObjsCt->usb_bt_meter_group);
    lv_obj_set_width(ObjsCt->usb_bt_label_plus10, 30);
    lv_obj_set_height(ObjsCt->usb_bt_label_plus10, 15);
    lv_obj_set_x(ObjsCt->usb_bt_label_plus10, 12);
    lv_obj_set_y(ObjsCt->usb_bt_label_plus10, -98);
    lv_obj_set_align(ObjsCt->usb_bt_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->usb_bt_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->usb_bt_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->usb_bt_label_plus6 = lv_label_create(ObjsCt->usb_bt_meter_group);
    lv_obj_set_width(ObjsCt->usb_bt_label_plus6, 30);
    lv_obj_set_height(ObjsCt->usb_bt_label_plus6, 15);
    lv_obj_set_x(ObjsCt->usb_bt_label_plus6, 12);
    lv_obj_set_y(ObjsCt->usb_bt_label_plus6, -68);
    lv_obj_set_align(ObjsCt->usb_bt_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->usb_bt_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->usb_bt_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->usb_bt_label_0db = lv_label_create(ObjsCt->usb_bt_meter_group);
    lv_obj_set_width(ObjsCt->usb_bt_label_0db, 30);
    lv_obj_set_height(ObjsCt->usb_bt_label_0db, 15);
    lv_obj_set_x(ObjsCt->usb_bt_label_0db, 12);
    lv_obj_set_y(ObjsCt->usb_bt_label_0db, -37);
    lv_obj_set_align(ObjsCt->usb_bt_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->usb_bt_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->usb_bt_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->usb_bt_label_minus10 = lv_label_create(ObjsCt->usb_bt_meter_group);
    lv_obj_set_width(ObjsCt->usb_bt_label_minus10, 30);
    lv_obj_set_height(ObjsCt->usb_bt_label_minus10, 15);
    lv_obj_set_x(ObjsCt->usb_bt_label_minus10, 12);
    lv_obj_set_y(ObjsCt->usb_bt_label_minus10, 0);
    lv_obj_set_align(ObjsCt->usb_bt_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->usb_bt_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->usb_bt_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->usb_bt_label_minus30 = lv_label_create(ObjsCt->usb_bt_meter_group);
    lv_obj_set_width(ObjsCt->usb_bt_label_minus30, 30);
    lv_obj_set_height(ObjsCt->usb_bt_label_minus30, 15);
    lv_obj_set_x(ObjsCt->usb_bt_label_minus30, 12);
    lv_obj_set_y(ObjsCt->usb_bt_label_minus30, 49);
    lv_obj_set_align(ObjsCt->usb_bt_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->usb_bt_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->usb_bt_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->usb_bt_label_minus50 = lv_label_create(ObjsCt->usb_bt_meter_group);
    lv_obj_set_width(ObjsCt->usb_bt_label_minus50, 30);
    lv_obj_set_height(ObjsCt->usb_bt_label_minus50, 15);
    lv_obj_set_x(ObjsCt->usb_bt_label_minus50, 12);
    lv_obj_set_y(ObjsCt->usb_bt_label_minus50, 99);
    lv_obj_set_align(ObjsCt->usb_bt_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->usb_bt_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->usb_bt_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->usb_bt_bar_right = lv_img_create(ObjsCt->usb_bt_meter_group);
    lv_img_set_src(ObjsCt->usb_bt_bar_right, &level2_bg);
    lv_obj_set_width(ObjsCt->usb_bt_bar_right, 15);
    lv_obj_set_height(ObjsCt->usb_bt_bar_right, 211);
    lv_obj_set_x(ObjsCt->usb_bt_bar_right, 27);
    lv_obj_set_y(ObjsCt->usb_bt_bar_right, 0);
    lv_obj_set_align(ObjsCt->usb_bt_bar_right, LV_ALIGN_CENTER);

    ObjsCt->usb_bt_bar_right_fg_mask = lv_obj_create(ObjsCt->usb_bt_meter_group);
    lv_obj_set_width(ObjsCt->usb_bt_bar_right_fg_mask, 15);
    lv_obj_set_height(ObjsCt->usb_bt_bar_right_fg_mask, 0);
    lv_obj_set_x(ObjsCt->usb_bt_bar_right_fg_mask, 27);
    lv_obj_set_y(ObjsCt->usb_bt_bar_right_fg_mask, -4);
    lv_obj_set_align(ObjsCt->usb_bt_bar_right_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->usb_bt_bar_right_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->usb_bt_bar_right_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->usb_bt_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->usb_bt_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->usb_bt_bar_right_fg = lv_img_create(ObjsCt->usb_bt_bar_right_fg_mask);
    lv_img_set_src(ObjsCt->usb_bt_bar_right_fg, &level_2);
    lv_obj_set_width(ObjsCt->usb_bt_bar_right_fg, 15);
    lv_obj_set_height(ObjsCt->usb_bt_bar_right_fg, 211);
    lv_obj_align(ObjsCt->usb_bt_bar_right_fg, LV_ALIGN_BOTTOM_MID, 0, 0);

    ObjsCt->usb_bt_title_box = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->usb_bt_title_box, 56);
    lv_obj_set_height(ObjsCt->usb_bt_title_box, 50);
    lv_obj_set_x(ObjsCt->usb_bt_title_box, -12);
    lv_obj_set_y(ObjsCt->usb_bt_title_box, 128);
    lv_obj_set_align(ObjsCt->usb_bt_title_box, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->usb_bt_title_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->usb_bt_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->usb_bt_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->usb_bt_title_box, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->usb_bt_title_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ObjsCt->usb_bt_title_box, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(ObjsCt->usb_bt_title_box);
    ObjsCt->usb_bt_title_lr = lv_label_create(ObjsCt->usb_bt_title_box);
    lv_obj_set_width(ObjsCt->usb_bt_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->usb_bt_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->usb_bt_title_lr, -1);
    lv_obj_set_y(ObjsCt->usb_bt_title_lr, -10);
    lv_obj_set_align(ObjsCt->usb_bt_title_lr, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->usb_bt_title_lr, "L       R");
    lv_obj_set_style_text_font(ObjsCt->usb_bt_title_lr, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->usb_bt_title_name = lv_label_create(ObjsCt->usb_bt_title_box);
    lv_obj_set_width(ObjsCt->usb_bt_title_name, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->usb_bt_title_name, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->usb_bt_title_name, -2);
    lv_obj_set_y(ObjsCt->usb_bt_title_name, 9);
    lv_obj_set_align(ObjsCt->usb_bt_title_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->usb_bt_title_name, "USB/BT");
    lv_obj_set_style_text_font(ObjsCt->usb_bt_title_name, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pc_otg_meter_group = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->pc_otg_meter_group, 71);
    lv_obj_set_height(ObjsCt->pc_otg_meter_group, 220);
    lv_obj_set_x(ObjsCt->pc_otg_meter_group, 53);
    lv_obj_set_y(ObjsCt->pc_otg_meter_group, -11);
    lv_obj_set_align(ObjsCt->pc_otg_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->pc_otg_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->pc_otg_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pc_otg_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pc_otg_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pc_otg_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pc_otg_slider_panel = lv_obj_create(ObjsCt->pc_otg_meter_group);  // 移入PFL meter group
    lv_obj_set_width(ObjsCt->pc_otg_slider_panel, 11);
    lv_obj_set_height(ObjsCt->pc_otg_slider_panel, 220);
    lv_obj_set_x(ObjsCt->pc_otg_slider_panel, -28);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->pc_otg_slider_panel, 0);
    lv_obj_set_align(ObjsCt->pc_otg_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->pc_otg_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->pc_otg_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->pc_otg_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->pc_otg_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pc_otg_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->pc_otg_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->pc_otg_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->pc_otg_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->pc_otg_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pc_otg_slider = lv_slider_create(ObjsCt->pc_otg_slider_panel);
    lv_slider_set_range(ObjsCt->pc_otg_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->pc_otg_slider, ArgsCt.pc_otg_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->pc_otg_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->pc_otg_slider, ArgsCt.pc_otg_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->pc_otg_slider, 7);
    lv_obj_set_height(ObjsCt->pc_otg_slider, 205);
    lv_obj_set_x(ObjsCt->pc_otg_slider, 0);
    lv_obj_set_y(ObjsCt->pc_otg_slider, -3);
    lv_obj_set_align(ObjsCt->pc_otg_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->pc_otg_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pc_otg_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pc_otg_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->pc_otg_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->pc_otg_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pc_otg_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->pc_otg_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->pc_otg_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->pc_otg_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->pc_otg_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    lv_obj_set_style_radius(ObjsCt->pc_otg_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pc_otg_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pc_otg_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->pc_otg_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->pc_otg_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pc_otg_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->pc_otg_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pc_otg_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pc_otg_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->pc_otg_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->pc_otg_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pc_otg_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->pc_otg_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->pc_otg_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->pc_otg_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->pc_otg_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->pc_otg_bar_left = lv_img_create(ObjsCt->pc_otg_meter_group);
    lv_img_set_src(ObjsCt->pc_otg_bar_left, &level2_bg);
    lv_obj_set_width(ObjsCt->pc_otg_bar_left, 15);
    lv_obj_set_height(ObjsCt->pc_otg_bar_left, 211);
    lv_obj_set_x(ObjsCt->pc_otg_bar_left, -12);
    lv_obj_set_y(ObjsCt->pc_otg_bar_left, 0);
    lv_obj_set_align(ObjsCt->pc_otg_bar_left, LV_ALIGN_CENTER);

    ObjsCt->pc_otg_bar_left_fg_mask = lv_obj_create(ObjsCt->pc_otg_meter_group);
    lv_obj_set_width(ObjsCt->pc_otg_bar_left_fg_mask, 15);
    lv_obj_set_height(ObjsCt->pc_otg_bar_left_fg_mask, 0);
    lv_obj_set_x(ObjsCt->pc_otg_bar_left_fg_mask, -12);
    lv_obj_set_y(ObjsCt->pc_otg_bar_left_fg_mask, -4);
    lv_obj_set_align(ObjsCt->pc_otg_bar_left_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->pc_otg_bar_left_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->pc_otg_bar_left_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pc_otg_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->pc_otg_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pc_otg_bar_left_fg = lv_img_create(ObjsCt->pc_otg_bar_left_fg_mask);
    lv_img_set_src(ObjsCt->pc_otg_bar_left_fg, &level_2);
    lv_obj_set_width(ObjsCt->pc_otg_bar_left_fg, 15);
    lv_obj_set_height(ObjsCt->pc_otg_bar_left_fg, 211);
    lv_obj_align(ObjsCt->pc_otg_bar_left_fg, LV_ALIGN_BOTTOM_MID, 0, 0);
    ObjsCt->pc_otg_label_plus10 = lv_label_create(ObjsCt->pc_otg_meter_group);
    lv_obj_set_width(ObjsCt->pc_otg_label_plus10, 30);
    lv_obj_set_height(ObjsCt->pc_otg_label_plus10, 15);
    lv_obj_set_x(ObjsCt->pc_otg_label_plus10, 12);
    lv_obj_set_y(ObjsCt->pc_otg_label_plus10, -98);
    lv_obj_set_align(ObjsCt->pc_otg_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pc_otg_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->pc_otg_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pc_otg_label_plus6 = lv_label_create(ObjsCt->pc_otg_meter_group);
    lv_obj_set_width(ObjsCt->pc_otg_label_plus6, 30);
    lv_obj_set_height(ObjsCt->pc_otg_label_plus6, 15);
    lv_obj_set_x(ObjsCt->pc_otg_label_plus6, 12);
    lv_obj_set_y(ObjsCt->pc_otg_label_plus6, -68);
    lv_obj_set_align(ObjsCt->pc_otg_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pc_otg_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->pc_otg_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pc_otg_label_0db = lv_label_create(ObjsCt->pc_otg_meter_group);
    lv_obj_set_width(ObjsCt->pc_otg_label_0db, 30);
    lv_obj_set_height(ObjsCt->pc_otg_label_0db, 15);
    lv_obj_set_x(ObjsCt->pc_otg_label_0db, 12);
    lv_obj_set_y(ObjsCt->pc_otg_label_0db, -37);
    lv_obj_set_align(ObjsCt->pc_otg_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pc_otg_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->pc_otg_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pc_otg_label_minus10 = lv_label_create(ObjsCt->pc_otg_meter_group);
    lv_obj_set_width(ObjsCt->pc_otg_label_minus10, 30);
    lv_obj_set_height(ObjsCt->pc_otg_label_minus10, 15);
    lv_obj_set_x(ObjsCt->pc_otg_label_minus10, 12);
    lv_obj_set_y(ObjsCt->pc_otg_label_minus10, 0);
    lv_obj_set_align(ObjsCt->pc_otg_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pc_otg_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->pc_otg_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pc_otg_label_minus30 = lv_label_create(ObjsCt->pc_otg_meter_group);
    lv_obj_set_width(ObjsCt->pc_otg_label_minus30, 30);
    lv_obj_set_height(ObjsCt->pc_otg_label_minus30, 15);
    lv_obj_set_x(ObjsCt->pc_otg_label_minus30, 12);
    lv_obj_set_y(ObjsCt->pc_otg_label_minus30, 49);
    lv_obj_set_align(ObjsCt->pc_otg_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pc_otg_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->pc_otg_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pc_otg_label_minus50 = lv_label_create(ObjsCt->pc_otg_meter_group);
    lv_obj_set_width(ObjsCt->pc_otg_label_minus50, 30);
    lv_obj_set_height(ObjsCt->pc_otg_label_minus50, 15);
    lv_obj_set_x(ObjsCt->pc_otg_label_minus50, 12);
    lv_obj_set_y(ObjsCt->pc_otg_label_minus50, 99);
    lv_obj_set_align(ObjsCt->pc_otg_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pc_otg_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->pc_otg_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pc_otg_bar_right = lv_img_create(ObjsCt->pc_otg_meter_group);
    lv_img_set_src(ObjsCt->pc_otg_bar_right, &level2_bg);
    lv_obj_set_width(ObjsCt->pc_otg_bar_right, 15);
    lv_obj_set_height(ObjsCt->pc_otg_bar_right, 211);
    lv_obj_set_x(ObjsCt->pc_otg_bar_right, 27);
    lv_obj_set_y(ObjsCt->pc_otg_bar_right, 0);
    lv_obj_set_align(ObjsCt->pc_otg_bar_right, LV_ALIGN_CENTER);

    ObjsCt->pc_otg_bar_right_fg_mask = lv_obj_create(ObjsCt->pc_otg_meter_group);
    lv_obj_set_width(ObjsCt->pc_otg_bar_right_fg_mask, 15);
    lv_obj_set_height(ObjsCt->pc_otg_bar_right_fg_mask, 0);
    lv_obj_set_x(ObjsCt->pc_otg_bar_right_fg_mask, 27);
    lv_obj_set_y(ObjsCt->pc_otg_bar_right_fg_mask, -4);
    lv_obj_set_align(ObjsCt->pc_otg_bar_right_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->pc_otg_bar_right_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->pc_otg_bar_right_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pc_otg_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->pc_otg_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pc_otg_bar_right_fg = lv_img_create(ObjsCt->pc_otg_bar_right_fg_mask);
    lv_img_set_src(ObjsCt->pc_otg_bar_right_fg, &level_2);
    lv_obj_set_width(ObjsCt->pc_otg_bar_right_fg, 15);
    lv_obj_set_height(ObjsCt->pc_otg_bar_right_fg, 211);
    lv_obj_align(ObjsCt->pc_otg_bar_right_fg, LV_ALIGN_BOTTOM_MID, 0, 0);

    ObjsCt->pc_otg_title_box = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->pc_otg_title_box, 56);
    lv_obj_set_height(ObjsCt->pc_otg_title_box, 50);
    lv_obj_set_x(ObjsCt->pc_otg_title_box, 60);
    lv_obj_set_y(ObjsCt->pc_otg_title_box, 128);
    lv_obj_set_align(ObjsCt->pc_otg_title_box, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->pc_otg_title_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->pc_otg_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pc_otg_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pc_otg_title_box, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pc_otg_title_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ObjsCt->pc_otg_title_box, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(ObjsCt->pc_otg_title_box);
    ObjsCt->pc_otg_title_lr = lv_label_create(ObjsCt->pc_otg_title_box);
    lv_obj_set_width(ObjsCt->pc_otg_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->pc_otg_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->pc_otg_title_lr, -1);
    lv_obj_set_y(ObjsCt->pc_otg_title_lr, -10);
    lv_obj_set_align(ObjsCt->pc_otg_title_lr, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pc_otg_title_lr, "L       R");
    lv_obj_set_style_text_font(ObjsCt->pc_otg_title_lr, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pc_otg_title_name = lv_label_create(ObjsCt->pc_otg_title_box);
    lv_obj_set_width(ObjsCt->pc_otg_title_name, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->pc_otg_title_name, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->pc_otg_title_name, -2);
    lv_obj_set_y(ObjsCt->pc_otg_title_name, 9);
    lv_obj_set_align(ObjsCt->pc_otg_title_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pc_otg_title_name, "PC/OTG");
    lv_obj_set_style_text_font(ObjsCt->pc_otg_title_name, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->main_meter_group = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->main_meter_group, 71);
    lv_obj_set_height(ObjsCt->main_meter_group, 220);
    lv_obj_set_x(ObjsCt->main_meter_group, 125);
    lv_obj_set_y(ObjsCt->main_meter_group, -11);
    lv_obj_set_align(ObjsCt->main_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->main_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->main_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->main_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->main_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->main_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

     ObjsCt->main_slider_panel = lv_obj_create(ObjsCt->main_meter_group);  // 移入PFL meter group
    lv_obj_set_width(ObjsCt->main_slider_panel, 11);
    lv_obj_set_height(ObjsCt->main_slider_panel, 220);
    lv_obj_set_x(ObjsCt->main_slider_panel, -28);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->main_slider_panel, 0);
    lv_obj_set_align(ObjsCt->main_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->main_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->main_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->main_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->main_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->main_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->main_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->main_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->main_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->main_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->main_slider = lv_slider_create(ObjsCt->main_slider_panel);
    lv_slider_set_range(ObjsCt->main_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->main_slider, ArgsCt.main_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->main_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->main_slider, ArgsCt.main_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->main_slider, 7);
    lv_obj_set_height(ObjsCt->main_slider, 205);
    lv_obj_set_x(ObjsCt->main_slider, 0);
    lv_obj_set_y(ObjsCt->main_slider, -3);
    lv_obj_set_align(ObjsCt->main_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->main_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->main_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->main_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->main_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->main_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->main_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->main_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->main_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->main_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->main_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    lv_obj_set_style_radius(ObjsCt->main_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->main_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->main_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->main_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->main_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->main_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->main_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->main_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->main_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->main_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->main_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->main_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->main_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->main_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->main_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->main_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->main_bar_left = lv_img_create(ObjsCt->main_meter_group);
    lv_img_set_src(ObjsCt->main_bar_left, &level2_bg);
    lv_obj_set_width(ObjsCt->main_bar_left, 15);
    lv_obj_set_height(ObjsCt->main_bar_left, 211);
    lv_obj_set_x(ObjsCt->main_bar_left, -12);
    lv_obj_set_y(ObjsCt->main_bar_left, 0);
    lv_obj_set_align(ObjsCt->main_bar_left, LV_ALIGN_CENTER);

    ObjsCt->main_bar_left_fg_mask = lv_obj_create(ObjsCt->main_meter_group);
    lv_obj_set_width(ObjsCt->main_bar_left_fg_mask, 15);
    lv_obj_set_height(ObjsCt->main_bar_left_fg_mask, 0);
    lv_obj_set_x(ObjsCt->main_bar_left_fg_mask, -12);
    lv_obj_set_y(ObjsCt->main_bar_left_fg_mask, -4);
    lv_obj_set_align(ObjsCt->main_bar_left_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->main_bar_left_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->main_bar_left_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->main_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->main_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->main_bar_left_fg = lv_img_create(ObjsCt->main_bar_left_fg_mask);
    lv_img_set_src(ObjsCt->main_bar_left_fg, &level_2);
    lv_obj_set_width(ObjsCt->main_bar_left_fg, 15);
    lv_obj_set_height(ObjsCt->main_bar_left_fg, 211);
    lv_obj_align(ObjsCt->main_bar_left_fg, LV_ALIGN_BOTTOM_MID, 0, 0);
    ObjsCt->main_label_plus10 = lv_label_create(ObjsCt->main_meter_group);
    lv_obj_set_width(ObjsCt->main_label_plus10, 30);
    lv_obj_set_height(ObjsCt->main_label_plus10, 15);
    lv_obj_set_x(ObjsCt->main_label_plus10, 12);
    lv_obj_set_y(ObjsCt->main_label_plus10, -98);
    lv_obj_set_align(ObjsCt->main_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->main_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->main_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->main_label_plus6 = lv_label_create(ObjsCt->main_meter_group);
    lv_obj_set_width(ObjsCt->main_label_plus6, 30);
    lv_obj_set_height(ObjsCt->main_label_plus6, 15);
    lv_obj_set_x(ObjsCt->main_label_plus6, 12);
    lv_obj_set_y(ObjsCt->main_label_plus6, -68);
    lv_obj_set_align(ObjsCt->main_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->main_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->main_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->main_label_0db = lv_label_create(ObjsCt->main_meter_group);
    lv_obj_set_width(ObjsCt->main_label_0db, 30);
    lv_obj_set_height(ObjsCt->main_label_0db, 15);
    lv_obj_set_x(ObjsCt->main_label_0db, 12);
    lv_obj_set_y(ObjsCt->main_label_0db, -37);
    lv_obj_set_align(ObjsCt->main_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->main_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->main_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->main_label_minus10 = lv_label_create(ObjsCt->main_meter_group);
    lv_obj_set_width(ObjsCt->main_label_minus10, 30);
    lv_obj_set_height(ObjsCt->main_label_minus10, 15);
    lv_obj_set_x(ObjsCt->main_label_minus10, 12);
    lv_obj_set_y(ObjsCt->main_label_minus10, 0);
    lv_obj_set_align(ObjsCt->main_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->main_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->main_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->main_label_minus30 = lv_label_create(ObjsCt->main_meter_group);
    lv_obj_set_width(ObjsCt->main_label_minus30, 30);
    lv_obj_set_height(ObjsCt->main_label_minus30, 15);
    lv_obj_set_x(ObjsCt->main_label_minus30, 12);
    lv_obj_set_y(ObjsCt->main_label_minus30, 49);
    lv_obj_set_align(ObjsCt->main_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->main_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->main_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->main_label_minus50 = lv_label_create(ObjsCt->main_meter_group);
    lv_obj_set_width(ObjsCt->main_label_minus50, 30);
    lv_obj_set_height(ObjsCt->main_label_minus50, 15);
    lv_obj_set_x(ObjsCt->main_label_minus50, 12);
    lv_obj_set_y(ObjsCt->main_label_minus50, 99);
    lv_obj_set_align(ObjsCt->main_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->main_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->main_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->main_bar_right = lv_img_create(ObjsCt->main_meter_group);
    lv_img_set_src(ObjsCt->main_bar_right, &level2_bg);
    lv_obj_set_width(ObjsCt->main_bar_right, 15);
    lv_obj_set_height(ObjsCt->main_bar_right, 211);
    lv_obj_set_x(ObjsCt->main_bar_right, 27);
    lv_obj_set_y(ObjsCt->main_bar_right, 0);
    lv_obj_set_align(ObjsCt->main_bar_right, LV_ALIGN_CENTER);

    ObjsCt->main_bar_right_fg_mask = lv_obj_create(ObjsCt->main_meter_group);
    lv_obj_set_width(ObjsCt->main_bar_right_fg_mask, 15);
    lv_obj_set_height(ObjsCt->main_bar_right_fg_mask, 0);
    lv_obj_set_x(ObjsCt->main_bar_right_fg_mask, 27);
    lv_obj_set_y(ObjsCt->main_bar_right_fg_mask, -4);
    lv_obj_set_align(ObjsCt->main_bar_right_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->main_bar_right_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->main_bar_right_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->main_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->main_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->main_bar_right_fg = lv_img_create(ObjsCt->main_bar_right_fg_mask);
    lv_img_set_src(ObjsCt->main_bar_right_fg, &level_2);
    lv_obj_set_width(ObjsCt->main_bar_right_fg, 15);
    lv_obj_set_height(ObjsCt->main_bar_right_fg, 211);
    lv_obj_align(ObjsCt->main_bar_right_fg, LV_ALIGN_BOTTOM_MID, 0, 0);

    ObjsCt->main_title_box = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->main_title_box, 56);
    lv_obj_set_height(ObjsCt->main_title_box, 50);
    lv_obj_set_x(ObjsCt->main_title_box, 133);
    lv_obj_set_y(ObjsCt->main_title_box, 127);
    lv_obj_set_align(ObjsCt->main_title_box, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->main_title_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->main_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->main_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->main_title_box, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->main_title_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ObjsCt->main_title_box, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(ObjsCt->main_title_box);
    ObjsCt->main_title_lr = lv_label_create(ObjsCt->main_title_box);
    lv_obj_set_width(ObjsCt->main_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->main_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->main_title_lr, -1);
    lv_obj_set_y(ObjsCt->main_title_lr, -10);
    lv_obj_set_align(ObjsCt->main_title_lr, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->main_title_lr, "L       R");
    lv_obj_set_style_text_font(ObjsCt->main_title_lr, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->main_title_name = lv_label_create(ObjsCt->main_title_box);
    lv_obj_set_width(ObjsCt->main_title_name, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->main_title_name, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->main_title_name, -2);
    lv_obj_set_y(ObjsCt->main_title_name, 9);
    lv_obj_set_align(ObjsCt->main_title_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->main_title_name, "MAIN");
    lv_obj_set_style_text_font(ObjsCt->main_title_name, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pfl_meter_group = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->pfl_meter_group, 71);
    lv_obj_set_height(ObjsCt->pfl_meter_group, 220);
    lv_obj_set_x(ObjsCt->pfl_meter_group, 197);
    lv_obj_set_y(ObjsCt->pfl_meter_group, -11);
    lv_obj_set_align(ObjsCt->pfl_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->pfl_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->pfl_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pfl_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pfl_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pfl_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pfl_slider_panel = lv_obj_create(ObjsCt->pfl_meter_group);  // 移入PFL meter group
    lv_obj_set_width(ObjsCt->pfl_slider_panel, 11);
    lv_obj_set_height(ObjsCt->pfl_slider_panel, 220);
    lv_obj_set_x(ObjsCt->pfl_slider_panel, -28);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->pfl_slider_panel, 0);
    lv_obj_set_align(ObjsCt->pfl_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->pfl_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->pfl_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->pfl_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->pfl_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pfl_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->pfl_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->pfl_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->pfl_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->pfl_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ObjsCt->pfl_slider = lv_slider_create(ObjsCt->pfl_slider_panel);
    lv_slider_set_range(ObjsCt->pfl_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->pfl_slider, ArgsCt.pfl_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->pfl_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->pfl_slider, ArgsCt.pfl_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->pfl_slider, 7);
    lv_obj_set_height(ObjsCt->pfl_slider, 205);
    lv_obj_set_x(ObjsCt->pfl_slider, 0);
    lv_obj_set_y(ObjsCt->pfl_slider, -3);
    lv_obj_set_align(ObjsCt->pfl_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->pfl_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pfl_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pfl_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->pfl_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->pfl_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pfl_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->pfl_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->pfl_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->pfl_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->pfl_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    lv_obj_set_style_radius(ObjsCt->pfl_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pfl_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pfl_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->pfl_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->pfl_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pfl_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->pfl_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pfl_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pfl_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->pfl_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->pfl_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pfl_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->pfl_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->pfl_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->pfl_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->pfl_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->pfl_bar_left = lv_img_create(ObjsCt->pfl_meter_group);
    lv_img_set_src(ObjsCt->pfl_bar_left, &level2_bg);
    lv_obj_set_width(ObjsCt->pfl_bar_left, 15);
    lv_obj_set_height(ObjsCt->pfl_bar_left, 211);
    lv_obj_set_x(ObjsCt->pfl_bar_left, -12);
    lv_obj_set_y(ObjsCt->pfl_bar_left, 0);
    lv_obj_set_align(ObjsCt->pfl_bar_left, LV_ALIGN_CENTER);

    ObjsCt->pfl_bar_left_fg_mask = lv_obj_create(ObjsCt->pfl_meter_group);
    lv_obj_set_width(ObjsCt->pfl_bar_left_fg_mask, 15);
    lv_obj_set_height(ObjsCt->pfl_bar_left_fg_mask, 0);
    lv_obj_set_x(ObjsCt->pfl_bar_left_fg_mask, -12);
    lv_obj_set_y(ObjsCt->pfl_bar_left_fg_mask, -4);
    lv_obj_set_align(ObjsCt->pfl_bar_left_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->pfl_bar_left_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->pfl_bar_left_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pfl_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->pfl_bar_left_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pfl_bar_left_fg = lv_img_create(ObjsCt->pfl_bar_left_fg_mask);
    lv_img_set_src(ObjsCt->pfl_bar_left_fg, &level_2);
    lv_obj_set_width(ObjsCt->pfl_bar_left_fg, 15);
    lv_obj_set_height(ObjsCt->pfl_bar_left_fg, 211);
    lv_obj_align(ObjsCt->pfl_bar_left_fg, LV_ALIGN_BOTTOM_MID, 0, 0);
    ObjsCt->pfl_label_plus10 = lv_label_create(ObjsCt->pfl_meter_group);
    lv_obj_set_width(ObjsCt->pfl_label_plus10, 30);
    lv_obj_set_height(ObjsCt->pfl_label_plus10, 15);
    lv_obj_set_x(ObjsCt->pfl_label_plus10, 12);
    lv_obj_set_y(ObjsCt->pfl_label_plus10, -98);
    lv_obj_set_align(ObjsCt->pfl_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pfl_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->pfl_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pfl_label_plus6 = lv_label_create(ObjsCt->pfl_meter_group);
    lv_obj_set_width(ObjsCt->pfl_label_plus6, 30);
    lv_obj_set_height(ObjsCt->pfl_label_plus6, 15);
    lv_obj_set_x(ObjsCt->pfl_label_plus6, 12);
    lv_obj_set_y(ObjsCt->pfl_label_plus6, -68);
    lv_obj_set_align(ObjsCt->pfl_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pfl_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->pfl_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pfl_label_0db = lv_label_create(ObjsCt->pfl_meter_group);
    lv_obj_set_width(ObjsCt->pfl_label_0db, 30);
    lv_obj_set_height(ObjsCt->pfl_label_0db, 15);
    lv_obj_set_x(ObjsCt->pfl_label_0db, 12);
    lv_obj_set_y(ObjsCt->pfl_label_0db, -37);
    lv_obj_set_align(ObjsCt->pfl_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pfl_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->pfl_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pfl_label_minus10 = lv_label_create(ObjsCt->pfl_meter_group);
    lv_obj_set_width(ObjsCt->pfl_label_minus10, 30);
    lv_obj_set_height(ObjsCt->pfl_label_minus10, 15);
    lv_obj_set_x(ObjsCt->pfl_label_minus10, 12);
    lv_obj_set_y(ObjsCt->pfl_label_minus10, -1);
    lv_obj_set_align(ObjsCt->pfl_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pfl_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->pfl_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pfl_label_minus30 = lv_label_create(ObjsCt->pfl_meter_group);
    lv_obj_set_width(ObjsCt->pfl_label_minus30, 30);
    lv_obj_set_height(ObjsCt->pfl_label_minus30, 15);
    lv_obj_set_x(ObjsCt->pfl_label_minus30, 12);
    lv_obj_set_y(ObjsCt->pfl_label_minus30, 48);
    lv_obj_set_align(ObjsCt->pfl_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pfl_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->pfl_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pfl_label_minus50 = lv_label_create(ObjsCt->pfl_meter_group);
    lv_obj_set_width(ObjsCt->pfl_label_minus50, 30);
    lv_obj_set_height(ObjsCt->pfl_label_minus50, 15);
    lv_obj_set_x(ObjsCt->pfl_label_minus50, 12);
    lv_obj_set_y(ObjsCt->pfl_label_minus50, 99);
    lv_obj_set_align(ObjsCt->pfl_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pfl_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->pfl_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pfl_bar_right = lv_img_create(ObjsCt->pfl_meter_group);
    lv_img_set_src(ObjsCt->pfl_bar_right, &level2_bg);
    lv_obj_set_width(ObjsCt->pfl_bar_right, 15);
    lv_obj_set_height(ObjsCt->pfl_bar_right, 211);
    lv_obj_set_x(ObjsCt->pfl_bar_right, 27);
    lv_obj_set_y(ObjsCt->pfl_bar_right, 0);
    lv_obj_set_align(ObjsCt->pfl_bar_right, LV_ALIGN_CENTER);

    ObjsCt->pfl_bar_right_fg_mask = lv_obj_create(ObjsCt->pfl_meter_group);
    lv_obj_set_width(ObjsCt->pfl_bar_right_fg_mask, 15);
    lv_obj_set_height(ObjsCt->pfl_bar_right_fg_mask, 0);
    lv_obj_set_x(ObjsCt->pfl_bar_right_fg_mask, 27);
    lv_obj_set_y(ObjsCt->pfl_bar_right_fg_mask, -4);
    lv_obj_set_align(ObjsCt->pfl_bar_right_fg_mask, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ObjsCt->pfl_bar_right_fg_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ObjsCt->pfl_bar_right_fg_mask, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pfl_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ObjsCt->pfl_bar_right_fg_mask, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->pfl_bar_right_fg = lv_img_create(ObjsCt->pfl_bar_right_fg_mask);
    lv_img_set_src(ObjsCt->pfl_bar_right_fg, &level_2);
    lv_obj_set_width(ObjsCt->pfl_bar_right_fg, 15);
    lv_obj_set_height(ObjsCt->pfl_bar_right_fg, 211);
    lv_obj_align(ObjsCt->pfl_bar_right_fg, LV_ALIGN_BOTTOM_MID, 0, 0);
    ObjsCt->pfl_title_box = lv_obj_create(ObjsCt->Spec_obj);
    lv_obj_set_width(ObjsCt->pfl_title_box, 56);
    lv_obj_set_height(ObjsCt->pfl_title_box, 50);
    lv_obj_set_x(ObjsCt->pfl_title_box, 204);
    lv_obj_set_y(ObjsCt->pfl_title_box, 128);
    lv_obj_set_align(ObjsCt->pfl_title_box, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->pfl_title_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->pfl_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->pfl_title_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->pfl_title_box, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->pfl_title_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ObjsCt->pfl_title_box, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(ObjsCt->pfl_title_box);
    ObjsCt->pfl_title_lr = lv_label_create(ObjsCt->pfl_title_box);
    lv_obj_set_width(ObjsCt->pfl_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->pfl_title_lr, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->pfl_title_lr, -1);
    lv_obj_set_y(ObjsCt->pfl_title_lr, -10);
    lv_obj_set_align(ObjsCt->pfl_title_lr, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pfl_title_lr, "L       R");
    lv_obj_set_style_text_font(ObjsCt->pfl_title_lr, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    ObjsCt->pfl_title_name = lv_label_create(ObjsCt->pfl_title_box);
    lv_obj_set_width(ObjsCt->pfl_title_name, LV_SIZE_CONTENT);
    lv_obj_set_height(ObjsCt->pfl_title_name, LV_SIZE_CONTENT);
    lv_obj_set_x(ObjsCt->pfl_title_name, -2);
    lv_obj_set_y(ObjsCt->pfl_title_name, 9);
    lv_obj_set_align(ObjsCt->pfl_title_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->pfl_title_name, "PFL");
    lv_obj_set_style_text_font(ObjsCt->pfl_title_name, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);


}

void ui_page_Spec_start(void)
{
    if(ObjsCt == NULL)
        return;

    ArgsCt.focus_target = 0;
    ArgsCt.edit_mode = 0;

    if(ArgsCt.timer == NULL)
    {
        ArgsCt.timer = lv_timer_create(timer_cb, 40, NULL);
    }
}

void ui_page_Spec_exit(uint8_t del)
{
    if(ArgsCt.timer)
    {
        lv_timer_del(ArgsCt.timer);
        ArgsCt.timer = NULL;
    }
    
    if(del)
    {
    }
    else
    {
    }
}

void ui_page_Spec_clean(void)
{
    lv_mem_free(ObjsCt);
    ObjsCt = NULL;
}

void ui_page_Spec_request(uint32_t delay)
{
    switch(ui_pmanagerCt.id_tar)
    {
        case UI_PAGE_SEL_7:
            break;
        default:
            ui_pmanager_switch(UI_PAGE_SEL_7, delay, true);
            break;
    }
}

void ui_page_Spec_encoder_up(void)
{
    if(ObjsCt == NULL) return;
    
    if(ArgsCt.edit_mode == 0)
    {
        switch(ArgsCt.focus_target)
        {
            case 0:
                lv_obj_set_style_border_color(ObjsCt->mic1_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->mic2_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->vol34_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->usb_bt_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: lv_obj_set_style_border_color(ObjsCt->pc_otg_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: lv_obj_set_style_border_color(ObjsCt->main_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: lv_obj_set_style_border_color(ObjsCt->pfl_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
        }
        
        if(ArgsCt.focus_target < 6)
        {
            ArgsCt.focus_target++;
        }
        else
        {
            ArgsCt.focus_target = 0;
        }
        
        switch(ArgsCt.focus_target)
        {
            case 0:
                lv_obj_set_style_border_color(ObjsCt->mic1_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->mic2_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->vol34_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->usb_bt_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: lv_obj_set_style_border_color(ObjsCt->pc_otg_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: lv_obj_set_style_border_color(ObjsCt->main_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: lv_obj_set_style_border_color(ObjsCt->pfl_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
        }
        
    }
    else 
    {
        switch(ArgsCt.focus_target)
        {
            case 0:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->mic1_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->mic1_slider, current_val, LV_ANIM_ON);
                    ArgsCt.mic1_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 1:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->mic2_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->mic2_slider, current_val, LV_ANIM_ON);
                    ArgsCt.mic2_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 2:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->vol34_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->vol34_slider, current_val, LV_ANIM_ON);
                    ArgsCt.vol34_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 3:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->usb_bt_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->usb_bt_slider, current_val, LV_ANIM_ON);
                    ArgsCt.usb_bt_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 4:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->pc_otg_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->pc_otg_slider, current_val, LV_ANIM_ON);
                    ArgsCt.pc_otg_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 5:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->main_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->main_slider, current_val, LV_ANIM_ON);
                    ArgsCt.main_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 6:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->pfl_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->pfl_slider, current_val, LV_ANIM_ON);
                    ArgsCt.pfl_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
        }
    }
}

void ui_page_Spec_encoder_down(void)
{
    if(ObjsCt == NULL) return; 
    
    if(ArgsCt.edit_mode == 0)
    {
        switch(ArgsCt.focus_target)
        {
            case 0:
                lv_obj_set_style_border_color(ObjsCt->mic1_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->mic2_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->vol34_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->usb_bt_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: lv_obj_set_style_border_color(ObjsCt->pc_otg_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: lv_obj_set_style_border_color(ObjsCt->main_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: lv_obj_set_style_border_color(ObjsCt->pfl_slider_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
        }
        
        if(ArgsCt.focus_target > 0)
        {
            ArgsCt.focus_target--;
        }
        else
        {
            ArgsCt.focus_target = 6;
        }
        
        switch(ArgsCt.focus_target)
        {
            case 0:
                lv_obj_set_style_border_color(ObjsCt->mic1_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->mic2_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->vol34_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->usb_bt_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: lv_obj_set_style_border_color(ObjsCt->pc_otg_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: lv_obj_set_style_border_color(ObjsCt->main_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: lv_obj_set_style_border_color(ObjsCt->pfl_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
        }
        
    }
    else
    {
        switch(ArgsCt.focus_target)
        {
            case 0:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->mic1_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->mic1_slider, current_val, LV_ANIM_ON);
                    ArgsCt.mic1_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 1:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->mic2_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->mic2_slider, current_val, LV_ANIM_ON);
                    ArgsCt.mic2_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 2:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->vol34_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->vol34_slider, current_val, LV_ANIM_ON);
                    ArgsCt.vol34_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 3:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->usb_bt_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->usb_bt_slider, current_val, LV_ANIM_ON);
                    ArgsCt.usb_bt_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 4:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->pc_otg_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->pc_otg_slider, current_val, LV_ANIM_ON);
                    ArgsCt.pc_otg_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 5:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->main_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->main_slider, current_val, LV_ANIM_ON);
                    ArgsCt.main_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 6:
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->pfl_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->pfl_slider, current_val, LV_ANIM_ON);
                    ArgsCt.pfl_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;

        }
    }
}

void ui_page_Spec_encoder_ok(void)
{
    if(ObjsCt == NULL) return;
    
    ArgsCt.edit_mode = !ArgsCt.edit_mode;
    
    lv_color_t text_color = ArgsCt.edit_mode ? lv_color_hex(0xFFA500) : lv_color_hex(0x00FF00);
    
    switch(ArgsCt.focus_target)
    {
        case 0:
                lv_obj_set_style_border_color(ObjsCt->mic1_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->mic2_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->vol34_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->usb_bt_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: 
                lv_obj_set_style_border_color(ObjsCt->pc_otg_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: 
                lv_obj_set_style_border_color(ObjsCt->main_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: 
                lv_obj_set_style_border_color(ObjsCt->pfl_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;    
    }
}