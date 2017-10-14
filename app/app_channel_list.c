#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "full_screen.h"
#include "app_pop.h"
#include "channel_edit.h"
#include "app_config.h"
#if TKGS_SUPPORT
#include "module/gxtkgs.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
extern void app_tkgs_cantwork_popup(void);
#endif


#define CMB_GROUP   "cmb_channel_list_group"
#define TXT_GROUP   "txt_group_sat_fav"
#define TXT_TP      "text_channel_list_tp"
#define LV_LIST     "listview_channel_list"
#define IMG_BOTTOM "img_cl_bottom"
#define IMG_RED "img_cl_red"
#define TXT_RED "text_cl_red"
#define IMG_GREEN "img_cl_green"
#define TXT_GREEN "text_cl_green"
#define IMG_YELLOW "img_cl_yellow"
#define TXT_YELLOW "text_cl_yellow"
#define FAV_MASK    (0x80000000)
#if ((MINI_256_COLORS_OSD_SUPPORT == 0)||(MINI_16_BITS_OSD_SUPPORT==0))
#define MOVE_FONT_ZOOM
#endif
/* Private variable------------------------------------------------------- */
#define ALL_SAT		0x5A5A5A5A
//bool g_Create;
/* widget macro -------------------------------------------------------- */

typedef enum
{
    LIST_MODE_PROG,
    LIST_MODE_SAT,
    LIST_MODE_FAV,
    LIST_MODE_TOTAL
}ListMode;

static int s_cur_sat_sel = -1;
static int s_cur_fav_sel = -1;

typedef struct channel_list ChannelList;
struct channel_list
{
    uint32_t first_in;// first in, when cmb change needn't play program again
    ListMode mode;
    AppIdOps  *id_ops[LIST_MODE_TOTAL];
    void (*init)(ChannelList*, ListMode);
};

extern int app_channel_list_list_change(GuiWidget *widget, void *usrdata);// for internal use
extern void channel_list_init(ChannelList *list, ListMode mode);
extern void app_create_ch_edit_fullsreeen(void);

ChannelList s_Cl = {
    .first_in = 0,
    .mode = LIST_MODE_PROG,
    .id_ops = {NULL},
    .init = channel_list_init
};

static inline void _show_group(char* str)
{
    if (str == NULL)
    {
        GUI_SetProperty(TXT_GROUP,  "state", "hide");
        GUI_SetProperty(CMB_GROUP,  "state", "show");
        GUI_SetProperty(IMG_BOTTOM,     "state", "show");
        GUI_SetProperty(TXT_TP,     "state", "show");

        GUI_SetProperty(IMG_RED,     "state", "show");
        GUI_SetProperty(TXT_RED,     "state", "show");
        GUI_SetProperty(IMG_GREEN,     "state", "show");
        GUI_SetProperty(TXT_GREEN,     "state", "show");
        GUI_SetProperty(IMG_YELLOW,     "state", "show");
        GUI_SetProperty(TXT_YELLOW,     "state", "show");
    }
    else
    {
    	  GUI_SetProperty("img_cl_body", "state", "hide");
        GUI_SetProperty("img_cl_body", "state", "show");
        GUI_SetProperty(CMB_GROUP,  "state", "hide");
        GUI_SetProperty(CMB_GROUP,  "draw_now", NULL);
        GUI_SetProperty(TXT_TP,     "state", "hide");
        
        GUI_SetProperty(IMG_RED,     "state", "hide");
        GUI_SetProperty(TXT_RED,     "state", "hide");
        GUI_SetProperty(IMG_GREEN,     "state", "hide");
        GUI_SetProperty(TXT_GREEN,     "state", "hide");
        GUI_SetProperty(IMG_YELLOW,     "state", "hide");
        GUI_SetProperty(TXT_YELLOW,     "state", "hide");

        GUI_SetProperty(IMG_BOTTOM,     "state", "hide");
        
        GUI_SetProperty(TXT_GROUP,  "string", str);
        GUI_SetProperty(TXT_GROUP,  "state", "show");
    }
}

static void _channel_list_cmb_group_build(void)
{
#define BUFFER_LENGTH_GROUP	(30)
	uint32_t i = 0;
	uint32_t total_id=0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH_GROUP] = {0};
	uint32_t *id_buffer = NULL;
	GxMsgProperty_NodeByIdGet node;
	AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];

	if(cl_id == NULL)
	    return;

	total_id = cl_id->id_total_get(cl_id);
	if(total_id == 0)
	{
		printf("\n_channel_list_cmb_group_build, total id error...%s,%d\n",__FILE__,__LINE__);
		return;
	}
	buffer_all = GxCore_Malloc(total_id*(BUFFER_LENGTH_GROUP));//need GxCore_Free
	if(buffer_all == NULL)
	{
		printf("\n_channel_list_cmb_group_build, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}
	memset(buffer_all, 0, total_id*(BUFFER_LENGTH_GROUP));

	//get all of id
	id_buffer = GxCore_Malloc(total_id*sizeof(uint32_t));//need GxCore_Free
	if(id_buffer == NULL)
	{
		printf("\n_channel_list_cmb_group_build, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}
	memset(id_buffer, 0, total_id*sizeof(uint32_t));
	cl_id->id_get(cl_id, id_buffer);

	//first one
	memset(buffer, 0, BUFFER_LENGTH_GROUP);
	sprintf(buffer,"[%s",STR_ID_ALL);
	strcat(buffer_all, buffer);

	for(i=1; i<total_id; i++)
	{
		if((id_buffer[i] & FAV_MASK) == 0)//sat
		{
			node.node_type = NODE_SAT;
			node.id = id_buffer[i];
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

			memset(buffer, 0, BUFFER_LENGTH_GROUP);
			sprintf(buffer,",%s",node.sat_data.sat_s.sat_name);
		}
		else //fav
		{
			node.node_type = NODE_FAV_GROUP;
			node.id = (id_buffer[i] & (~FAV_MASK));
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

			memset(buffer, 0, BUFFER_LENGTH_GROUP);
			sprintf(buffer,",%s",node.fav_item.fav_name);
		}
		strcat(buffer_all, buffer);
	}
	strcat(buffer_all, "]");
	GUI_SetProperty("cmb_channel_list_group","content",buffer_all);

	GxCore_Free(buffer_all);
	GxCore_Free(id_buffer);
}

//根据当前view_info找到现在是第几个组
static int _channel_list_get_cur_group_num(void)
{
	uint32_t i = 0;
	uint32_t total_id=0;
	uint32_t fav_id=0;
	uint32_t *id_buffer = NULL;
	AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];

	if(cl_id == NULL)
	    return -1;

	total_id = cl_id->id_total_get(cl_id);
	if(total_id == 0)
	{
		printf("\n_channel_list_get_cur_group_num, total_id..%s,%d\n",__FILE__,__LINE__);
		return -1;
	}
	//get all of id
	id_buffer = GxCore_Malloc(total_id*sizeof(uint32_t));//need GxCore_Free
	if(id_buffer == NULL)
	{
		printf("\n_channel_list_get_cur_group_num, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return -1;
	}
	memset(id_buffer, 0, total_id*sizeof(uint32_t));
	cl_id->id_get(cl_id, id_buffer);

	if(0 == g_AppPlayOps.normal_play.view_info.group_mode)//all satellite
	{
		GxCore_Free(id_buffer);
		return 0;
	}
	else if(1 == g_AppPlayOps.normal_play.view_info.group_mode)//sat
	{
		for(i=1; i<total_id; i++)
		{
			if((id_buffer[i] & FAV_MASK) == 0)//sat
			{
				if(g_AppPlayOps.normal_play.view_info.sat_id == id_buffer[i])
                {
                    GxCore_Free(id_buffer);
					return i;
                }
			}
		}
	}
	else if(2 == g_AppPlayOps.normal_play.view_info.group_mode)//fav 
	{
		for(i=1; i<total_id; i++)
		{
			if((id_buffer[i] & FAV_MASK) != 0)//fav
			{
				fav_id = (id_buffer[i] & (~FAV_MASK));
				if(g_AppPlayOps.normal_play.view_info.fav_id == fav_id)
                {
                    GxCore_Free(id_buffer);
					return i;
                }
			}
		}
	}

    GxCore_Free(id_buffer);
	//default all
	return 0;
}

void porg_list_init(ChannelList *list, ListMode mode)
{
    int sel = 0;
    int active_sel = -1;

    _show_group(NULL);

    //all satellite
    sel = g_AppPlayOps.normal_play.play_count;
    GUI_SetProperty(LV_LIST, "select", &sel);
#if (MINI_256_COLORS_OSD_SUPPORT > 0)
    active_sel = -1;
#else
    active_sel = sel;
#endif
    GUI_SetProperty(LV_LIST, "active", &active_sel);
    //init cmb select
    sel = _channel_list_get_cur_group_num();
    GUI_SetProperty("cmb_channel_list_group", "select", &sel);

    GUI_SetFocusWidget(LV_LIST);

    //	g_Create = FALSE;
    list->mode = mode;
    // app_channel_list_group_change play do here, fix passwd problem
    g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
	

}

void sat_fav_list_init(ChannelList *list, ListMode mode)
{
    int sel = -1;
    int active_sel = -1;
    //uint32_t sel_id = 0;
    AppIdOps *id_ops = NULL;
    uint32_t id_total = 0;
    uint32_t *id_buf = NULL;
    int i = 0;
    GxMsgProperty_NodeByIdGet node = {0};
   // GxMsgProperty_NodeNumGet node_fav= {0};

    if (mode == LIST_MODE_SAT)
        _show_group(STR_ID_SATELLITE);
    else
        _show_group(STR_ID_FAV);

    list->mode = mode;
    id_ops = list->id_ops[list->mode];

    // get_cur_group_select
    if (list->mode == LIST_MODE_FAV)
    {
        if(g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_FAV) //fav
        {
            if((id_total=id_ops->id_total_get(id_ops)) != 0)
            {
                id_buf = GxCore_Malloc(id_total*sizeof(uint32_t));
                if (id_buf != NULL)
                {
                    id_ops->id_get(id_ops, id_buf);
                    //get the node.prog_data.favorite_flag , when in NODE_PROG mode
#if 1 //no need to find node by pos ,because one prog maybe have two or more fav group.
			for(i=1; i<id_total; i++)
			{
				if((id_buf[i] & FAV_MASK) != 0)//fav
				{					
					if(g_AppPlayOps.normal_play.view_info.fav_id == (id_buf[i] & (~FAV_MASK)))
			                {
			                 sel = i;
					   break;
			                }
				}
			}	
#else
                    node.node_type = NODE_PROG;
                    node.pos = g_AppPlayOps.normal_play.play_count;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

                    //search the sel_id of this favorite program
                    node_fav.node_type = NODE_FAV_GROUP;
                    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_fav);
                    for(i=0;i<node_fav.node_num;i++)
                    {
                        if(0 != (node.prog_data.favorite_flag&(1<<i)))
                        {
                            sel_id = i+1;//prog  --->multi fav group?
                            break;
                        }
                    }

                    for(i=0;i<id_total;i++)
                    {
                        if (id_buf[i] == ALL_SAT)
                            ;
                        else if((id_buf[i]&(~FAV_MASK)) == sel_id)
                        {
                            sel = i;
                            break;
                        }
                    }
#endif
                    GxCore_Free(id_buf);
                    id_buf = NULL;
                }
            }
        }

        s_cur_fav_sel = sel;
    }
    else if(list->mode == LIST_MODE_SAT)
    {
        if(g_AppPlayOps.normal_play.view_info.group_mode != GROUP_MODE_SAT) //all or fav
        {
            sel = 0;
        }
        else //sat
        {
            if((id_total=id_ops->id_total_get(id_ops)) != 0)
            {
                id_buf = GxCore_Malloc(id_total*sizeof(uint32_t));
                if (id_buf != NULL)
                {
                    id_ops->id_get(id_ops, id_buf);
                    //get the node.prog_data.sat_id, when in NODE_PROG mode
                    node.node_type = NODE_PROG;
                    node.pos = g_AppPlayOps.normal_play.play_count;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

                    for(i=0;i<id_total;i++)
                    {
                        if (id_buf[i] == ALL_SAT)
                            ;
                        else if((id_buf[i]&(~FAV_MASK)) == node.prog_data.sat_id)
                        {
                            sel = i;
                            break;
                        }
                    }

                    GxCore_Free(id_buf);
                    id_buf = NULL;
                }
            }
        }
        s_cur_sat_sel = sel;
    }

    GUI_SetFocusWidget(LV_LIST);
    GUI_SetProperty(LV_LIST, "update_all", NULL);
#if (MINI_256_COLORS_OSD_SUPPORT > 0)
    active_sel = -1;
#else
    active_sel = sel;
#endif
	if(active_sel < 0)
	{
		active_sel = 0;
	}
    GUI_SetProperty(LV_LIST, "active", &active_sel);
    if(sel < 0)
        sel = 0;
    GUI_SetProperty(LV_LIST, "select", &sel);
}

static status_t porg_list_get_data(ChannelList *list, void *usrdata)
{
#define CHANNEL_NUM_LEN	    (10)
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByPosGet node;
	static char buffer[CHANNEL_NUM_LEN];
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	node.node_type = NODE_PROG;
	node.pos = item->sel;
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

	//col-0: num
	item->x_offset = 2;
	item->image = NULL;
	memset(buffer, 0, CHANNEL_NUM_LEN);
#if TKGS_SUPPORT
	if(GX_TKGS_OFF != app_tkgs_get_operatemode())
	{
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			sprintf(buffer, "%04d", prog_ext_info.tkgs.logicnum);
		}
		else
		{
			sprintf(buffer, "%04d", (item->sel+1));
		}
	}
	else
	{
		sprintf(buffer, "%04d", (item->sel+1));
	}
#else
	sprintf(buffer, "%04d", (item->sel+1));
#endif
	item->string = buffer;

	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;

	item->string = (char*)node.prog_data.prog_name;

    //col-2: $
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        item->image = "s_icon_money.bmp";
    }
    else
    {
        item->image = NULL;
    }

    //col-3: lock
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        item->image = "s_icon_lock.bmp";
	}
    else
    {
        item->image = NULL;
    }

	 //col-4: hd
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;

    if (g_AppPvrOps.state == PVR_RECORD && node.prog_data.id == g_AppPvrOps.env.prog_id)
    {
        item->image = "s_ts_red.bmp";
    }
    else
    {
        if (node.prog_data.definition == GXBUS_PM_PROG_HD)
        {
            item->image = "s_icon_hd2.bmp";
        }
        else
        {
            item->image = NULL;
        }
    }
	
    return GXCORE_SUCCESS;
}

static status_t sat_fav_get_data(ChannelList *list, void *usrdata)
{
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByIdGet node;
    AppIdOps *id_ops = list->id_ops[list->mode];
    uint32_t id_total = 0;
    uint32_t *id_buf = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
	item->image = NULL;
	item->string = NULL;
	
    if ((id_total=id_ops->id_total_get(id_ops)) == 0)
        return GXCORE_ERROR;

    id_buf = GxCore_Malloc(id_total*sizeof(uint32_t));
    if (id_buf == NULL)     return GXCORE_ERROR;

    id_ops->id_get(id_ops, id_buf);

   	//col-1: sat name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;

    if (id_buf[item->sel] == ALL_SAT)
    {
        item->string = STR_ID_ALL;
    }
    else
    {
        if (list->mode == LIST_MODE_FAV)
        {
            node.node_type = NODE_FAV_GROUP;
            node.id = id_buf[item->sel]&(~FAV_MASK);

            //get node
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
            item->string = (char*)node.fav_item.fav_name;
        }
        else if (list->mode == LIST_MODE_SAT)
        {
            node.node_type = NODE_SAT;
            node.id = id_buf[item->sel];
	    printf("\nsat ############### %d  node.id:%d\n",item->sel,node.id);

            //get node
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
            item->string = (char*)node.sat_data.sat_s.sat_name;
        }
        else
        {}
    }

    GxCore_Free(id_buf);
    id_buf = NULL;

    return GXCORE_SUCCESS;
}

void channel_list_init(ChannelList *list, ListMode mode)
{
    if(list->mode == mode)
    {
	// inorder app_channel_list_group_change not do play, fix passwd problem
        s_Cl.first_in = 1;
    }
    switch(mode)
    {
        case LIST_MODE_PROG:
            porg_list_init(list, mode);
            break;
        case LIST_MODE_SAT:
        case LIST_MODE_FAV:
            sat_fav_list_init(list, mode);
            break;
        default:
            break;
    }
}

void sat_fav_keypress(ChannelList *list, uint32_t key, int sel)
{
    AppIdOps *id_ops = list->id_ops[list->mode];
    GxBusPmViewInfo view_info;
	uint32_t *id_buf = NULL;

    switch (key)
    {
        case STBK_OK:
            id_buf = GxCore_Malloc(id_ops->id_total_get(id_ops)*sizeof(uint32_t));
            if((id_buf == NULL) || (sel < 0))
                return;

            id_ops->id_get(id_ops, id_buf);

            app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
            if (id_buf[sel] == ALL_SAT)
            {
                view_info.group_mode = GROUP_MODE_ALL;	//all
            }
            else
            {
                if((id_buf[sel] & FAV_MASK) == 0)//sat
                {
                    view_info.group_mode = GROUP_MODE_SAT;
                    view_info.sat_id = id_buf[sel];
                }
                else//fav
                {
                    view_info.group_mode = GROUP_MODE_FAV;
                    view_info.fav_id = id_buf[sel]&(~FAV_MASK);
                }
            }

            GxCore_Free(id_buf);
            id_buf = NULL;

            g_AppFullArb.state.pause = STATE_OFF;
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
            g_AppPlayOps.play_list_create(&(view_info));
            list->init(list, LIST_MODE_PROG);
            break;

        case STBK_MENU:
        case STBK_EXIT:
            list->init(list, LIST_MODE_PROG);
            GUI_SetProperty(LV_LIST, "update_all", NULL);
            break;
        default:
            break;
    }
    // inorder to refresh tp info
    app_channel_list_list_change(NULL, NULL);
}

static void _fav_flag_to_id_add(uint32_t flag)//flag != 0
{
	uint32_t i=0;
    GxMsgProperty_NodeNumGet node_num = {0};
	GxMsgProperty_NodeByPosGet node_get = {0};
    AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
    AppIdOps *fav_id = s_Cl.id_ops[LIST_MODE_FAV];

    if ((flag == 0) || (cl_id == NULL) || (fav_id == NULL))
        return;

    node_num.node_type = NODE_FAV_GROUP;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);

	for(i=0; i<node_num.node_num; i++)
	{
		if(0 != (flag&(1<<i)))
		{
            node_get.node_type = NODE_FAV_GROUP;
            node_get.pos = i;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

            cl_id->id_add(cl_id, 0, node_get.fav_item.id|FAV_MASK);
            fav_id->id_add(fav_id, 0, node_get.fav_item.id|FAV_MASK);
		}
	}
}

static void _get_valid_group_id(void)
{
	GxBusPmViewInfo view_info;
	GxBusPmViewInfo view_info_bak;
	GxMsgProperty_NodeNumGet prog_num = {0};
	GxMsgProperty_NodeNumGet sat_num = {0};
	GxMsgProperty_NodeNumGet fav_num = {0};
	GxMsgProperty_NodeByPosGet node_get = {0};
	uint32_t i=0;
	uint32_t fav_flag=0;
    AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
    AppIdOps *sat_id = s_Cl.id_ops[LIST_MODE_SAT];
    AppIdOps *fav_id = s_Cl.id_ops[LIST_MODE_FAV];

    if ((sat_id == NULL) || (cl_id == NULL) || (fav_id == NULL))  
        return;

    // get total sat
    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);

    // get total fav
    fav_num.node_type = NODE_FAV_GROUP;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &fav_num);

    // get and sort to total program
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	view_info_bak = view_info;	
	view_info.group_mode = 0;	//all
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));
    
    // get total prog 
    prog_num.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &prog_num);


	// id manage
	if ((cl_id->id_open(cl_id, MANAGE_DIFF_LIST, sat_num.node_num+fav_num.node_num+1) != GXCORE_SUCCESS)
            || (sat_id->id_open(sat_id, MANAGE_DIFF_LIST, sat_num.node_num+1) != GXCORE_SUCCESS)
            || (fav_id->id_open(fav_id, MANAGE_DIFF_LIST, fav_num.node_num) != GXCORE_SUCCESS)) 
		return;

	//add 0: all satellite
	cl_id->id_add(cl_id, 0, ALL_SAT);
	sat_id->id_add(sat_id, 0, ALL_SAT);

	//get sat id
	for(i=0; i<prog_num.node_num; i++)
	{
		node_get.node_type = NODE_PROG;
		node_get.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

		cl_id->id_add(cl_id, 0, node_get.prog_data.sat_id);
        sat_id->id_add(sat_id, 0, node_get.prog_data.sat_id);
	}
    
	//get fav id
	for(i=0; i<prog_num.node_num; i++)
	{
		node_get.node_type = NODE_PROG;
		node_get.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

        // collect fav flag
        fav_flag |= node_get.prog_data.favorite_flag;
	}

    _fav_flag_to_id_add(fav_flag);

	//restore
	view_info = view_info_bak;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

}

#define CHANNEL_LIST
#ifdef MOVE_FONT_ZOOM
static void app_channel_list_get_data_font(void)
{
	int select = 0, i = 0;
	int index = 0;
	GuiWidget *value = NULL;

	char *item_name[12] = {"listitem_channel_1", "listitem_channel_2",
			               "listitem_channel_3", "listitem_channel_4",
                           "listitem_channel_5", "listitem_channel_6",
                           "listitem_channel_7", "listitem_channel_8",
                           "listitem_channel_9", "listitem_channel_10"};

	//GUI_GetProperty(LV_LIST, "select", &select);
	for(i = 0; i <10; i++)
	{
		GUI_SetProperty(item_name[i], "font", "roboto");
	}

	for(index = 0; index<10;index++)
	{
		GUI_GetProperty(LV_LIST, "focus_widget", &value);

		if((value&&(value->name!=NULL)) && (0 == strcmp(value->name, item_name[index])))
		{
			select = index;
			break;
		}
	}

	GUI_SetProperty(item_name[select], "font", "font_prog");//font_mid
	return;
}
#endif

SIGNAL_HANDLER int app_channel_list_create(GuiWidget *widget, void *usrdata)
{
    int sel = 0;
    int active_sel = -1;

    for (sel=0; sel<LIST_MODE_TOTAL; sel++)
    {
        if( s_Cl.id_ops[sel] != NULL)
        {
            del_id_ops(s_Cl.id_ops[sel]);
            s_Cl.id_ops[sel] = NULL;
        }
        s_Cl.id_ops[sel] = new_id_ops();
    }

    //-- prepare valid group id & commonbox item , before any list init --//
    _get_valid_group_id();
    _channel_list_cmb_group_build();
    //--prepare end--//

    // TODO shenbin !!!!

    //init cmb select
    sel = _channel_list_get_cur_group_num();
    if (sel != 0)
    {
        // sel=0 all satellite, won't call app_channel_list_group_change, so keep this 0
        s_Cl.first_in = 1;
        GUI_SetProperty("cmb_channel_list_group", "select", &sel);
    }
    else
    {
        GUI_SetProperty(LV_LIST, "update_all", NULL);
        sel = g_AppPlayOps.normal_play.play_count;
        GUI_SetProperty(LV_LIST, "select", &sel);
#if (MINI_256_COLORS_OSD_SUPPORT > 0)
        active_sel = -1;
#else
        active_sel = sel;
#endif
        GUI_SetProperty(LV_LIST, "active", &active_sel);
    }
    app_channel_list_list_change(NULL, NULL);
    GUI_SetFocusWidget(LV_LIST);
#ifdef MOVE_FONT_ZOOM
    app_channel_list_get_data_font();
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_list_destroy(GuiWidget *widget, void *usrdata)
{
    uint32_t i = 0;

    for (i=0; i<LIST_MODE_TOTAL; i++)
    {
        del_id_ops(s_Cl.id_ops[i]);
        s_Cl.id_ops[i] = NULL;
    }
	s_Cl.mode = LIST_MODE_PROG;
	return EVENT_TRANSFER_STOP;
}

static void _ch_list_pop_sort_exit(PopDlgRet ret, uint32_t pos)
{
#if (MINI_256_COLORS_OSD_SUPPORT == 1)
	int32_t unactive = -1;
	GUI_SetProperty(LV_LIST, "active", &unactive);
#endif

	if(ret == POP_VAL_OK)
	{
		GUI_SetProperty(LV_LIST, "update_all", NULL);
		GUI_SetProperty(LV_LIST, "select", &pos);
#if (MINI_256_COLORS_OSD_SUPPORT == 0)
        GUI_SetProperty(LV_LIST, "active", &pos);
#endif	
		g_AppPlayOps.normal_play.play_count = pos;		
		#if RECALL_LIST_SUPPORT
		//g_AppPlayOps.add_recall_list(&(g_AppPlayOps.normal_play));
		g_AppPlayOps.recall_play[0] = g_AppPlayOps.normal_play;
		#else
		g_AppPlayOps.recall_play = g_AppPlayOps.normal_play; //lost recall after sort 
		#endif
		g_AppPlayOps.current_play= g_AppPlayOps.normal_play;
		if(g_AppFullArb.state.pause == STATE_ON)
		{
			g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
			g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
		}
	}
}

SIGNAL_HANDLER int app_channel_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
                case STBK_SAT:
                    if(s_Cl.mode == LIST_MODE_SAT)
                    {
                        break;
                    }
                    GUI_SetProperty("wnd_channel_list", "state", "hide");
                    if (s_Cl.mode != LIST_MODE_SAT)
                    {
                        s_Cl.init(&s_Cl, LIST_MODE_SAT);
                    }
                    GUI_SetInterface("flush",NULL);
                    GUI_SetProperty("wnd_channel_list", "state", "show");
                    break;

                case STBK_FAV:
                    {
                        AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
                        AppIdOps *sat_id = s_Cl.id_ops[LIST_MODE_SAT];

                        if((cl_id == NULL) || (sat_id == NULL) || s_Cl.mode == LIST_MODE_FAV)
                        {
                            break;
                        }
                        if (cl_id->id_total_get(cl_id) == sat_id->id_total_get(sat_id))
                        {
                            PopDlg pop;

                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_NO_BTN;
                            pop.str = STR_ID_NO_FAV;
                            pop.timeout_sec = 2;
                            pop.mode = POP_MODE_UNBLOCK;
                            popdlg_create(&pop);
                            break;
                        }
						GUI_SetProperty("wnd_channel_list", "state", "hide");
                        if (s_Cl.mode != LIST_MODE_FAV)
                        {
                            s_Cl.init(&s_Cl, LIST_MODE_FAV);
                        }
						GUI_SetInterface("flush",NULL);
						GUI_SetProperty("wnd_channel_list", "state", "show");
                    }
                    break;
				case STBK_DOWN:
				case STBK_UP:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					{
						ret = EVENT_TRANSFER_STOP;
					}
					break;
					
				case STBK_EXIT:
				case STBK_MENU:
                    {
						GUI_EndDialog("wnd_channel_list");
                        GUI_SetInterface("flush", NULL);
                        app_create_dialog("wnd_channel_info");
					}
				case VK_BOOK_TRIGGER:
					{
						GUI_EndDialog("wnd_channel_list");
                        GUI_SetInterface("flush", NULL);
						app_create_dialog("wnd_channel_info");//错误 #44897
					}
					break;

#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
					{
						GUI_EndDialog("wnd_channel_list");
                        GUI_SetInterface("flush", NULL);
						GUI_SendEvent("wnd_full_screen", event);
					}
					break;
#endif
				case STBK_GREEN: //find
					GUI_EndDialog("wnd_channel_list");
                    GUI_SetInterface("flush", NULL);
					app_create_dialog("wnd_find");
					break;
				case STBK_YELLOW://edit
					GUI_EndDialog("wnd_channel_list");
                    GUI_SetInterface("flush", NULL);
					app_create_ch_edit_fullsreeen();

                    if(g_AppPvrOps.state != PVR_DUMMY)
                    {
                        app_create_dialog("wnd_channel_info");
                    }
					break;
				case STBK_RED: //sort
					if (g_AppPvrOps.state != PVR_DUMMY)
					{
						PopDlg pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_NO_BTN;
						pop.format = POP_FORMAT_DLG;
						pop.str = STR_ID_STOP_PVR_FIRST;
						pop.mode = POP_MODE_UNBLOCK;
						pop.timeout_sec = 3;
						popdlg_create(&pop);
						break;
					}

#if TKGS_SUPPORT
					if(GX_TKGS_AUTOMATIC == app_tkgs_get_operatemode())
					{
						app_tkgs_cantwork_popup();
						break;
					}
					else
#endif
					{
						PopDlgPos pop_pos = {0};
#if (MINI_256_COLORS_OSD_SUPPORT == 1)
						uint32_t sel=0;
						GUI_GetProperty(LV_LIST, "select", &sel);
						GUI_SetProperty(LV_LIST, "active", &sel);
#endif
						app_pop_sort_create(pop_pos, g_AppPlayOps.normal_play.play_count, _ch_list_pop_sort_exit);
						break;
					}

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_channel_list_lost_focus(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;

	if (s_Cl.mode == LIST_MODE_PROG)
	{
		sel = g_AppPlayOps.normal_play.play_count;
		GUI_SetProperty(LV_LIST, "select", &sel);
		GUI_SetProperty(LV_LIST, "active", &sel);
	}
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_channel_list_got_focus(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;

    GxMsgProperty_NodeByIdGet node;

	if (s_Cl.mode == LIST_MODE_PROG)
    {
        sel = g_AppPlayOps.normal_play.play_count;
        GUI_SetProperty(LV_LIST, "select", &sel);
        GUI_SetProperty(LV_LIST, "active", &sel);
    }
        //else if (s_Cl.mode == LIST_MODE_FAV)
	if(0)
	{
		printf("\n             fav mode get focus \n");	
		node.node_type = NODE_FAV_GROUP;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		printf("\n get focus   node.fav_item.id:%d\n",node.fav_item.id);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_list_group_change(GuiWidget *widget, void *usrdata)
{
#define _LEN 50
    uint32_t sel = 0;
    int active_sel = -1;
    uint32_t total_id=0;
    uint32_t *id_buffer = NULL;
    AppIdOps *cl_id = s_Cl.id_ops[LIST_MODE_PROG];
    static GxMsgProperty_NodeByPosGet ProgNode;
    static GxMsgProperty_NodeByIdGet TpNode;
    static GxMsgProperty_NodeByIdGet SatNode;
    char buffer[_LEN];
#if DOUBLE_S2_SUPPORT
    char buffertemp[_LEN];
    char* Tuner[2] = {"T1, ","T2, "};
#endif
    uint32_t old_id = 0;

    if(cl_id == NULL)
        return EVENT_TRANSFER_STOP;

    total_id = cl_id->id_total_get(cl_id);
    if(total_id == 0)
    {
        printf("\app_channel_list_group_change, total id failed...%s,%d\n",__FILE__,__LINE__);
        return 0;
    }
    //get all of id
    id_buffer = GxCore_Malloc(total_id*sizeof(uint32_t));//need GxCore_Free
    if(id_buffer == NULL)
    {
        printf("\app_channel_list_group_change, malloc failed...%s,%d\n",__FILE__,__LINE__);
        return 1;
    }

    if (s_Cl.first_in == 0)
    {
        GUI_GetProperty(LV_LIST, "select", &sel);
        ProgNode.node_type = NODE_PROG;
        ProgNode.pos = sel;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
        old_id = ProgNode.prog_data.id;
    }

    memset(id_buffer, 0, total_id*sizeof(uint32_t));
    cl_id->id_get(cl_id, id_buffer);

    GUI_GetProperty("cmb_channel_list_group", "select", &sel);
    if(0 == sel)//all satellite
    {
        g_AppPlayOps.normal_play.view_info.group_mode = 0;
    }
    else//sat or fav
    {
        if((id_buffer[sel] & FAV_MASK) == 0)//sat
        {
            g_AppPlayOps.normal_play.view_info.group_mode = 1;
            g_AppPlayOps.normal_play.view_info.sat_id = id_buffer[sel];
        }
        else//fav
        {
            g_AppPlayOps.normal_play.view_info.group_mode = 2;
            g_AppPlayOps.normal_play.view_info.fav_id = (id_buffer[sel] & (~FAV_MASK));
        }
    }

    g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));

    //update listview show
    GUI_SetProperty(LV_LIST,"update_all",NULL);
    sel = g_AppPlayOps.normal_play.play_count;
    GUI_SetProperty(LV_LIST,"select",&sel);
#if (MINI_256_COLORS_OSD_SUPPORT > 0)
    active_sel = -1;
#else
    active_sel = sel;
#endif
    GUI_SetProperty(LV_LIST, "active", &active_sel);

    //get prog node
    ProgNode.node_type = NODE_PROG;
    ProgNode.pos = sel;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);

    //get tp node
    TpNode.node_type = NODE_TP;
    TpNode.id = ProgNode.prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);

    SatNode.node_type = NODE_SAT;
    SatNode.id = ProgNode.prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &SatNode);

    memset(buffer, 0 ,_LEN);
#if DEMOD_DVB_T
    if(GXBUS_PM_SAT_DVBT2 == SatNode.sat_data.type)
    {
        char* bandwidth_str[3] = {"8M", "7M","6M"};

        if(TpNode.tp_data.tp_dvbt2.bandwidth <BANDWIDTH_AUTO )
            sprintf(buffer, "%d.%d/%s",TpNode.tp_data.frequency/1000, \
                    (TpNode.tp_data.frequency/100)%10,bandwidth_str[TpNode.tp_data.tp_dvbt2.bandwidth]);

    }
#endif
#if DEMOD_DVB_C
    if(GXBUS_PM_SAT_C == SatNode.sat_data.type)
    {
        char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

        if(TpNode.tp_data.tp_c.modulation < QAM_AUTO)
            sprintf(buffer,"%d /%s /%d", TpNode.tp_data.frequency/1000  \
                    ,modulation_str[TpNode.tp_data.tp_c.modulation],TpNode.tp_data.tp_c.symbol_rate/1000);
    }
#endif

#if DEMOD_DTMB
    if(GXBUS_PM_SAT_DTMB == SatNode.sat_data.type)
    {
    	if(GXBUS_PM_SAT_1501_DTMB == SatNode.sat_data.sat_dtmb.work_mode)
	    {
	        char* bandwidth_str[3] = {"8M", "7M","6M"};// HAVE 7M?

	        if(TpNode.tp_data.tp_dtmb.symbol_rate < BANDWIDTH_AUTO )
	            sprintf(buffer, "%d.%d/%s",TpNode.tp_data.frequency/1000, \
	                    (TpNode.tp_data.frequency/100)%10,bandwidth_str[TpNode.tp_data.tp_dtmb.symbol_rate]);
	    }
		else
		{
			char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

	        if(TpNode.tp_data.tp_dtmb.modulation < QAM_AUTO)
	            sprintf(buffer,"%d /%s /%d", TpNode.tp_data.frequency/1000  \
	                    ,modulation_str[TpNode.tp_data.tp_dtmb.modulation],TpNode.tp_data.tp_dtmb.symbol_rate/1000);
		}

    }
#endif


#if DEMOD_DVB_S
    if(GXBUS_PM_SAT_S == SatNode.sat_data.type)
    {
        char* PolarStr[2] = {"H", "V"};
        sprintf(buffer, "%d / %s / %d",
                TpNode.tp_data.frequency,
                PolarStr[TpNode.tp_data.tp_s.polar],
                TpNode.tp_data.tp_s.symbol_rate);
    }
#endif

#if DOUBLE_S2_SUPPORT
    memset(buffertemp,0,_LEN);
    sprintf(buffertemp, "%s",Tuner[ProgNode.prog_data.tuner]);
    strcat(buffertemp,buffer);
    memcpy(buffer,buffertemp,_LEN);
#endif
    GUI_SetProperty(TXT_TP,"string",buffer);

    memset(buffer, 0, _LEN);
    sprintf(buffer,",%s",SatNode.sat_data.sat_s.sat_name);
    GUI_SetProperty(CMB_GROUP,"string",buffer);

    // first in (lock) program needn't play again
    if (s_Cl.first_in == 1)
    {
        s_Cl.first_in = 0;
    }
    else
    {
		if(old_id != ProgNode.prog_data.id)
		{
			g_AppFullArb.state.pause = STATE_OFF;
			g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
			g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
		}
	}

    GxCore_Free(id_buffer);
    return EVENT_TRANSFER_STOP;
}

#define CHANNEL_LIST_LISTVIEW

SIGNAL_HANDLER int app_channel_list_list_change(GuiWidget *widget, void *usrdata)
{
#define TP_LEN	50
    uint32_t sel=0;
    static GxMsgProperty_NodeByPosGet ProgNode;
    static GxMsgProperty_NodeByIdGet TpNode;
    char buffer[TP_LEN];
#if DOUBLE_S2_SUPPORT
    char BufferTemp[TP_LEN];
    char* tuner[2] = {"T1, ","T2, "};
#endif
    //char* PolarStr[2] = {"H", "V"};

    GUI_GetProperty(LV_LIST, "select", &sel);
    //get prog node
    ProgNode.node_type = NODE_PROG;
    ProgNode.pos = sel;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);

    //get tp node
    TpNode.node_type = NODE_TP;
    TpNode.id = ProgNode.prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);
    memset(buffer, 0 ,TP_LEN);

    GxMsgProperty_NodeByIdGet SatNode;
    SatNode.node_type = NODE_SAT;
    SatNode.id = ProgNode.prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &SatNode);

#if DEMOD_DVB_T
    if(GXBUS_PM_SAT_DVBT2 == SatNode.sat_data.type)
    {
        char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};

        if(TpNode.tp_data.tp_dvbt2.bandwidth < BANDWIDTH_AUTO )
            sprintf(buffer, "%d.%d/%s",TpNode.tp_data.frequency/1000, \
                    (TpNode.tp_data.frequency/100)%10,bandwidth_str[TpNode.tp_data.tp_dvbt2.bandwidth]);
    }
#endif
#if DEMOD_DVB_C
    if(GXBUS_PM_SAT_C == SatNode.sat_data.type)
    {
        char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

        if(TpNode.tp_data.tp_c.modulation < QAM_AUTO)
            sprintf(buffer,"%d /%s /%d", TpNode.tp_data.frequency/1000  \
                    ,modulation_str[TpNode.tp_data.tp_c.modulation],TpNode.tp_data.tp_c.symbol_rate/1000);
    }
#endif

#if DEMOD_DTMB
    if(GXBUS_PM_SAT_DTMB == SatNode.sat_data.type)
    {
    	if(GXBUS_PM_SAT_1501_DTMB == SatNode.sat_data.sat_dtmb.work_mode)
	    {
	        char* bandwidth_str[3] = {"8M", "7M","6M"};// HAVE 7M?

	        if(TpNode.tp_data.tp_dtmb.symbol_rate < BANDWIDTH_AUTO )
	            sprintf(buffer, "%d.%d/%s",TpNode.tp_data.frequency/1000, \
	                    (TpNode.tp_data.frequency/100)%10,bandwidth_str[TpNode.tp_data.tp_dtmb.symbol_rate]);
	    }
		else
		{
			char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

	        if(TpNode.tp_data.tp_dtmb.modulation < QAM_AUTO)
	            sprintf(buffer,"%d /%s /%d", TpNode.tp_data.frequency/1000  \
	                    ,modulation_str[TpNode.tp_data.tp_dtmb.modulation],TpNode.tp_data.tp_dtmb.symbol_rate/1000);
		}

    }
#endif


#if DEMOD_DVB_S
    if(GXBUS_PM_SAT_S == SatNode.sat_data.type)
    {
        char* PolarStr[2] = {"H", "V"};
        sprintf(buffer, "%d / %s / %d",
                TpNode.tp_data.frequency,
                PolarStr[TpNode.tp_data.tp_s.polar],
                TpNode.tp_data.tp_s.symbol_rate);
    }
#endif

#if DOUBLE_S2_SUPPORT
    memset(BufferTemp, 0 ,TP_LEN);
    sprintf(BufferTemp,"%s",tuner[ProgNode.prog_data.tuner]);
    strcat(BufferTemp,buffer);
    memcpy(buffer,BufferTemp,TP_LEN);
#endif
    GUI_SetProperty(TXT_TP,"string",buffer);

#ifdef MOVE_FONT_ZOOM
    app_channel_list_get_data_font();
#endif

#if (MINI_256_COLORS_OSD_SUPPORT > 0)
    int active_sel = -1;
    ChannelList *list = &s_Cl;

    if(list->mode == LIST_MODE_PROG)
    {
        if(g_AppPlayOps.normal_play.play_count == sel)
        {
            active_sel = -1;
        }
        else
        {
            active_sel = g_AppPlayOps.normal_play.play_count;
        }
    }
    else if(list->mode == LIST_MODE_SAT)
    {
        if(s_cur_sat_sel == sel)
        {
            active_sel = -1;
        }
        else
        {
            active_sel = s_cur_sat_sel;
        }
    }
    else if(list->mode == LIST_MODE_FAV)
    {
        if(s_cur_fav_sel == sel)
        {
            active_sel = -1;
        }
        else
        {
            active_sel = s_cur_fav_sel;
        }
    }
    GUI_SetProperty(LV_LIST, "active", &active_sel);
#endif

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_list_list_get_total(GuiWidget *widget, void *usrdata)
{
    ChannelList *list = &s_Cl;
    AppIdOps *id_ops = list->id_ops[list->mode];
    uint32_t total = 0;

    if(id_ops != NULL)
    {
        if (list->mode == LIST_MODE_PROG)
        {
            total = g_AppPlayOps.normal_play.play_total;
        }
        else
        {
            total = id_ops->id_total_get(id_ops);
        }
    }
    return total;
}

SIGNAL_HANDLER int app_channel_list_list_get_data(GuiWidget *widget, void *usrdata)
{
    if (s_Cl.mode == LIST_MODE_PROG)
    {
        porg_list_get_data(&s_Cl, usrdata);
    }
    else
    {
        sat_fav_get_data(&s_Cl, usrdata);
    }

	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_channel_list_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t sel=0;
    pvr_state cur_pvr = PVR_DUMMY;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_DOWN:
                case STBK_UP:
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

#if (TURN_OVER_MODE == 0)
                case STBK_LEFT:
                case STBK_RIGHT:
                    {
                        //save cur prog num of cur sat
                        if (s_Cl.mode == LIST_MODE_PROG)
                        {
                            if (g_AppPvrOps.state != PVR_DUMMY)
                            {
                                PopDlg pop;

                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_YES_NO;

                                if(g_AppPvrOps.state == PVR_TP_RECORD)
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_TP_RECORD;
                                }
                                else if(g_AppPvrOps.state == PVR_TMS_AND_REC)
                                {
#if CA_SUPPORT
                                    if(app_extend_control_pvr_change())
                                    {
                                        pop.str = STR_ID_STOP_REC_INFO;
                                        cur_pvr = PVR_RECORD;
                                    }
                                    else
#endif
                                    {
                                        pop.str = STR_ID_STOP_TMS_INFO;
                                        cur_pvr = PVR_TIMESHIFT;
                                    }
                                }
                                else if(g_AppPvrOps.state == PVR_RECORD)
                                {
#if CA_SUPPORT
                                    if(app_extend_control_pvr_change())
                                    {
                                        pop.str = STR_ID_STOP_REC_INFO;
                                        cur_pvr = PVR_RECORD;
                                    }
#endif
                                }
                                else
                                {
                                    pop.str = STR_ID_STOP_TMS_INFO;
                                    cur_pvr = PVR_TIMESHIFT;
                                }

                                if(cur_pvr != PVR_DUMMY)
                                {
                                    if (POP_VAL_OK != popdlg_create(&pop))
                                    {
                                        break;
                                    }

                                    if(cur_pvr == PVR_TIMESHIFT)
                                    {
                                        app_pvr_tms_stop();
                                    }
                                    else
                                    {
                                        app_pvr_stop();
                                    }
                                    GxMsgProperty_NodeByPosGet node_prog = {0};
                                    node_prog.node_type = NODE_PROG;
                                    node_prog.pos = g_AppPlayOps.normal_play.play_count;
                                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
                                    if((g_AppPvrOps.state == PVR_RECORD && node_prog.prog_data.id == g_AppPvrOps.env.prog_id)
                                            || cur_pvr == PVR_TIMESHIFT)
                                    {
                                        g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
                                        g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
                                        //g_AppPmt.start(&g_AppPmt);
                                    }
                                }
                            }
                            GUI_SendEvent("cmb_channel_list_group", event);
                        }
                    }
                    break;
#endif

                case STBK_EXIT:
                case STBK_MENU:
                case VK_BOOK_TRIGGER:
                    {
                        if (1)//(s_Cl.mode == LIST_MODE_PROG)
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                        else
                        {
                            sat_fav_keypress(&s_Cl, event->key.sym, 0);
                        }
                    }
                    break;

                case STBK_RED:
                case STBK_GREEN:
                case STBK_YELLOW:
                    if(s_Cl.mode == LIST_MODE_PROG)
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_OK:
                    {
                        GxMsgProperty_NodeByPosGet node_prog_tag = {0};
                        GxMsgProperty_NodeByPosGet node_prog_tmp = {0};
                        int active_sel = -1;
                        int i;

                        GUI_GetProperty(LV_LIST, "select", &sel);
                        node_prog_tag.node_type = NODE_PROG;
                        node_prog_tag.pos = sel;
                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tag);



                        // add in 20121107 for TIME SHIFT OK press,
                        //start
                        if(((PVR_TMS_AND_REC == g_AppPvrOps.state)
                                    || (PVR_TIMESHIFT == g_AppPvrOps.state))
                                && (node_prog_tag.prog_data.id == g_AppPvrOps.env.prog_id)
                                && (LIST_MODE_PROG == s_Cl.mode))
                        {
                            GUI_EndDialog("wnd_channel_list");
                            GUI_SetInterface("flush",NULL);
                            app_create_dialog("wnd_channel_info");
                            break;
                        }
                        //end
                        if (g_AppPvrOps.state != PVR_DUMMY)
                        {
                            PopDlg pop;
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_YES_NO;

                            if(g_AppPvrOps.state == PVR_TP_RECORD)
                            {
                                pop.str = STR_ID_STOP_REC_INFO;
                                cur_pvr = PVR_TP_RECORD;
                            }
                            else if(g_AppPvrOps.state == PVR_TMS_AND_REC)
                            {
#if CA_SUPPORT
                                if(app_extend_control_pvr_change())
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_RECORD;
                                }
                                else
#endif
                                {
                                    pop.str = STR_ID_STOP_TMS_INFO;
                                    cur_pvr = PVR_TIMESHIFT;
                                }
                            }
                            else if(g_AppPvrOps.state == PVR_RECORD)
                            {
#if CA_SUPPORT
                                if(app_extend_control_pvr_change())
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_RECORD;
                                }
#endif
                            }
                            else
                            {
                                pop.str = STR_ID_STOP_TMS_INFO;
                                cur_pvr = PVR_TIMESHIFT;
                            }

                            if(cur_pvr != PVR_DUMMY)
                            {
                                if (POP_VAL_OK != popdlg_create(&pop))
                                {
                                    break;
                                }
                                g_AppPlayOps.normal_play.play_count = sel;

                                if(cur_pvr == PVR_TIMESHIFT)
                                {
                                    app_pvr_tms_stop();
                                }
                                else
                                {
                                    app_pvr_stop();
                                }
                            }
                        }
#if (MINI_256_COLORS_OSD_SUPPORT > 0)
                        active_sel = -1;
#else
                        active_sel = sel;
#endif

                        if (s_Cl.mode == LIST_MODE_PROG)
                        {
                            GUI_SetProperty(LV_LIST, "active", &active_sel);//
                            node_prog_tmp.node_type = NODE_PROG;
                            node_prog_tmp.pos = g_AppPlayOps.normal_play.play_count;
                            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);

                            if((cur_pvr != PVR_DUMMY)
                                    || (node_prog_tag.prog_data.id != node_prog_tmp.prog_data.id)
                                    || (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
                                    || (g_AppFullArb.state.pause == STATE_ON))
                            {
                                if(cur_pvr == PVR_RECORD)
                                {
                                    GUI_SetProperty(LV_LIST,"update_all",NULL);
                                    for(i = 0; i < g_AppPlayOps.normal_play.play_total; i++)
                                    {
                                        node_prog_tmp.node_type = NODE_PROG;
                                        node_prog_tmp.pos = i;
                                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);
                                        if(node_prog_tmp.prog_data.id == node_prog_tag.prog_data.id)
                                        {
                                            sel = i;
                                            break;
                                        }
                                    }
                                    cur_pvr = PVR_DUMMY;
                                }
                                GUI_SetProperty(LV_LIST, "select", &sel);
                                g_AppFullArb.state.pause = STATE_OFF;
                                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                g_AppPlayOps.program_play(PLAY_MODE_POINT, sel);
                            }
                            else
                            {
                                GUI_EndDialog("wnd_channel_list");
                                GUI_SetProperty("wnd_channel_list","draw_now",NULL);
                                app_create_dialog("wnd_channel_info");
                            }
                        }
                        else
                        {
                            sat_fav_keypress(&s_Cl, event->key.sym, sel);
							//GUI_SetProperty(LV_LIST, "active", &sel);
							//GUI_SetProperty(LV_LIST, "select", &sel);
                        }
                        break;
                    }

                case STBK_SAT:
                case STBK_FAV:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
#if TURN_OVER_MODE
                case STBK_LEFT:
#endif
                case STBK_PAGE_UP:
                    {
                        uint32_t sel;
                        GUI_GetProperty(LV_LIST, "select", &sel);
                        if (sel == 0)
                        {
                            sel = app_channel_list_list_get_total(NULL,NULL)-1;
                            GUI_SetProperty(LV_LIST, "select", &sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            event->key.sym = STBK_PAGE_UP;
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;
#if TURN_OVER_MODE
                case STBK_RIGHT:
#endif
                case STBK_PAGE_DOWN:
                    {
                        uint32_t sel;
                        GUI_GetProperty(LV_LIST, "select", &sel);
                        if (app_channel_list_list_get_total(NULL,NULL)
                                == sel+1)
                        {
                            sel = 0;
                            GUI_SetProperty(LV_LIST, "select", &sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            event->key.sym = STBK_PAGE_DOWN;
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

