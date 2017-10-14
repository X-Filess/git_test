#include "../include/nstv_platform.h"
#include "../../../include/cascam_os.h"
#include "../../../include/cas/cascam_nstv.h"
#include "../include/porting/gxcas.h"
#include "../include/porting/cd_api.h"

static void nstv_thread_deal(void *arg)
{
	while(1) {
		unsigned long type;
		void *p = NULL;
		GxCas_WaitEvent(&type, &p);
		switch(type) {
			case GXCAS_CD_EMAIL_NOTIFY:
				{
					GxCas_CdEmailNotify *info = (GxCas_CdEmailNotify *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_mail_notify, show_flag = %d, mail_id = %d\033[0m\n", info->byShow, info->dwEmailId);
					nstv_osd_mail_notify(info->byShow, info->dwEmailId);
					break;
				}
			case GXCAS_CD_ENTITLE_CHANGED:
				{
					CDCA_U16 wTvsID = *(CDCA_U16 *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_entitlement_notify, operator_id = %d\033[0m\n", wTvsID);
					nstv_osd_entitlement_notify(wTvsID);
					break;
				}
			case GXCAS_CD_DETITLE:
				{
					CDCA_U8 bstatus = *(CDCA_U8 *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_detitle_notify, detitle_status = %d\033[0m\n", bstatus);
					nstv_osd_detitle_notify(bstatus);
					break;
				}
#if NSCAS_SMC_VERSION
			case GXCAS_CD_CARD_IN:
				cascam_printf("\033[35mNSTV, %s, %d, CARD IN\033[0m\n", __FUNCTION__, __LINE__);
				break;
			case GXCAS_CD_CARD_OUT:
				cascam_printf("\033[35mNSTV, %s, %d, CARD OUT\033[0m\n", __FUNCTION__, __LINE__);
				break;
			case GXCAS_CD_SHOW_IPPV:
				{
					GxCas_CdIppvBuyInfo *info = (GxCas_CdIppvBuyInfo *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_ippx_notify, message = %d, ecm_id = %d,\033[0m\n", info->byMessageType, info->wEcmPid);
					nstv_osd_ippx_notify(info->byMessageType, info->wEcmPid, (void *)&info->pIppvProgram);
					break;
				}
			case GXCAS_CD_HIDE_IPPV:
				{
					CDCA_U16 value = *(CDCA_U16 *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_ippx_hide, ecm_pid = %d\033[0m\n", value);
					nstv_osd_ippx_hide(value);
					break;
				}
#endif
			case GXCAS_CD_OSD_SHOW:
				{
					GxCas_CdShowOsdMessage *info = (GxCas_CdShowOsdMessage *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_screen_display_notify, type = %d\033[0m\n", info->byStyle);
					nstv_osd_screen_display_notify(info->byStyle, info->szMessage);
					break;
				}
			case GXCAS_CD_OSD_HIDE:
				{
					CDCA_U8 value = *(CDCA_U8 *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_screen_display_hide, type = %d\033[0m\n", value);
					nstv_osd_screen_display_hide(value);
					break;
				}
			case GXCAS_CD_LOCK_SERVICE:
				{
					SCDCALockService *info = (SCDCALockService *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_lock_service_control\033[0m\n");
					nstv_lock_service_control(info);
					break;
				}
			case GXCAS_CD_UNLOCK_SERVICE:
				{
                    cascam_printf("\n\033[35mNSTV, GXCAS_CD_UNLOCK_SERVICE\033[0m\n");
					nstv_lock_service_control(NULL);
					break;
				}
			case GXCAS_CD_SHOW_BUY_MESSAGE:
				{
					GxCas_CdBuyMessage *info = (GxCas_CdBuyMessage *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_message_notify, ecm_pid = %d, message = %d\033[0m\n", info->wEcmPid, info->byMessageType);
					nstv_osd_message_notify(info->wEcmPid, info->byMessageType);
					break;
				}
			case GXCAS_CD_SHOW_FINGER_MESSAGE:
				{
					GxCas_CdFingerMessage *info = (GxCas_CdFingerMessage *)p;
                    if(info->fingerMsg)
                    {
                        cascam_printf("\n\033[35mNSTV, nstv_osd_fingerprint_notify, ecm_pid = %d, message = %s\033[0m\n", info->wEcmPid, info->fingerMsg);
                    }
                    else
                    {
                        cascam_printf("\n\033[35mNSTV, show fingerprint error, ecm_pid = %d\033[0m\n", info->wEcmPid);
                    }
					nstv_osd_fingerprint_notify(info->wEcmPid, info->fingerMsg);
					break;
				}
			case GXCAS_CD_HIDE_FINGER_MESSAGE:
				{
					GxCas_CdFingerMessage *info = (GxCas_CdFingerMessage *)p;
                    cascam_printf("\n\033[35mNSTV, hide fingerprint, ecm_pid = %d\033[0m\n", info->wEcmPid);
					nstv_osd_fingerprint_notify(info->wEcmPid, NULL);
					break;
				}
			case GXCAS_CD_SHOW_CURTAIN_NOTIFY:
				{
					GxCas_CdShowCurtainNotify *info = (GxCas_CdShowCurtainNotify *)p;
                    cascam_printf("\n\033[35mNSTV, nstv_osd_curtain_notify, ecmpid = %d, wCurtainCode = %d\033[0m\n",info->wEcmPid, info->wCurtainCode);
					nstv_osd_curtain_notify(info->wEcmPid, info->wCurtainCode);
					break;
				}
			case GXCAS_CD_SHOW_PROGRESS:
                {
                   GxCas_CdProgressStrip *info = (GxCas_CdProgressStrip*)p;
                   cascam_printf("\n\033[35mNSTV, GXCAS_CD_SHOW_PROGRESS, update_status = %d, process = %d\n\033[0m", info->byMark, info->byProgress);
                   nstv_osd_card_update_notify(info->byMark, info->byProgress);
                }
				break;
			case GXCAS_CD_ACTION_REQUEST_EXIT:
				{
					GxCas_CdActionRequestExit *info = (GxCas_CdActionRequestExit *)p;
                    if(cascam_strlen((char*)(info->pbyData)) > 0)
                        cascam_printf("\n\033[35mNSTV, nstv_action_notify, action_type = %d, len = %d, data = %s\033[0m", info->byActionType, info->byLen, info->pbyData);
                    else
                        cascam_printf("\n\033[35mNSTV, nstv_action_notify, action_type = %d, len = %d, data = %s\033[0m", info->byActionType, info->byLen, "NULL");
                    nstv_action_notify(info->wTvsID, info->byActionType);
					break;
				}
#if NSCAS_SMC_VERSION
            case GXCAS_CD_SHOW_FEEDINGREQUEST:
                {
                    CDCA_BOOL status = *(CDCA_BOOL *)p;
                    cascam_printf("nstv_action_notify: GXCAS_CD_SHOW_FEEDINGREQUEST status = %d\n", status);
                    app_nstv_card_parent_child_osd(status);
                    break;
				}
#endif
			default:
                cascam_printf("\n\033[35mNSTV, %s, %d, msg_id = 0x%x\033[0m\n", __FUNCTION__, __LINE__, type);
				break;
		}
        cascam_thread_delay(200);
		cascam_free(p);
	}
}

void nstv_event_task(void)
{
	int thread = 0;
	cascam_thread_create("nstv_event", &thread, nstv_thread_deal, NULL, 30*1024, 10);
}
