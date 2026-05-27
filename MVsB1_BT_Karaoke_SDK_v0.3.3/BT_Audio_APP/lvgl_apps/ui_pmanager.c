#include "app_config.h"
#include "type.h"
#include "ui_pmanager.h"
#include "adlist.h"
#include "debug.h"

/*################################### 宏定义 ###################################*/

/*################################### 变量(静态声明) ###################################*/

/*################################### 变量(全局声明) ###################################*/

/*################################### 变量(静态定义) ###################################*/
static list *page_list = NULL;

/*################################### 变量(全局定义) ###################################*/
ui_pmanagerContext ui_pmanagerCt;

ui_PageContext ui_PageSel1Ct = {
    .id         = UI_PAGE_SEL_1,
    .screen     = NULL,
    .auto_del   = TRUE,
    .init_cb    = ui_page_Mic1_init,
    .start_cb   = ui_page_Mic1_start,
    .exit_cb    = ui_page_Mic1_exit,
    .clean_cb   = ui_page_Mic1_clean,
};

ui_PageContext ui_PageSel2Ct = {
    .id         = UI_PAGE_SEL_2,
    .screen     = NULL,
    .auto_del   = TRUE,
    .init_cb    = ui_page_Mic2_init,
    .start_cb   = ui_page_Mic2_start,
    .exit_cb    = ui_page_Mic2_exit,
    .clean_cb   = ui_page_Mic2_clean,
};

ui_PageContext ui_PageSel3Ct = {
    .id         = UI_PAGE_SEL_3,
    .screen     = NULL,
    .auto_del   = TRUE,
    .init_cb    = ui_page_Aux1_init,
    .start_cb   = ui_page_Aux1_start,
    .exit_cb    = ui_page_Aux1_exit,
    .clean_cb   = ui_page_Aux1_clean,
};
ui_PageContext ui_PageSel4Ct = {
    .id         = UI_PAGE_SEL_4,
    .screen     = NULL,
    .auto_del   = TRUE,
    .init_cb    = ui_page_Aux2_init,
    .start_cb   = ui_page_Aux2_start,
    .exit_cb    = ui_page_Aux2_exit,
    .clean_cb   = ui_page_Aux2_clean,
};
ui_PageContext ui_PageSel5Ct = {
    .id         = UI_PAGE_SEL_5,
    .screen     = NULL,
    .auto_del   = TRUE,
    .init_cb    = ui_page_Aux3_init,
    .start_cb   = ui_page_Aux3_start,
    .exit_cb    = ui_page_Aux3_exit,
    .clean_cb   = ui_page_Aux3_clean,
};

ui_PageContext ui_PageSel6Ct = {
    .id         = UI_PAGE_SEL_6,
    .screen     = NULL,
    .auto_del   = TRUE,
    .init_cb    = ui_page_MainOut_init,
    .start_cb   = ui_page_MainOut_start,
    .exit_cb    = ui_page_MainOut_exit,
    .clean_cb   = ui_page_MainOut_clean,
};

ui_PageContext ui_PageSel7Ct = {
    .id         = UI_PAGE_SEL_7,
    .screen     = NULL,
    .auto_del   = TRUE,
    .init_cb    = ui_page_Spec_init,
    .start_cb   = ui_page_Spec_start,
    .exit_cb    = ui_page_Spec_exit,
    .clean_cb   = ui_page_Spec_clean,
};

ui_PageContext ui_PageSel8Ct = {
    .id         = UI_PAGE_SEL_8,
    .screen     = NULL,
    .auto_del   = TRUE,
    .init_cb    = ui_page_Dsp_init,
    .start_cb   = ui_page_Dsp_start,
    .exit_cb    = ui_page_Dsp_exit,
    .clean_cb   = ui_page_Dsp_clean,
};
/*################################### 函数(静态声明) ###################################*/
static ui_PageContext* ui_pmanager_create(uint8_t id);
static listNode* ui_pmanager_find(uint8_t id);
static void ui_pmanager_add(ui_PageContext *page);

/*################################### 函数(全局声明) ###################################*/

/*################################### 函数(静态定义) ###################################*/
static void screen_del_cb(lv_event_t *e)
{
    ui_PageContext *page= (ui_PageContext *)lv_event_get_user_data(e);
    
#ifdef CFG_FUNC_DEBUG_EN
    switch(page->id)
    {
        case UI_PAGE_SEL_1:
            printf("sel1 deleted\n");
            break;
        case UI_PAGE_SEL_2:
            printf("sel2 deleted\n");
            break;
        case UI_PAGE_SEL_3:
            printf("sel3 deleted\n");
            break;
        case UI_PAGE_SEL_4:
            printf("sel4 deleted\n");
            break;
        case UI_PAGE_SEL_5:
            printf("sel5 deleted\n");
            break;
        case UI_PAGE_SEL_6:
            printf("sel6 deleted\n");
            break;
        case UI_PAGE_SEL_7:
            printf("sel7 deleted\n");
            break;
        case UI_PAGE_SEL_8:
            printf("sel8 deleted\n");
            break;
        default:
            break;
    }
#endif

    page->clean_cb();
    page->screen = NULL;
}

static void screen_unloaded_cb(lv_event_t *e)
{
    ui_PageContext *page= (ui_PageContext *)lv_event_get_user_data(e);

#ifdef CFG_FUNC_DEBUG_EN
    switch(page->id)
    {
        case UI_PAGE_SEL_1:
            printf("sel1 unloaded\n");
            break;
        case UI_PAGE_SEL_2:
            printf("sel2 unloaded\n");
            break;
        case UI_PAGE_SEL_3:
            printf("sel3 unloaded\n");
            break;
        case UI_PAGE_SEL_4:
            printf("sel4 unloaded\n");
            break;
        case UI_PAGE_SEL_5:
            printf("sel5 unloaded\n");
            break;
        case UI_PAGE_SEL_6:
            printf("sel6 unloaded\n");
            break;
        case UI_PAGE_SEL_7:
            printf("sel7 unloaded\n");
            break;
        case UI_PAGE_SEL_8:
            printf("sel8 unloaded\n");
            break;
        default:
            break;
    }
#endif

    page->exit_cb(page->auto_del);
}

static void screen_loaded_cb(lv_event_t *e)
{
    ui_PageContext *page= (ui_PageContext *)lv_event_get_user_data(e);

#ifdef CFG_FUNC_DEBUG_EN
    switch(page->id)
    {
        case UI_PAGE_SEL_1:
            printf("sel1 loaded\n");
            break;
        case UI_PAGE_SEL_2:
            printf("sel2 loaded\n");
            break;
        case UI_PAGE_SEL_3:
            printf("sel3 loaded\n");
            break;
        case UI_PAGE_SEL_4:
            printf("sel4 loaded\n");
            break;
        case UI_PAGE_SEL_5:
            printf("sel5 loaded\n");
            break;
        case UI_PAGE_SEL_6:
            printf("sel6 loaded\n");
            break;
        case UI_PAGE_SEL_7:
            printf("sel7 loaded\n");
            break;
        case UI_PAGE_SEL_8:
            printf("sel8 loaded\n");
            break;
        default:
            break;
    }
#endif

    ui_pmanagerCt.id_cur = ui_pmanagerCt.id_tar;
	page->start_cb();
}

/**
 *@根据id查找页面，找到后创建此页面
 *@形参id:页面ID
 *@返回listNode*:NULL表示无此id页面，非NULL表示找到
 */
static ui_PageContext* ui_pmanager_create(uint8_t id)
{
	ui_PageContext* page = NULL;
	listNode *node = ui_pmanager_find(id);
    
	if(node != NULL)//找到
	{
		page = node->value;//[XH]node成立，value即page必然成立

        if(page->screen == NULL)//如果目标page的scr未创建，则马上创建
        {
            page->screen = lv_obj_create(NULL);
            lv_obj_add_event_cb(page->screen, screen_del_cb, LV_EVENT_DELETE, page);
            lv_obj_add_event_cb(page->screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, page);
            lv_obj_add_event_cb(page->screen, screen_loaded_cb, LV_EVENT_SCREEN_LOADED, page);

            page->init_cb(page->screen);
        }
	}
	
	return page;
}

/**
 *@根据id查找结点
 *@形参id:页面ID
 *@返回listNode*:NULL表示无此id结点，非NULL表示找到
 */
static listNode* ui_pmanager_find(uint8_t id)
{
	ui_PageContext *page = NULL;
	listNode *node = NULL;
	listIter *it   = listGetIterator(page_list, AL_START_HEAD);

    if(it != NULL)
    {
    	while( (node = listNext(it)) ) 
        {
    		page = node->value;//[XH]node成立，value即page必然成立
    		if(id == page->id) 
            {
                break;
            }
    	}
    	listReleaseIterator(it);
    }
	
	return node;
}

/**
 *@将指定page加入链表
 *@形参page:page指针
 *@返回:  无
 */
static void ui_pmanager_add(ui_PageContext *page)
{
	listNode *node = ui_pmanager_find(page->id);
	if(node != NULL) 
	{
		return;
	}
	
    page_list = listAddNodeTail(page_list, page);
#ifdef CFG_FUNC_DEBUG_EN
	if(page_list != NULL)
    {
		printf("[XH]ui_pmanager_add OK\n");
    }
    else
    {
        printf("[XH]ui_pmanager_add NG\n");
    }
#endif
}

/**
 *@将指定page从链表移除
 *@形参id:页面ID
 *@返回:  无
 */
//static void ui_pmanager_remove(uint8_t id)
//{
//	if(page_list == NULL) return;
//	listNode *node = ui_pmanager_find(id);
//	listDelNode(page_list, node);
//}

/*################################### 函数(全局定义) ###################################*/
void ui_pmanager_init(void)
{
    /***** 创建链表 *****/
    page_list = listCreate();
#ifdef CFG_FUNC_DEBUG_EN
    if(page_list != NULL)
    {
        printf("[XH]page_list create OK\n");
    }
#endif

    /***** 初始化ui_pmanager *****/
    ui_pmanagerCt.id_cur = UI_PAGE_SEL_1;
    ui_pmanagerCt.id_tar = UI_PAGE_SEL_1;
    // ui_pmanagerCt.FirstLogoFinishFlag = FALSE;

    /***** 链表节点 *****/
    ui_pmanager_add(&ui_PageSel1Ct);
    ui_pmanager_add(&ui_PageSel2Ct);
    ui_pmanager_add(&ui_PageSel3Ct);
    ui_pmanager_add(&ui_PageSel4Ct);
    ui_pmanager_add(&ui_PageSel5Ct);
    ui_pmanager_add(&ui_PageSel6Ct);
    ui_pmanager_add(&ui_PageSel7Ct);
    ui_pmanager_add(&ui_PageSel8Ct);
    // ui_pmanager_add(&ui_PagePcCt);
    // ui_pmanager_add(&ui_PageUDiskCt);
    // ui_pmanager_add(&ui_PageRecCt);
    // ui_pmanager_add(&ui_PageRecPlaybackCt);

    /***** 创建必要界面 + 开机LOGO界面 *****/
    ui_PageContext *page = ui_pmanager_create(UI_PAGE_SEL_1);
    if(page != NULL)
    {
        lv_scr_load_anim(page->screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
}

/**
 *@页面切换回调
 *@参数id: 页面ID
 *@参数del: 是否删除切换前的页面
 */
void ui_pmanager_switch(uint8_t id, uint32_t delay, uint8_t del)//[XH]暂时只在timer_cb和request调用
{
    // if(ui_pmanagerCt.FirstLogoFinishFlag == FALSE) return;

#ifdef CFG_FUNC_DEBUG_EN
    printf("[XH]ui_pmanager_switch(cur:%d, tar:%d)\n", (int)ui_pmanagerCt.id_cur, (int)ui_pmanagerCt.id_tar);
#endif

    /***** 新界面面创建 *****/
    listNode *node = ui_pmanager_find(id);
    if(node == NULL) return;

    ui_PageContext *page = node->value;//[XH]node成立，value即page必然成立
    if(page->screen == NULL)//如果目标page的scr未创建，则马上创建
    {
        page->screen = lv_obj_create(NULL);
        lv_obj_add_event_cb(page->screen, screen_del_cb, LV_EVENT_DELETE, page);
        lv_obj_add_event_cb(page->screen, screen_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, page);
        lv_obj_add_event_cb(page->screen, screen_loaded_cb, LV_EVENT_SCREEN_LOADED, page);
        
        page->init_cb(page->screen);
    }
    ui_pmanagerCt.id_tar = id;

    /***** 新界面动画 *****/
    lv_scr_load_anim_t dir;
    switch(id)
    {
        case UI_PAGE_SEL_1:
            dir = LV_SCR_LOAD_ANIM_MOVE_LEFT;
            lv_scr_load_anim(page->screen, dir, 200, delay, del);
            break;
        case UI_PAGE_SEL_2:
            dir = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
            lv_scr_load_anim(page->screen, dir, 200, delay, del);
            break;
        case UI_PAGE_SEL_5:
            dir = LV_SCR_LOAD_ANIM_MOVE_LEFT;
            lv_scr_load_anim(page->screen, dir, 200, delay, del);
            break;
        default:
            dir = LV_SCR_LOAD_ANIM_MOVE_LEFT;
            lv_scr_load_anim(page->screen, dir, 200, delay, del);
            break;
    }
}
