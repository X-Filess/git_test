/*****************************************************************************
* 			  CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009-2014, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_unicable.c
* Author    : 	gechq
* Project   :	unicable Foundation
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   1.0  	2014.06	          glen           Creation
*****************************************************************************/

//public header files
#include "app.h"
#include "app_module.h"
#include "app_pop.h"
//#include "cable/app_cable_search.h"
#include "app_frontend.h"

#if UNICABLE_SUPPORT
enum UNICABLE_SET_POPUP_BOX
{
	UNICABLE_POPUP_BOX_ITEM_LNB_FRE = 0,
	UNICABLE_POPUP_BOX_ITEM_IF_CHANNEL,
	UNICABLE_POPUP_BOX_ITEM_CENTRE_FRE,
	UNICABLE_POPUP_BOX_ITEM_SAT_POSITION,
	UNICABLE_POPUP_BOX_ITEM_OK
};

extern int app_antenna_get_cur_sat_info(GxMsgProperty_NodeByPosGet * p_node_sat);
extern  int app_antenna_timer_unicable_set_frontend(void);
extern  int app_antenna_unicable_sat_para_save(GxMsgProperty_NodeByPosGet* p_node_sat );
extern void app_lnb_freq_sel_to_data(uint16_t cur_sel, GxBusPmDataSat *p_sat);
static GxMsgProperty_NodeByPosGet CurAtnSat;
#if 1
static char* s_UnicableLnbFreq[3] = {"9750/10600","9750/10700",
    "9750/10750"};
#define UNICABLE_LNB_FREQ	"[9750/10600,9750/10700,9750/10750]"
#else
static char* s_UnicableLnbFreq[15] = {"9750/10600","9750/10700",
    "9750/10750","5150/5750","5750/5150",
        "5150","5750","5950","9750","10000","10600","10700","10750","11250","11300"};
#define UNICABLE_LNB_FREQ	"[9750/10600,9750/10700,9750/10750,9750,10000,10600,10700,10750,11250,11300]"
#endif
#define SAT_POSITTION	"[A,B]"
#define IF_CHANNEL	"[1,2,3,4,5,6,7,8]"
static int _unicable_lnbfreq_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_LNB_FREQ;
	pop_list.item_num = sizeof(s_UnicableLnbFreq)/sizeof(*s_UnicableLnbFreq);
	pop_list.item_content = s_UnicableLnbFreq;
	pop_list.sel = sel;
	pop_list.show_num= false;

	ret = poplist_create(&pop_list);

	return ret;
}

status_t app_unicable_popup_ok_keypress(void)
{
	status_t ret = GXCORE_ERROR;

	uint32_t box_sel;
	uint32_t cmb_sel;
	uint32_t if_num_sel;
	
	char *atoi_str = NULL;
	
	//CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = 0x20;
	GUI_GetProperty("box_unicable_popup_para", "select", &box_sel);
	if(UNICABLE_POPUP_BOX_ITEM_LNB_FRE== box_sel)
	{
		GUI_GetProperty("cmbox_unicable_lnb_freq", "select", &cmb_sel);
		cmb_sel = _unicable_lnbfreq_pop(cmb_sel);
		GUI_SetProperty("cmbox_unicable_lnb_freq", "select", &cmb_sel);
	}
	else if(UNICABLE_POPUP_BOX_ITEM_OK == box_sel)
	{
		GUI_GetProperty("cmbox_unicable_lnb_freq", "select", &box_sel);
		CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = box_sel;

		GUI_GetProperty("cmb_unicable_popup_if_channel", "select", &if_num_sel);
		CurAtnSat.sat_data.sat_s.unicable_para.if_channel = if_num_sel;
		//centre fre
		GUI_GetProperty("edit_unicable_popup_centre_freq", "string", &atoi_str);
		box_sel =  atoi(atoi_str);
		CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]=box_sel;
		
		GUI_GetProperty("cmb_unicable_popup_sat_pos", "select", &box_sel);
		CurAtnSat.sat_data.sat_s.unicable_para.sat_pos = box_sel;
		app_antenna_unicable_sat_para_save(&CurAtnSat);
		
		//GUI_SetProperty("cmbox_antenna_lnb_freq", "select", &cur_num);
		ret = GXCORE_SUCCESS;
	}
	GUI_SetProperty("box_unicable_popup_para", "update", NULL);

	return ret;
}

/*
GxCalculateSetFreqAndIniFreq:计算Unicable锁定的下行频率与中频
p_sat->sat_s.lnb1：卫星低本振
p_sat->sat_s.lnb2:	    卫星高本�?
pSetTPFreq:		Unicable锁定的下行频�?(out)
InitialFrequency:	Unicable锁定中频(out)
return:LnbIndex	
*/
int app_calculate_set_freq_and_initfreq(GxBusPmDataSat *p_sat, GxBusPmDataTP *pTpPara,
										uint32_t *pSetTPFreq,uint32_t* pInitialFrequency)
{
//	uint32_t TempLocalFreq=0;
	int lnb_index=0;
	uint32_t if_channel=0;
	uint16_t lnb_fre = 0;

	//GxMsgProperty_NodeByPosGet CurAtnSat;
	#define DIVERSION_FREQUENCY     (11700)

	//app_antenna_get_cur_sat_info(&CurAtnSat);

	if_channel = p_sat->sat_s.unicable_para.if_channel;
	

	if(p_sat->sat_s.lnb1>7000)//KU波段
	{
		if((p_sat->sat_s.lnb2==0)||(p_sat->sat_s.lnb1==p_sat->sat_s.lnb2))//单本�?
		{
			(*pInitialFrequency)=pTpPara->frequency-p_sat->sat_s.lnb1;
			if(!pTpPara->tp_s.polar)
			{
				lnb_index=2;
			}
			else
			{
				lnb_index=0;
			}
			(*pSetTPFreq)=p_sat->sat_s.unicable_para.centre_fre[if_channel];
		}
		else//双本�?
		{
			if (pTpPara->frequency >= DIVERSION_FREQUENCY)
			{
				lnb_fre = p_sat->sat_s.lnb2;   
				if(!pTpPara->tp_s.polar)
				{
					lnb_index=3;
				}
				else
				{
					lnb_index=1;
				}
			}
			else
			{
				lnb_fre = p_sat->sat_s.lnb1;
				if(!pTpPara->tp_s.polar)
					{
						lnb_index=2;
					}
					else
					{
						lnb_index=0;
					}
			}
			(*pInitialFrequency)=pTpPara->frequency-lnb_fre;
			(*pSetTPFreq)=p_sat->sat_s.unicable_para.centre_fre[if_channel];

		}

	}

	printf("_calculate_set_freq_and_initfreq,pSetTPFreq=%d,pInitialFrequency=%d\n",(*pSetTPFreq),(*pInitialFrequency));

	return lnb_index;


}

#if 0

/*
GxCalculateSetFreqAndIniFreq:计算Unicable锁定的下行频率与中频
p_sat->sat_s.lnb1：卫星低本振
p_sat->sat_s.lnb2:	    卫星高本振
pSetTPFreq:		Unicable锁定的下行频率(out)
InitialFrequency:	Unicable锁定中频(out)
return:LnbIndex	
*/
int app_calculate_set_freq_and_initfreq__(GxBusPmDataSat *p_sat, GxBusPmDataTP *pTpPara,
										uint32_t *pSetTPFreq,uint32_t* pInitialFrequency)
{
	uint32_t TempLocalFreq=0;
	int lnb_index=0;
	uint32_t if_channel=0;
	GxMsgProperty_NodeByPosGet CurAtnSat;

	app_antenna_get_cur_sat_info(&CurAtnSat);

	if_channel = CurAtnSat.sat_data.sat_s.unicable_para.if_channel;
	
	if(p_sat->sat_s.lnb1>p_sat->sat_s.lnb2)
	{
		TempLocalFreq=p_sat->sat_s.lnb1;
		p_sat->sat_s.lnb1=p_sat->sat_s.lnb2;
		p_sat->sat_s.lnb2=TempLocalFreq;
	}

	if(p_sat->sat_s.lnb1>7000&&p_sat->sat_s.lnb2>7000)//KU波段
	{
		if(p_sat->sat_s.lnb1==p_sat->sat_s.lnb2)//单本振
		{
			(*pInitialFrequency)=pTpPara->frequency-p_sat->sat_s.lnb1;
			if(!pTpPara->tp_s.polar)
			{
				lnb_index=2;
			}
			else
			{
				lnb_index=0;
			}
			(*pSetTPFreq)=p_sat->sat_s.lnb1+CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel];
		}
		else//双本振
		{
			if((pTpPara->frequency-p_sat->sat_s.lnb2)>950&&(pTpPara->frequency-p_sat->sat_s.lnb2)<2150)//高本振
			{
				(*pInitialFrequency)=pTpPara->frequency-p_sat->sat_s.lnb2;
				if(!pTpPara->tp_s.polar)
				{
					lnb_index=3;
				}
				else
				{
					lnb_index=1;
				}
				(*pSetTPFreq)=p_sat->sat_s.lnb2+CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel];

			}
			else
			{
				(*pInitialFrequency)=pTpPara->frequency-p_sat->sat_s.lnb1;
				if(!pTpPara->tp_s.polar)
				{
					lnb_index=2;
				}
				else
				{
					lnb_index=0;
				}
				(*pSetTPFreq)=p_sat->sat_s.lnb1+CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel];
			}
			
		}

	}
	else//C波段
	{
		if(p_sat->sat_s.lnb1==p_sat->sat_s.lnb2)//单本振
		{
			(*pInitialFrequency)=p_sat->sat_s.lnb1-pTpPara->frequency;
			if(!pTpPara->tp_s.polar)
			{
				lnb_index=2;
			}
			else
			{
				lnb_index=0;
			}
			(*pSetTPFreq)=p_sat->sat_s.lnb1-CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel];
		}
		else
		{
			if(pTpPara->tp_s.polar==0)//水平
			{
				(*pInitialFrequency)=p_sat->sat_s.lnb1-pTpPara->frequency;
				lnb_index=3;
				(*pSetTPFreq)=p_sat->sat_s.lnb1-CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel];

			}
			else if(pTpPara->tp_s.polar==1)//垂直
			{
				(*pInitialFrequency)=p_sat->sat_s.lnb2-pTpPara->frequency;
				lnb_index=0;
				(*pSetTPFreq)=p_sat->sat_s.lnb2-CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel];

			}
			else
			{				
				lnb_index=0;
				(*pInitialFrequency)=p_sat->sat_s.lnb1-pTpPara->frequency;
				(*pSetTPFreq)=p_sat->sat_s.lnb1-CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel];
			}

		}

	}
	
	return lnb_index;


}
#endif
void app_unicable_init_if_channel_num(GxBusPmDataSat *p_sat)
{
	p_sat->sat_s.unicable_para.centre_fre[0] = 1210;
	p_sat->sat_s.unicable_para.centre_fre[1] = 1420 ;
	p_sat->sat_s.unicable_para.centre_fre[2] = 1680 ;
	p_sat->sat_s.unicable_para.centre_fre[3] = 2040 ;
	p_sat->sat_s.unicable_para.centre_fre[4] = 1248 ;
	p_sat->sat_s.unicable_para.centre_fre[5] = 1516 ;
	p_sat->sat_s.unicable_para.centre_fre[6] = 1632 ;
	p_sat->sat_s.unicable_para.centre_fre[7] = 1748;	
}
#endif
	
SIGNAL_HANDLER int app_unicable_set_popup_create(GuiWidget *widget, void *usrdata)
{
#if UNICABLE_SUPPORT

#define STR_LEN	20

	uint32_t sel=0;
	uint32_t if_channel=0;
	//GxMsgProperty_NodeByPosGet CurAtnSat;
	
	char buffer[8] = {0};

	app_antenna_get_cur_sat_info(&CurAtnSat);
	if_channel = CurAtnSat.sat_data.sat_s.unicable_para.if_channel;
	//title
	GUI_SetProperty("text_unicable_popup_title", "string", STR_ID_UNICABLE_SET);
	GUI_SetProperty("cmbox_unicable_lnb_freq", "content", UNICABLE_LNB_FREQ);
	GUI_SetProperty("cmb_unicable_popup_if_channel", "content", IF_CHANNEL);
	if(0 == CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel])
	{
		CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel] = 1210;
	}
	sprintf(buffer,"%04d",CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel]);
	GUI_SetProperty("edit_unicable_popup_centre_freq","string",buffer);
	//SAT_POSITTION
	GUI_SetProperty("cmb_unicable_popup_sat_pos", "content", SAT_POSITTION);
	
	//app_lnb_freq_data_to_sel(&(CurAtnSat.sat_data), &sel);
	sel = CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index;
	if(sel >= ID_UNICANBLE_TATAL_LNB)
	{
		sel = 0;
	}
	GUI_SetProperty("cmbox_unicable_lnb_freq", "select", &sel);
	app_lnb_freq_sel_to_data(sel, &CurAtnSat.sat_data);
	sel = CurAtnSat.sat_data.sat_s.unicable_para.if_channel;
	if(sel > 0x10)
	{
		sel = 0;
	}
	GUI_SetProperty("cmb_unicable_popup_if_channel", "select", &sel);
	sel = CurAtnSat.sat_data.sat_s.unicable_para.sat_pos;
	GUI_SetProperty("cmb_unicable_popup_sat_pos", "select", &sel);
	
	GUI_SetProperty("img_unicablie_popup_ok_back", "state", "hide");
	GUI_SetProperty("box_unicable_popup_para", "state", "show");
	GUI_SetFocusWidget("box_unicable_popup_para");
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_unicable_set_popup_destroy(GuiWidget *widget, void *usrdata)
{
	//sat_list_show_help_info();
	return EVENT_TRANSFER_STOP;
}



SIGNAL_HANDLER int app_unicable_set_popup_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
#if UNICABLE_SUPPORT	
	GUI_Event *event = NULL;

	//GxMsgProperty_NodeByPosGet node_sat;
	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_EXIT:
				case STBK_MENU:
					{
						GUI_EndDialog("wnd_unicable_set_popup");
					}
					break;

				case STBK_OK:
				//	app_antenna_get_cur_sat_info(&node_sat);
					if((app_unicable_popup_ok_keypress() == GXCORE_SUCCESS))
					{
						GUI_EndDialog("wnd_unicable_set_popup");
					}
					break;
					
				default:
					break;
			}
		default:
			break;
	}
#endif
	return ret;
}


SIGNAL_HANDLER int app_unicable_popup_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
#if UNICABLE_SUPPORT
	GUI_Event *event = NULL;
	uint32_t box_sel;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey(event->key.sym))
			{
				case STBK_UP:
					{
						GUI_GetProperty("box_unicable_popup_para", "select", &box_sel);
						if(UNICABLE_POPUP_BOX_ITEM_LNB_FRE == box_sel)
						{
							box_sel = UNICABLE_POPUP_BOX_ITEM_OK;
							GUI_SetProperty("box_unicable_popup_para", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						
					}
					break;

				case STBK_DOWN:					
					{
						GUI_GetProperty("box_unicable_popup_para", "select", &box_sel);
						if(UNICABLE_POPUP_BOX_ITEM_OK == box_sel)
						{
							box_sel = UNICABLE_POPUP_BOX_ITEM_LNB_FRE;
							GUI_SetProperty("box_unicable_popup_para", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
					}
					break;

				case STBK_LEFT:
				case STBK_RIGHT:
					
					break;
                    	
				default:
					break;
			}
		default:
			break;
	}
#endif
	return ret;
}

SIGNAL_HANDLER int app_unicable_cmb_lnb_freq_change(GuiWidget *widget, void *usrdata)
{
#if UNICABLE_SUPPORT
	uint32_t box_sel = 0;
	//uint32_t if_num_sel = 0;
	//GxMsgProperty_NodeByPosGet CurAtnSat;
	//char buffer[8] = {0};

	//app_antenna_get_cur_sat_info(&CurAtnSat);
	GUI_GetProperty("cmbox_unicable_lnb_freq", "select", &box_sel);
	
	printf("---%s %d---\n",__FUNCTION__,box_sel);
	app_lnb_freq_sel_to_data(box_sel, &CurAtnSat.sat_data);
/*
	GUI_GetProperty("cmb_unicable_popup_if_channel", "select", &if_num_sel);
	switch(if_num_sel)
	{
		case 0:
			CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel] = 1210;
			break;
		case 1:
			CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel] = 1210;
			break;
		case 2:
			CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel] = 1210;
			break;
		case 3:
			CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel] = 1210;
			break;
		case 4:
			
		case 5:
		case 6:
		case 7:
			CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel] = 950;
			break;
		default:
			break;
			
	}
	
	sprintf(buffer,"%04d",CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]);
	GUI_SetProperty("edit_unicable_popup_centre_freq","string",buffer);
*/
	app_antenna_timer_unicable_set_frontend();
	
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_unicable_cmb_if_channel_num_change(GuiWidget *widget, void *usrdata)
{
#if UNICABLE_SUPPORT
	uint32_t if_num_sel = 0;
	//GxMsgProperty_NodeByPosGet CurAtnSat;
	char buffer[8] = {0};

	//app_antenna_get_cur_sat_info(&CurAtnSat);
	GUI_GetProperty("cmb_unicable_popup_if_channel", "select", &if_num_sel);

	
	sprintf(buffer,"%04d",CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]);
	GUI_SetProperty("edit_unicable_popup_centre_freq","string",buffer);
	
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_unicable_edit_if_fre_change(GuiWidget *widget, void *usrdata)
{
#if UNICABLE_SUPPORT
	int box_fre_sel = 0;
	uint32_t if_num_sel = 0;
	//GxMsgProperty_NodeByPosGet CurAtnSat;
	char buffer[8] = {0};
	char *atoi_str = NULL;
		
	
	//app_antenna_get_cur_sat_info(&CurAtnSat);
	//centre fre
	GUI_GetProperty("edit_unicable_popup_centre_freq", "string", &atoi_str);
	GUI_GetProperty("cmb_unicable_popup_if_channel", "select", &if_num_sel);
	box_fre_sel =  atoi(atoi_str);
	printf("fre_change=%d\n",box_fre_sel);
	if(box_fre_sel <950 || box_fre_sel > 2150)
	{
		CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]=950;
	}
	else
	{
		CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]=box_fre_sel;
	}
	sprintf(buffer,"%04d",CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]);
	GUI_SetProperty("edit_unicable_popup_centre_freq","string",buffer);	
#endif
	return EVENT_TRANSFER_STOP;
}

