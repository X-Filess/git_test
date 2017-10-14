/*****************************************************************************
*                       CONFIDENTIAL
*          Hangzhou NationalChip Science and Technology Co., Ltd.
*               (C)2008, All right reserved
******************************************************************************
******************************************************************************
*   File Name : ui_keyboard_sub.h
*   Author : zhangyg 
*   Project : 
*   Type : APP
******************************************************************************
*   Purpose : 模块头文件
******************************************************************************
*   Release History:
    VERSION        Date         AUTHOR          Description
 
    hello, please write revision information here.
*****************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __UI_KEYBOARD_LAMGIAGE_H__
#define __UI_KEYBOARD_LAMGIAGE_H__
/* Includes --------------------------------------------------------------- */


/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif
/* Exported Macros -------------------------------------------------------- */
//#define KEY_LANGUAGE_MAX	10
#define BUTTON_NUM		3
#define KEY_ROW_COUNT	12
#define STR_BUF_LEN		45
#define KEY_COUNT 		43
#define KEY_LAN_X		11
#define KEY_LAN_Y		3
#define KEY_LAM_CLR_NUM	42//-34
#define KEY_LAN_DEL_NUM	41//	40
#define KEY_LAN_CAPS_NUM	35
#define KEY_LAN_SPACE_NUM	38
#define KEY_LAN_NUMBER_NUM	36
#define KEY_LAN_SAVE_OK_NUM	40 //41
#define KEY_LAN_LEFT_CURSOR	37
#define KEY_LAN_RIGHT_CURSOR	39
#define KEY_CHANGE_LANGUAGE	34//-42

//char no space
#define KEY_LAN_INVALID_CHAR_NAME ",[]"
#define FILE_CHAR_NAME_INVALID 	"\x5c\x2f"



#define WND_LANG_KEYBOARD				"wnd_keyboard_language"
#define EDIT_LANG_KEYBOARD_INPUT 		"edit_lamgiage_keyboard_input"

#define WND_LANG_FOCUS_BMP56X38			"s_kb_focus.bmp"
#define WND_LANG_FOCUS_BMP254X38			"s_kb_focus254.bmp"
#define WND_LANG_FOCUS_LEFT_ARROW		"s_kb_focus_left.bmp"
#define WND_LANG_FOCUS_RIGHT_ARROW		"s_kb_focus_right.bmp"
#define WND_LANG_FOCUS_BACKSPACE			"s_kb_focus_arrow.bmp"
#define WND_LANG_FOCUS_CLEAR122			"s_kb_focus122.bmp"
#define WND_LANG_FOCUS_BLUE_P122			"s_kb_focus_blue.bmp"


#define WND_LANG_UNFOCUS_BMP56X38			"s_kb_unfocus.bmp"
#define WND_LANG_UNFOCUS_BMP254X38			"s_kb_unfocus254.bmp"
#define WND_LANG_UNFOCUS_LEFT_ARROW			"s_kb_unfocus_left.bmp"
#define WND_LANG_UNFOCUS_RIGHT_ARROW		"s_kb_unfocus_right.bmp"
#define WND_LANG_UNFOCUS_BACKSPACE			"s_kb_unfocus_arrow.bmp"
#define WND_LANG_UNFOCUS_CLEAR122			"s_kb_unfocus122.bmp"
#define WND_LANG_UNFOCUS_RED					"s_kb_focus_red.bmp"
#define WND_LANG_UNFOCUS_GREEN				"s_kb_focus_green.bmp"
#define WND_LANG_UNFOCUS_YELLOW				"s_kb_focus_yellow.bmp"

/*enum TIME_SET_LANG_NAME 
{
	ITEM_ENGLISH = 0,
	ITEM_VIETNAMESE,
	ITEM_ARABIC,
	ITEM_RUSSIAN,
	ITEM_FRENCH,
	ITEM_PORTUGUESE,
	ITEM_TURKISH,
	ITEM_SPAIN,
	ITEM_GERMAN,
	ITEM_FARSI,
	ITEM_LANGUAGE_TOTAL
};*/

enum TIME_SET_LANG_NAME 
{
	ITEM_ENGLISH = 0,
	ITEM_VIETNAMESE,
	ITEM_ARABIC,
	ITEM_TURKISH,
	ITEM_RUSSIAN,
	ITEM_PORTUGUESE,
	ITEM_SPAIN,
	ITEM_FRENCH,
	ITEM_GERMAN,
	ITEM_FARSI,
	ITEM_LANGUAGE_TOTAL
};

#define ARABIC_KEYBOARD

typedef struct 
{
  uint16_t Start;
  uint16_t End;
} ArabicMap;

enum
{
	KEYBOARD_BUTTON101 = 0,
	KEYBOARD_BUTTON102,
	KEYBOARD_BUTTON103,
	KEYBOARD_BUTTON104,
	KEYBOARD_BUTTON105,
	KEYBOARD_BUTTON106,
	KEYBOARD_BUTTON107,
	KEYBOARD_BUTTON108,
	KEYBOARD_BUTTON109,
	KEYBOARD_BUTTON110,
	KEYBOARD_BUTTON111,
	KEYBOARD_BUTTON112,
	KEYBOARD_BUTTON201,
	KEYBOARD_BUTTON202,
	KEYBOARD_BUTTON203,
	KEYBOARD_BUTTON204,
	KEYBOARD_BUTTON205,
	KEYBOARD_BUTTON206,
	KEYBOARD_BUTTON207,
	KEYBOARD_BUTTON208,
	KEYBOARD_BUTTON209,
	KEYBOARD_BUTTON210,
	KEYBOARD_BUTTON211,
	KEYBOARD_BUTTON212,
	KEYBOARD_BUTTON301,
	KEYBOARD_BUTTON302,
	KEYBOARD_BUTTON303,
	KEYBOARD_BUTTON304,
	KEYBOARD_BUTTON305,
	KEYBOARD_BUTTON306,
	KEYBOARD_BUTTON307,
	KEYBOARD_BUTTON308,
	KEYBOARD_BUTTON309,
	KEYBOARD_BUTTON310,
	KEYBOARD_BUTTON311,
	KEYBOARD_BUTTON401,
	KEYBOARD_BUTTON402,
	KEYBOARD_BUTTON403,
	KEYBOARD_BUTTON404,
	KEYBOARD_BUTTON405,
	KEYBOARD_BUTTON406,
	KEYBOARD_BUTTON407,
	KEYBOARD_BUTTON408,
	KEYBOARD_BUTTON_SHIFT
};

typedef void (* lang_keyboard_proc)(void);
/* Exported Constants ----------------------------------------------------- */
/* Exported Types --------------------------------------------------------- */
#ifdef ARABIC_KEYBOARD
//Language about Arabic ---20090730
#define GUI_KEY_A_1 	0xC1//0x0621
#define GUI_KEY_A_2		0xC2//0x0622
#define GUI_KEY_A_3 	0xC3//0x0623
#define GUI_KEY_A_4		0xC4//0x0624
#define GUI_KEY_A_5 	0xC5//0x0625
#define GUI_KEY_A_6		0xC6//0x0626
#define GUI_KEY_A_7 	0xC7//0x0627
#define GUI_KEY_A_8		0xC8//0x0628
#define GUI_KEY_A_9 	0xC9//0x0629
#define GUI_KEY_A_10	0xCA//0x062A
#define GUI_KEY_A_11 	0xCB//0x062B
#define GUI_KEY_A_12 	0xCC//0x062C
#define GUI_KEY_A_13 	0xCD//0x062D
#define GUI_KEY_A_14 	0xCE//0x062E
#define GUI_KEY_A_15 	0xCF//0x062F
#define GUI_KEY_A_16 	0xD0//0x0630
#define GUI_KEY_A_17 	0xD1//0x0631
#define GUI_KEY_A_18 	0xD2//0x0632
#define GUI_KEY_A_19 	0xD3//0x0633
#define GUI_KEY_A_20 	0xD4//0x0634
#define GUI_KEY_A_21 	0xD5//0x0635
#define GUI_KEY_A_22 	0xD6//0x0636
#define GUI_KEY_A_23 	0xD8//0x0637
#define GUI_KEY_A_24 	0xD9//0x0638
#define GUI_KEY_A_25 	0xDA//0x0639
#define GUI_KEY_A_26 	0xDB//0x063A
#define GUI_KEY_A_27 	0xDC//0x0640
#define GUI_KEY_A_28 	0xDD//0x0641
#define GUI_KEY_A_29 	0xDE//0x0642
#define GUI_KEY_A_30 	0xDF//0x0643
#define GUI_KEY_A_31 	0xE1//0x0644
#define GUI_KEY_A_32 	0xE3//0x0645
#define GUI_KEY_A_33 	0xE4//0x0646
#define GUI_KEY_A_34 	0xE5//0x0647
#define GUI_KEY_A_35 	0xE6//0x0648
#define GUI_KEY_A_36 	0xEC//0x0649
#define GUI_KEY_A_37 	0xED//0x064A
#define GUI_KEY_A_38 	0xF0//0x060C
#define GUI_KEY_A_39 	0xBA//0x061B
#define GUI_KEY_A_40 	0xBF//0x061F
//#define GUI_KEY_A_38 	0x064B
//#define GUI_KEY_A_39 	0x064C
//#define GUI_KEY_A_40 	0x064D
//#define GUI_KEY_A_41 	0x064E
//#define GUI_KEY_A_42 	0x064F
//#define GUI_KEY_A_43 	0x0650
//#define GUI_KEY_A_44 	0x0651
//#define GUI_KEY_A_45 	0x0652

//keyboard about arabic -
#define WIN_KEY_A_1 	GUI_KEY_A_1
#define WIN_KEY_A_2		GUI_KEY_A_2
#define WIN_KEY_A_3 	GUI_KEY_A_3
#define WIN_KEY_A_4		GUI_KEY_A_4
#define WIN_KEY_A_5 	GUI_KEY_A_5
#define WIN_KEY_A_6		GUI_KEY_A_6
#define WIN_KEY_A_7 	GUI_KEY_A_7
#define WIN_KEY_A_8		GUI_KEY_A_8
#define WIN_KEY_A_9 	GUI_KEY_A_9
#define WIN_KEY_A_10	GUI_KEY_A_10
#define WIN_KEY_A_11 	GUI_KEY_A_11
#define WIN_KEY_A_12 	GUI_KEY_A_12
#define WIN_KEY_A_13 	GUI_KEY_A_13
#define WIN_KEY_A_14 	GUI_KEY_A_14
#define WIN_KEY_A_15 	GUI_KEY_A_15
#define WIN_KEY_A_16 	GUI_KEY_A_16
#define WIN_KEY_A_17 	GUI_KEY_A_17
#define WIN_KEY_A_18 	GUI_KEY_A_18
#define WIN_KEY_A_19 	GUI_KEY_A_19
#define WIN_KEY_A_20 	GUI_KEY_A_20
#define WIN_KEY_A_21 	GUI_KEY_A_21
#define WIN_KEY_A_22 	GUI_KEY_A_22
#define WIN_KEY_A_23 	GUI_KEY_A_23
#define WIN_KEY_A_24 	GUI_KEY_A_24
#define WIN_KEY_A_25 	GUI_KEY_A_25
#define WIN_KEY_A_26 	GUI_KEY_A_26
#define WIN_KEY_A_27 	GUI_KEY_A_27
#define WIN_KEY_A_28 	GUI_KEY_A_28
#define WIN_KEY_A_29 	GUI_KEY_A_29
#define WIN_KEY_A_30 	GUI_KEY_A_30
#define WIN_KEY_A_31 	GUI_KEY_A_31
#define WIN_KEY_A_32 	GUI_KEY_A_32
#define WIN_KEY_A_33 	GUI_KEY_A_33
#define WIN_KEY_A_34 	GUI_KEY_A_34
#define WIN_KEY_A_35 	GUI_KEY_A_35
#define WIN_KEY_A_36 	GUI_KEY_A_36
#define WIN_KEY_A_37 	GUI_KEY_A_37
#define WIN_KEY_A_38 	GUI_KEY_A_38
#define WIN_KEY_A_39 	GUI_KEY_A_39
#define WIN_KEY_A_40 	GUI_KEY_A_40
//#define WIN_KEY_A_41 	GUI_KEY_A_41
//#define WIN_KEY_A_42 	GUI_KEY_A_42
//#define WIN_KEY_A_43 	GUI_KEY_A_43
//#define WIN_KEY_A_44 	GUI_KEY_A_44
//#define WIN_KEY_A_45 	GUI_KEY_A_45
//#define WIN_KEY_A_46 	GUI_KEY_A_46
//#define WIN_KEY_A_47 	GUI_KEY_A_47
//#define WIN_KEY_A_48 	GUI_KEY_A_48
#define	WIN_KEY_ASCII33	    33
#define	WIN_KEY_ASCII33	    33
#define	WIN_KEY_ASCII64     64
#define	WIN_KEY_ASCII35     35
#define	WIN_KEY_ASCII36     36
#define	WIN_KEY_ASCII37     37
#define	WIN_KEY_ASCII94     94
#define	WIN_KEY_ASCII38     38
#define	WIN_KEY_ASCII42     42
#define	WIN_KEY_ASCII47     47
#define	WIN_KEY_ASCII63     63
#define	WIN_KEY_ASCII40     40
#define	WIN_KEY_ASCII41     41
#define	WIN_KEY_ASCII46     46
#define	WIN_KEY_ASCII44     44
#define	WIN_KEY_ASCII95     95
#define	WIN_KEY_ASCII45     45
#define	WIN_KEY_ASCII43     43
#define	WIN_KEY_ASCII61	    61
#define 	WIN_KEY_SPACE		32
#define 	WIN_KEY_DEL		KEY_LAN_DEL_NUM//30
#define	WIN_KEY_KEYBROAD_EXIT 1001
//end arabic keyboard

#endif

/* Exported Variables ----------------------------------------------------- */

extern int gxgdi_iconv(const unsigned char *from_str, 
		unsigned char **to_str,
		unsigned int from_size,
		unsigned int *to_size,
		unsigned char *iso_str);

#ifdef __cplusplus
}
#endif
#endif /* __UI_KEYBOARD_SUB_H__ */
/* End of file -------------------------------------------------------------*/











