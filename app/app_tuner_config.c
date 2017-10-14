#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "module/config/gxconfig.h"
#include "app_utility.h"
#include "app_config.h"

#define TUNER_TYPE_NUM 56
#define DEMOD_TYPE_NUM 9
typedef struct TunerType
{
	uint8_t Typeid;
	char Typename[32];
}TunerType;


static TunerType s_tuner_type[TUNER_TYPE_NUM] =
{
	{1,"ALPSTDAE"},
	{2,"ALPSTDQE3"},
	{3,"DCF877X"},
	{4,"DTCISA8X"},
	{5,"LG_TDCC_G0X1F"},
	{6,"MT2060"},
	{7,"PHILIPSCD1316LF"},
	{8,"PHILIPSCD1600LF"},
	{9,"SAMSUNG_H_NIM"},
	{10,"SAMSUNGDTQS243PH172C"},

	{11,"SHARP6389"},
	{12,"THOMSON_704XA"},
	{13,"THOMSONDCT707XX"},
	{14,"YINHE"},
	{21,"TDAC3"},
	{22,"MXL5007T"},
	{23,"TDA18218"},
	{24,"FC2580"},
	{25,"RDA5880"},
	{26,"FT2117"},

	{31,"SHARP1302"},
	{32,"CE5039"},
	{33,"AV2020"},
	{34,"TS2808"},
	{35,"WZ5001"},
	{36,"MAX2117"},
	{37,"MAX2118"},
	{38,"MAX2120"},
	{39,"STB6000"},
	{40,"LW10039"},

	{41,"AV2011"},
	{42,"STV6110"},
	{43,"RDA5812"},
	{44,"SHARP7306"},
	{45,"STV6110A"},
	{46,"LW37"},
	{47,"ZL10037"},
	{48,"SHARP6093"},
	{49,"TDAC2"},
	{50,"NT220X"},

	{51,"XINLIANYUV9"},
	{52,"TDAC7"},
	{53,"TDA18250"},
	{54,"MTV600"},
	{55,"RDA5815M"},
	{57,"TDA18250A"},
	{56,"R836"},
	{58,"TDA18273"},
	{60,"MXL241SF"},
	{80,"MXL603"},

	{70,"R848_S"},
	{71,"R848_T"},
	{72,"R848_C"},
	{73,"R848_DTMB"},
	{90,"MULTI_MODE"},
	{99,"INVALID"},
};

typedef struct TunerConfig
{
	uint32_t DemodType;
	uint32_t DemodIcid;
	uint32_t ChipAddr;
	uint32_t TunerType;
	uint32_t TunerAddr;
	uint32_t IqSwap;
	uint32_t HV;
	uint32_t SpimCfg;
	uint32_t Bsp;
	uint32_t Lnb;
	uint32_t TsOut;
}TunerConfig;

enum TUNER_CFG_ITEM_NAME
{
	ITEM_TUNERID = 0,
	ITEM_DEMODTYPE ,
	ITEM_DEMODI2CID,
	ITEM_CHIPADDR,
	ITEM_TUNERTYPE,
	ITEM_TUNERADDR,
	ITEM_IQSWAP,
	ITEM_HV,
	ITEM_SPIMCFG,
	ITEM_BSP,
	ITEM_LNB,
	ITEM_TSOUT,
	ITEM_CONFIG_TOTAL
};

static char *s_tuner_choice_back[]=
{
	"img_tuner_config_item_choice1",
	"img_tuner_config_item_choice2",
	"img_tuner_config_item_choice3",
	"img_tuner_config_item_choice4",
	"img_tuner_config_item_choice5",
	"img_tuner_config_item_choice6",
	"img_tuner_config_item_choice7",
	"img_tuner_config_item_choice8",
	"img_tuner_config_item_choice9",
	"img_tuner_config_item_choice10",
	"img_tuner_config_item_choice11",
	"img_tuner_config_item_choice12",
};
static char *s_tuner_content[]=
{
	"cmb_tuner_config_opt1",
	"cmb_tuner_config_opt2",
	"cmb_tuner_config_opt3",
	"edit_tuner_config_opt4",
	"cmb_tuner_config_opt5",
	"edit_tuner_config_opt6",
	"cmb_tuner_config_opt7",
	"cmb_tuner_config_opt8",
	"cmb_tuner_config_opt9",
	"cmb_tuner_config_opt10",
	"cmb_tuner_config_opt11",
	"cmb_tuner_config_opt12",
};

static char *s_tuner_title[] = 
{
	"text_tuner_config_item1",
	"text_tuner_config_item2",
	"text_tuner_config_item3",
	"text_tuner_config_item4",
	"text_tuner_config_item5",
	"text_tuner_config_item6",
	"text_tuner_config_item7",
	"text_tuner_config_item8",
	"text_tuner_config_item9",
	"text_tuner_config_item10",
	"text_tuner_config_item11",
	"text_tuner_config_item12",
};

static char *s_tuner_title_name[] =
{
	"Tuner id",
	"Demod Type",
	"Demod I2c-id",
	"Demod Chip-addr",
	"Tuner Type",
	"Tuner Addr",
	"Iq-swap",
	"HV",
	"Spim-cfg",
	"BSP",
	"LNB",
	"TS-OUT"
};
#define IMG_BAR_HALF_UNFOCUS "s_bar_half_unfocus.bmp"
#define IMG_BAR_HALF_FOCUS "s_bar_half_focus_l.bmp"
#define IMG_BAR_HALF_FOCUS_R "s_bar_half_focus_r.bmp"
#define IMG_BAR_HALF_UNFOCUS_ARROW "s_bar_half_unfocus_arrow.bmp"
#define IMG_BAR_HALF_FOCUS_ARROW "s_bar_half_focus_arrow.bmp"

#define BUF_LENGTH 32
 uint32_t s_tuner_sel = 0;
//static TunerConfig s_tuner_one = {4,2,0x80,30,0xcb,0,1,7,0,0,1};
static void _tuner_config_cmb_tuner_type_rebuild(void)
{
	uint8_t i;
	char* buffer_all = NULL;
	char buffer[BUF_LENGTH] ={0};
	buffer_all = GxCore_Malloc(TUNER_TYPE_NUM*BUF_LENGTH);
	if(buffer_all==NULL)
		return;
	memset(buffer_all, 0, TUNER_TYPE_NUM * BUF_LENGTH);
	strcat(buffer_all, "[");

	for(i=0;i<TUNER_TYPE_NUM;i++)
	{
		memset(buffer, 0, BUF_LENGTH);
		if(i==55)
		{
			sprintf(buffer,"%s]",s_tuner_type[i].Typename);
			strcat(buffer_all, buffer);
		}
		else
		{
			sprintf(buffer,"%s,",s_tuner_type[i].Typename);
			strcat(buffer_all, buffer);
		}
	}
	GUI_SetProperty(s_tuner_content[ITEM_TUNERTYPE],"content",buffer_all);
	GxCore_Free(buffer_all);
}

static int _tuner_config_demod_type_pop(int sel)
{
	static char* demod_type[DEMOD_TYPE_NUM] = {"GX1001","GX1101","GX1201","GX113x","GX1501C","GX1211","GX1801","GX3211","GX1503"};
	PopList pop_list;
	int ret =-1;
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = "Demod Type";
	pop_list.item_num = DEMOD_TYPE_NUM;
	pop_list.item_content = demod_type;
	pop_list.sel = sel;
	pop_list.show_num= true;
	ret = poplist_create(&pop_list);
	return ret;
}
static int _tuner_config_tuner_type_pop(int sel)
{
	char **tunertype_info = NULL;
	PopList pop_list;
	int ret =-1;
	int i =0 ;
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = "Tuner Type";
	pop_list.item_num = TUNER_TYPE_NUM;
	tunertype_info = (char**)GxCore_Malloc(TUNER_TYPE_NUM * sizeof(char*));
	if(tunertype_info == NULL)
		return ret;
	for(i = 0; i < TUNER_TYPE_NUM; i++)
	{
		tunertype_info[i] = s_tuner_type[i].Typename;
	}
	pop_list.item_content = tunertype_info;
	pop_list.sel = sel;
	pop_list.show_num= true;
	ret = poplist_create(&pop_list);
	GxCore_Free(tunertype_info);
	return ret;
}
SIGNAL_HANDLER int app_tuner_config_create(GuiWidget *widget, void *usrdata)
{

	uint32_t cmb_sel;
	TunerConfig tuner_one;
	char TunerAddr[8];
	char ChipAddr[8];

	tuner_one.DemodType = 4;
	tuner_one.DemodIcid = 2;
	tuner_one.ChipAddr = 0x80;
	tuner_one.TunerType = 30;
	tuner_one.TunerAddr = 0xc6;
	tuner_one.IqSwap = 0;
	tuner_one.HV = 1;
	tuner_one.SpimCfg = 7;
	tuner_one.Bsp = 0;
	tuner_one.Lnb = 0;
	tuner_one.TsOut = 1;

	s_tuner_sel = 0;
	GUI_SetProperty(s_tuner_choice_back[s_tuner_sel],"img" ,IMG_BAR_HALF_FOCUS);

	_tuner_config_cmb_tuner_type_rebuild();
	cmb_sel = tuner_one.DemodType - 1;
	GUI_SetProperty(s_tuner_content[ITEM_DEMODTYPE],"select" ,&cmb_sel);
	GUI_SetProperty(s_tuner_content[ITEM_DEMODI2CID],"select",&(tuner_one.DemodIcid));
	memset(ChipAddr, 0, 8);
	sprintf(ChipAddr,"%d", tuner_one.ChipAddr);
	GUI_SetProperty(s_tuner_content[ITEM_CHIPADDR], "string",ChipAddr);
	GUI_SetProperty(s_tuner_content[ITEM_TUNERTYPE],"select",&(tuner_one.TunerType));
	memset(TunerAddr, 0, 8);
	sprintf(TunerAddr,"%d", tuner_one.TunerAddr);
	GUI_SetProperty(s_tuner_content[ITEM_TUNERADDR],"string",TunerAddr);
	GUI_SetProperty(s_tuner_content[ITEM_HV],"select" ,&(tuner_one.HV));
	GUI_SetProperty(s_tuner_content[ITEM_SPIMCFG],"select",&(tuner_one.SpimCfg));
	GUI_SetProperty(s_tuner_content[ITEM_TSOUT],"select",&(tuner_one.TsOut));
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tuner_config_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tuner_config_got_focus(GuiWidget *widget, void *usrdata)
{
	if(s_tuner_sel == ITEM_CHIPADDR)
	{
		GUI_SetProperty(s_tuner_content[s_tuner_sel], "unfocus_img", IMG_BAR_HALF_UNFOCUS);
	}
	else
	{
		GUI_SetProperty(s_tuner_content[s_tuner_sel], "unfocus_img", IMG_BAR_HALF_UNFOCUS_ARROW);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tuner_config_lost_focus(GuiWidget *widget, void *usrdata)
{
	if(s_tuner_sel == ITEM_CHIPADDR)
	{
		GUI_SetProperty(s_tuner_content[s_tuner_sel], "unfocus_img", IMG_BAR_HALF_FOCUS_R);
	}
	else
	{
		GUI_SetProperty(s_tuner_content[s_tuner_sel], "unfocus_img", IMG_BAR_HALF_FOCUS_ARROW);
	}
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_tuner_config_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	int cmb_sel;
	switch(event->type)
	{
		case GUI_SERVICE_MSG: 
			break; 
		case GUI_KEYDOWN: 
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("wnd_tuner_config");
					break;
				case STBK_UP:
					GUI_SetProperty(s_tuner_choice_back[s_tuner_sel],"img" ,IMG_BAR_HALF_UNFOCUS);
					GUI_SetProperty(s_tuner_title[s_tuner_sel],"string", s_tuner_title_name[s_tuner_sel]);
					if(s_tuner_sel == ITEM_TUNERID)
					{
						s_tuner_sel = ITEM_TSOUT;
					}
					else
					{
						s_tuner_sel--;
					}
					GUI_SetProperty(s_tuner_choice_back[s_tuner_sel],"img" ,IMG_BAR_HALF_FOCUS);
					GUI_SetProperty(s_tuner_title[s_tuner_sel],"string", s_tuner_title_name[s_tuner_sel]);
					GUI_SetFocusWidget(s_tuner_content[s_tuner_sel]);
					break;
				case STBK_DOWN:
					GUI_SetProperty(s_tuner_choice_back[s_tuner_sel],"img" ,IMG_BAR_HALF_UNFOCUS);
					GUI_SetProperty(s_tuner_title[s_tuner_sel],"string", s_tuner_title_name[s_tuner_sel]);
					if(s_tuner_sel == ITEM_TSOUT)
					{
						s_tuner_sel = ITEM_TUNERID;
					}
					else
					{
						s_tuner_sel++;
					}
					GUI_SetProperty(s_tuner_choice_back[s_tuner_sel],"img" ,IMG_BAR_HALF_FOCUS);
					GUI_SetProperty(s_tuner_title[s_tuner_sel],"string", s_tuner_title_name[s_tuner_sel]);
					GUI_SetFocusWidget(s_tuner_content[s_tuner_sel]);
					break;
				case STBK_OK:
					GUI_GetProperty(s_tuner_content[s_tuner_sel], "select", &cmb_sel);
					if(s_tuner_sel == ITEM_DEMODTYPE)
					{
						cmb_sel = _tuner_config_demod_type_pop(cmb_sel);
					}
					if(s_tuner_sel == ITEM_TUNERTYPE)
					{
						cmb_sel = _tuner_config_tuner_type_pop(cmb_sel);
					}
					GUI_GetProperty(s_tuner_content[s_tuner_sel], "select", &cmb_sel);
					break;
				case STBK_EXIT:
				case STBK_MENU:
					 GUI_SetProperty(s_tuner_choice_back[s_tuner_sel],"img" ,IMG_BAR_HALF_UNFOCUS);
					 GUI_EndDialog("wnd_tuner_config");
					 break;
				case STBK_RED:
					{
						TunerConfig newtuner;
						int cmb_sel;
						char *addr_str = NULL;
						char buf[64] = {0};
						GUI_GetProperty(s_tuner_content[ITEM_DEMODTYPE], "select", &cmb_sel);
						newtuner.DemodType= cmb_sel+1;
						GUI_GetProperty(s_tuner_content[ITEM_DEMODI2CID], "select", &(newtuner.DemodIcid));
						GUI_GetProperty(s_tuner_content[ITEM_CHIPADDR], "string", &addr_str);
						newtuner.ChipAddr= atoi(addr_str);
						GUI_GetProperty(s_tuner_content[ITEM_TUNERTYPE], "select", &(newtuner.TunerType));
						GUI_GetProperty(s_tuner_content[ITEM_TUNERADDR], "string", &addr_str);
						newtuner.TunerAddr= atoi(addr_str);
						GUI_GetProperty(s_tuner_content[ITEM_IQSWAP], "select", &(newtuner.IqSwap));
						GUI_GetProperty(s_tuner_content[ITEM_HV], "select", &(newtuner.HV));
						GUI_GetProperty(s_tuner_content[ITEM_SPIMCFG], "select", &(newtuner.SpimCfg)); 
						GUI_GetProperty(s_tuner_content[ITEM_BSP], "select", &(newtuner.Bsp));
						GUI_GetProperty(s_tuner_content[ITEM_LNB], "select", &(newtuner.Lnb)); 
						GUI_GetProperty(s_tuner_content[ITEM_TSOUT], "select", &(newtuner.TsOut)); 
						memset(buf,0,64);
						sprintf(buf,"|%d:%d:0x%x:%d:0:0x%x:@%d:%d:%d:%d:%d:%d",newtuner.DemodType,newtuner.DemodIcid, newtuner.ChipAddr,s_tuner_type[newtuner.TunerType].Typeid,newtuner.TunerAddr,newtuner.IqSwap,newtuner.HV,newtuner.SpimCfg,newtuner.Bsp,newtuner.Lnb,newtuner.TsOut);
						printf("%s\n",buf);
					}
					 break;
				default:
					break;
			}
		default:
			break;

	}
	return EVENT_TRANSFER_STOP;
}
