#include "app.h"
#include "jansson.h"

#define AP_RECORD_FILE "/home/gx/ap_psk_record"

static handle_t s_ap_record_mutex = -1;

static int _save_object(json_t * pObj, char * pMac, char * pSsid, char * pPsk)
{
    if(json_object_set(pObj, "mac", json_string(pMac)) < 0)
        return -1;

    if(json_object_set(pObj, "ssid", json_string(pSsid)) < 0)
        return -1;

    if(json_object_set(pObj, "psk", json_string(pPsk)) < 0)
        return -1;

    return 0;
}

status_t app_wifi_ap_save(char * mac, char * ssid, char * psk)
{
    json_t *pArray   = NULL;
    json_t *pObj     = NULL;
    json_t *pMac     = NULL;

    const char *pMacGet = NULL;
    uint32_t i = 0;
    uint32_t nSize = 0;

    if(IS_EMPTY_OR_NULL(mac)) {
        printf("[AP SAVE] mac is NULL!\n");
        return GXCORE_ERROR;
    }

    if(ssid && strlen(ssid) == 0)
        ssid = "NO NAME";

    if(psk && strlen(psk) == 0)
        psk = "12345678";

    if(-1 == s_ap_record_mutex) {
        GxCore_MutexCreate(&s_ap_record_mutex);
    }

    if(GxCore_FileExists(AP_RECORD_FILE) != GXCORE_FILE_EXIST) {
        pObj = json_object();
        if(_save_object(pObj, mac, ssid, psk) < 0) {
            json_decref(pObj);
            return GXCORE_ERROR;
        }

        pArray = json_array();
        if(json_array_append(pArray, pObj) < 0) {
            json_decref(pObj);
            json_decref(pArray);
            return GXCORE_ERROR;
        }

        if((json_dump_file(pArray, AP_RECORD_FILE, 4) < 0)) {
            printf("[AP SAVE0] dump file failed!\n");
            json_decref(pObj);
            json_decref(pArray);
            return GXCORE_ERROR;
        }        

    } else {
        GxCore_MutexLock(s_ap_record_mutex);
        pArray = json_load_file(AP_RECORD_FILE, 0, NULL);
        if(!json_is_array(pArray)) {   
            printf("[AP SAVE] load file failed!\n");
            json_decref(pArray);
            GxCore_MutexUnlock(s_ap_record_mutex);
            return GXCORE_ERROR;
        }   

        nSize = json_array_size(pArray);
        for(i = 0; i < nSize; i++)
        {   
            pObj = json_array_get(pArray, i); 
            pMac = json_object_get(pObj, "mac");
            pMacGet = json_string_value(pMac);
            if(0 == strcmp(pMacGet, mac)) {
                break;
            }
        }

        pObj = json_object();
        if(_save_object(pObj, mac, ssid, psk) < 0) {
            json_decref(pObj);
            json_decref(pArray);
            GxCore_MutexUnlock(s_ap_record_mutex);
            return GXCORE_ERROR;
        }

        if(i == nSize) { //no found
            if(20 == nSize) {
                printf(" @@@@@@@@@@@@@@@@@@@ >> delete old ap record!\n");
                if(json_array_remove(pArray, 0) < 0) {
                    json_decref(pObj);
                    json_decref(pArray);
                    GxCore_MutexUnlock(s_ap_record_mutex);
                    return GXCORE_ERROR;
                }
            }

            if(json_array_append(pArray, pObj) < 0) {
                json_decref(pObj);
                json_decref(pArray);
                GxCore_MutexUnlock(s_ap_record_mutex);
                return GXCORE_ERROR;
            }
        }
        else
        {
            if(json_array_set(pArray, i, pObj) < 0) {
                json_decref(pObj);
                json_decref(pArray);
                GxCore_MutexUnlock(s_ap_record_mutex);
                return GXCORE_ERROR;
            }
        }

        //printf("\n\nJson : %s\n", json_dumps(pArray, 2));
        if(json_dump_file(pArray, AP_RECORD_FILE, 4) < 0) {
            printf("[AP SAVE] dump file failed!\n");
            json_decref(pObj);
            json_decref(pArray);
            GxCore_MutexUnlock(s_ap_record_mutex);
            return GXCORE_ERROR;
        }        
        GxCore_MutexUnlock(s_ap_record_mutex);
    }

    json_decref(pObj);
    json_decref(pArray);
    return GXCORE_SUCCESS;
}

status_t app_wifi_ap_load(char * mac, char * ssid, char * psk)
{
    json_t *pArray    = NULL;
    json_t *pObj      = NULL;
    json_t *pMac      = NULL;
    //json_t *pSsid = NULL;
    json_t *pPsk  = NULL;

    const char *pMacGet = NULL ;
    //const char *pSsidGet = NULL ;
    const char *pPskGet = NULL ;
    int i = 0;
    uint32_t nSize = 0;

    if(IS_EMPTY_OR_NULL(mac)) {
        printf("[AP LOAD] mac is NULL!\n");
        return GXCORE_ERROR;
    }

    if(GxCore_FileExists(AP_RECORD_FILE) != GXCORE_FILE_EXIST){
        printf("[AP LOAD] file /home/gx/ap_psk_record unexist!\n");
        return GXCORE_ERROR;
    }
    if(-1 == s_ap_record_mutex) {
        GxCore_MutexCreate(&s_ap_record_mutex);
    }

    GxCore_MutexLock(s_ap_record_mutex);
    pArray = json_load_file(AP_RECORD_FILE, 0, NULL);
    //printf("Json : %s\n", json_dumps(pArray, 2));
    if(!json_is_array(pArray)) {   
        printf("[AP LOAD] load file failed!\n");
        json_decref(pArray);
        GxCore_MutexUnlock(s_ap_record_mutex);
        return GXCORE_ERROR;
    }   
    
    nSize = json_array_size(pArray);
    for(i = 0; i < nSize; i++)
    {   
        pObj = json_array_get(pArray, i); 
        pMac = json_object_get(pObj, "mac");
        pMacGet = json_string_value(pMac);
        if(0 == strcmp(pMacGet, mac)) {
            pPsk = json_object_get(pObj, "psk");
            pPskGet = json_string_value(pPsk);
            strcpy(psk, pPskGet);
            break;
        }
    }

    if(json_dump_file(pArray, AP_RECORD_FILE, 4) < 0) {
        printf("[AP LOAD] dump file failed!\n");
        json_decref(pArray);
        GxCore_MutexUnlock(s_ap_record_mutex);
        return GXCORE_ERROR;
    }        
    GxCore_MutexUnlock(s_ap_record_mutex);

    json_decref(pArray);
    if(i < nSize)
        return GXCORE_SUCCESS;

    return GXCORE_ERROR;
}

