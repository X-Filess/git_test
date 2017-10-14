/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_ippv_notify.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.10		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"

#if NSCAS_SUPPORT
#define WND_IPPV_NOTIFY             "wnd_nstv_ippv_notify"
#define PATH_XML                    "ca/nstv/wnd_nstv_ippv_notify.xml"
#define TXT_TITLE                   "txt_nstv_ippv_notify_title"
#define TXT_CONTENT0                "txt_nstv_ippv_notify_content0"
#define TXT_CONTENT1                "txt_nstv_ippv_notify_content1"
#define TXT_CONTENT2                "txt_nstv_ippv_notify_content2"
#define TXT_CONTENT3                "txt_nstv_ippv_notify_content3"
#define TXT_NOTICE1                 "txt_nstv_ippv_notify_content4"
#define TXT_NOTICE2                 "txt_nstv_ippv_notify_content5"
#define BOX_IPPV_PRICE              "box_nstv_ippv_notify_control"
#define CMB_IPPV_PRICE              "cmb_nstv_ippv_notify_chose"
#define BOX_IPPV_PIN                "box_nstv_ippv_notify_pin"
#define EDIT_IPPV_PIN               "edit_nstv_ippv_notify_pin"

#define BTN_YES                     "btn_nstv_ippv_notify_control1"
#define BTN_NO                      "btn_nstv_ippv_notify_control2"
/*
 *资料来看，IPP购买和ECMPID相关，也就是说有可能一个节目音视频需要单独购买
 *目前软件只考虑同时只弹出一个购买框，后续根据实际环境再做修改支持了。
 * */
static CASCAMNSTVIPPVNotifyClass thiz_nstv_ippv_notify = {0};
static CASCAMNSTVIPPVBuyClass thiz_nstv_ippv_buy = {0};
static char thiz_buy_flag = 0;

static void _create_dialog_deal_with(void)
{
    if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_info")))
    {
        GUI_EndDialog("wnd_channel_info");
    }
    if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_info_signal")))
    {
        GUI_EndDialog("wnd_channel_info_signal");
    }
}

void app_nstv_ippv_notify_create(void *param)
{
    CASCAMNSTVIPPVNotifyClass *p = NULL;
    static char init_flag = 0;

    if(NULL == param)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    p = (CASCAMNSTVIPPVNotifyClass*)param;
    if(0 == memcmp(p, &thiz_nstv_ippv_notify, sizeof(CASCAMNSTVIPPVNotifyClass)))
    {
        printf("\nWARNING, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    _create_dialog_deal_with();
    // new notify, need end diaog, create new
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_IPPV_NOTIFY))
    {
        GUI_EndDialog(WND_IPPV_NOTIFY);
    }
    memcpy(&thiz_nstv_ippv_notify, p, sizeof(CASCAMNSTVIPPVNotifyClass));
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_IPPV_NOTIFY, PATH_XML);
    }
    app_create_dialog(WND_IPPV_NOTIFY);
}

void app_nstv_ippv_notify_exit(char *param)
{
    unsigned short ecm_pid = 0;
    if(NULL == param)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return;
    }
    ecm_pid = (param[1] << 8) | (param[0]);
    if(ecm_pid == thiz_nstv_ippv_notify.ecm_id)
    {
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_IPPV_NOTIFY))
        {
            GUI_EndDialog(WND_IPPV_NOTIFY);
        }
        memset(&thiz_nstv_ippv_notify, 0, sizeof(CASCAMNSTVIPPVNotifyClass));
    }
}

static void  _mjd_to_ymd(unsigned int   mjd,
                            unsigned short   *year,
                            unsigned char  *month,
                            unsigned char  *day)
{
	unsigned int  uMjd;
	unsigned int  y, m, k;


	uMjd = (unsigned int)mjd;

	if( uMjd < 15079 ) { /* MJD Lower than 1900/3/1 ? */
		uMjd += 0x10000; /* Adjust MJD */
	}

	y = ( uMjd * 100 - 1507820 ) / 36525;					/* Calculate Y', M' */
	m = ( uMjd * 10000 - 149561000 - ( ( y * 36525 ) / 100 ) * 10000 ) / 306001;

	*day = (unsigned char)( uMjd - 14956 - ( ( y * 36525 ) / 100 ) - ( ( m * 306001 ) / 10000 ) );
	/* Calculate Day */
	k = ( ( m == 14 ) || ( m == 15 ) ) ? 1 : 0;/* If M'=14 or M'=15 then K=1 else K=0 */

	*year = (unsigned short )( y + k ) + 1900;	/* Calculate Year */
	*month = (unsigned char)( m - 1 - k * 12 );	/* Calculate Month */
#if 0
	*weekDay = (unsigned char)( ( ( uMjd + 2 ) % 7 ) + 1 ); /* Calculate Week Day */
	if (*weekDay >= 7)
	{
		*weekDay = *weekDay % 7;
	}
#endif
}

static void  _ymd_to_mjd(unsigned int   *mjd,
                            unsigned short   year,
                            unsigned char  month,
                            unsigned char  day)
{// 1900/01/01
	unsigned int  uMjd;
	unsigned int  l;

	if( month == 1 || month ==2)
	{
		l = 1;
	}
	else
	{
		l = 0;
	}

	uMjd = 14956 + day + (unsigned int)((year - l)*36525/100) + (unsigned int)((month + 1 + l*12) * 306001/10000);
	*mjd = (unsigned short )uMjd;
}

// for ui
static char *thiz_ippv_price_widget[NSTV_IPPV_PRICE_MAX] = {
    TXT_NOTICE1,
    TXT_NOTICE2,
};
static char *thiz_focus_widget[] = {
    BOX_IPPV_PRICE,
    BOX_IPPV_PIN,
    BTN_YES,
    BTN_NO,
};
enum{
    IPPV_PRICE_,
    IPPV_PIN_,
    IPPV_YES_,
    IPPV_NO_,
};

static unsigned char thiz_focus_index = IPPV_PIN_;
static int _ippv_exit(PopDlgRet ret)
{
    // exit menu
    GUI_EndDialog(WND_IPPV_NOTIFY);
    memset(&thiz_nstv_ippv_notify, 0, sizeof(CASCAMNSTVIPPVNotifyClass));
   return 0;
}

static int _exit_for_yes(void)
{
    int ret = 0;

    // init buy param
    int index = 0;
    char *str = NULL;
    GUI_GetProperty(EDIT_IPPV_PIN, "string", &str);
    if((NULL == str) || (6 != strlen(str)))
    {
        // notice: invild PIN
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_UNBLOCK;
        pop.type = POP_TYPE_NO_BTN;
        pop.timeout_sec = 3;
        pop.str = NSTV_OSD_INVALID_PIN;
        popdlg_create(&pop);

        GUI_SetProperty(EDIT_IPPV_PIN, "clear", NULL);
        return -1;
    }
    memcpy(thiz_nstv_ippv_buy.pin, str, 6);

    GUI_GetProperty(CMB_IPPV_PRICE, "select", &index);
    thiz_nstv_ippv_buy.price = thiz_nstv_ippv_notify.buy_price.unit_info[index].price;
    thiz_nstv_ippv_buy.price_code = thiz_nstv_ippv_notify.buy_price.unit_info[index].price_code;
    thiz_nstv_ippv_buy.ecm_pid = thiz_nstv_ippv_notify.ecm_id;
    thiz_nstv_ippv_buy.ippt_flag = thiz_nstv_ippv_notify.ippt_flag;
    thiz_buy_flag = 1;

    if(thiz_nstv_ippv_notify.exit_func)
    {
        CASCAMNSTVIPPVBuyReturnClass *ippv_ret = NULL;
        char *notice = NULL;
        notice = thiz_nstv_ippv_notify.exit_func(thiz_buy_flag, &thiz_nstv_ippv_buy);
        if(notice)
        {
            ippv_ret = (CASCAMNSTVIPPVBuyReturnClass*)notice;
            if((ippv_ret->notice) && (ippv_ret->status == 0))
            {
                // need pop up a dialog for notice
                PopDlg  pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.mode = POP_MODE_UNBLOCK;
                pop.type = POP_TYPE_NO_BTN;
                pop.timeout_sec = 3;
                pop.str = ippv_ret->notice;
                pop.exit_cb = _ippv_exit;
                popdlg_create(&pop);
            }
            else if(ippv_ret->notice)
            {
                 // need pop up a dialog for notice
                PopDlg  pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.mode = POP_MODE_UNBLOCK;
                pop.type = POP_TYPE_NO_BTN;
                pop.timeout_sec = 3;
                pop.str = ippv_ret->notice;
                popdlg_create(&pop);
                GUI_SetProperty(EDIT_IPPV_PIN, "clear", NULL);
            }
            return 0;
        }
    }

    GUI_EndDialog(WND_IPPV_NOTIFY);
    memset(&thiz_nstv_ippv_notify, 0, sizeof(CASCAMNSTVIPPVNotifyClass));
    return ret;
}
static int _exit_for_no(void)
{
    int ret = 0;
    thiz_buy_flag = 0;
    thiz_nstv_ippv_buy.ecm_pid = thiz_nstv_ippv_notify.ecm_id;
    if(thiz_nstv_ippv_notify.exit_func)
    {
        char *notice = NULL;
        notice = thiz_nstv_ippv_notify.exit_func(thiz_buy_flag, &thiz_nstv_ippv_buy);
        if(notice)
        {
#if 0
            // need pop up a dialog for notice
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.mode = POP_MODE_UNBLOCK;
            pop.type = POP_TYPE_NO_BTN;
            pop.str = notice;
            pop.exit_cb = _ippv_exit;
            pop.timeout_sec = 3;
            popdlg_create(&pop);
            return 0;
#endif
        }
    }

    GUI_EndDialog(WND_IPPV_NOTIFY);
    memset(&thiz_nstv_ippv_notify, 0, sizeof(CASCAMNSTVIPPVNotifyClass));
    return ret;
}

SIGNAL_HANDLER int On_wnd_nstv_ippv_notify_create(GuiWidget *widget, void *usrdata)
{
    char buf[50] = {0};
    int i = 0;

    // title
    if(thiz_nstv_ippv_notify.message)
    {
        GUI_SetProperty(TXT_TITLE, "string", thiz_nstv_ippv_notify.message);
    }
    // operator id display
    sprintf(buf, "%s   %u", NSTV_OPERATOR_ID, thiz_nstv_ippv_notify.operator_id);
    GUI_SetProperty(TXT_CONTENT0, "string", buf);
    // product id display
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s:   %u", NSTV_PRODUCT_ID,thiz_nstv_ippv_notify.product_id);
    GUI_SetProperty(TXT_CONTENT1, "string", buf);
    // time display
    memset(buf, 0, sizeof(buf));
    if(NSTV_IPPT == thiz_nstv_ippv_notify.ippt_flag)
    {
        sprintf(buf, "%s:   %d (%s)", NSTV_OSD_INTERVAL_TIME, thiz_nstv_ippv_notify.time.interval_min, NSTV_OSD_MINUTE);
    }
    else
    {
        unsigned int mjd = 0;
        unsigned short year = 0;
        unsigned char month = 0;
        unsigned char day = 0;
        _ymd_to_mjd(&mjd, (2000-1900), 1, 1);
        mjd += thiz_nstv_ippv_notify.time.expired_date;
        _mjd_to_ymd(mjd, &year, &month, &day);
        sprintf(buf, "%s:   %04d-%02d-%02d", NSTV_OSD_EXPIRED_TIME, year, month, day);
    }
    GUI_SetProperty(TXT_CONTENT2, "string", buf);

    // wallet id display
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s:   %d", NSTV_OSD_WALLET_ID, thiz_nstv_ippv_notify.wallet_id);
    GUI_SetProperty(TXT_CONTENT3, "string", buf);

    // price notice
    for(i = 0; i < thiz_nstv_ippv_notify.buy_price.unit_count; i++)
    {
        memset(buf, 0, sizeof(buf));
        switch(thiz_nstv_ippv_notify.buy_price.unit_info[i].price_code)
        {
            case NSTV_IPPVVIEW:
                sprintf(buf, "%d, %s:    %d %s",
                            (i + 1),
                            NSTV_OSD_IPPV_PRICE_TYPE1,
                            thiz_nstv_ippv_notify.buy_price.unit_info[i].price,
                            NSTV_OSD_IPPV_POINT);
                break;
            case NSTV_IPPVVIEWTAPING:
                sprintf(buf, "%d, %s:    %d %s",
                            (i + 1),
                            NSTV_OSD_IPPV_PRICE_TYPE2,
                            thiz_nstv_ippv_notify.buy_price.unit_info[i].price,
                            NSTV_OSD_IPPV_POINT);
                break;
            default:
                printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
                break;
        }
        GUI_SetProperty(thiz_ippv_price_widget[i], "string", buf);
        //GUI_SetProperty(thiz_ippv_price_widget[i], "forecolor", "[#000000,#000000,#000000]");
    }

    // price chose
    memset(buf, 0, sizeof(buf));
    if(thiz_nstv_ippv_notify.buy_price.unit_count > 0)
    {
        strcat(buf, "[");

        for(i = 0; i < thiz_nstv_ippv_notify.buy_price.unit_count; i++)
        {
            char temp[10] = {0};
            sprintf(temp,"%d,", (i+1));
            strcat(buf, temp);
        }
        memcpy(buf + strlen(buf)-1, "]", 1);
    }
    else
    {
        strcat(buf, "[NONE]");
    }
    GUI_SetProperty(CMB_IPPV_PRICE, "content", buf);

    // for PIN
    GUI_SetProperty(EDIT_IPPV_PIN, "default_intaglio", "-");
    GUI_SetProperty(EDIT_IPPV_PIN, "intaglio", "*");

    // chose button
    thiz_focus_index = IPPV_PIN_;
    GUI_SetFocusWidget(thiz_focus_widget[thiz_focus_index]);
    thiz_buy_flag = 0;
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int On_wnd_nstv_ippv_notify_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int On_wnd_nstv_ippv_notify_keypress(GuiWidget *widget, void *usrdata)
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
#if DLNA_SUPPORT
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(NULL, NULL);
                    break;
#endif
                case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;
                case STBK_RED:
                //case STBK_UP:
                    if(IPPV_PRICE_ == thiz_focus_index)
                    {
                        thiz_focus_index = IPPV_YES_;
                    }
                    else if(IPPV_PIN_ != thiz_focus_index)
                    {
                        thiz_focus_index = IPPV_PIN_;
                    }
                    else
                    {
                        thiz_focus_index--;
                    }
                    GUI_SetFocusWidget(thiz_focus_widget[thiz_focus_index]);
                    break;
                case STBK_GREEN:
                //case STBK_DOWN:
                    if((IPPV_YES_ == thiz_focus_index)
                            || (IPPV_NO_ == thiz_focus_index))
                    {
                        thiz_focus_index = IPPV_PRICE_;
                    }
                    else
                    {
                        thiz_focus_index++;
                    }
                    GUI_SetFocusWidget(thiz_focus_widget[thiz_focus_index]);
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:
                    if(IPPV_YES_ == thiz_focus_index)
                    {
                        thiz_focus_index = IPPV_NO_;
                    }
                    else if(IPPV_NO_ == thiz_focus_index)
                    {
                        thiz_focus_index = IPPV_YES_;
                    }
                    GUI_SetFocusWidget(thiz_focus_widget[thiz_focus_index]);
                    break;
                case STBK_UP:
                case STBK_DOWN:
                    _exit_for_no(); 
                    GUI_SendEvent("wnd_full_screen", event);
                    ret = EVENT_TRANSFER_STOP;
                    break;
				case STBK_OK:
                    if(IPPV_YES_ == thiz_focus_index)
                    {// exit for yes
                        _exit_for_yes(); 
                    }
                    else if(IPPV_NO_ == thiz_focus_index)
                    {// exit for no
                        _exit_for_no(); 
                    }
	                break;
			    case STBK_EXIT:
                case STBK_MENU:
                    _exit_for_no(); 
                    break;
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int On_box_nstv_ippv_notify_control_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
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
                case STBK_UP:
                case STBK_DOWN:
                    GUI_SendEvent(WND_IPPV_NOTIFY, event);
                    ret = EVENT_TRANSFER_STOP;
                    break;
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}
SIGNAL_HANDLER int On_box_nstv_ippv_notify_pin_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
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
                case STBK_UP:
                case STBK_DOWN:
                    GUI_SendEvent(WND_IPPV_NOTIFY, event);
                    ret = EVENT_TRANSFER_STOP;
                    break;
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}


#else
SIGNAL_HANDLER int On_wnd_nstv_ippv_notify_create(GuiWidget *widget, void *usrdata)
{
    return 0;
}
SIGNAL_HANDLER int On_wnd_nstv_ippv_notify_destroy(GuiWidget *widget, void *usrdata)
{
    return 0;
}
SIGNAL_HANDLER int On_wnd_nstv_ippv_notify_keypress(GuiWidget *widget, void *usrdata)
{
    return 0;
}
SIGNAL_HANDLER int On_box_nstv_ippv_notify_control_keypress(GuiWidget *widget, void *usrdata)
{
    return 0;
}
SIGNAL_HANDLER int On_box_nstv_ippv_notify_pin_keypress(GuiWidget *widget, void *usrdata)
{
    return 0;
}
#endif
