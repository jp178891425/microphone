#ifndef UI_PAGE_Dsp_H
#define UI_PAGE_Dsp_H

#ifdef __cplusplus
extern "C" {
#endif

#include "type.h"
#include "lvgl.h"
#include "ui_pmanager.h"

/*################################### 本地宏 ###################################*/

/*################################### 数据类型 ###################################*/

typedef struct _DspObjsContext_{
    lv_obj_t *Dsp_obj;
    
    // 26个Panel（Panel9-35，排除Slider容器Panel33/36/37）
    lv_obj_t *ui_panel9;
    lv_obj_t *ui_label23;
    lv_obj_t *ui_panel10;
    lv_obj_t *ui_label24;
    lv_obj_t *ui_panel11;
    lv_obj_t *ui_label25;
    lv_obj_t *ui_panel12;
    lv_obj_t *ui_label26;
    lv_obj_t *ui_panel13;
    lv_obj_t *ui_label27;
    lv_obj_t *ui_panel14;
    lv_obj_t *ui_label28;
    lv_obj_t *ui_panel15;
    lv_obj_t *ui_label29;
    lv_obj_t *ui_panel16;
    lv_obj_t *ui_label30;
    lv_obj_t *ui_panel17;
    lv_obj_t *ui_label31;
    lv_obj_t *ui_panel18;
    lv_obj_t *ui_label32;
    lv_obj_t *ui_panel19;
    lv_obj_t *ui_label33;
    lv_obj_t *ui_panel20;
    lv_obj_t *ui_label34;
    lv_obj_t *ui_panel21;
    lv_obj_t *ui_label39;
    lv_obj_t *ui_panel22;
    lv_obj_t *ui_label40;
    lv_obj_t *ui_panel23;
    lv_obj_t *ui_label41;
    lv_obj_t *ui_panel24;
    lv_obj_t *ui_label42;
    lv_obj_t *ui_panel25;
    lv_obj_t *ui_label43;
    lv_obj_t *ui_panel26;
    lv_obj_t *ui_label44;
    lv_obj_t *ui_panel27;
    lv_obj_t *ui_label45;
    lv_obj_t *ui_panel30;
    lv_obj_t *ui_label46;
    lv_obj_t *ui_panel31;
    lv_obj_t *ui_label47;
    lv_obj_t *ui_panel32;
    lv_obj_t *ui_label48;
    lv_obj_t *ui_panel34;
    lv_obj_t *ui_label49;
    lv_obj_t *ui_panel35;
    lv_obj_t *ui_label50;
    
    // Slider相关控件（只显示，不参与编码器控制）
    lv_obj_t *ui_panel33;
    lv_obj_t *ui_slider3;
    lv_obj_t *ui_label17;
    lv_obj_t *ui_label18;
    lv_obj_t *ui_panel36;
    lv_obj_t *ui_slider1;
    lv_obj_t *ui_label19;
    lv_obj_t *ui_label21;
    lv_obj_t *ui_panel37;
    lv_obj_t *ui_slider2;
    lv_obj_t *ui_label20;
    lv_obj_t *ui_label22;
} DspObjsContext;

typedef struct _DspArgsContext_
{
    uint8_t focus_target;      // 当前焦点 (0-26: 0-23=DSP panel, 24=REV slider, 25=VOL slider, 26=2000ms slider)
    uint8_t edit_mode;         // 编辑模式：0=浏览模式，1=编辑slider
    uint8_t current_dsp;       // 当前选中的DSP (0-23)
    
    // 每个DSP的参数(24种DSP × 3个参数)
    uint8_t dsp_rev[24];       // REV值：0-20
    uint8_t dsp_vol[24];       // VOL值：0-20
    uint8_t dsp_delay[24];     // Delay值：0-20 (对应0-2000ms)
} DspArgsContext;

/*################################### 全局变量 ###################################*/

/*################################### 全局函数 ###################################*/
void ui_page_Dsp_init(lv_obj_t *screen);
void ui_page_Dsp_start(void);
void ui_page_Dsp_exit(uint8_t del);
void ui_page_Dsp_clean(void);
void ui_page_Dsp_request(uint32_t delay);

// 编码器控制函数
void ui_page_Dsp_encoder_up(void);
void ui_page_Dsp_encoder_down(void);
void ui_page_Dsp_encoder_ok(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
