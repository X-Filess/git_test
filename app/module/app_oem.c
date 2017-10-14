/*
 * =====================================================================================
 *
 *       Filename:  app_oem.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年05月10日 14时20分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "gxcore.h"
#include "tree.h"
#include "parser.h"
#include "app_module.h"

typedef unsigned char xmlChar;

static const struct
{
    app_oem_type type;
    char * gname;
    char * iname;
}oem_item[] = {
    { OEM_LANG_OSD,         "language",         "osd language"      },
    { OEM_LANG_SUBT,        "language",         "subtitle language" },
    { OEM_LANG_TTX,         "language",         "teletext language" },
    { OEM_HW_VERSION,          "common",           "hardware version"},
    { OEM_SW_VERSION,          "common",           "software version"},
    { OEM_TRANS,            "common",           "transparence"      },
    { OEM_VOLUME,           "common",           "volume"            },
    { OEM_TV_STANDARD,      "common",           "tv standard"       },
    { OEM_TIME_ZONE,        "common",           "time zone"         },
    { OEM_SUMMER_TIME,      "common",           "summer time"       },
    { OEM_SEARCH_MODE,      "common",           "search mode"       },
    { OEM_SCART_CONF,      "scart config",           "SCART"       }, 
    { OEM_SPDIF_CONF,      "spdif config",           "SPDIF"       }, 
   // { OEM_TUNER_TYPE,       "frontend",         "tuner type"        },
    {OEM_TUNER_CONFIG,   "frontend",         "tuner config"},
    //{ OEM_DEMOD_TYPE,       "frontend",         "demod type"        },
    //{ OEM_DEMOD_CHIPADDR,   "frontend",         "demod chip address"},
    //{ OEM_IQ_SWAP,          "frontend",         "iq swap"           },
    //{ OEM_HV_SWITCH,        "frontend",         "hv switch"         },
    { OEM_TYPE_MAX,         NULL,               NULL                }
}; 

#define APP_OEM_PATH    "/dvb/theme/default.xml"
#define OEM_STR         (xmlChar*)"default"
#define ITEM_STR        (xmlChar*)"item"
#define GROUP_NAME      (xmlChar*)"name"
#define ITEM_DISP       (xmlChar*)"display"
#define ITEM_VAL        (xmlChar*)"value"
#define LIST_VAL        (xmlChar*)"value"


/*************** xml parser start ****************/
static status_t info_get_list_item(GXxmlNodePtr xml_item, xmlChar *name, char * buf, size_t size)
{
	GXxmlNodePtr item = NULL;
	xmlChar *value = NULL;
	xmlChar *iname = NULL;

	item =  xml_item->childs;
	while (item != NULL) 
	{
        	iname = GXxmlGetProp(item, LIST_VAL);

		if (GXxmlStrcmp(iname, name) == 0) 
		{
		    value = GXxmlGetProp(item, ITEM_DISP);
		    strncpy(buf, (char*)value, size);

		    GxCore_Free(iname);
		    GxCore_Free(value);
		    
		    return GXCORE_SUCCESS;
		}
		GxCore_Free(iname);
		item = item->next;
    	}

	GxCore_Free(value);

	return GXCORE_ERROR;
}
static status_t info_get(GXxmlNodePtr GXxml_item, xmlChar *name, char * buf, size_t size,int mode)
{
	GXxmlNodePtr item = NULL;
	xmlChar *value = NULL;
	xmlChar *iname = NULL;

	item =  GXxml_item->childs;
	while (item != NULL) 
	{
        iname = GXxmlGetProp(item, ITEM_DISP);

		if (GXxmlStrcmp(iname, name) == 0) 
        {
            value = GXxmlGetProp(item, ITEM_VAL);
            strncpy(buf, (char*)value, size);
	     if(mode)
	     {
			info_get_list_item(item,value,buf, size);
	     }
            GxCore_Free(iname);
            GxCore_Free(value);
            
            return GXCORE_SUCCESS;
        }
        GxCore_Free(iname);
        item = item->next;
    }

	GxCore_Free(value);

	return GXCORE_ERROR;
}

status_t oem_parser(const char *filename, app_oem_type type, char * buf, size_t size,int mode)
{
	GXxmlDocPtr doc;
	xmlChar *gname = NULL;

	if(NULL == filename)
	{
		return GXCORE_ERROR;
	}

	doc = GXxmlParseFile(filename);
	if (NULL == doc) 
	{
		printf( "[app oem]GXxml parse error.\n");
		return GXCORE_ERROR;
	}

    // root
	GXxmlNodePtr root = doc->root;
	if (root == NULL) 
	{
		printf( "[app oem]GXxml empty\n");
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	if (0 !=  GXxmlStrcmp(root->name, OEM_STR))
	{
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

    // group
	GXxmlNodePtr group = root->childs;
	while (NULL != group) 
	{
		gname = GXxmlGetProp(group, GROUP_NAME);

		if (gname != NULL &&
			0 ==  GXxmlStrcmp(gname, (xmlChar*)(oem_item[type].gname)))
		{
            info_get(group, (xmlChar*)(oem_item[type].iname), buf, size,mode);
            GxCore_Free(gname);
            GXxmlFreeDoc(doc);
            return GXCORE_SUCCESS;
 		}

 		GxCore_Free(gname);
 		gname = NULL;
		group = group->next;
	}

	GXxmlFreeDoc(doc);
	return GXCORE_ERROR;
}

/*************** GXxml parser end ****************/

status_t app_oem_get(app_oem_type type, char * buf, size_t size)
{
	#ifdef LINUX_OS
	return oem_parser(WORK_PATH"theme/default.xml", type, buf, size,0);
	#else
    return oem_parser(APP_OEM_PATH, type, buf, size,0);
	#endif
}

status_t app_oem_get_float(app_oem_type type, float *result)
{
#define BUF_LEN 256
    char buf[BUF_LEN] = {0};

    if (app_oem_get(type, buf, BUF_LEN) != GXCORE_SUCCESS)
        return GXCORE_ERROR;

    *result = atof(buf);
    return GXCORE_SUCCESS;
}

status_t app_oem_get_int(app_oem_type type, int32_t *result)
{
#define BUF_LEN 256
    char buf[BUF_LEN] = {0};

    if (app_oem_get(type, buf, BUF_LEN) != GXCORE_SUCCESS)
        return GXCORE_ERROR;

    *result = atoi(buf);
    return GXCORE_SUCCESS;
}

status_t app_oem_get_list_value(app_oem_type type, char * buf, size_t size)
{
	#ifdef LINUX_OS
	return oem_parser(WORK_PATH"theme/default.xml", type, buf, size,0);
	#else
    	return oem_parser(APP_OEM_PATH, type, buf, size,1);
	#endif
}
