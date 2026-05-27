#include "type.h"
#include "ui_pmanager.h"
#include "ui_page_main_out.h"
// #include "i2c_slave_example.h"
#include "my_i2c_driver.h"
// 电平相关定义
#define LEVEL_MAX 25
#define NUM_BARS 4

static MainOutObjsContext *ObjsCt;
static MainOutArgsContext ArgsCt;
static uint8_t args_initialized;

static uint8_t levels[NUM_BARS];  // 存储电平数据: [main_left, main_right, pfl_left, pfl_right]

// 电平绘制事件回调函数
static void meter_draw_event_cb(lv_event_t *e)
{
    switch(lv_event_get_code(e))
    {
        case LV_EVENT_REFR_EXT_DRAW_SIZE:
            lv_event_set_ext_draw_size(e, 0);
            break;
        case LV_EVENT_COVER_CHECK:
            lv_event_set_cover_res(e, LV_COVER_RES_COVER);
            break;
        case LV_EVENT_DRAW_POST:
        {
            lv_obj_t *obj = lv_event_get_target(e);
            lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
            lv_area_t area;
            lv_obj_get_coords(obj, &area);

            uint8_t idx = 0;
            // 确定当前对象对应的索引
            if(obj == ObjsCt->ui_main_bar_left) idx = 0;
            else if(obj == ObjsCt->ui_main_bar_right) idx = 1;
            else if(obj == ObjsCt->ui_pfl_bar_left) idx = 2;
            else if(obj == ObjsCt->ui_pfl_bar_right) idx = 3;
            
            uint8_t lv_cur = levels[idx];

            lv_draw_rect_dsc_t dsc;
            lv_draw_rect_dsc_init(&dsc);
            dsc.bg_opa = LV_OPA_COVER;
            dsc.border_opa = LV_OPA_TRANSP;
            dsc.blend_mode = LV_BLEND_MODE_NORMAL;

            int h = area.y2 - area.y1;
            int seg_h = h / LEVEL_MAX;
            if(seg_h < 2) seg_h = 2;
            int block_h = seg_h - 1;
            if(block_h < 1) block_h = 1;
            lv_area_t a;
            a.x1 = area.x1;
            a.x2 = area.x2;
            for(uint8_t s = 0; s < LEVEL_MAX; s++)
            {
                int y_base = area.y2 - s * seg_h;
                a.y1 = y_base - block_h;
                a.y2 = s == 0 ? y_base : y_base - 1;
                if(a.y1 < area.y1) a.y1 = area.y1;
                lv_color_t col = lv_color_hex(0x64FF00);
                if(s >= 15 && s <= 17) col = lv_color_hex(0xFFE100);
                else if(s >= 18) col = lv_color_hex(0xFF0000);
                dsc.bg_color = s < lv_cur ? col : lv_color_hex(0x000000);
                draw_ctx->draw_rect(draw_ctx, &dsc, &a);
            }
            break;
        }
        default:
            break;
    }
}

// 更新电平数据显示
// void ui_page_MainOut_update_levels(void)
// {
//     // 从I2C从机4读取main和pfl电平数据
//     I2C_SlaveDevice *slave4 = I2C_Get_Slave_Data(4);
//     if(slave4 != NULL && slave4->status == 1)
//     {
//         // 适配panel高度的电平计算
//         levels[0] = (slave4->data[0] * LEVEL_MAX) / 255;   // main left
//         levels[1] = (slave4->data[1] * LEVEL_MAX) / 255;   // main right
//         levels[2] = (slave4->data[2] * LEVEL_MAX) / 255;   // pfl left
//         levels[3] = (slave4->data[3] * LEVEL_MAX) / 255;   // pfl right
        
//         // 触发重绘
//         if(ObjsCt->ui_main_bar_left) lv_obj_invalidate(ObjsCt->ui_main_bar_left);
//         if(ObjsCt->ui_main_bar_right) lv_obj_invalidate(ObjsCt->ui_main_bar_right);
//         if(ObjsCt->ui_pfl_bar_left) lv_obj_invalidate(ObjsCt->ui_pfl_bar_left);
//         if(ObjsCt->ui_pfl_bar_right) lv_obj_invalidate(ObjsCt->ui_pfl_bar_right);
//     }
// }

// event funtions

// build funtions

void ui_page_MainOut_init(lv_obj_t *screen)
{
    {
        ObjsCt = (MainOutObjsContext *)lv_mem_alloc(sizeof(MainOutObjsContext));
        memset(ObjsCt, 0, sizeof(MainOutObjsContext));
        if(!args_initialized)
        {
            memset(&ArgsCt, 0, sizeof(MainOutArgsContext));
            ArgsCt.focus_target = 0;
            ArgsCt.edit_mode = 0;
            // 初始化slider值为默认值10
            ArgsCt.eq_63_slider_value = 10;      
            ArgsCt.eq_160_slider_value = 10;     
            ArgsCt.eq_400_slider_value = 10;     
            ArgsCt.eq_1000_slider_value = 10;    
            ArgsCt.eq_2500_slider_value = 10;    
            ArgsCt.eq_6300_slider_value = 10;    
            ArgsCt.eq_16k_slider_value = 10;     
            ArgsCt.main_vol_slider_value = 10;   
            ArgsCt.pfl_vol_slider_value = 10;       
            args_initialized = 1;
        }
    }

    ObjsCt->MainOut_obj = lv_obj_create(screen);
    lv_obj_set_size(ObjsCt->MainOut_obj, 480, 320);
    lv_obj_set_style_bg_color(ObjsCt->MainOut_obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->MainOut_obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ObjsCt->MainOut_obj, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ObjsCt->ui_eq_panel = lv_obj_create(ObjsCt->MainOut_obj);
    lv_obj_set_width(ObjsCt->ui_eq_panel, 317);
    lv_obj_set_height(ObjsCt->ui_eq_panel, 280);
    lv_obj_set_x(ObjsCt->ui_eq_panel, -74);
    lv_obj_set_y(ObjsCt->ui_eq_panel, -10);
    lv_obj_set_align(ObjsCt->ui_eq_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ObjsCt->ui_eq_12db = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_12db, 30);
    lv_obj_set_height(ObjsCt->ui_eq_12db, 16);
    lv_obj_set_x(ObjsCt->ui_eq_12db, -139);
    lv_obj_set_y(ObjsCt->ui_eq_12db, -128);
    lv_obj_set_align(ObjsCt->ui_eq_12db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_12db, "12");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_12db, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_12db, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_12db, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_12db, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_6db = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_6db, 30);
    lv_obj_set_height(ObjsCt->ui_eq_6db, 16);
    lv_obj_set_x(ObjsCt->ui_eq_6db, -139);
    lv_obj_set_y(ObjsCt->ui_eq_6db, -69);
    lv_obj_set_align(ObjsCt->ui_eq_6db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_6db, "6");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_6db, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_6db, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_6db, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_6db, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_0db = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_0db, 30);
    lv_obj_set_height(ObjsCt->ui_eq_0db, 16);
    lv_obj_set_x(ObjsCt->ui_eq_0db, -139);
    lv_obj_set_y(ObjsCt->ui_eq_0db, -9);
    lv_obj_set_align(ObjsCt->ui_eq_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_0db, "0db");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_0db, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_0db, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_0db, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_0db, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_neg6db = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_neg6db, 30);
    lv_obj_set_height(ObjsCt->ui_eq_neg6db, 16);
    lv_obj_set_x(ObjsCt->ui_eq_neg6db, -141);
    lv_obj_set_y(ObjsCt->ui_eq_neg6db, 51);
    lv_obj_set_align(ObjsCt->ui_eq_neg6db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_neg6db, "-6");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_neg6db, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_neg6db, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_neg6db, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_neg6db, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_neg12db = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_neg12db, 30);
    lv_obj_set_height(ObjsCt->ui_eq_neg12db, 16);
    lv_obj_set_x(ObjsCt->ui_eq_neg12db, -140);
    lv_obj_set_y(ObjsCt->ui_eq_neg12db, 111);
    lv_obj_set_align(ObjsCt->ui_eq_neg12db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_neg12db, "-12");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_neg12db, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_neg12db, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_neg12db, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_neg12db, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_63Hz = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_63Hz, 50);
    lv_obj_set_height(ObjsCt->ui_eq_63Hz, 12);
    lv_obj_set_x(ObjsCt->ui_eq_63Hz, -101);
    lv_obj_set_y(ObjsCt->ui_eq_63Hz, 129);
    lv_obj_set_align(ObjsCt->ui_eq_63Hz, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_63Hz, "63Hz");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_63Hz, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_63Hz, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_63Hz, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_63Hz, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_160Hz = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_160Hz, 50);
    lv_obj_set_height(ObjsCt->ui_eq_160Hz, 12);
    lv_obj_set_x(ObjsCt->ui_eq_160Hz, -61);
    lv_obj_set_y(ObjsCt->ui_eq_160Hz, 129);
    lv_obj_set_align(ObjsCt->ui_eq_160Hz, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_160Hz, "160Hz");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_160Hz, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_160Hz, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_160Hz, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_160Hz, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_400Hz = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_400Hz, 50);
    lv_obj_set_height(ObjsCt->ui_eq_400Hz, 12);
    lv_obj_set_x(ObjsCt->ui_eq_400Hz, -19);
    lv_obj_set_y(ObjsCt->ui_eq_400Hz, 129);
    lv_obj_set_align(ObjsCt->ui_eq_400Hz, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_400Hz, "400Hz");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_400Hz, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_400Hz, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_400Hz, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_400Hz, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_1000Hz = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_1000Hz, 50);
    lv_obj_set_height(ObjsCt->ui_eq_1000Hz, 12);
    lv_obj_set_x(ObjsCt->ui_eq_1000Hz, 22);
    lv_obj_set_y(ObjsCt->ui_eq_1000Hz, 129);
    lv_obj_set_align(ObjsCt->ui_eq_1000Hz, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_1000Hz, "1KHz");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_1000Hz, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_1000Hz, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_1000Hz, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_1000Hz, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_2500Hz = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_2500Hz, 50);
    lv_obj_set_height(ObjsCt->ui_eq_2500Hz, 12);
    lv_obj_set_x(ObjsCt->ui_eq_2500Hz, 60);
    lv_obj_set_y(ObjsCt->ui_eq_2500Hz, 129);
    lv_obj_set_align(ObjsCt->ui_eq_2500Hz, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_2500Hz, "2.5KHz");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_2500Hz, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_2500Hz, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_2500Hz, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_2500Hz, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_6300Hz = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_6300Hz, 50);
    lv_obj_set_height(ObjsCt->ui_eq_6300Hz, 12);
    lv_obj_set_x(ObjsCt->ui_eq_6300Hz, 100);
    lv_obj_set_y(ObjsCt->ui_eq_6300Hz, 129);
    lv_obj_set_align(ObjsCt->ui_eq_6300Hz, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_6300Hz, "6.3KHz");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_6300Hz, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_6300Hz, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_6300Hz, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_6300Hz, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_16kHz = lv_label_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_16kHz, 50);
    lv_obj_set_height(ObjsCt->ui_eq_16kHz, 12);
    lv_obj_set_x(ObjsCt->ui_eq_16kHz, 138);
    lv_obj_set_y(ObjsCt->ui_eq_16kHz, 129);
    lv_obj_set_align(ObjsCt->ui_eq_16kHz, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_16kHz, "16KHz");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_16kHz, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_16kHz, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_16kHz, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_16kHz, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_63_slider_panel = lv_obj_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_63_slider_panel, 33);
    lv_obj_set_height(ObjsCt->ui_eq_63_slider_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_63_slider_panel, -102);
    lv_obj_set_y(ObjsCt->ui_eq_63_slider_panel, -7);
    lv_obj_set_align(ObjsCt->ui_eq_63_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_63_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_63_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_63_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_63_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_63_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_63_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_63_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_63_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_63_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_63_slider = lv_slider_create(ObjsCt->ui_eq_63_slider_panel);
    lv_slider_set_range(ObjsCt->ui_eq_63_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_63_slider, ArgsCt.eq_63_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_63_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_63_slider, ArgsCt.eq_63_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_63_slider, 10);
    lv_obj_set_height(ObjsCt->ui_eq_63_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_63_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_63_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_63_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_63_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_63_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_63_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_63_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_63_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_63_slider, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_63_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_63_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_63_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_63_slider, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_63_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_63_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_63_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_63_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_63_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_63_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_63_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_63_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_160_slider_panel = lv_obj_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_160_slider_panel, 33);
    lv_obj_set_height(ObjsCt->ui_eq_160_slider_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_160_slider_panel, -61);
    lv_obj_set_y(ObjsCt->ui_eq_160_slider_panel, -7);
    lv_obj_set_align(ObjsCt->ui_eq_160_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_160_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_160_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_160_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_160_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_160_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_160_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_160_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_160_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_160_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_160_slider = lv_slider_create(ObjsCt->ui_eq_160_slider_panel);
    lv_slider_set_range(ObjsCt->ui_eq_160_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_160_slider, ArgsCt.eq_160_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_160_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_160_slider, ArgsCt.eq_160_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_160_slider, 10);
    lv_obj_set_height(ObjsCt->ui_eq_160_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_160_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_160_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_160_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_160_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_160_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_160_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_160_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_160_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_160_slider, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_160_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_160_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_160_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_160_slider, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_160_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_160_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_160_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_160_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_160_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_160_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_160_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_160_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_400_slider_panel = lv_obj_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_400_slider_panel, 33);
    lv_obj_set_height(ObjsCt->ui_eq_400_slider_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_400_slider_panel, -20);
    lv_obj_set_y(ObjsCt->ui_eq_400_slider_panel, -7);
    lv_obj_set_align(ObjsCt->ui_eq_400_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_400_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_400_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_400_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_400_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_400_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_400_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_400_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_400_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_400_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_400_slider = lv_slider_create(ObjsCt->ui_eq_400_slider_panel);
    lv_slider_set_range(ObjsCt->ui_eq_400_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_400_slider, ArgsCt.eq_400_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_400_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_400_slider, ArgsCt.eq_400_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_400_slider, 10);
    lv_obj_set_height(ObjsCt->ui_eq_400_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_400_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_400_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_400_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_400_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_400_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_400_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_400_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_400_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_400_slider, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_400_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_400_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_400_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_400_slider, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_400_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_400_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_400_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_400_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_400_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_400_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_400_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_400_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_1000_slider_panel = lv_obj_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_1000_slider_panel, 33);
    lv_obj_set_height(ObjsCt->ui_eq_1000_slider_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_1000_slider_panel, 21);
    lv_obj_set_y(ObjsCt->ui_eq_1000_slider_panel, -7);
    lv_obj_set_align(ObjsCt->ui_eq_1000_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_1000_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_1000_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_1000_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_1000_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_1000_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_1000_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_1000_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_1000_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_1000_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_1000_slider = lv_slider_create(ObjsCt->ui_eq_1000_slider_panel);
    lv_slider_set_range(ObjsCt->ui_eq_1000_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_1000_slider, ArgsCt.eq_1000_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_1000_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_1000_slider, ArgsCt.eq_1000_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_1000_slider, 10);
    lv_obj_set_height(ObjsCt->ui_eq_1000_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_1000_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_1000_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_1000_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_1000_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_1000_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_1000_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_1000_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_1000_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_1000_slider, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_1000_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_1000_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_1000_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_1000_slider, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_1000_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_1000_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_1000_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_1000_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_1000_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_1000_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_1000_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_1000_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_2500_slider_panel = lv_obj_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_2500_slider_panel, 33);
    lv_obj_set_height(ObjsCt->ui_eq_2500_slider_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_2500_slider_panel, 61);
    lv_obj_set_y(ObjsCt->ui_eq_2500_slider_panel, -7);
    lv_obj_set_align(ObjsCt->ui_eq_2500_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_2500_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_2500_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_2500_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_2500_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_2500_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_2500_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_2500_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_2500_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_2500_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_2500_slider = lv_slider_create(ObjsCt->ui_eq_2500_slider_panel);
    lv_slider_set_range(ObjsCt->ui_eq_2500_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_2500_slider, ArgsCt.eq_2500_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_2500_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_2500_slider, ArgsCt.eq_2500_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_2500_slider, 10);
    lv_obj_set_height(ObjsCt->ui_eq_2500_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_2500_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_2500_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_2500_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_2500_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_2500_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_2500_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_2500_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_2500_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_2500_slider, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_2500_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_2500_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_2500_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_2500_slider, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_2500_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_2500_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_2500_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_2500_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_2500_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_2500_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_2500_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_2500_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_6300_slider_panel = lv_obj_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_6300_slider_panel, 33);
    lv_obj_set_height(ObjsCt->ui_eq_6300_slider_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_6300_slider_panel, 101);
    lv_obj_set_y(ObjsCt->ui_eq_6300_slider_panel, -7);
    lv_obj_set_align(ObjsCt->ui_eq_6300_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_6300_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_6300_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_6300_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_6300_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_6300_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_6300_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_6300_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_6300_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_6300_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_6300_slider = lv_slider_create(ObjsCt->ui_eq_6300_slider_panel);
    lv_slider_set_range(ObjsCt->ui_eq_6300_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_6300_slider, ArgsCt.eq_6300_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_6300_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_6300_slider, ArgsCt.eq_6300_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_6300_slider, 10);
    lv_obj_set_height(ObjsCt->ui_eq_6300_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_6300_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_6300_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_6300_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_6300_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_6300_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_6300_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_6300_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_6300_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_6300_slider, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_6300_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_6300_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_6300_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_6300_slider, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_6300_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_6300_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_6300_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_6300_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_6300_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_6300_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_6300_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_6300_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_16k_slider_panel = lv_obj_create(ObjsCt->ui_eq_panel);
    lv_obj_set_width(ObjsCt->ui_eq_16k_slider_panel, 33);
    lv_obj_set_height(ObjsCt->ui_eq_16k_slider_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_16k_slider_panel, 139);
    lv_obj_set_y(ObjsCt->ui_eq_16k_slider_panel, -7);
    lv_obj_set_align(ObjsCt->ui_eq_16k_slider_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_16k_slider_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_16k_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_16k_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_16k_slider_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_16k_slider_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_16k_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_16k_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_16k_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_16k_slider_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_16k_slider = lv_slider_create(ObjsCt->ui_eq_16k_slider_panel);
    lv_slider_set_range(ObjsCt->ui_eq_16k_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_16k_slider, ArgsCt.eq_16k_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_16k_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_16k_slider, ArgsCt.eq_16k_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_16k_slider, 10);
    lv_obj_set_height(ObjsCt->ui_eq_16k_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_16k_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_16k_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_16k_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_16k_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_16k_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_16k_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_16k_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_16k_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_16k_slider, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_16k_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_16k_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_16k_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_16k_slider, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_16k_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_16k_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_16k_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_16k_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_16k_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_16k_slider, 5, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_16k_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_16k_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_level_panel = lv_obj_create(ObjsCt->MainOut_obj);
    lv_obj_set_width(ObjsCt->ui_eq_level_panel, 160);
    lv_obj_set_height(ObjsCt->ui_eq_level_panel, 278);
    lv_obj_set_x(ObjsCt->ui_eq_level_panel, 153);
    lv_obj_set_y(ObjsCt->ui_eq_level_panel, -10);
    lv_obj_set_align(ObjsCt->ui_eq_level_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_level_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ObjsCt->ui_eq_level_lr_name = lv_label_create(ObjsCt->ui_eq_level_panel);
    lv_obj_set_width(ObjsCt->ui_eq_level_lr_name, 160);
    lv_obj_set_height(ObjsCt->ui_eq_level_lr_name, 14);
    lv_obj_set_x(ObjsCt->ui_eq_level_lr_name, 0);
    lv_obj_set_y(ObjsCt->ui_eq_level_lr_name, 131);
    lv_obj_set_align(ObjsCt->ui_eq_level_lr_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_level_lr_name, "            L         R           L         R");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_level_lr_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_level_lr_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_level_lr_name, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_level_lr_name, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Main电平显示组件
    ObjsCt->ui_main_meter_group = lv_obj_create(ObjsCt->ui_eq_level_panel);
    lv_obj_set_width(ObjsCt->ui_main_meter_group, 70);  // 半个level panel宽度，减去间隔
    lv_obj_set_height(ObjsCt->ui_main_meter_group, 260);
    lv_obj_set_x(ObjsCt->ui_main_meter_group, -30);  // 左侧位置
    lv_obj_set_y(ObjsCt->ui_main_meter_group, -8);
    lv_obj_set_align(ObjsCt->ui_main_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_main_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_main_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_main_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_main_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_main_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_main_vol_panel = lv_obj_create(ObjsCt->ui_main_meter_group);  // 移入Main meter group
    lv_obj_set_width(ObjsCt->ui_eq_main_vol_panel, 12);
    lv_obj_set_height(ObjsCt->ui_eq_main_vol_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_main_vol_panel, -28);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->ui_eq_main_vol_panel, 0);
    lv_obj_set_align(ObjsCt->ui_eq_main_vol_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_main_vol_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_main_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_main_vol_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_main_vol_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_main_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_main_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_main_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_main_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_main_vol_slider = lv_slider_create(ObjsCt->ui_eq_main_vol_panel);
    lv_slider_set_range(ObjsCt->ui_eq_main_vol_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_main_vol_slider, ArgsCt.main_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_main_vol_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_main_vol_slider, ArgsCt.main_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_main_vol_slider, 8);
    lv_obj_set_height(ObjsCt->ui_eq_main_vol_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_main_vol_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_main_vol_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_main_vol_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_main_vol_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_main_vol_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_main_vol_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_main_vol_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_main_vol_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_main_vol_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_main_vol_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_main_vol_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_main_vol_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_main_vol_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_main_vol_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_main_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    // PFL电平显示组件 - 直接在level panel上创建
    ObjsCt->ui_pfl_meter_group = lv_obj_create(ObjsCt->ui_eq_level_panel);
    lv_obj_set_width(ObjsCt->ui_pfl_meter_group, 70);  // 半个level panel宽度，减去间隔
    lv_obj_set_height(ObjsCt->ui_pfl_meter_group, 260);
    lv_obj_set_x(ObjsCt->ui_pfl_meter_group, 45);  // 右侧位置
    lv_obj_set_y(ObjsCt->ui_pfl_meter_group, -8);
    lv_obj_set_align(ObjsCt->ui_pfl_meter_group, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_pfl_meter_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_pfl_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_pfl_meter_group, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_pfl_meter_group, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_pfl_meter_group, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_pfl_vol_panel = lv_obj_create(ObjsCt->ui_pfl_meter_group);  // 移入PFL meter group
    lv_obj_set_width(ObjsCt->ui_eq_pfl_vol_panel, 12);
    lv_obj_set_height(ObjsCt->ui_eq_pfl_vol_panel, 260);
    lv_obj_set_x(ObjsCt->ui_eq_pfl_vol_panel, -28);  // 调整到左侧位置，为电平条让出右侧空间
    lv_obj_set_y(ObjsCt->ui_eq_pfl_vol_panel, 0);
    lv_obj_set_align(ObjsCt->ui_eq_pfl_vol_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_pfl_vol_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ObjsCt->ui_eq_pfl_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_pfl_vol_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_pfl_vol_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_pfl_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_pfl_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_pfl_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_pfl_vol_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_pfl_vol_slider = lv_slider_create(ObjsCt->ui_eq_pfl_vol_panel);
    lv_slider_set_range(ObjsCt->ui_eq_pfl_vol_slider, 0, 20);  // 设置slider范围为0-20
    lv_slider_set_value(ObjsCt->ui_eq_pfl_vol_slider, ArgsCt.pfl_vol_slider_value, LV_ANIM_OFF);
    if(lv_slider_get_mode(ObjsCt->ui_eq_pfl_vol_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ObjsCt->ui_eq_pfl_vol_slider, ArgsCt.pfl_vol_slider_value, LV_ANIM_OFF);
    lv_obj_set_width(ObjsCt->ui_eq_pfl_vol_slider, 8);
    lv_obj_set_height(ObjsCt->ui_eq_pfl_vol_slider, 245);
    lv_obj_set_x(ObjsCt->ui_eq_pfl_vol_slider, 0);
    lv_obj_set_y(ObjsCt->ui_eq_pfl_vol_slider, -3);
    lv_obj_set_align(ObjsCt->ui_eq_pfl_vol_slider, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_pfl_vol_slider, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_pfl_vol_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_slider, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_pfl_vol_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_pfl_vol_slider, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_pfl_vol_slider, lv_color_hex(0x21EE16), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_pfl_vol_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_slider, lv_color_hex(0x5A5151), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_pfl_vol_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_pfl_vol_slider, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ObjsCt->ui_eq_pfl_vol_slider, lv_color_hex(0xCDCC33), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ObjsCt->ui_eq_pfl_vol_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ObjsCt->ui_eq_pfl_vol_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ObjsCt->ui_eq_pfl_vol_slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    
    ObjsCt->ui_main_out_panel = lv_obj_create(ObjsCt->MainOut_obj);
    lv_obj_set_width(ObjsCt->ui_main_out_panel, 317);
    lv_obj_set_height(ObjsCt->ui_main_out_panel, 21);
    lv_obj_set_x(ObjsCt->ui_main_out_panel, -74);
    lv_obj_set_y(ObjsCt->ui_main_out_panel, 143);
    lv_obj_set_align(ObjsCt->ui_main_out_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_main_out_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ObjsCt->ui_main_out_name = lv_label_create(ObjsCt->ui_main_out_panel);
    lv_obj_set_width(ObjsCt->ui_main_out_name, 317);
    lv_obj_set_height(ObjsCt->ui_main_out_name, 21);
    lv_obj_set_align(ObjsCt->ui_main_out_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_main_out_name, "MAIN OUT");
    lv_obj_set_style_text_color(ObjsCt->ui_main_out_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_main_out_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_main_out_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_main_out_name, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_eq_level_name_panel = lv_obj_create(ObjsCt->MainOut_obj);
    lv_obj_set_width(ObjsCt->ui_eq_level_name_panel, 160);
    lv_obj_set_height(ObjsCt->ui_eq_level_name_panel, 21);
    lv_obj_set_x(ObjsCt->ui_eq_level_name_panel, 153);
    lv_obj_set_y(ObjsCt->ui_eq_level_name_panel, 143);
    lv_obj_set_align(ObjsCt->ui_eq_level_name_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_eq_level_name_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ObjsCt->ui_eq_level_name = lv_label_create(ObjsCt->ui_eq_level_name_panel);
    lv_obj_set_width(ObjsCt->ui_eq_level_name, 160);
    lv_obj_set_height(ObjsCt->ui_eq_level_name, 21);
    lv_obj_set_align(ObjsCt->ui_eq_level_name, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_eq_level_name, "    MAIN      PFL");
    lv_obj_set_style_text_color(ObjsCt->ui_eq_level_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ObjsCt->ui_eq_level_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ObjsCt->ui_eq_level_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ObjsCt->ui_eq_level_name, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_main_bar_left = lv_obj_create(ObjsCt->ui_main_meter_group);
    lv_obj_set_width(ObjsCt->ui_main_bar_left, 12);
    lv_obj_set_height(ObjsCt->ui_main_bar_left, 260);
    lv_obj_set_x(ObjsCt->ui_main_bar_left, -10);  // 左声道电平条，向左偏移
    lv_obj_set_y(ObjsCt->ui_main_bar_left, -5);
    lv_obj_set_align(ObjsCt->ui_main_bar_left, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_main_bar_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_main_bar_left, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_main_bar_left, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ObjsCt->ui_main_bar_left, meter_draw_event_cb, LV_EVENT_ALL, NULL);

    ObjsCt->ui_main_bar_right = lv_obj_create(ObjsCt->ui_main_meter_group);
    lv_obj_set_width(ObjsCt->ui_main_bar_right, 12);
    lv_obj_set_height(ObjsCt->ui_main_bar_right, 260);
    lv_obj_set_x(ObjsCt->ui_main_bar_right, 25);  // 右声道电平条，向右偏移
    lv_obj_set_y(ObjsCt->ui_main_bar_right, -5);
    lv_obj_set_align(ObjsCt->ui_main_bar_right, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_main_bar_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_main_bar_right, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_main_bar_right, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ObjsCt->ui_main_bar_right, meter_draw_event_cb, LV_EVENT_ALL, NULL);

    // Main电平标签
    ObjsCt->ui_main_label_plus10 = lv_label_create(ObjsCt->ui_main_meter_group);
    lv_obj_set_width(ObjsCt->ui_main_label_plus10, 30);
    lv_obj_set_height(ObjsCt->ui_main_label_plus10, 15);
    lv_obj_set_x(ObjsCt->ui_main_label_plus10, 12);
    lv_obj_set_y(ObjsCt->ui_main_label_plus10, -118);
    lv_obj_set_align(ObjsCt->ui_main_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_main_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->ui_main_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_main_label_plus6 = lv_label_create(ObjsCt->ui_main_meter_group);
    lv_obj_set_width(ObjsCt->ui_main_label_plus6, 30);
    lv_obj_set_height(ObjsCt->ui_main_label_plus6, 15);
    lv_obj_set_x(ObjsCt->ui_main_label_plus6, 12);
    lv_obj_set_y(ObjsCt->ui_main_label_plus6, -70);
    lv_obj_set_align(ObjsCt->ui_main_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_main_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->ui_main_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_main_label_0db = lv_label_create(ObjsCt->ui_main_meter_group);
    lv_obj_set_width(ObjsCt->ui_main_label_0db, 30);
    lv_obj_set_height(ObjsCt->ui_main_label_0db, 15);
    lv_obj_set_x(ObjsCt->ui_main_label_0db, 13);
    lv_obj_set_y(ObjsCt->ui_main_label_0db, -22);
    lv_obj_set_align(ObjsCt->ui_main_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_main_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->ui_main_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_main_label_minus10 = lv_label_create(ObjsCt->ui_main_meter_group);
    lv_obj_set_width(ObjsCt->ui_main_label_minus10, 30);
    lv_obj_set_height(ObjsCt->ui_main_label_minus10, 15);
    lv_obj_set_x(ObjsCt->ui_main_label_minus10, 14);
    lv_obj_set_y(ObjsCt->ui_main_label_minus10, 26);
    lv_obj_set_align(ObjsCt->ui_main_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_main_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->ui_main_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_main_label_minus30 = lv_label_create(ObjsCt->ui_main_meter_group);
    lv_obj_set_width(ObjsCt->ui_main_label_minus30, 30);
    lv_obj_set_height(ObjsCt->ui_main_label_minus30, 15);
    lv_obj_set_x(ObjsCt->ui_main_label_minus30, 14);
    lv_obj_set_y(ObjsCt->ui_main_label_minus30, 74);
    lv_obj_set_align(ObjsCt->ui_main_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_main_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->ui_main_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_main_label_minus50 = lv_label_create(ObjsCt->ui_main_meter_group);
    lv_obj_set_width(ObjsCt->ui_main_label_minus50, 30);
    lv_obj_set_height(ObjsCt->ui_main_label_minus50, 15);
    lv_obj_set_x(ObjsCt->ui_main_label_minus50, 15);
    lv_obj_set_y(ObjsCt->ui_main_label_minus50, 122);
    lv_obj_set_align(ObjsCt->ui_main_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_main_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->ui_main_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_pfl_bar_left = lv_obj_create(ObjsCt->ui_pfl_meter_group);
    lv_obj_set_width(ObjsCt->ui_pfl_bar_left, 12);
    lv_obj_set_height(ObjsCt->ui_pfl_bar_left, 260);
    lv_obj_set_x(ObjsCt->ui_pfl_bar_left, -10);  // 左声道电平条，向左偏移
    lv_obj_set_y(ObjsCt->ui_pfl_bar_left, -5);
    lv_obj_set_align(ObjsCt->ui_pfl_bar_left, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_pfl_bar_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_pfl_bar_left, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_pfl_bar_left, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ObjsCt->ui_pfl_bar_left, meter_draw_event_cb, LV_EVENT_ALL, NULL);

    ObjsCt->ui_pfl_bar_right = lv_obj_create(ObjsCt->ui_pfl_meter_group);
    lv_obj_set_width(ObjsCt->ui_pfl_bar_right, 12);
    lv_obj_set_height(ObjsCt->ui_pfl_bar_right, 260);
    lv_obj_set_x(ObjsCt->ui_pfl_bar_right, 25);  // 右声道电平条，向右偏移
    lv_obj_set_y(ObjsCt->ui_pfl_bar_right, -5);
    lv_obj_set_align(ObjsCt->ui_pfl_bar_right, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ObjsCt->ui_pfl_bar_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ObjsCt->ui_pfl_bar_right, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ObjsCt->ui_pfl_bar_right, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ObjsCt->ui_pfl_bar_right, meter_draw_event_cb, LV_EVENT_ALL, NULL);

    // PFL电平标签
    ObjsCt->ui_pfl_label_plus10 = lv_label_create(ObjsCt->ui_pfl_meter_group);
    lv_obj_set_width(ObjsCt->ui_pfl_label_plus10, 30);
    lv_obj_set_height(ObjsCt->ui_pfl_label_plus10, 15);
    lv_obj_set_x(ObjsCt->ui_pfl_label_plus10, 12);
    lv_obj_set_y(ObjsCt->ui_pfl_label_plus10, -118);
    lv_obj_set_align(ObjsCt->ui_pfl_label_plus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_pfl_label_plus10, "+10");
    lv_obj_set_style_text_font(ObjsCt->ui_pfl_label_plus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_pfl_label_plus6 = lv_label_create(ObjsCt->ui_pfl_meter_group);
    lv_obj_set_width(ObjsCt->ui_pfl_label_plus6, 30);
    lv_obj_set_height(ObjsCt->ui_pfl_label_plus6, 15);
    lv_obj_set_x(ObjsCt->ui_pfl_label_plus6, 12);
    lv_obj_set_y(ObjsCt->ui_pfl_label_plus6, -70);
    lv_obj_set_align(ObjsCt->ui_pfl_label_plus6, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_pfl_label_plus6, "+6");
    lv_obj_set_style_text_font(ObjsCt->ui_pfl_label_plus6, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_pfl_label_0db = lv_label_create(ObjsCt->ui_pfl_meter_group);
    lv_obj_set_width(ObjsCt->ui_pfl_label_0db, 30);
    lv_obj_set_height(ObjsCt->ui_pfl_label_0db, 15);
    lv_obj_set_x(ObjsCt->ui_pfl_label_0db, 13);
    lv_obj_set_y(ObjsCt->ui_pfl_label_0db, -22);
    lv_obj_set_align(ObjsCt->ui_pfl_label_0db, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_pfl_label_0db, "0db");
    lv_obj_set_style_text_font(ObjsCt->ui_pfl_label_0db, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_pfl_label_minus10 = lv_label_create(ObjsCt->ui_pfl_meter_group);
    lv_obj_set_width(ObjsCt->ui_pfl_label_minus10, 30);
    lv_obj_set_height(ObjsCt->ui_pfl_label_minus10, 15);
    lv_obj_set_x(ObjsCt->ui_pfl_label_minus10, 14);
    lv_obj_set_y(ObjsCt->ui_pfl_label_minus10, 26);
    lv_obj_set_align(ObjsCt->ui_pfl_label_minus10, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_pfl_label_minus10, "-10");
    lv_obj_set_style_text_font(ObjsCt->ui_pfl_label_minus10, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_pfl_label_minus30 = lv_label_create(ObjsCt->ui_pfl_meter_group);
    lv_obj_set_width(ObjsCt->ui_pfl_label_minus30, 30);
    lv_obj_set_height(ObjsCt->ui_pfl_label_minus30, 15);
    lv_obj_set_x(ObjsCt->ui_pfl_label_minus30, 14);
    lv_obj_set_y(ObjsCt->ui_pfl_label_minus30, 74);
    lv_obj_set_align(ObjsCt->ui_pfl_label_minus30, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_pfl_label_minus30, "-30");
    lv_obj_set_style_text_font(ObjsCt->ui_pfl_label_minus30, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    ObjsCt->ui_pfl_label_minus50 = lv_label_create(ObjsCt->ui_pfl_meter_group);
    lv_obj_set_width(ObjsCt->ui_pfl_label_minus50, 30);
    lv_obj_set_height(ObjsCt->ui_pfl_label_minus50, 15);
    lv_obj_set_x(ObjsCt->ui_pfl_label_minus50, 15);
    lv_obj_set_y(ObjsCt->ui_pfl_label_minus50, 122);
    lv_obj_set_align(ObjsCt->ui_pfl_label_minus50, LV_ALIGN_CENTER);
    lv_label_set_text(ObjsCt->ui_pfl_label_minus50, "-50");
    lv_obj_set_style_text_font(ObjsCt->ui_pfl_label_minus50, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

}


void ui_page_MainOut_start(void)
{
    if(ObjsCt == NULL)
        return;

    ArgsCt.focus_target = 0;
    ArgsCt.edit_mode = 0;

    lv_obj_set_style_border_color(ObjsCt->ui_eq_63_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_160_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_400_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_1000_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_2500_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_6300_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ObjsCt->ui_eq_16k_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // 初始化电平数据
    for(uint8_t i = 0; i < NUM_BARS; i++)
    {
        levels[i] = 0;
    }
    // ui_page_MainOut_update_levels();
}

void ui_page_MainOut_exit(uint8_t del)
{
    if(del)
    {

    }
    else
    {

    }
    SendBuf[9] = 0x00;//退出当前页面，让从机关闭LED灯
    MasterSendExample(0x27);
}

void ui_page_MainOut_clean(void)
{
    lv_mem_free(ObjsCt);
    ObjsCt = NULL;
}

void ui_page_MainOut_request(uint32_t delay)
{
    switch(ui_pmanagerCt.id_tar)
    {

        case UI_PAGE_SEL_6:
            break;

        default:
            ui_pmanager_switch(UI_PAGE_SEL_6, delay, true);
            break;
    }
    SendBuf[9] = 0x01;//退出当前页面，让从机关闭LED灯
    MasterSendExample(0x27);
}

void ui_page_MainOut_encoder_up(void)
{
    if(ObjsCt == NULL) return;
    
    if(ArgsCt.edit_mode == 0)
    {
        switch(ArgsCt.focus_target)
        {
            case 0:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_63_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_160_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_400_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_1000_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: lv_obj_set_style_border_color(ObjsCt->ui_eq_2500_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: lv_obj_set_style_border_color(ObjsCt->ui_eq_6300_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: lv_obj_set_style_border_color(ObjsCt->ui_eq_16k_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 7: lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 8: lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
        }
        
        if(ArgsCt.focus_target < 8)
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
                lv_obj_set_style_border_color(ObjsCt->ui_eq_63_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_160_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_400_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_1000_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_2500_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_6300_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_16k_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 7: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 8: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
        }
        
    }
    else 
    {
        switch(ArgsCt.focus_target)
        {
            case 0: // 63Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_63_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_63_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_63_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 1: // 160Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_160_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_160_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_160_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 2: // 400Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_400_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_400_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_400_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 3: // 1000Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_1000_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_1000_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_1000_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 4: // 2500Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_2500_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_2500_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_2500_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 5: // 6300Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_6300_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_6300_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_6300_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 6: // 16kHz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_16k_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_16k_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_16k_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 7: // Main音量slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_main_vol_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_main_vol_slider, current_val, LV_ANIM_ON);
                    ArgsCt.main_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 8: // PFL音量slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_pfl_vol_slider);
                    current_val += 1;
                    if(current_val > 20) current_val = 20;
                    lv_slider_set_value(ObjsCt->ui_eq_pfl_vol_slider, current_val, LV_ANIM_ON);
                    ArgsCt.pfl_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
        }
    }
}

void ui_page_MainOut_encoder_down(void)
{
    if(ObjsCt == NULL) return; 
    
    if(ArgsCt.edit_mode == 0)
    {
        switch(ArgsCt.focus_target)
        {
            case 0:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_63_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_160_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_400_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_1000_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: lv_obj_set_style_border_color(ObjsCt->ui_eq_2500_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: lv_obj_set_style_border_color(ObjsCt->ui_eq_6300_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: lv_obj_set_style_border_color(ObjsCt->ui_eq_16k_slider_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 7: lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 8: lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_panel, lv_color_hex(0x5A5151), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
        }
        
        if(ArgsCt.focus_target > 0)
        {
            ArgsCt.focus_target--;
        }
        else
        {
            ArgsCt.focus_target = 8; // 循环回最后一个元素
        }
        
        switch(ArgsCt.focus_target)
        {
            case 0:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_63_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_160_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_400_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_1000_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_2500_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_6300_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_16k_slider_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 7: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 8: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_panel, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
        }
        
    }
    else
    {
        switch(ArgsCt.focus_target)
        {
            case 0: // 63Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_63_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_63_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_63_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 1: // 160Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_160_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_160_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_160_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 2: // 400Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_400_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_400_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_400_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 3: // 1000Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_1000_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_1000_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_1000_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 4: // 2500Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_2500_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_2500_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_2500_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 5: // 6300Hz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_6300_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_6300_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_6300_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 6: // 16kHz EQ slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_16k_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_16k_slider, current_val, LV_ANIM_ON);
                    ArgsCt.eq_16k_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 7: // Main音量slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_main_vol_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_main_vol_slider, current_val, LV_ANIM_ON);
                    ArgsCt.main_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
            case 8: // PFL音量slider
                {
                    int16_t current_val = lv_slider_get_value(ObjsCt->ui_eq_pfl_vol_slider);
                    current_val -= 1;
                    if(current_val < 0) current_val = 0;
                    lv_slider_set_value(ObjsCt->ui_eq_pfl_vol_slider, current_val, LV_ANIM_ON);
                    ArgsCt.pfl_vol_slider_value = (uint8_t)current_val;  // 保存值
                }
                break;
        }
    }
}

void ui_page_MainOut_encoder_ok(void)
{
    if(ObjsCt == NULL) return;
    
    ArgsCt.edit_mode = !ArgsCt.edit_mode;
    
    lv_color_t text_color = ArgsCt.edit_mode ? lv_color_hex(0xFFA500) : lv_color_hex(0x00FF00);
    
    switch(ArgsCt.focus_target)
    {
        case 0:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_63_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 1:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_160_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 2:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_400_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 3:
                lv_obj_set_style_border_color(ObjsCt->ui_eq_1000_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 4: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_2500_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 5: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_6300_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 6: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_16k_slider_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 7: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_main_vol_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            case 8: 
                lv_obj_set_style_border_color(ObjsCt->ui_eq_pfl_vol_panel, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
    }
}

// 所有slider的编码器控制功能已完成，支持所有EQ sliders和音量sliders
