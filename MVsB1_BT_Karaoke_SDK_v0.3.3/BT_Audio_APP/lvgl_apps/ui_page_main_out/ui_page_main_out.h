#ifndef _UI_PAGE_MAIN_OUT_H_
#define _UI_PAGE_MAIN_OUT_H_


/*################################### include ###################################*/
#include "type.h"
#include "lvgl.h"

/*################################### define/enum ###################################*/

/*################################### typedef ###################################*/
typedef struct _MainOutObjsContext_{
    lv_obj_t *MainOut_obj;
    lv_obj_t *ui_eq_panel;
    lv_obj_t *ui_eq_63_slider_panel;
    lv_obj_t *ui_eq_160_slider_panel;
    lv_obj_t *ui_eq_400_slider_panel;
    lv_obj_t *ui_eq_1000_slider_panel;
    lv_obj_t *ui_eq_2500_slider_panel;
    lv_obj_t *ui_eq_6300_slider_panel;
    lv_obj_t *ui_eq_16k_slider_panel;
    lv_obj_t *ui_eq_63_slider;
    lv_obj_t *ui_eq_160_slider;
    lv_obj_t *ui_eq_400_slider;
    lv_obj_t *ui_eq_1000_slider;
    lv_obj_t *ui_eq_2500_slider;
    lv_obj_t *ui_eq_6300_slider;
    lv_obj_t *ui_eq_16k_slider;
    lv_obj_t *ui_eq_63Hz; 
    lv_obj_t *ui_eq_160Hz; 
    lv_obj_t *ui_eq_400Hz; 
    lv_obj_t *ui_eq_1000Hz; 
    lv_obj_t *ui_eq_2500Hz; 
    lv_obj_t *ui_eq_6300Hz; 
    lv_obj_t *ui_eq_16kHz; 
    lv_obj_t *ui_eq_12db;
    lv_obj_t *ui_eq_6db;
    lv_obj_t *ui_eq_0db;
    lv_obj_t *ui_eq_neg6db;
    lv_obj_t *ui_eq_neg12db;
    lv_obj_t *ui_main_out_panel; 
    lv_obj_t *ui_main_out_name; 
    lv_obj_t *ui_gain_args; 
    lv_obj_t *ui_eq_level_panel; 
    lv_obj_t *ui_eq_main_vol_panel; 
    lv_obj_t *ui_eq_main_vol_slider; 
    // lv_obj_t *ui_eq_main_level_panel;  // 已移除
    lv_obj_t *ui_eq_pfl_vol_panel; 
    lv_obj_t *ui_eq_pfl_vol_slider; 
    // lv_obj_t *ui_eq_pfl_level_panel;   // 已移除
    lv_obj_t *ui_eq_level_lr_name; 
    lv_obj_t *ui_eq_level_name_panel; 
    lv_obj_t *ui_eq_level_name;
    lv_obj_t *ui_eq_level_pfl_name;
    
    // Main电平显示组件
    lv_obj_t *ui_main_meter_group;
    // lv_obj_t *ui_main_track_box;    // 已移除
    // lv_obj_t *ui_main_midline;      // 已移除
    lv_obj_t *ui_main_tick_top;
    lv_obj_t *ui_main_tick_bottom;
    lv_obj_t *ui_main_bar_left;
    lv_obj_t *ui_main_bar_right;
    lv_obj_t *ui_main_label_plus10;
    lv_obj_t *ui_main_label_plus6;
    lv_obj_t *ui_main_label_0db;
    lv_obj_t *ui_main_label_minus10;
    lv_obj_t *ui_main_label_minus30;
    lv_obj_t *ui_main_label_minus50;
    lv_obj_t *ui_main_title_box;
    lv_obj_t *ui_main_title_lr;
    lv_obj_t *ui_main_title_name;
    
    // PFL电平显示组件
    lv_obj_t *ui_pfl_meter_group;
    // lv_obj_t *ui_pfl_track_box;     // 已移除
    // lv_obj_t *ui_pfl_midline;       // 已移除
    lv_obj_t *ui_pfl_tick_top;
    lv_obj_t *ui_pfl_tick_bottom;
    lv_obj_t *ui_pfl_bar_left;
    lv_obj_t *ui_pfl_bar_right;
    lv_obj_t *ui_pfl_label_plus10;
    lv_obj_t *ui_pfl_label_plus6;
    lv_obj_t *ui_pfl_label_0db;
    lv_obj_t *ui_pfl_label_minus10;
    lv_obj_t *ui_pfl_label_minus30;
    lv_obj_t *ui_pfl_label_minus50;
    lv_obj_t *ui_pfl_title_box;
    lv_obj_t *ui_pfl_title_lr;
    lv_obj_t *ui_pfl_title_name;
} MainOutObjsContext;

typedef struct _MainOutArgsContext_
{
    uint8_t focus_target;
    uint8_t edit_mode;      // 编辑模式, 0=面板选择模式, 1=参数编辑模式
    
    // Slider值存储变量
    uint8_t eq_63_slider_value;     
    uint8_t eq_160_slider_value;    
    uint8_t eq_400_slider_value;    
    uint8_t eq_1000_slider_value;   
    uint8_t eq_2500_slider_value;   
    uint8_t eq_6300_slider_value;   
    uint8_t eq_16k_slider_value;    
    uint8_t main_vol_slider_value;  
    uint8_t pfl_vol_slider_value;   
} MainOutArgsContext;

/*################################### 全局变量 ###################################*/

/*################################### 全局函数 ###################################*/
void ui_page_MainOut_init(lv_obj_t *screen);
void ui_page_MainOut_start(void);
void ui_page_MainOut_exit(uint8_t del);
void ui_page_MainOut_clean(void);
void ui_page_MainOut_request(uint32_t delay);

// 编码器控制函数
void ui_page_MainOut_encoder_up(void);
void ui_page_MainOut_encoder_down(void);
void ui_page_MainOut_encoder_ok(void);

// 电平更新函数
void ui_page_MainOut_update_levels(void);
#endif