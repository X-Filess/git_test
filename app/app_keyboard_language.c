#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_keyboard_language.h"
/***********************************************************************************/
// cur_sel: from 1 to max_sel
/***********************************************************************************/
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
#define MINI_KEYBOARD_STATUS
#endif
//lang_keyboard_proc s_lang_keyboard_proc = NULL;
static char s_lang_keyboard_input[STR_BUF_LEN + 1] = {0};
static char* s_invalid_char = KEY_LAN_INVALID_CHAR_NAME;

static char *s_english_keymap[KEY_COUNT][4] = {
	{ "q","Q","1","button_key_0",},
	{ "w","W","2","button_key_1",},
	{ "e","E","3","button_key_2",},
	{ "r","R","4","button_key_3",},
	{ "t","T","5","button_key_4",},
	{ "y","Y","6","button_key_5",},
	{ "u","U","7","button_key_6",},
	{ "i","I","8","button_key_7",},
	{ "o","O","9","button_key_8",},
	{ "p","P","0","button_key_9",},
	{ "[","{","-","button_key_10",},
	{ "]","}","=","button_key_11",},
	{ "a","A","!","button_key_12",},
	{ "s","S","@","button_key_13",},
	{ "d","D","#","button_key_14",},
	{ "f","F","$","button_key_15",},
	{ "g","G","%","button_key_16",},
	{ "h","H","^","button_key_17",},
	{ "j","J","&","button_key_18",},
	{ "k","K","*","button_key_19",},
	{ "l","L","(","button_key_20",},
	{ ";",":",")","button_key_21",},
	{ "\'","\"","_","button_key_22",},
	{ "\\","|","+","button_key_23",},
	{ "z","Z","~","button_key_24",},
	{ "x","X","`","button_key_25",},
	{ "c","C","\"","button_key_26",},
	{ "v","V","\\","button_key_27",},
	{ "b","B","/","button_key_28",},
	{ "n","N","|","button_key_29",},
	{ "m","M",".","button_key_30", },
	{ ",","<","<","button_key_31",},
	{ ".",">",">","button_key_32",},
	{ "/","?","?","button_key_33",},
	{ "English","English","English","button_key_34",},
	{ "a/A","A/a","A/a","button_key_35",},
	{ "123","123","123","button_key_36",},
	{ "","","","button_key_37",},
	{ "\x20","\x20","\x20","button_key_38",},
	{ "","","","button_key_39",},
	{ "OK","OK","OK","button_key_40",},
	{ "","","","button_key_41",},
	{ "Clr(F1)","Clr(F1)","Clr(F1)","button_key_42",},
};

static  char* s_arabic_keymap[KEY_COUNT][2] = {
	{"\xd8\xa1","\xd9\x89",},//"\x11\x06\x21\0", //  \x11\x06\x49\0
	{"\xd8\xa2","\xd9\x8a",},//"\x11\x06\x22\0", "\x11\x06\x4a\0",
	{"\xd8\xa3","\xd9\x8b",},//"\x11\x06\x23\0","\x11\x06\x4b\0",
	{"\xd8\xa4","\xd9\x8c",},//"\x11\x06\x24\0","\x11\x06\x4c\0",
	{"\xd8\xa5","\xd9\x8d",},//"\x11\x06\x25\0","\x11\x06\x4d\0",
	{"\xd8\xa6","\xd9\x8e",},//"\x11\x06\x26\0","\x11\x06\x4e\0",
	{"\xd8\xa7","\xd9\x8f",},//"\x11\x06\x27\0","\x11\x06\x4f\0",
	{"\xd8\xa8","\xd9\x80",}, //"\x11\x06\x28\0""\x11\x06\x40\0",
	{"\xd8\xa9","\xd9\x90",}, //"\x11\x06\x29\0","\x11\x06\x50\0",
	{"\xd8\xaa","\xd9\x91",},//"\x11\x06\x2A\0","\x11\x06\x51\0",
	{"\xd8\xab","\xd9\x92",}, //"\x11\x06\x2B\0","\x11\x06\x52\0",},
	{"\xd8\xac","\xd9\x93",},//"\x11\x06\x2C\0""\x11\x06\x53\0",
	{"\xd8\xad","\xd9\x94",},//"\x11\x06\x2D\0","\x11\x06\x54\0"
	{"\xd8\xae","\xd9\x95",},//"\x11\x06\x2E\0","\x11\x06\x55\0",
	{"\xd8\xaf" ,"\xd9\xa0",},//"\x11\x06\x2F\0","\x11\x06\x60\0",
	{"\xd8\xb0","\xd9\xa1",},//"\x11\x06\x30\0","\x11\x06\x61\0",
	{"\xd8\xb1","\xd9\xa2",},//"\x11\x06\x31\0","\x11\x06\x62\0",
	{"\xd8\xb2","\xd9\xa3",},//"\x11\x06\x32\0","\x11\x06\x63\0",
	{"\xd8\xb3","\xd9\xa4",},//"\x11\x06\x33\0","\x11\x06\x64\0",
	{"\xd8\xb4","\xd9\xa5",},//"\x11\x06\x34\0","\x11\x06\x65\0"
	{"\xd8\xb5","\xd9\xa6",},//"\x11\x06\x35\0","\x11\x06\x66\0",
	{"\xd8\xb6","\xd9\xa7",},//"\x11\x06\x36\0","\x11\x06\x67\0",
	{"\xd8\xb7","\xd9\xa8",},//"\x11\x06\x37\0","\x11\x06\x68\0",
	{"\xd8\xb8","\xd9\xb5",},//"\x11\x06\x38\0","\x11\x06\x75\0",
	{"\xd8\xb9","\xd9\xb6",},//"\x11\x06\x39\0","\x11\x06\x76\0",
	{"\xd8\xba","\xd9\xb7",}, //"\x11\x06\x3A\0","\x11\x06\x77\0",
	{"\xd9\x81","\xd9\xb8",},//"\x11\x06\x41\0","\x11\x06\x78\0",
	{"\xd9\x82","\xd9\xb9",},//"\x11\x06\x42\0","\x11\x06\x79\0",
	{"\xd9\x83","\xd9\xba",},//"\x11\x06\x43\0","\x11\x06\x7a\0",
	{"\xd9\x84","\xd9\xbb",},//"\x11\x06\x44\0","\x11\x06\x7b\0",
	{"\xd9\x85","\xd9\xbc",},//"\x11\x06\x45\0","\x11\x06\x7c\0",
	{"\xd9\x86","\xd9\xbd",}, //"\x11\x06\x46\0","\x11\x06\x7d\0",
	{"\xd9\x87","\xd9\xbe",}, //"\x11\x06\x47\0","\x11\x06\x7e\0",
	{"\xd9\x88","\xd9\xbf",},//"\x11\x06\x48\0","\x11\x06\x7f\0",
	{"Arabic","Arabic",},
	{"1/2\0","2/2\0",},  //{"A/a\0","\x11\x06\x49\0",},
	{"123\0","123\0",},
	{"\0","\0",},//{"\xd9\x88","\x11\x06\x48\0",},
	{"\x20","\x20", },
	{"\0","\0",},//{"\xd9\x8a","\x11\x06\x4a\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
	{"Clr(F1)","Clr(F1)",}
};

static  char* s_vietnamese_keymap[KEY_COUNT][2] = {
	{"\x61","\x41",},
	{"\xc4\x83","\xc4\x82",},
	{"\xc3\xa2","\xc3\x82",},
	{"\x62","\x42",},
	{"\x63","\x43",},
	{"\x64","\x44",},
	{"\xc4\x91","\xc4\x90",},
	{"\x65","\x45",},
	{"\xc3\xaa","\xc3\x8a",},
	{"\x67","\x47",},//"g,G"
	{"\x68","\x48",},
	{"\x69","\x49",},
	{"\x6b","\x4b",},
	{"\x6c","\x4c",},
	{"\x6d","\x4d",},
	{"\x6e","\x4e",},
	{"\x6f","\x4f",},//"o,O"
	{"\xc3\xb4","\xc3\x94",},
	{"\xc6\xa1","\xc6\xa0",},
	{"\x70","\x50",},
	{"\x71","\x51",},
	{"\x72","\x52",},
	{"\x73","\x53",},
	{"\x74","\x54",},
	{"\x75","\x55",},
	{"\xc6\xb0","\xc6\xaf",},
	{"\x76","\x56",},
	{"\x78","\x58",},
	{"\x79","\x59",},
	{"(","(",},
	{")",")",},
	{".",".",},
	{"#","#",},
	{"$","$",},
	{"Vietnamese","Vietnamese",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
	{"Clr(F1)","Clr(F1)",}
};

static  char* s_russian_keymap[KEY_COUNT][2] = {
	{"\xd0\xB0","\xd0\x90",},
	{"\xd0\xB1","\xd0\x91",},
	{"\xd0\xB2","\xd0\x92",},
	{"\xd0\xB3","\xd0\x93",},
	{"\xd0\xb4","\xd0\x94",},
	{"\xd0\xb5","\xd0\x95",},
	{"\xd0\xb6","\xd0\x96",},
	{"\xd0\xb7","\xd0\x97",},
	{"\xd0\xb8","\xd0\x98",},
	{"\xd0\xb9","\xd0\x99",},
	{"\xd0\xba","\xd0\x9a",},
	{"\xd0\xbb","\xd0\x9b",},
	{"\xd0\xbc","\xd0\x9c",},
	{"\xd0\xbd","\xd0\x9d",},
	{"\xd0\xbe","\xd0\x9e",},
	{"\xd0\xbf","\xd0\x9f",},
	{"\xd1\x80","\xd0\xa0",},
	{"\xd1\x81","\xd0\xa1",},
	{"\xd1\x82","\xd0\xa2",},
	{"\xd1\x83","\xd0\xa3",},
	{"\xd1\x84","\xd0\xa4",},
	{"\xd1\x85","\xd0\xa5",},
	{"\xd1\x86","\xd0\xa6",},
	{"\xd1\x87","\xd0\xa7",},
	{"\xd1\x88","\xd0\xa8",},
	{"\xd1\x89","\xd0\xa9",},
	{"\xd1\x8a","\xd0\xaa",},
	{"\xd1\x8b","\xd0\xab",},
	{"\xd1\x8c","\xd0\xac",},
	{"\xd1\x8d","\xd0\xad",},
	{"\xd1\x8e","\xd0\xae",},
	{"\xd1\x8f","\xd0\xaf",},
	{"\xd1\x91","\xd0\x81",},
	{"\xd1\x94","\xd0\x84",},
	{"Russian","Russian",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
	{"Clr(F1)","Clr(F1)",}
};

static  char* s_turkish_keymap[KEY_COUNT][2] = {
	{"\xc3\xa0","\xc3\x80",},
	{"\xc3\xa1","\xc3\x81",},
	{"\xc3\xa2","\xc3\x82",},
	{"\xc3\xa3","\xc3\x83",},
	{"\xc3\xa4","\xc3\x84",},
	{"\xc3\xa5","\xc3\x85",},
	{"\xc3\xa6","\xc3\x86",},
	{"\xc3\xa7","\xc3\x87",},
	{"\xc3\xa8","\xc3\x88",},
	{"\xc3\xa9","\xc3\x89",},
	{"\xc3\xaa","\xc3\x8a",},
	{"\xc3\xab","\xc3\x8b",},
	{"\xc3\xac","\xc3\x8c",},
	{"\xc3\xad","\xc3\x8d",},
	{"\xc3\xae","\xc3\x8e",},
	{"\xc3\xaf","\xc3\x8f",},
	{"\xc4\x9f","\xc4\x9e",},
	{"\xc3\xb1","\xc3\x91",},
	{"\xc3\xb2","\xc3\x92",},
	{"\xc3\xb3","\xc3\x93",},
	{"\xc3\xb4","\xc3\x94",},
	{"\xc3\xb5","\xc3\x95",},
	{"\xc3\xb6","\xc3\x96",},
	{"\xc3\xb7","\xc3\x97",},
	{"\xc3\xb8","\xc3\x98",},
	{"\xc3\xb9","\xc3\x99",},
	{"\xc3\xba","\xc3\x9a",},
	{"\xc3\xbb","\xc3\x9b",},
	{"\xc3\xbc","\xc3\x9c",},
	{"\xc4\xb1","\xc4\xb0",},
	{"\xc5\x9f","\xc5\x9e",},
	{"\xc3\xbe","\xc3\x9f",},
	{"\xc5\xa1","\xc5\xa0",},
	{"\xc5\x93","\xc5\x92",},
	//{"\xc2\xa1","\xc5\xb8",},
	{"Turkish","Turkish",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
	{"Clr(F1)","Clr(F1)"}
};

char *g_LangName___[] = { 
	"English",
	"Vietnamese",
	"Arabic",
	"Turkish",
	"Russian",
	"Portuguese",
	"Spain",
	"French",
};

static uint8_t s_lan_keyboard_caps = 0;
static int8_t s_lan_keyboard_x = 3;
static int8_t s_lan_keyboard_y = 3;
static int s_select_language_status = 0;
static uint8_t thiz_pos_num = 0;
static PopKeyboard s_keyboard_full;

extern int app_lang_get_menu_lang_status(void);

/*
U-00000000 - U-0000007F 	: 0xxxxxxx  
U-00000080 - U-000007FF 	: 110xxxxx 10xxxxxx  
U-00000800 - U-0000FFFF	: 1110xxxx 10xxxxxx 10xxxxxx  
U-00010000 - U-001FFFFF  	: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
U-00200000 - U-03FFFFFF	: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
U-04000000 - U-7FFFFFFF	: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
*/
static signed char calculte_utf8_length(const char* InStr, unsigned char StrLen)
{
	const char *p = InStr;
	if((p == NULL) ||(StrLen == 0) || (strlen(InStr) < StrLen))
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if((p[0] >> 7) == 0)
	{
		return 1; // one byte
	}
	else if((p[0] >>5) == 0x06)
	{
		if(StrLen < 2)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if((p[1] >>6) == 0x02)
				return 2;// two bytes
			else
			{
				printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else if((p[0] >>4) == 0x0e)
	{
		if(StrLen < 3)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if(((p[1] >>6) == 0x02) && ((p[2] >>6) == 0x02))
				return 3;// three bytes
			else
			{
				printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else if((p[0] >>3) == 0x1e)
	{
		if(StrLen < 4)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if(((p[1] >>6) == 0x02) && ((p[2] >>6) == 0x02) && ((p[3] >>6) == 0x02))
				return 4;// four bytes
			else
			{
				printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else if((p[0] >>2) == 0x3e)
	{
		if(StrLen < 5)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if(((p[1] >>6) == 0x02) && ((p[2] >>6) == 0x02) && ((p[3] >>6) == 0x02)  && ((p[4] >>6) == 0x02))
				return 5;// five bytes
			else
			{
				printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else if((p[0] >>1) == 0x7e)
	{
		if(StrLen < 6)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if(((p[1] >>6) == 0x02) && ((p[2] >>6) == 0x02) && ((p[3] >>6) == 0x02) && ((p[4] >>6) == 0x02))
				return 6;// five bytes
			else
			{
				printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	
}

static char get_utf8_position(const char* InStr, unsigned char CurSelIndex)
{
	int TempLen = 0 ;
	int TempLen2 = 0;
	signed char SkipLen = 0; 
	char FindFlag = 0;
	char Count = 0;

	if((InStr == NULL) || (strlen(InStr)  < CurSelIndex))
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	TempLen = strlen(InStr) ;
	if((TempLen == 0) &&(CurSelIndex == 0))
	{
		return 0;
	}
  // find the insert position
       while(TempLen2 < TempLen)
       	{
		SkipLen = calculte_utf8_length(InStr + TempLen2 , TempLen - TempLen2);
		if(SkipLen < 0)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		if(Count == CurSelIndex)
		{
			FindFlag = 1;
			break;
		}
		TempLen2 += SkipLen;
		Count++;
	}
	if(FindFlag)
		return TempLen2;
	else
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	
}

static signed char init_utf8_position(const char* InStr,unsigned int *LastCharSel, unsigned int *MaxCharCount)
{
	int TempLen = strlen(InStr) ;
	int TempLen2 = 0;
	signed char SkipLen = 0; 
//	char FindFlag = 0;
	char Count = 0;
  // find the insert position
       while(TempLen2 < TempLen)
       	{
		SkipLen = calculte_utf8_length(InStr + TempLen2 , TempLen - TempLen2);
		if(SkipLen < 0)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		TempLen2 += SkipLen;
		Count++;
	}
	if(Count == 0)
		*LastCharSel = 0;
	else
		*LastCharSel = Count;
	*MaxCharCount = Count;
	return (TempLen2 - SkipLen);
	
}


static char strInsert(char* strRetDate,const char* strUseDate,unsigned char strInser)
{ 
#define TEMP_BUFFER_LEN 256
	char InsertLen = 0;
	signed char SkipLen = 0;
	int TempLen = strlen(strUseDate) ;
	int TempLen2 = 0;
	char Bufstring[STR_BUF_LEN] = {0};

	if((strRetDate == NULL) || (strUseDate == NULL) || (TempLen == 0) )
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	InsertLen = calculte_utf8_length(strUseDate, TempLen);
	if((InsertLen < 0) || (InsertLen != TempLen))
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	TempLen = strlen(strRetDate);
	if((TempLen == 0) && (strlen(strUseDate) < TEMP_BUFFER_LEN))
	{
		goto TAG1;
	}
	if(strInser == (TEMP_BUFFER_LEN-1))
	{//when strInser = 255 It's mean Insert word to head.
	 //为了解决光标在第一位时插入字符却在第一位后面的问题。
		goto TAG1;
	}
	if((strInser >= TempLen) || ((TempLen + strlen(strUseDate)) > TEMP_BUFFER_LEN))
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	
	memset(Bufstring, 0, sizeof(Bufstring));
	TempLen2 = get_utf8_position(strRetDate,strInser);
	if(TempLen2 < 0)
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	
	SkipLen = calculte_utf8_length(strRetDate + TempLen2, TempLen - TempLen2);
	if((SkipLen < 0) || (SkipLen > (TempLen - TempLen2)))
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if((TempLen > 0) && (TempLen2 == (TempLen - SkipLen)))
	{
		memcpy(Bufstring,strRetDate,TempLen);
		strcat(Bufstring,strUseDate);
		memcpy(strRetDate, Bufstring, STR_BUF_LEN);
	}
	else if((TempLen2 + SkipLen) == 0)
	{
TAG1:	memcpy(Bufstring,strUseDate,InsertLen);
		strcat(Bufstring,strRetDate);
		memcpy(strRetDate, Bufstring, STR_BUF_LEN);
	}
	else
	{
	      	if((TempLen2 + SkipLen) > 0)
	  	{
		       memcpy(Bufstring,strRetDate,TempLen2+SkipLen);
			strcat(Bufstring,strUseDate);
			memcpy(Bufstring + strlen(Bufstring),strRetDate+TempLen2 + SkipLen,TempLen - (TempLen2 + SkipLen));
			memcpy(strRetDate, Bufstring, STR_BUF_LEN);
	  	}
		else
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}	
	}
	return InsertLen;
}

//
static unsigned int strDel(char* str,char SelToDel)
{
	signed int SkipLen = 0;
	int Temp = 0;
	int TempLen = 0;
	char Bufstring[STR_BUF_LEN] = {0};
	
	if((str == NULL) || ((TempLen = strlen(str)) < SelToDel))
        	return -1;
	Temp = get_utf8_position((const char*)str, SelToDel);
	if(Temp < 0)
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	
	SkipLen = calculte_utf8_length(str + Temp , TempLen - Temp);
	if(SkipLen < 0)
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	
	if(TempLen > (Temp + SkipLen))
	{
		memset(Bufstring, 0, sizeof(Bufstring));
		memcpy((void*)(Bufstring), 
			(void*)(str),  Temp);
	 	memcpy((void*)(Bufstring + Temp), 
	 		(void*)(str+Temp + SkipLen), TempLen - (Temp + SkipLen));
		memcpy(str, Bufstring, STR_BUF_LEN);
	}
	else if(TempLen ==  (Temp + SkipLen))
	{
		str[Temp] = '\0';
	}
	else
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

    	return SkipLen;
}

static int insert_in_different_lang(AppInput *pInput, const char *pLangMap, const char *pEnglishMap)
{
	int Temp = 0;
	int StringLen = 0;
	unsigned int CurPos = 0;

	if((NULL == pInput) || (NULL == pLangMap) || (NULL == pEnglishMap))
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if((pInput->cur_sel) == 0 && (pInput->cur_max > 0))
	{
		CurPos = 255;
	}
	else
	{
		CurPos = (pInput->cur_sel > 0)?(pInput->cur_sel - 1):0;
	}
	
	StringLen = strlen(s_lang_keyboard_input);
	if((2 == s_lan_keyboard_caps))
	{
		Temp = strlen(pEnglishMap);
	}
	else
	{
		Temp = strlen(pLangMap);
	}
	if((StringLen + Temp) <= pInput->max_len)
	{
		 pInput->cur_max++;
	}
	else
	{
		printf("\nKEYBOARD, reach end\n");
		return  -1;
	}
	Temp = 0;
	if(2 == s_lan_keyboard_caps)
	{
		
		Temp = strInsert(s_lang_keyboard_input,pEnglishMap,CurPos);
		if(Temp < 0)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	else
	{
		
		Temp = strInsert(s_lang_keyboard_input,pLangMap,CurPos);
		if(Temp < 0)
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	//if(StringLen > 0)
		pInput->cur_sel ++;
	return 0;
}

static int app_lang_change_language_in_keyboard(int select_mode)
{
	int i=0;

	if(ITEM_ENGLISH == select_mode)
	{
		if(s_lan_keyboard_caps >= 3)
		{
			s_lan_keyboard_caps = 0;
		}
	}
	else
	{
		if(s_lan_keyboard_caps >= 2)
		{
			s_lan_keyboard_caps = 0;
		}
	}
	
	for(i = 0; i < KEY_COUNT; i++)
	{
		if((ITEM_ARABIC == select_mode)||(ITEM_FARSI == select_mode))
		{	
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_arabic_keymap[i][s_lan_keyboard_caps]);				
		}
		else if(ITEM_TURKISH == select_mode)
		{
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_turkish_keymap[i][s_lan_keyboard_caps]);
		}
		else if(ITEM_VIETNAMESE == select_mode)
		{
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_vietnamese_keymap[i][s_lan_keyboard_caps]);
		}
		else if(ITEM_RUSSIAN == select_mode)
		{
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_russian_keymap[i][s_lan_keyboard_caps]);
		}
		else
		{
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_english_keymap[i][s_lan_keyboard_caps]);
		}

		if( i == KEY_LAN_SPACE_NUM)
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", "Space");
	}

	// 20140426
	if((ITEM_ARABIC == select_mode)||(ITEM_FARSI == select_mode))
	{// set the edit cursor, set to the last char
		init_utf8_position((const char*)s_lang_keyboard_input,&(g_AppInput.cur_sel),&(g_AppInput.cur_max));	
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(g_AppInput.cur_sel));
	}
	return 0;
}

static int app_language_get_move_key_row_num(int key)
{
	int select_key_map_num = 0;
	int select_key_x_num = 0;
	int fouces_button =0;

	switch(key)
	{
		case STBK_UP:
			if(s_lan_keyboard_y == KEY_LAN_Y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y-1;
				if((s_lan_keyboard_x>= KEYBOARD_BUTTON104)&&(s_lan_keyboard_x<= KEYBOARD_BUTTON107))
				{
					s_lan_keyboard_x = 3;
				}
				else if((s_lan_keyboard_x>= 7)&&(s_lan_keyboard_x<= 11))
				{
					select_key_x_num = s_lan_keyboard_x -3 ;
					if(select_key_x_num >= 7)
					{
						select_key_x_num = 7;
					}
					s_lan_keyboard_x = select_key_x_num;
				}
			}
			else if(2 == s_lan_keyboard_y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
				if((s_lan_keyboard_x>= KEYBOARD_BUTTON104)&&(s_lan_keyboard_x<= KEYBOARD_BUTTON108))
				{
					select_key_x_num = 3+s_lan_keyboard_x;
					s_lan_keyboard_x = select_key_x_num;
				}
			}
			else if(1 == s_lan_keyboard_y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
				if(s_lan_keyboard_x== KEYBOARD_BUTTON111)
				{
					s_lan_keyboard_x = s_lan_keyboard_x+1;
				}
			}
			else
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
			}
			break;
		case STBK_DOWN:
			if(s_lan_keyboard_y == KEY_LAN_Y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y-1;
				if((s_lan_keyboard_x>= KEYBOARD_BUTTON104)&&(s_lan_keyboard_x<= KEYBOARD_BUTTON107))
				{
					s_lan_keyboard_x = 3;
				}
				else if((s_lan_keyboard_x>= 7)&&(s_lan_keyboard_x<= 11))
				{
					select_key_x_num = s_lan_keyboard_x -3 ;
					if(select_key_x_num >= 7)
					{
						select_key_x_num = 7;
					}
					s_lan_keyboard_x = select_key_x_num;
				}
			}
			else if(2 == s_lan_keyboard_y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
				if(s_lan_keyboard_x== KEYBOARD_BUTTON112)
				{
					s_lan_keyboard_x = KEYBOARD_BUTTON111;
				}
			}
			else if(0 == s_lan_keyboard_y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
				if((s_lan_keyboard_x>= 4)&&(s_lan_keyboard_x<= 7))
				{
					select_key_x_num = s_lan_keyboard_x+3;
					s_lan_keyboard_x = select_key_x_num;
				}
			}
			else
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
			}
			break;
		case STBK_LEFT:
		case STBK_RIGHT:
		case STBK_OK:
			if(s_lan_keyboard_y == KEY_LAN_Y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y - 1;
			}
			else
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
			}
			break;
		default:
			break;
	}

	fouces_button = select_key_map_num+s_lan_keyboard_x;
//printf("______________________key_x_num=%d.---%d\n",s_lan_keyboard_x,fouces_button);	
	if(fouces_button > KEY_COUNT)
	{
		printf("____key_x_num=%d.---%d\n",select_key_x_num,fouces_button);	
		fouces_button = KEY_COUNT -1;
	}
	return fouces_button;
}

static char* app_language_change_fouce_pic(int key)
{
	int row_num = 0;
	static int recall_row_num = KEY_LAN_SPACE_NUM;
	char *img_name = WND_LANG_FOCUS_BMP56X38;
#ifdef MINI_KEYBOARD_STATUS
	if(KEY_LAN_DEL_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_BACKSPACE;
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else if(KEY_LAN_RIGHT_CURSOR == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_RIGHT_ARROW;
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else if(KEY_LAN_LEFT_CURSOR == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_LEFT_ARROW;
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else
	{
		 if(KEY_LAN_CAPS_NUM == recall_row_num)
		{
			img_name = "[key_red,key_red,key_red]";
		}
		else if(KEY_LAN_NUMBER_NUM == recall_row_num)
		{
			img_name = "[key_green,key_green,key_green]";
		}
		else if(KEY_LAN_SAVE_OK_NUM == recall_row_num)
		{
			img_name = "[key_yellow,key_yellow,key_yellow]";
		}
		else if(KEY_LAN_DEL_NUM == recall_row_num)
		{
			img_name = "[key_blue,key_blue,key_blue]";
		}
		else
		{
			img_name = "[#f4f4f4,#f4f4f4,#f4f4f4]";
		}
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"backcolor", img_name);
	}
	
	GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM], "forecolor", "[#000000,#000000,#000000]");
#else
	if(KEY_LAN_DEL_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_BACKSPACE;
	}
	else if(KEY_LAN_RIGHT_CURSOR == recall_row_num)
	{
		//if(s_select_language_status == ITEM_ARABIC )
			//img_name = WND_LANG_UNFOCUS_BMP56X38;
		//else
			img_name = WND_LANG_UNFOCUS_RIGHT_ARROW;
	}
	else if(KEY_LAN_SPACE_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_BMP254X38;
	}
	else if(KEY_LAN_LEFT_CURSOR == recall_row_num)
	{
		//if(s_select_language_status == ITEM_ARABIC )
			//img_name = WND_LANG_UNFOCUS_BMP56X38;
		//else
			img_name = WND_LANG_UNFOCUS_LEFT_ARROW;
	}
	else if(KEY_LAN_CAPS_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_RED;
	}
	else if(KEY_LAN_NUMBER_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_GREEN;
	}
	else if(KEY_LAN_SAVE_OK_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_YELLOW;
	}
	else if(KEY_LAM_CLR_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_CLEAR122;
	}
	else if(KEY_CHANGE_LANGUAGE == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_CLEAR122;
	}
	else
	{
		img_name = WND_LANG_UNFOCUS_BMP56X38;
	}
	
	GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"unfocus_img", img_name);
	GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM], "forecolor", "[#000000,#000000,#000000]");
#endif
	row_num = app_language_get_move_key_row_num(key);
	recall_row_num = row_num;

 #ifdef MINI_KEYBOARD_STATUS	
	if(KEY_LAN_DEL_NUM == row_num)
	{
		img_name = WND_LANG_FOCUS_BACKSPACE;
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else if(KEY_LAN_RIGHT_CURSOR == row_num)
	{
		img_name = WND_LANG_FOCUS_RIGHT_ARROW;
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else if(KEY_LAN_LEFT_CURSOR == row_num)
	{
		img_name = WND_LANG_FOCUS_LEFT_ARROW;
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else
	{
		img_name = "[key_focus_color,key_focus_color,key_focus_color]";
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"backcolor", img_name);
	}
	
	GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM], "forecolor", "[#f4f4f4,#f4f4f4,#f4f4f4]");
 #else
	if(KEY_LAN_DEL_NUM == row_num)
	{
		img_name = WND_LANG_FOCUS_BACKSPACE;
	}
	else if(KEY_LAN_RIGHT_CURSOR == row_num)
	{
		//if(s_select_language_status == ITEM_ARABIC )
			//img_name = WND_LANG_FOCUS_BMP56X38;
		//else
			img_name = WND_LANG_FOCUS_RIGHT_ARROW;
	}
	else if(KEY_LAN_SPACE_NUM == row_num)
	{
		img_name = WND_LANG_FOCUS_BMP254X38;
	}
	else if((KEY_LAN_LEFT_CURSOR == row_num) /*&&(s_select_language_status != ITEM_ARABIC )*/)
	{
		img_name = WND_LANG_FOCUS_LEFT_ARROW;
	}
	else if(KEY_LAM_CLR_NUM == row_num)
	{
		img_name = WND_LANG_FOCUS_CLEAR122;
	}
	else if(KEY_CHANGE_LANGUAGE == row_num)
	{
		img_name = WND_LANG_FOCUS_CLEAR122;//WND_LANG_FOCUS_BLUE_P122;
	}
	else
	{
		img_name = WND_LANG_FOCUS_BMP56X38;
	}
	
	GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"unfocus_img", img_name);
	GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM], "forecolor", "[#f4f4f4,#f4f4f4,#f4f4f4]");
#endif
	return img_name;
}

static void  app_lang_keyboard_green_key_func(int sel_num)
{
	int i = 0;
	static uint8_t old_keyboard_caps = 0;
	if(sel_num >= BUTTON_NUM-1)
	{
		sel_num = 0;
	}
/*
	if(s_lan_keyboard_caps == BUTTON_NUM-1)
	{
		s_lan_keyboard_caps = (s_lan_keyboard_caps)%2;
	}
	else
	{
		s_lan_keyboard_caps = BUTTON_NUM-1;
	}
*/
	if(s_lan_keyboard_caps != 2)
	{
		old_keyboard_caps = s_lan_keyboard_caps;
		s_lan_keyboard_caps = 2;
		for(i = 0; i < KEY_COUNT; i++)
		{
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_english_keymap[i][2]);
			if( i == KEY_LAN_SPACE_NUM)
			{
				GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", "Space");
			}
			if( i == KEY_CHANGE_LANGUAGE)
			{
				GUI_SetProperty(s_english_keymap[KEY_CHANGE_LANGUAGE][BUTTON_NUM], "string", g_LangName___[s_select_language_status]);
			}

		}
		//s_select_language_status = 0;
	}
	else
	{
		s_lan_keyboard_caps = old_keyboard_caps;
		app_lang_change_language_in_keyboard(s_select_language_status);
	}
	if(s_select_language_status == ITEM_ARABIC)
	{
		GUI_SetProperty(s_english_keymap[KEY_LAN_CAPS_NUM][BUTTON_NUM], "string", s_arabic_keymap[KEY_LAN_CAPS_NUM][old_keyboard_caps]);
	}
	else
	{
		GUI_SetProperty(s_english_keymap[KEY_LAN_CAPS_NUM][BUTTON_NUM], "string", s_english_keymap[KEY_LAN_CAPS_NUM][old_keyboard_caps]);
	}
	
	return;
}
static void app_lang_keyboard_yellow_key_func(void)
{
	uint8_t len = strlen(s_lang_keyboard_input);
	bool blank = true;
	bool invalid_str = false;
	uint8_t tag = 0;
	uint8_t i;

	for(i = 0; i < len ; i++)
	{
		if(s_lang_keyboard_input[i] != ' ')
		{
			blank = false;
			invalid_str = false;
			break;
		}
	}

	for(i = 0; i < len ; i++)
	{	
		char *p = strchr(s_invalid_char, s_lang_keyboard_input[i]);
		if(p != NULL)
		{
			tag++;	
		}
		
		p = strchr(FILE_CHAR_NAME_INVALID, s_lang_keyboard_input[i]);
		if(p != NULL)
		{
			invalid_str = true;
		}
	}

	if(s_keyboard_full.usr_data != NULL)
	{
		//if(strchr(s_keyboard_full.usr_data,',') != NULL &&(tag != 0))
		if(((strcmp(s_keyboard_full.usr_data, KEY_LAN_INVALID_CHAR_NAME)==0)&&(tag != 0))
			||((strcmp(s_keyboard_full.usr_data, FILE_CHAR_NAME_INVALID)==0)&&invalid_str)
			)
		{
			blank = true;
		}
	}

	if (blank == true)
	{
		PopDlg pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		//pop.mode = POP_MODE_UNBLOCK;
		if(NULL != s_keyboard_full.usr_data)
		{
			if((strcmp(s_keyboard_full.usr_data, KEY_LAN_INVALID_CHAR_NAME)==0)&&(len!=0))
				pop.str = STR_ID_ERR_FAVNAME;
			else if((strstr(s_keyboard_full.usr_data, ",")!=NULL)&&(len!=0))
				pop.str = STR_ID_ERR_PVRNAME; //"Please enter other char";
			else if(strstr(s_keyboard_full.usr_data, FILE_CHAR_NAME_INVALID)!=NULL)
				pop.str = STR_ID_ERR_FILENAME; 
			else
				pop.str = STR_ID_ENTER_NAME;
			
			if(s_lang_keyboard_input[0]==0x20)
			{
				pop.str = STR_ID_ERR_INITIAL_SPACE;
			}
		}
		else
		{
			pop.str = STR_ID_ENTER_NAME;
		}
		popdlg_create(&pop);
		return;
	}

	GUI_EndDialog(WND_LANG_KEYBOARD);
	GUI_SetInterface("flush",NULL);	
	if (s_keyboard_full.release_cb != NULL)
	{
		s_keyboard_full.release_cb(&s_keyboard_full);
	}
	return;
}
static void app_lang_keyboard_blue_key_func(void)
{
	AppInput *input = &g_AppInput;
	unsigned int Temp = 0;
	unsigned int CurPos = 0;
	uint8_t str_len = 0;
	
	str_len = strlen(s_lang_keyboard_input);
	if((str_len == 0)   || (input->cur_sel > str_len))
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
	return;
	}
	CurPos = (input->cur_sel > 0)?(input->cur_sel -1):0;
	Temp = strDel(s_lang_keyboard_input,CurPos);
	if((Temp < 0) || ( str_len  < Temp))
	{
	printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
	return ;
	}
	str_len = str_len - Temp;
	if(input->cur_sel > 0)
	{
	input->cur_sel--;
	}
	if(input->cur_max > 0)
	{
	input->cur_max--;
	}

	if((strlen(s_lang_keyboard_input) == 0)||(str_len == 0))
	{
	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
	memset(s_lang_keyboard_input, 0, STR_BUF_LEN);
	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
	}
	else
	{
		int arabic_sel = 0;
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
		if( ITEM_ARABIC == s_select_language_status)
		{
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &arabic_sel);
		}
		else
		{
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
		}
	}
}

static void app_language_keyboard_move(uint16_t key)
{
	switch(key)
	{
		case STBK_UP:	
			if(s_lan_keyboard_y == 0)
			{
				s_lan_keyboard_y = KEY_LAN_Y;
			}
			else
			{
				s_lan_keyboard_y--;
			}
			break;
		case STBK_DOWN:			
			if(s_lan_keyboard_y == KEY_LAN_Y)
			{
				s_lan_keyboard_y = 0;
			}
			else
			{
				s_lan_keyboard_y++;
			}
			break;
		case STBK_LEFT:				
			if(s_lan_keyboard_x == 0)
			{
				s_lan_keyboard_x = KEY_LAN_X;
				if(s_lan_keyboard_y == 2)
				{
					s_lan_keyboard_x = 10;
				}
				else if(s_lan_keyboard_y == 3)
				{
					s_lan_keyboard_x = 7;
				}
				else
				{
					s_lan_keyboard_x = KEY_LAN_X;
				}
			}
			else
			{
				s_lan_keyboard_x--;
			}
			break;
		case STBK_RIGHT:
			if((s_lan_keyboard_x == KEY_LAN_X)||((s_lan_keyboard_x == 10)&&(s_lan_keyboard_y == 2))
				||((s_lan_keyboard_x == 7)&&(s_lan_keyboard_y == 3)))
			{
				s_lan_keyboard_x = 0;
			}
			else
			{
				s_lan_keyboard_x++;
			}
			break;
		default:
			break;
	}

	app_language_change_fouce_pic(key);
	//GUI_SetFocusWidget(s_english_keymap[row_num][BUTTON_NUM]);
	return;
}

static int app_language_keyboard_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_LANG;
	pop_list.item_num = 5;//LANG_MAX;//
	pop_list.item_content = g_LangName___;
	pop_list.sel = sel;
	pop_list.show_num= false;

	ret = poplist_create(&pop_list);
	GUI_SetProperty(s_english_keymap[KEY_CHANGE_LANGUAGE][BUTTON_NUM], "string", g_LangName___[ret]);

	//s_lan_keyboard_x = 0;
	//s_lan_keyboard_y = 0;
	
	app_language_change_fouce_pic(STBK_RIGHT);
	if( ret != s_select_language_status)
	{
		s_lan_keyboard_caps = 0;
		thiz_pos_num = 0;
		app_lang_change_language_in_keyboard(ret);
	}
	s_select_language_status = ret;
	return ret;
}

static void app_language_keyboard_ok(int key)
{
        AppInput *input = &g_AppInput;
	//uint8_t key_num_recall_status = 0; 
	uint8_t key_num = 0;//s_lan_keyboard_x+KEY_ROW_COUNT*s_lan_keyboard_y;
	uint8_t max_len = 0;
//	uint8_t str_len = 0;
//	uint8_t Temp = 0;
	
	key_num =  app_language_get_move_key_row_num(key);
	if(key_num == KEY_LAM_CLR_NUM)
	{
		input->cur_sel = 0;
		input->cur_max = 0;
		memset(s_lang_keyboard_input, 0, sizeof(s_lang_keyboard_input));
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
	}
	else if(key_num == KEY_LAN_DEL_NUM) 
		/*||(((ITEM_ARABIC == s_select_language_status)||(ITEM_FARSI == s_select_language_status)) &&(key_num == KEY_LAN_CAPS_NUM))*/
	{
		#if 1
		app_lang_keyboard_blue_key_func();
		#else
		 str_len = strlen(s_lang_keyboard_input);
		 if((str_len == 0)   || (input->cur_sel > str_len))
		 {
		 	printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return;
		 }
		 CurPos = (input->cur_sel > 0)?(input->cur_sel -1):0;
		Temp = strDel(s_lang_keyboard_input,CurPos);
		if((Temp < 0) || ( str_len  < Temp))
		{
			printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return ;
		}
		str_len = str_len - Temp;
		if(input->cur_sel > 0)
		{
			input->cur_sel--;
		}
		if(input->cur_max > 0)
		{
			input->cur_max--;
		}

		if((strlen(s_lang_keyboard_input) == 0)||(str_len == 0))
		{
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
			memset(s_lang_keyboard_input, 0, STR_BUF_LEN);
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
		}
		else
		{
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
	 		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
		}
		#endif
	}
	else if(key_num == KEY_LAN_CAPS_NUM)
	{
		s_lan_keyboard_caps = (s_lan_keyboard_caps+1)%2;
		thiz_pos_num = s_lan_keyboard_caps;
		app_lang_change_language_in_keyboard(s_select_language_status);	
	}
	else if(key_num == KEY_LAN_NUMBER_NUM)
	{
		#if 1
		app_lang_keyboard_green_key_func(thiz_pos_num);
		#else
		uint8_t i = 0;
		key_num_recall_status = s_lan_keyboard_caps;
		if(key_num_recall_status == BUTTON_NUM-1)
		{
			s_lan_keyboard_caps = (s_lan_keyboard_caps+1)%2;
		}
		else
		{
			s_lan_keyboard_caps = BUTTON_NUM-1;
		}
		
		if(2 == s_lan_keyboard_caps)
		{
			for(i = 0; i < KEY_COUNT; i++)
			{
				if(i != KEY_LAN_SPACE_NUM)
					GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_english_keymap[i][s_lan_keyboard_caps]);
				else
					GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", "Space");
			}
		}
		else
		{
			app_lang_change_language_in_keyboard(s_select_language_status);
		}
		#endif
	}
	else if(key_num == KEY_LAN_SAVE_OK_NUM)
	{
		app_lang_keyboard_yellow_key_func();
#if 0
		{
			uint8_t len = strlen(s_lang_keyboard_input);
			bool blank = true;
			uint8_t index;

			for(index = 0; index < len ; index++)
			{
				if(s_lang_keyboard_input[index] != ' ')
				{
					blank = false;
					break;
				}
			}

			for(index = 0; index < len ; index++)
			{
				if(((s_lang_keyboard_input[index] == 0x2c)||(s_lang_keyboard_input[index] == 0x5d)||(s_lang_keyboard_input[index] == 0x5b))
					&&(s_keyboard_full.usr_data != NULL && strcmp(s_keyboard_full.usr_data, ",")==0))
				{
					blank = true;
					break;
				}
				else
				{
					blank = false;
				}
			}

			if (blank == true)
			{
				PopDlg pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_OK;
				pop.mode = POP_MODE_UNBLOCK;
				if(NULL != s_keyboard_full.usr_data)
				{
					if(strcmp(s_keyboard_full.usr_data, ",")==0)
						pop.str = STR_ID_ERR_FAVNAME;//"Please enter other char";
					else
						pop.str = STR_ID_ENTER_NAME;
				}
				else
				{
					pop.str = STR_ID_ENTER_NAME;
				}
				popdlg_create(&pop);
				return;
			}
		}
		 GUI_EndDialog(WND_LANG_KEYBOARD);
               GUI_SetInterface("flush",NULL);	
                if (s_keyboard_full.release_cb != NULL)
                {
                	s_keyboard_full.release_cb(&s_keyboard_full);
                }
#endif
	}
	else if(KEY_LAN_LEFT_CURSOR == key_num)
	{
		if((s_select_language_status == ITEM_ARABIC) ||(s_select_language_status == ITEM_FARSI)
               || (0 == strlen(s_lang_keyboard_input)))
		{
			return;
		}
		if (input->cur_sel != 0)
		{
			input->cur_sel--;
			input->first_click = 0;
		}
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
	}
	else if(KEY_LAN_RIGHT_CURSOR == key_num)
	{
		if((s_select_language_status == ITEM_ARABIC) ||(s_select_language_status == ITEM_FARSI)
               || (0 == strlen(s_lang_keyboard_input)))
		{
			return;
		}
		if(input->cur_sel <= input->cur_max - 1)
		{
			input->cur_sel++;
			input->first_click = 0;
		}
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
	}
	else if(KEY_CHANGE_LANGUAGE == key_num)
	{
        s_select_language_status = app_language_keyboard_pop(s_select_language_status);
	}
	else
	{
		max_len = s_keyboard_full.max_num;
		if(max_len > STR_BUF_LEN)
		{
			max_len = STR_BUF_LEN-1;
		}
		
		if(strlen(s_lang_keyboard_input) < max_len)
		{
			if((ITEM_ARABIC == s_select_language_status)||(ITEM_FARSI == s_select_language_status))
			{
				insert_in_different_lang(input, s_arabic_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}
			else if(ITEM_VIETNAMESE == s_select_language_status)
			{
				insert_in_different_lang(input, s_vietnamese_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}
			else if(ITEM_RUSSIAN== s_select_language_status)
			{
				insert_in_different_lang(input, s_russian_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}
			else if(ITEM_TURKISH== s_select_language_status)
			{
				insert_in_different_lang(input, s_turkish_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}
			else
			{
				insert_in_different_lang(input, s_english_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}

			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
			if(ITEM_ARABIC == s_select_language_status )
			{
				int32_t arabic_sel = 0; /*arabic write from right to left*/
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &arabic_sel);
			}
			else
			{
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
			}
		}
	}

}

SIGNAL_HANDLER int app_language_keyboard_create(const char* widgetname, void *usrdata)
{
	AppInput *input = &g_AppInput;
	signed char ret_value = 0;

	s_select_language_status = 0;//app_lang_get_menu_lang_status();
	s_lan_keyboard_caps = 0;
	app_lang_change_language_in_keyboard(s_select_language_status);

	s_lan_keyboard_x = 0;
	s_lan_keyboard_y = 0;
	app_language_change_fouce_pic(STBK_RIGHT);
	GUI_SetFocusWidget(EDIT_LANG_KEYBOARD_INPUT);

	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", input->name_get());
	if((s_keyboard_full.in_name == NULL) || (strlen(s_keyboard_full.in_name) == 0))
	{
		input->cur_sel = 0;
		input->cur_max = 0;
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
	}
	else
	{
		ret_value = init_utf8_position(s_lang_keyboard_input,&(input->cur_sel),&(input->cur_max));
		if(-1 == ret_value)
		{
			input->cur_sel = 0;
			input->cur_max = 0;
			memset(s_lang_keyboard_input, 0, STR_BUF_LEN);
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
		}
	}
	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_language_keyboard_destroy(const char* widgetname, void *usrdata)
{
    thiz_pos_num = 0;
	return EVENT_TRANSFER_STOP;
}

static void inputnum_by_remoter(const char *inputnum)
{
       AppInput *input = &g_AppInput;

	unsigned int Len = 0;
	unsigned int LenOri = 0;
	char Ret = 0;
	unsigned int CurPos = 0;
	uint8_t max_len = 0;

	if((inputnum ==NULL) ||(input == NULL))
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return;
	}
	Len = strlen(inputnum);
	LenOri = strlen(s_lang_keyboard_input);
	if((Len != 1) ||(LenOri + Len) > input->max_len)
	{
		printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return;
	}

	max_len = s_keyboard_full.max_num;
	if(max_len > STR_BUF_LEN)
	{
		max_len = STR_BUF_LEN - 1;
	}
	if( strlen(s_lang_keyboard_input) < max_len )
	{
		CurPos = (input->cur_sel > 0)?(input->cur_sel - 1):0;
		Ret = strInsert(s_lang_keyboard_input,inputnum,CurPos);
		if(Ret == Len)
		{
			input->cur_sel++;
			input->cur_max++;
		}
       		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
	}
}
SIGNAL_HANDLER int app_language_keyboard_keypress(const char* widgetname, void *usrdata)
{
	AppInput *input = &g_AppInput;
	GUI_Event *event = NULL;
	//static int s_lan_keyboard_caps_old = 0;
//	uint8_t i = 0;
	char num[2] = {'\0','\0'};
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;
			case STBK_UP:
			case STBK_DOWN:
			case STBK_LEFT:
			case STBK_RIGHT:
				app_language_keyboard_move(event->key.sym);
				break;
			case STBK_OK:
				app_language_keyboard_ok(event->key.sym);
				break;
			case STBK_EXIT:
				 //memset(s_lang_keyboard_input, 0, STR_BUF_LEN);
				 GUI_EndDialog(WND_LANG_KEYBOARD);
		                GUI_SetProperty(WND_LANG_KEYBOARD, "draw_now", NULL); 
		                s_keyboard_full.in_ret = POP_VAL_CANCEL;
		                if (s_keyboard_full.release_cb != NULL)
		                {
		                	s_keyboard_full.release_cb(&s_keyboard_full);
		                }
		                s_keyboard_full.out_name = NULL;
				break;
			case STBK_F1:
				memset(s_lang_keyboard_input, 0, STR_BUF_LEN);
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);

				input->cur_sel = 0;
				input->cur_max = 0;
				if(s_keyboard_full.change_cb != NULL)
				{
					s_keyboard_full.change_cb(&s_keyboard_full);
				}
				break;
			//case STBK_SUBT:
			case STBK_BLUE:
			#if 1
				app_lang_keyboard_blue_key_func();
			#else
			{
				unsigned int Temp = 0;
				unsigned int CurPos = 0;
				str_len = strlen(s_lang_keyboard_input);
				if((str_len == 0)  || (input->cur_sel > str_len))
				{
					printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
                    break;
				}
				CurPos = (input->cur_sel > 0)?(input->cur_sel -1):0;
				Temp = strDel(s_lang_keyboard_input,CurPos);
				if((Temp < 0) || ( str_len  < Temp))
				{
					printf("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
					break;
				}
				str_len = str_len - Temp;
                if(input->cur_sel > 0)
		        {
			        input->cur_sel--;
		        }
		        if(input->cur_max > 0)
		        {
			        input->cur_max--;
		        }

				if((strlen(s_lang_keyboard_input) == 0)||(str_len == 0))
				{
					GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
					memset(s_lang_keyboard_input, 0, STR_BUF_LEN);
					GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
				}
				else
				{
					GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
					GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
				}
			}
			#endif
				break;
			case STBK_RED:
				
				//s_lan_keyboard_caps_old = (s_lan_keyboard_caps_old + 1)%2;
				//s_lan_keyboard_caps = s_lan_keyboard_caps_old;
				s_lan_keyboard_caps = (s_lan_keyboard_caps+1)%2;
				thiz_pos_num = s_lan_keyboard_caps;
				app_lang_change_language_in_keyboard(s_select_language_status);
				break;
			case STBK_GREEN:
				app_lang_keyboard_green_key_func(thiz_pos_num);//caps_num
				break;
			case STBK_YELLOW:
				app_lang_keyboard_yellow_key_func();
				break;
			//case STBK_BLUE:
				//s_select_language_status = app_language_keyboard_pop(s_select_language_status);
				//break;
			case STBK_0:    
				memset(num,'0',1);
				inputnum_by_remoter(num);
				break;
			case STBK_1:			   
				memset(num,'1',1);
				inputnum_by_remoter(num);
				break;	
			case STBK_2:			     
				memset(num,'2',1);
				inputnum_by_remoter(num);
				break;
			case STBK_3:     
				memset(num,'3',1);
				inputnum_by_remoter(num);
				break;
			case STBK_4:  
				memset(num,'4',1);
				inputnum_by_remoter(num);
				break;
			case STBK_5:    
				memset(num,'5',1);
				inputnum_by_remoter(num);
				break;
			case STBK_6:      
				memset(num,'6',1);
				inputnum_by_remoter(num);
				break;
			case STBK_7:      
				memset(num,'7',1);
				inputnum_by_remoter(num);
				break;
			case STBK_8:      
				memset(num,'8',1);
				inputnum_by_remoter(num);
				break;
			case STBK_9:  
				memset(num,'9',1);
				inputnum_by_remoter(num);
				break;
			default:
				break;
		}
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_lang_edit_keyboard_keypress(GuiWidget *widget, void *usrdata)
{
    int key_num = 0;
    GUI_Event *event = (GUI_Event *)usrdata;
    
    if(GUI_KEYDOWN ==  event->type)
    {
    int key = find_virtualkey_ex(event->key.scancode,event->key.sym);

    GUI_SendEvent(WND_LANG_KEYBOARD, event);
    if (s_keyboard_full.change_cb != NULL)
    {
        if(((key>=STBK_0)&&(key<=STBK_9))
                ||(key==STBK_OK)||(key==STBK_BLUE))
        {
            key_num =  app_language_get_move_key_row_num(STBK_OK);
            if((key_num < KEY_CHANGE_LANGUAGE)||(key_num == KEY_LAN_DEL_NUM)
                    ||(key_num == KEY_LAN_SPACE_NUM)||(key_num == KEY_LAM_CLR_NUM))
            {//printf(" [%s] [%s] [%d] key_num =%d \n",__FILE__,__FUNCTION__,__LINE__,key_num);
                s_keyboard_full.change_cb(&s_keyboard_full);
            }
        }
    }
    }
    //keyboard EVENT_TRANSFER_STOP,so the UI can't receive the change msg.
    //add change callback here, just response the num key and Del key.
    return EVENT_TRANSFER_STOP;
}

status_t multi_language_keyboard_create(PopKeyboard *keyboard)
{
#if (MINI_16_BITS_OSD_SUPPORT)
#define KB_SET_POS_X 350
#define KB_IN_POS_X  400
#define KB_IN_POS_Y  80
#else
#define KB_SET_POS_X 436
#define KB_IN_POS_X  500
#define KB_IN_POS_Y  100
#endif
    int32_t pos_x = 0;
    int32_t pos_y = 0;

    memset(s_lang_keyboard_input, 0, STR_BUF_LEN);
    if (GXCORE_ERROR == g_AppInput.init(keyboard->in_name, keyboard->max_num))
    {
        return GXCORE_ERROR;
    }

    if(keyboard->max_num > STR_BUF_LEN)
    {
        keyboard->max_num =  STR_BUF_LEN-1;
    }

    if(keyboard->in_name != NULL)
    {
        memcpy(s_lang_keyboard_input, keyboard->in_name, strlen(keyboard->in_name));
    }
    s_keyboard_full.in_ret = POP_VAL_OK;
    memcpy(&s_keyboard_full, keyboard, sizeof(PopKeyboard));
    s_keyboard_full.out_name = s_lang_keyboard_input;

    app_create_dialog(WND_LANG_KEYBOARD);

    if((keyboard->pos.x==KB_IN_POS_X)&&(keyboard->pos.y==KB_IN_POS_Y))
    {
        pos_x = KB_SET_POS_X; 
        pos_y = KB_IN_POS_Y;
        s_keyboard_full.pos.x = KB_IN_POS_X;
        GUI_SetProperty(WND_LANG_KEYBOARD, "move_window_x", &pos_x);
        GUI_SetProperty(WND_LANG_KEYBOARD, "move_window_y", &pos_y); 
    }
    else
    {
        pos_x = keyboard->pos.x;
        pos_y = keyboard->pos.y;
        s_keyboard_full.pos.x = keyboard->pos.x;
    }

    if(keyboard->usr_data != NULL)
    {
        GUI_SetProperty("txt_keyboard_name", "string", keyboard->usr_data); 
    }
    return GXCORE_SUCCESS;
}


