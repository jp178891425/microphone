/*
 * @Author: wujunpeng
 * @Date: 2025-11-27 12:21:55
 * @LastEditors: Do not edit
 * @LastEditTime: 2026-05-17 17:46:17
 * @FilePath: \lvgl_digtial_v01\MVsB1_BT_Karaoke_SDK_v0.3.3\BT_Audio_APP\bt_audio_app_src\lvgl_apps\ui_pmanager.h
 */
#ifndef _UI_PMANGER_H_
#define _UI_PMANGER_H_

/*################################### include ###################################*/
#include "lvgl.h"
#include "ui_page_mic1/ui_page_mic1.h"
#include "ui_page_mic2/ui_page_mic2.h"
#include "ui_page_aux1/ui_page_aux1.h"
#include "ui_page_aux2/ui_page_aux2.h"
#include "ui_page_aux3/ui_page_aux3.h"
#include "ui_page_aux4/ui_page_aux4.h"
#include "ui_page_spec/ui_page_spec.h"
#include "ui_page_dsp/ui_page_dsp.h"
#include "ui_page_main_out/ui_page_main_out.h"
/*################################### define/enum ###################################*/
enum {
    UI_PAGE_NULL = -1,
    UI_PAGE_SEL_1 = 0,      // MIC1
    UI_PAGE_SEL_2,          // MIC2
    UI_PAGE_SEL_3,          // AUX1
    UI_PAGE_SEL_4,          // AUX2
    UI_PAGE_SEL_5,          // AUX3
    UI_PAGE_SEL_6,          // MAIN OUT
    UI_PAGE_SEL_7,          // SPEC
    UI_PAGE_SEL_8,          // DSP
};

/*################################### typedef ###################################*/
typedef struct _ui_PageContext_ {
    uint8_t id;
    bool auto_del;
    lv_obj_t *screen;
    
    void(*init_cb)(lv_obj_t *parent);   //包含style初始化
    void(*start_cb)(void);              
    void(*exit_cb)(uint8_t del);        //0：保持后台运行; 1：删除整个页面
    void(*clean_cb)(void);              //资源回收
}ui_PageContext;

typedef struct _ui_pmanagerContext_{
    uint8_t id_cur, id_tar;
    // uint8_t FirstLogoFinishFlag;//首次开机未完成，暂时屏蔽ui_page_xx_request()的处理，等待LOGO结束，自动跳转
}ui_pmanagerContext;
extern ui_pmanagerContext ui_pmanagerCt;

/*################################### 全局变量 ###################################*/
extern ui_PageContext ui_PageSel1Ct;
extern ui_PageContext ui_PageSel2Ct;
extern ui_PageContext ui_PageSel3Ct;
extern ui_PageContext ui_PageSel4Ct;
extern ui_PageContext ui_PageSel5Ct;
extern ui_PageContext ui_PageSel6Ct;
extern ui_PageContext ui_PageSel7Ct;
extern ui_PageContext ui_PageSel8Ct;
// extern ui_PageContext ui_PageUDiskCt;
// extern ui_PageContext ui_PageNoCt;
// extern ui_PageContext ui_PageRecCt;
// extern ui_PageContext ui_PageRecPlaybackCt;

/*################################### 全局函数 ###################################*/
void ui_pmanager_init(void);
void ui_pmanager_switch(uint8_t id, uint32_t delay, uint8_t del);

#endif
