/*
 * @Author: wujunpeng
 * @Date: 2025-11-28 16:22:21
 * @LastEditors: Do not edit
 * @LastEditTime: 2026-02-28 18:02:05
 * @FilePath: \lvgl_digtial_v01\MVsB1_BT_Karaoke_SDK_v0.3.3\BT_Audio_APP\bt_audio_app_src\lvgl_apps\ui_page_spec\ui_page_spec.h
 */
#ifndef UI_PAGE_Spec_H
#define UI_PAGE_Spec_H

#include "type.h"
#include "lvgl.h"

typedef struct _SpecObjsContext_
{
    lv_obj_t *Spec_obj;

    lv_obj_t *mic1_meter_group;
    lv_obj_t *mic1_bar;
    lv_obj_t *mic1_bar_fg;
    lv_obj_t *mic1_bar_fg_mask;
    lv_obj_t *mic1_title_box;
    lv_obj_t *mic2_meter_group;
    lv_obj_t *reserved_panel19;
    lv_obj_t *mic2_title_box;
    lv_obj_t *vol34_bar_left;
    lv_obj_t *vol34_bar_left_fg;
    lv_obj_t *vol34_bar_left_fg_mask;
    lv_obj_t *vol34_meter_group;
    lv_obj_t *vol34_bar_right;
    lv_obj_t *vol34_bar_right_fg;
    lv_obj_t *vol34_bar_right_fg_mask;
    lv_obj_t *vol34_title_box;
    lv_obj_t *usb_bt_meter_group;
    lv_obj_t *usb_bt_bar_left;
    lv_obj_t *usb_bt_bar_left_fg;
    lv_obj_t *usb_bt_bar_left_fg_mask;
    lv_obj_t *usb_bt_bar_right;
    lv_obj_t *usb_bt_bar_right_fg;
    lv_obj_t *usb_bt_bar_right_fg_mask;
    lv_obj_t *usb_bt_title_box;
    lv_obj_t *pc_otg_meter_group;
    lv_obj_t *pc_otg_bar_left;
    lv_obj_t *pc_otg_bar_left_fg;
    lv_obj_t *pc_otg_bar_left_fg_mask;
    lv_obj_t *pc_otg_bar_right;
    lv_obj_t *pc_otg_bar_right_fg;
    lv_obj_t *pc_otg_bar_right_fg_mask;
    lv_obj_t *pc_otg_title_box;
    lv_obj_t *main_meter_group;
    lv_obj_t *main_bar_left;
    lv_obj_t *main_bar_left_fg;
    lv_obj_t *main_bar_left_fg_mask;
    lv_obj_t *main_bar_right;
    lv_obj_t *main_bar_right_fg;
    lv_obj_t *main_bar_right_fg_mask;
    lv_obj_t *main_title_box;
    lv_obj_t *pfl_meter_group;
    lv_obj_t *pfl_bar_left;
    lv_obj_t *pfl_bar_left_fg;
    lv_obj_t *pfl_bar_left_fg_mask;
    lv_obj_t *pfl_bar_right;
    lv_obj_t *pfl_bar_right_fg;
    lv_obj_t *pfl_bar_right_fg_mask;
    lv_obj_t *pfl_title_box;
    lv_obj_t *mic2_bar;
    lv_obj_t *mic2_bar_fg;
    lv_obj_t *mic2_bar_fg_mask;

    lv_obj_t *mic1_slider_panel;
    lv_obj_t *mic1_slider;
    lv_obj_t *mic1_label_plus10;
    lv_obj_t *mic1_label_plus6;
    lv_obj_t *mic1_label_0db;
    lv_obj_t *mic1_label_minus10;
    lv_obj_t *mic1_label_minus30;
    lv_obj_t *mic1_label_minus50;
    lv_obj_t *mic1_title_lr;
    lv_obj_t *mic1_title_name;
    lv_obj_t *mic2_slider_panel;
    lv_obj_t *mic2_slider;
    lv_obj_t *mic2_label_plus10;
    lv_obj_t *mic2_label_plus6;
    lv_obj_t *mic2_label_0db;
    lv_obj_t *mic2_label_minus10;
    lv_obj_t *mic2_label_minus30;
    lv_obj_t *mic2_label_minus50;
    lv_obj_t *mic2_title_lr;
    lv_obj_t *mic2_title_name;
    lv_obj_t *vol34_slider_panel;
    lv_obj_t *vol34_slider;
    lv_obj_t *vol34_label_plus10;
    lv_obj_t *vol34_label_plus6;
    lv_obj_t *vol34_label_0db;
    lv_obj_t *vol34_label_minus10;
    lv_obj_t *vol34_label_minus30;
    lv_obj_t *vol34_label_minus50;
    lv_obj_t *vol34_title_lr;
    lv_obj_t *vol34_title_name;
    lv_obj_t *usb_bt_slider_panel;
    lv_obj_t *usb_bt_slider;
    lv_obj_t *usb_bt_label_plus10;
    lv_obj_t *usb_bt_label_plus6;
    lv_obj_t *usb_bt_label_0db;
    lv_obj_t *usb_bt_label_minus10;
    lv_obj_t *usb_bt_label_minus30;
    lv_obj_t *usb_bt_label_minus50;
    lv_obj_t *usb_bt_title_lr;
    lv_obj_t *usb_bt_title_name;
    lv_obj_t *pc_otg_slider_panel;
    lv_obj_t *pc_otg_slider;
    lv_obj_t *pc_otg_label_plus10;
    lv_obj_t *pc_otg_label_plus6;
    lv_obj_t *pc_otg_label_0db;
    lv_obj_t *pc_otg_label_minus10;
    lv_obj_t *pc_otg_label_minus30;
    lv_obj_t *pc_otg_label_minus50;
    lv_obj_t *pc_otg_title_lr;
    lv_obj_t *pc_otg_title_name;
    lv_obj_t *main_slider_panel;
    lv_obj_t *main_slider;
    lv_obj_t *main_label_plus10;
    lv_obj_t *main_label_plus6;
    lv_obj_t *main_label_0db;
    lv_obj_t *main_label_minus10;
    lv_obj_t *main_label_minus30;
    lv_obj_t *main_label_minus50;
    lv_obj_t *main_title_lr;
    lv_obj_t *main_title_name;
    lv_obj_t *pfl_slider_panel;
    lv_obj_t *pfl_slider;
    lv_obj_t *pfl_label_plus10;
    lv_obj_t *pfl_label_plus6;
    lv_obj_t *pfl_label_0db;
    lv_obj_t *pfl_label_minus10;
    lv_obj_t *pfl_label_minus30;
    lv_obj_t *pfl_label_minus50;
    lv_obj_t *pfl_title_lr;
    lv_obj_t *pfl_title_name;
} SpecObjsContext;

typedef struct _SpecArgsContext_
{
    uint8_t tick;
    lv_timer_t *timer;

    uint8_t focus_target;
    uint8_t edit_mode;      // 编辑模式, 0=面板选择模式, 1=参数编辑模式
    
    uint8_t mic1_vol_slider_value;  
    uint8_t mic2_vol_slider_value;  
    uint8_t vol34_vol_slider_value;  
    uint8_t usb_bt_vol_slider_value;  
    uint8_t pc_otg_vol_slider_value;  
    uint8_t main_vol_slider_value;  
    uint8_t pfl_vol_slider_value;  
} SpecArgsContext;

void ui_page_Spec_init(lv_obj_t *screen);
void ui_page_Spec_start(void);
void ui_page_Spec_exit(uint8_t del);
void ui_page_Spec_clean(void);
void ui_page_Spec_request(uint32_t delay);

void ui_page_Spec_encoder_up(void);
void ui_page_Spec_encoder_down(void);
void ui_page_Spec_encoder_ok(void);
#endif
