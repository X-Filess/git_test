#include "app.h"
#include "pmp_subtitle.h"
#include "gdi_core.h"

#if MINI_16_BITS_OSD_SUPPORT
#define SUBT_WIDTH 944 
#define SUBT_HEIGHT 26
#else
#define SUBT_WIDTH 1180
#define SUBT_HEIGHT 42
#endif

#define MAX_FONT_NAME			(10)

struct gdi_font {
    char font_name[MAX_FONT_NAME];
    int family;
    int xsize;
    int ysize;
    int font_size;
    void* font_data;
    #ifdef GUI_TTF
    gdi_font_prop *index_table;
    #endif
    struct gdi_font* next;
};

static handle_t s_subt_mutex = -1;
pmp_subt_para* subt_para = NULL;

static int	            s_device_handle = -1;
static handle_t         s_vpu_handle =-1;
static uint16_t*           s_surface_buffer;
static GuiSurface			    *s_spp_surface;

static uint32_t         s_spu_width;
static uint32_t         s_spu_height;
static GxVpuProperty_LayerViewport s_view_port;
static struct gdi_font *old_font = NULL;

extern int gdi_text_len(void *string);
extern struct gdi_font *gxgdi_set_font(char *pFontName);
extern struct gdi_font *gxgdi_get_font(void);
extern int gdi_lock(void);
extern int gdi_unlock(void);
extern void hd_set_layer_surface(LayerSurface layer);

static void _point_spu(int x0,int y0,int yuv)
{
	int y, u, v;
	uint16_t value;

    if(s_surface_buffer == NULL)
        return;

	y = yuv >> 16 & 0xff;
	u = yuv >> 8 & 0xff;
	v = yuv & 0xff;

	value =((y>>2)<<10 )|((u>>4)<<6)|((v>>4)<<2)|0x3;
	s_surface_buffer[s_spu_width*y0+x0] = ((value>>8)&0xff)|((value&0xff)<<8);
}

static void _clean_spu(void)
{
	int i=0;

    if(s_surface_buffer == NULL)
        return;

	for(i = 0; i < s_spu_width * s_spu_height; i++)
	{
		s_surface_buffer[i] = 0x2002;
	}
}

static int _hide_spu(void)
{
	GxVpuProperty_LayerEnable       enable;

	enable.layer  = GX_LAYER_SPP;
	enable.enable = 0;
	return GxAVSetProperty(s_device_handle, s_vpu_handle,
					GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));
}

static int _show_spu(void)
{
	GxVpuProperty_LayerEnable       enable;

	enable.layer  = GX_LAYER_SPP;
	enable.enable = 1;
	return GxAVSetProperty(s_device_handle, s_vpu_handle,
					GxVpuPropertyID_LayerEnable, (void*)&enable, sizeof(GxVpuProperty_LayerEnable));
}

static void _destroy_spu(void)
{
    GxVpuProperty_LayerOnTop  SetLayerTop;

    _hide_spu();
    _clean_spu();

    SetLayerTop.layer  = GX_LAYER_SPP;
    SetLayerTop.enable = 0;
    GxAVSetProperty(s_device_handle, s_vpu_handle,
            GxVpuPropertyID_LayerOnTop, (void*)&SetLayerTop, sizeof(GxVpuProperty_LayerOnTop));

    if(s_spp_surface)
    {
        hd_free_surface(s_spp_surface);
        s_spp_surface = NULL;
        s_surface_buffer = NULL;
    }

    hd_set_layer_surface(0);//0, LAYER_MAIN_SPP_SURFACE

    if(s_view_port.rect.width && s_view_port.rect.height)
    {
        GxAVSetProperty(s_device_handle,s_vpu_handle,
                GxVpuPropertyID_LayerViewport, &s_view_port, sizeof(GxVpuProperty_LayerViewport));
    }

    GxAvdev_SppUnlock();

    if(s_vpu_handle != -1)
    {
        GxAvdev_CloseModule(s_device_handle, s_vpu_handle);
        s_vpu_handle = -1;
    }

    if(s_device_handle != -1)
    {
        GxAvdev_DestroyDevice(s_device_handle);
        s_device_handle = -1;
    }
}

static int _init_spu(PlayerWindow* rect)
{
	GxVpuProperty_LayerMainSurface MainSurface;
	GxVpuProperty_LayerEnable SetLayerEnable;
	GxVpuProperty_LayerOnTop  SetLayerTop;
	GxVpuProperty_Resolution  VirtResolution;
	GxVpuProperty_LayerViewport ViewPort;
	GAL_Rect gui_rect = {0};
    GuiSurface *spp_surface = NULL;
	int ret = 0;

	if(rect == NULL)
		return -1;

	if(s_device_handle == -1)
	{
		s_device_handle = GxAvdev_CreateDevice(0);
	}

	if(s_vpu_handle == -1)
	{
		s_vpu_handle = GxAvdev_OpenModule(s_device_handle, GXAV_MOD_VPU, 0);
	}

	if(s_vpu_handle < 0)
    {
		GxAvdev_DestroyDevice(s_device_handle);
		return 0;
	}

    GxAvdev_SppLock();

    memset(&s_view_port, 0, sizeof(s_view_port));
    s_view_port.layer = GX_LAYER_SPP;
    ret |= GxAVGetProperty(s_device_handle,s_vpu_handle,
			      GxVpuPropertyID_LayerViewport, &s_view_port, sizeof(GxVpuProperty_LayerViewport));

    ret |= GxAVGetProperty(s_device_handle,s_vpu_handle,
			      GxVpuPropertyID_VirtualResolution, &VirtResolution, sizeof(GxVpuProperty_Resolution));

	if((rect->width == 0)||( rect->height == 0))
	{
		rect->width = APP_XRES;
		rect->height = APP_YRES;
	}

	s_spu_width = rect->width;
	s_spu_height = rect->height;
	GxSubtPrintf("rect[%d %d][%d %d]", s_spu_width, s_spu_width,s_spu_height, s_spu_height );

	SetLayerEnable.layer  = GX_LAYER_SPP;
	SetLayerEnable.enable = 0;
	ret |= GxAVSetProperty(s_device_handle,s_vpu_handle,
			      GxVpuPropertyID_LayerEnable, &SetLayerEnable, sizeof(GxVpuProperty_LayerEnable));

	gui_rect.x = 0;
    gui_rect.y = 0;
    gui_rect.w = s_spu_width;
    gui_rect.h = s_spu_height;
    spp_surface = hd_surface_clone(GX_LAYER_SPP, &gui_rect, 2);

	MainSurface.layer = GX_LAYER_SPP;
	MainSurface.surface = spp_surface->hw_surface;
	ret |= GxAVSetProperty(s_device_handle,s_vpu_handle,
			      GxVpuPropertyID_LayerMainSurface, &MainSurface, sizeof(GxVpuProperty_LayerMainSurface));
	s_spp_surface = spp_surface;
	s_surface_buffer = (uint16_t*)spp_surface->data;

	_clean_spu();

    ViewPort.layer = GX_LAYER_SPP;
	ViewPort.rect.x = 0;
	ViewPort.rect.y = 0;
	ViewPort.rect.width	= VirtResolution.xres;
	ViewPort.rect.height= VirtResolution.yres;
	ret |= GxAVSetProperty(s_device_handle,s_vpu_handle,
			      GxVpuPropertyID_LayerViewport, &ViewPort, sizeof(GxVpuProperty_LayerViewport));

    SetLayerTop.layer  = GX_LAYER_SPP;
	SetLayerTop.enable = 1;
	GxAVSetProperty(s_device_handle, s_vpu_handle,
					GxVpuPropertyID_LayerOnTop, (void*)&SetLayerTop, sizeof(GxVpuProperty_LayerOnTop));

	SetLayerEnable.layer = MainSurface.layer;
	SetLayerEnable.enable = 1;
	ret |= GxAVSetProperty(s_device_handle,s_vpu_handle,
			      GxVpuPropertyID_LayerEnable, &SetLayerEnable, sizeof(GxVpuProperty_LayerEnable));

	return 0;
}

static int _draw_spu(void* data)
{
    int x, y,i;
    PlayerSubSpuPage* spu = (PlayerSubSpuPage*)data;
    uint32_t start_col = 0, start_row = 0;

    start_col = spu->start_col;
    start_row = spu->start_row;

	_clean_spu();
    GxSubtPrintf("spu[%d %d ] [%d %d]\n", start_col, spu->start_col, start_row, spu->start_row);

    for (y = 0; y < spu->height; ++y)
    {
        for (x = 0; x < spu->width; ++x)
        {
            int res;
            if (spu->image[x] && (int)spu->aimage[x])
            {
                if(spu->custom)
                {
                    for(i=0;i<4;i++)
                    {
                        if(spu->image[x]==(spu->cuspal[i]>>16))
                        {
                            res = spu->cuspal[i];
                            _point_spu(start_col+x,start_row+y,res);
                            break;
                        }
                    }
                }
                else
                {
                    for(i=0;i<16;i++)
                    {
                        if(spu->image[x]==(spu->global_palette[i]>>16))
                        {
                            res = spu->global_palette[i];
                            _point_spu(start_col+x,start_row+y,res);
                            break;
                        }
                    }
                }
            }
        }
        spu->image += spu->stride;
        spu->aimage += spu->stride;
    }
    return 0;
}

int app_subt_init(PlayerWindow* rect,int type)
{
	GxSubtPrintf("subt_init type = %d\n", type);
	if(subt_para == NULL)
		return -1;
	subt_para->type  = type;

    if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
	{
		GUI_SetProperty(subt_para->widget_text, "string", NULL);
		GUI_SetProperty(subt_para->widget_text, "state", "show");
	}

	return 0;
}

void app_subt_destroy(int handle, int type)
{
    GxSubtPrintf("subt_destory\n");
    if(subt_para == NULL)
        return ;
    if(type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
    {
        GUI_SetProperty(subt_para->widget_text, "string", NULL);
        GUI_SetProperty(subt_para->widget_text, "state", "hide");
    }
}

int app_subt_show(int handle)
{
	GxSubtPrintf("subt_show\n");
	if(subt_para == NULL)
        return -1;

    if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
	{
		subt_para->state = 1;
		if(subt_para->osd_state == 1)
			GUI_SetProperty(subt_para->widget_text, "state", "show");
	}
	return 0;
}

int  app_subt_hide(int handle)
{
	GxSubtPrintf("subt_hide\n");
	if(subt_para == NULL)
		return -1;
    if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
	{
		subt_para->state = 0;
		GUI_SetProperty(subt_para->widget_text, "state", "hide");
	}
	return 0;
}

int app_subt_draw(void* data, int num, int type)
{
    if((subt_para == NULL) || (data == NULL))
        return -1;

    if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
    {
        if(subt_para->state == 1 && subt_para->osd_state == 1)
        {
            PlayerSubTextPage* subpg = (PlayerSubTextPage*)data;
            GuiUpdateString canvas_string;
            GuiUpdateRect canvas_rect;
            int cur_page_texts_len = 0;
            int i = 0;
            int width = 0;
            int cur_subt_width = 0;

            if(subpg->lines >= PLAYER_MAX_SUB_LINE)
                return -1;

            GxCore_MutexLock(s_subt_mutex);
			gdi_lock();//防止改变text_draw时 font大小被修改
            gxgdi_set_font("roboto");
			gdi_unlock();
            for(i = 0; i < subpg->lines; i++)
            {
                if(subpg->text[i])
                {
                    cur_page_texts_len += strlen(subpg->text[i]);
                    width = gdi_text_len(subpg->text[i]);
                    if(cur_subt_width < width)
                        cur_subt_width = width;
                }
            }
            cur_page_texts_len += 8;//for '\n'

            //buf
            if(cur_page_texts_len > subt_para->texts_len)
            {
                subt_para->texts_len = cur_page_texts_len;
                subt_para->texts_buf = GxCore_Realloc(subt_para->texts_buf, cur_page_texts_len);
                if(NULL == subt_para->texts_buf)
                {
                    GxCore_MutexUnlock(s_subt_mutex);
                    return -1;
                }
            }
            memset(subt_para->texts_buf, 0, subt_para->texts_len);

            //split
            for(i = 0; i < subpg->lines; i++)
            {
                if(subpg->text[i])
                {
                    if(0 != i)
                        strcat(subt_para->texts_buf, "\n");
                    strcat(subt_para->texts_buf, subpg->text[i]);
                }
            }

            if(cur_subt_width > SUBT_WIDTH)
                cur_subt_width = SUBT_WIDTH;

            canvas_rect.x = (SUBT_WIDTH - (cur_subt_width+40))/2;
            canvas_rect.y = (PLAYER_MAX_SUB_LINE-subpg->lines)*SUBT_HEIGHT;
            canvas_rect.w = (cur_subt_width + 40);
            canvas_rect.h = (subpg->lines)*SUBT_HEIGHT;
            canvas_rect.color = "subt_background";
            GUI_SetProperty(subt_para->widget_text, "rectangle", (void*)&canvas_rect);

            canvas_string.x = (SUBT_WIDTH - cur_subt_width * 2)/2;
            canvas_string.y = (PLAYER_MAX_SUB_LINE-subpg->lines)*SUBT_HEIGHT;;
            canvas_string.xsize = cur_subt_width * 2;
            canvas_string.ysize = (subpg->lines)*SUBT_HEIGHT;
            canvas_string.color = 0xFFFFFF;
            canvas_string.alignment = "hcentre|vcentre";
            canvas_string.string = subt_para->texts_buf;
            canvas_string.inter_line = 0;
            //GxSubtPrintf("###%s###\n", subt_para->texts_buf);
            GUI_SetProperty(subt_para->widget_text, "string",(void*)&canvas_string);

            GxCore_MutexUnlock(s_subt_mutex);
        }
    }
    return 0;
}

void  app_subt_clear(int handle)
{
	if(subt_para == NULL)
		return;

	if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
	{
		if(subt_para->state == 1 && subt_para->osd_state == 1)
		{
			GuiUpdateRect canvas_rect;
			GxCore_MutexLock(s_subt_mutex);

			canvas_rect.x = 0;
			canvas_rect.y=0;
			canvas_rect.w = SUBT_WIDTH;
			canvas_rect.h = PLAYER_MAX_SUB_LINE*SUBT_HEIGHT;
			canvas_rect.color = "subt_clear";
			GUI_SetProperty(subt_para->widget_text, "rectangle", (void*)&canvas_rect);

			GUI_SetProperty(subt_para->widget_text, "state", "hide");

			GxCore_MutexUnlock(s_subt_mutex);
		}
	}
}


static int _player_subt_show(int handle)
{
    if(subt_para == NULL)
        return 0;

    if(subt_para->type == PLAYER_SUB_ENC_SPU)
    {
        _show_spu();
    }
    else
    {
        GxMsgProperty_AppSubtitle subt;
        subt.handle = handle;
        app_send_msg_exec(GXMSG_SUBTITLE_SHOW, &subt);
    }
    return 0;
}

static int _player_subt_hide(int handle)
{
    if(subt_para == NULL)
        return 0;

    if(subt_para->type == PLAYER_SUB_ENC_SPU)
    {
        _hide_spu();
    }
    else
    {
        GxMsgProperty_AppSubtitle subt;
        subt.handle = handle;
        app_send_msg_exec(GXMSG_SUBTITLE_HIDE, &subt);
    }
    return 0;
}

static int _player_subt_draw(void* data, int num,int type)
{
	if(subt_para == NULL)
		return -1;

	if(subt_para->type == PLAYER_SUB_ENC_SPU)
	{
		return _draw_spu(data);
	}
	else
	{
		int i;
		PlayerSubTextPage page;
		PlayerSubTextPage *pdate = data;
		GxMsgProperty_AppSubtitle subt;

        memcpy(&page, pdate, sizeof(PlayerSubTextPage));
		page.next = NULL;
		memset(page.text, 0, sizeof(page.text));
		for(i=0;i<page.lines;i++)
		{
			page.text[i] = GxCore_Strdup(pdate->text[i]);
		}

		subt.data = GxCore_Malloc(sizeof(PlayerSubTextPage));
		memcpy(subt.data, &page, sizeof(PlayerSubTextPage));
		subt.num = num;
		subt.type = type;
		app_send_msg_exec(GXMSG_SUBTITLE_DRAW, &subt);
	}

	return 0;
}

static void _player_subt_clear(int handle)
{
	if(subt_para == NULL)
		return;

	if(subt_para->type == PLAYER_SUB_ENC_SPU)
	{
		_clean_spu();
	}
	else
	{
		GxMsgProperty_AppSubtitle subt;
		subt.handle = handle;
		app_send_msg_exec(GXMSG_SUBTITLE_CLEAR, &subt);
	}
}

static int _player_subt_init(PlayerWindow* rect,int type)
{
	GxSubtPrintf("subt_init type = %d\n", type);
	if(subt_para == NULL)
		return -1;
	subt_para->type  = type;

	if(subt_para->type == PLAYER_SUB_ENC_SPU)
	{
		return _init_spu(rect);
	}
	else if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
	{
		GxMsgProperty_AppSubtitle subt;
		subt.rect= *rect;
		subt.type = type;
		app_send_msg_exec(GXMSG_SUBTITLE_INIT, &subt);
	}
	return 0;
}

static void _player_subt_destroy(int handle)
{
	GxSubtPrintf("subt_destory\n");
	if(subt_para == NULL)
		return ;

	if(subt_para->type == PLAYER_SUB_ENC_SPU)
	{
		_destroy_spu();
	}
	else if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
	{
		GxMsgProperty_AppSubtitle subt;
		subt.handle = handle;
		app_send_msg_exec(GXMSG_SUBTITLE_DESTROY, &subt);
	}
}
static bool check_base_name(const char* check_file, const char* play_file)
{
	int check_file_len = 0;
	int play_file_len = 0;
	int base_name_len = 0;
	int i = 0;

	if(NULL == check_file || NULL == play_file)
	{
		return FALSE;
	}

	check_file_len = strlen(check_file);
	play_file_len = strlen(play_file);

	for(i = play_file_len - 1; i >= 0 ; i--)
	{
		if('.' == play_file[i])
		{
			break;
		}
	}
	if(0 == i) return FALSE;

	base_name_len = i;

	if(check_file_len <= base_name_len)
	{
		//GxSubtPrintf( "=_check_extend_name '%s', '%s'= name_len < extend_len\n", name, extend);
		return FALSE;
	}

	if(0 == strncasecmp((const char*)check_file,play_file, base_name_len))
	{
		return TRUE;
	}

	return FALSE;
}

static char* subt_getfile(void)
{
	GxDirent* ent = NULL;
	int count = 0;
	int i = 0;
	play_movie_info movie_info;
	explorer_para* explorer_subt = NULL;

	if(NULL == explorer_view) return NULL;
	explorer_subt = explorer_opendir(explorer_view->path, SUFFIX_SUBT);
	if(NULL == explorer_subt) return NULL;

	play_movie_get_info(&movie_info);

	count = explorer_subt->nents;
	for(i = 0; i < count; i++)
	{
		ent = explorer_subt->ents + i;
		if(NULL == ent)
		{
			explorer_closedir(explorer_subt);
			return NULL;
		}

		if(GX_FILE_REGULAR == ent->ftype)
		{
			if(check_base_name(ent->fname, movie_info.name))
			{
				char* path = NULL;
				path = explorer_static_path_strcat(movie_info.path, ent->fname);
				explorer_closedir(explorer_subt);
				return path;
			}
		}
	}

	explorer_closedir(explorer_subt);
	return NULL;
}

int subtitle_create(char* widget_text)
{
	GxSubtPrintf("subtitle_create\n");

	old_font = gxgdi_get_font();
	GxSubtPrintf("old_font:[%s] \n", old_font->font_name);

	if(s_subt_mutex == -1)
	{
		GxCore_MutexCreate(&s_subt_mutex);
	}

	subt_para = GxCore_Malloc(sizeof(pmp_subt_para));
	APP_CHECK_P(subt_para, 1);
	memset(subt_para, 0, sizeof(pmp_subt_para));

	//texts
	subt_para->texts_len = 1024*2;
	subt_para->texts_buf = GxCore_Malloc(subt_para->texts_len);
	APP_CHECK_P(subt_para->texts_buf, 1);

	//widget
	subt_para->widget_text = GxCore_Strdup(widget_text);
	APP_CHECK_P(subt_para->widget_text, 1)
	GUI_SetProperty(subt_para->widget_text, "state", "hide");

	subt_para->cur_subt = 0;
	subt_para->state = 1;
	subt_para->osd_state = 1;
	pmpset_set_int(PMPSET_MOVIE_SUBT_VISIBILITY, PMPSET_TONE_ON);

	return 0;
}

int subtitle_destroy(void)
{
	APP_CHECK_P(subt_para, 1);
	GxSubtPrintf("subtitle_destroy\n");

	gxgdi_set_font(old_font->font_name);

	if(s_subt_mutex != -1)
	{
		GxCore_MutexDelete(s_subt_mutex);
		s_subt_mutex = -1;
	}

    //hide
    app_subt_hide(0);

	//stop
	subtitle_stop();

	//texts
	subt_para->texts_len = 0;
    APP_FREE(subt_para->texts_buf);

	//widget
	APP_FREE(subt_para->widget_text);

	//para
	APP_FREE(subt_para);

	return 0;
}

int subtitle_start(char* file, pmp_subt_load_type type)
{
	char* file_load = NULL;

	if(NULL == subt_para)
        return -1;

	//unload old
	subtitle_stop();

	if(type ==  PMP_SUBT_LOAD_OUTSIDE || type ==  PMP_SUBT_LOAD_INIT )
	{
		//get file
		if(NULL == file)
		{
			file_load = subt_getfile();
		}
		else
		{
			file_load = file;
		}
		GxSubtPrintf("file: %s\n", file_load);
	}

	if(file_load != NULL)//outside subt
	{
		subt_para->para.type = PLAYER_SUB_TYPE_FILE;
		subt_para->para.render = PLAYER_SUB_RENDER_SPP;
		subt_para->file = GxCore_Strdup(file_load);
		if(NULL == subt_para->file)
			return -1;
		subt_para->para.file_name = subt_para->file;
	}
	else//inside subt
	{
		if(type ==  PMP_SUBT_LOAD_OUTSIDE)
		{
			subt_para->para.type = PLAYER_SUB_TYPE_FILE;
			subt_para->para.file_name = NULL;
			return 0;
		}

		subt_para->para.type = PLAYER_SUB_TYPE_INSIDE;
		subt_para->para.render = PLAYER_SUB_RENDER_SPP;
		subt_para->para.file_name = NULL;
	}

	subt_para->para.init = _player_subt_init;
	subt_para->para.destory = _player_subt_destroy;
	subt_para->para.show = _player_subt_show;
	subt_para->para.hide = _player_subt_hide;
	subt_para->para.draw = _player_subt_draw;
	subt_para->para.clear = _player_subt_clear;
	subt_para->type = -1;
	subt_para->list = GxPlayer_MediaSubLoad(PMP_PLAYER_AV,&(subt_para->para));
	GxSubtPrintf("load: %s\n", subt_para->file);

	return 0;
}

int subtitle_stop(void)
{
	if(NULL == subt_para)
		return -1;

	subt_para->cur_subt = 0;

	if(subt_para->file)
		GxSubtPrintf("subtitle_stop: %s\n", subt_para->file);

	//unload
	if(subt_para->list)
	{
		GxPlayer_MediaSubUnLoad(PMP_PLAYER_AV, subt_para->list);
		subt_para->list= NULL;
	}

	return 0;
}

int subtitle_pause(void)
{
	GxSubtPrintf("subtitle_pause\n");

	if(subt_para == NULL)
		return -1;

	subt_para->osd_state = 0;

	if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
	{
		app_subt_clear(0);
		gxgdi_set_font(old_font->font_name);
		GUI_SetProperty(subt_para->widget_text, "state", "hide");
	}
	return 0;
}

int subtitle_resume(void)
{
	GxSubtPrintf("subtitle_resume\n");

	if(subt_para == NULL)
		return -1;

	subt_para->osd_state = 1;

	if(subt_para->type == PLAYER_SUB_ENC_UTF8 || subt_para->type == PLAYER_SUB_ENC_ANSI)
	{
		if(subt_para->state == 1)
			GUI_SetProperty(subt_para->widget_text, "state", "show");
	}
	return 0;
}

pmp_subt_para* subtitle_get(void)
{
	return subt_para;
}

