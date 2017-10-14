#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_full_keyboard.h"

full_keyboard_proc s_full_keyboard_proc = NULL;
char s_full_keyboard_input[BUF_LEN] = {0};	
char *s_keymap[KEY_NUM][3] = {
	{ "`","`","button_key_0",},
	{ "~","~","button_key_1",},
	{ "!","!","button_key_2",},
	{ "@","@","button_key_3",},
	{ "#","#","button_key_4",},
	{ "$","$","button_key_5",},
	{ "%","%","button_key_6",},
	{ "^","^","button_key_7",},
	{ "&","&","button_key_8",},
	{ "*","*","button_key_9",},
	{ "?","?","button_key_10",},
	{ "CLR","CLR","button_key_11",},
	{ "(","(","button_key_12",},
	{ ")",")","button_key_13",},
	{ "{","{","button_key_14",},
	{ "}","}","button_key_15",},
	{ "[","[","button_key_16",},
	{ "]","]","button_key_17",},
	{ "<","<","button_key_18",},
	{ ">",">","button_key_19",},
	{ ",",",","button_key_20",},
	{ ".",".","button_key_21",},
	{ "|","|","button_key_22",},
	{ "DEL","DEL","button_key_23",},
	{ "0","0","button_key_24",},
	{ "1","1","button_key_25",},
	{ "2","2","button_key_26",},
	{ "3","3","button_key_27",},
	{ "4","4","button_key_28",},
	{ "5","5","button_key_29",},
	{ "6","6","button_key_30",},
	{ "7","7","button_key_31",},
	{ "8","8","button_key_32",},
	{ "9","9","button_key_33",},
	{ "/","/","button_key_34",},
	{ "\\","\\","button_key_35",},
	{ "a","A","button_key_36",},
	{ "b","B","button_key_37",},
	{ "c","C","button_key_38",},
	{ "d","D","button_key_39",},
	{ "e","E","button_key_40",},
	{ "f","F","button_key_41",},
	{ "g","G","button_key_42",},
	{ "h","H","button_key_43",},
	{ "i","I","button_key_44",},
	{ " "," ","button_key_45",},
	{ "_","_","button_key_46",},
	{ "A-a","A-a","button_key_47",},
	{ "j","J","button_key_48",},
	{ "k","K","button_key_49",},
	{ "l","L","button_key_50",},
	{ "m","M","button_key_51",},
	{ "n","N","button_key_52",},
	{ "o","O","button_key_53",},
	{ "p","P","button_key_54",},
	{ "q","Q","button_key_55",},
	{ "r","R","button_key_56",},
	{ "-","-","button_key_57",},
	{ "+","+","button_key_58",},
	{ "=","=","button_key_59",},
	{ "s","S","button_key_60",},
	{ "t","T","button_key_61",},
	{ "u","U","button_key_62",},
	{ "v","V","button_key_63",},
	{ "w","W","button_key_64",},
	{ "x","X","button_key_65",},
	{ "y","Y","button_key_66",},
	{ "z","Z","button_key_67",},
	{ ":",":","button_key_68",},
	{ "\"","\"","button_key_69",},
	{ ";",";","button_key_70",},
	{ "'","'","button_key_71",}
};
static uint8_t s_keyboard_caps = 0;
static int8_t s_keyboard_x = 0;
static int8_t s_keyboard_y = 3;

void full_keyboard_move(uint16_t key)
{
	switch(key)
	{
		case STBK_UP:	
			if(s_keyboard_y == 0)
				s_keyboard_y = KEY_Y;
			else
				s_keyboard_y--;
			break;
		case STBK_DOWN:			
			if(s_keyboard_y == KEY_Y)
				s_keyboard_y = 0;
			else
				s_keyboard_y++;
			break;
		case STBK_LEFT:				
			if(s_keyboard_x == 0)
				s_keyboard_x = KEY_X;
			else
				s_keyboard_x--;
			break;
		case STBK_RIGHT:
			if(s_keyboard_x == KEY_X)
				s_keyboard_x = 0;
			else
				s_keyboard_x++;
			break;
		default:
			break;
	}

	GUI_SetFocusWidget(s_keymap[s_keyboard_x+12*s_keyboard_y][2]);
}

void full_keyboard_ok(void)
{
	uint8_t key_num = s_keyboard_x+12*s_keyboard_y;

	if(s_keyboard_x+12*s_keyboard_y == 45)
	{
		GUI_SetProperty(s_keymap[45][2],"focus_img", "s_button_space_unfocus.bmp");
	}
	else
	{
	GUI_SetProperty(s_keymap[key_num][2],"backcolor", "[#f4f4f4,#f4f4f4,#31314a]");
	}
	GUI_SetInterface("flush",NULL);
	GxCore_ThreadDelay(30);
	if(s_keyboard_x+12*s_keyboard_y == 45)
	{
		GUI_SetProperty(s_keymap[45][2], "focus_img", "s_button_space_focus.bmp");
	}
	else
	{
	GUI_SetProperty(s_keymap[key_num][2],"backcolor", "[#f4f4f4,#31314a,#31314a]");
	}
	GUI_SetInterface("flush",NULL);
	
	if(key_num == KEY_CLR_NUM)
	{
		memset(s_full_keyboard_input, 0, BUF_LEN);
		GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", NULL);
	}
	else if(key_num == KEY_DEL_NUM)
	{
		s_full_keyboard_input[strlen(s_full_keyboard_input)-1] = 0;
		if(strlen(s_full_keyboard_input) == 0)
			GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", NULL);
		else
			GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
	}
	else if(key_num == KEY_CAPS_NUM)
	{
		uint8_t i = 0;
		
		s_keyboard_caps = (s_keyboard_caps+1)%2;

		for(i = 0; i < KEY_NUM; i++)
			GUI_SetProperty(s_keymap[i][2], "string", s_keymap[i][s_keyboard_caps]);
	}
	else
	{
		if(strlen(s_full_keyboard_input) < BUF_LEN-1)
		{
			strcat(s_full_keyboard_input,s_keymap[key_num][s_keyboard_caps]);
			GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
		}
	}

	
}

SIGNAL_HANDLER int app_full_keyboard_create(const char* widgetname, void *usrdata)
{
	uint8_t i = 0;

	memset(s_full_keyboard_input, 0, BUF_LEN);
	
	for(i = 0; i < KEY_NUM; i++)
	{
		GUI_SetProperty(s_keymap[i][2], "string", s_keymap[i][s_keyboard_caps]);
	}

	if(s_keyboard_x > KEY_X)
		s_keyboard_x = 0;
	if(s_keyboard_x > KEY_Y)
		s_keyboard_y = 3;
	GUI_SetFocusWidget(s_keymap[s_keyboard_x+12*s_keyboard_y][2]);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_full_keyboard_destroy(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_full_keyboard_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_UP:
			case STBK_DOWN:
			case STBK_LEFT:
			case STBK_RIGHT:
				full_keyboard_move(event->key.sym);
				break;
			case STBK_OK:
				full_keyboard_ok();
				break;
			case STBK_EXIT:
				memset(s_full_keyboard_input, 0, BUF_LEN);
			case STBK_ZOOM:
			case STBK_GREEN:
				GUI_EndDialog(WND_FULL_KEYBOARD);
				if(s_full_keyboard_proc)
				{
					s_full_keyboard_proc();
					s_full_keyboard_proc = NULL;
				}
				break;
			case STBK_AUDIO:
			case STBK_RED:
				s_full_keyboard_input[strlen(s_full_keyboard_input)-1] = 0;
				if(strlen(s_full_keyboard_input) == 0)
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", NULL);
				else
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				break;
			case STBK_PAUSE_STB:
			case STBK_BLUE:
				{
					uint8_t i = 0;
		
					s_keyboard_caps = (s_keyboard_caps+1)%2;

					for(i = 0; i < KEY_NUM; i++)
						GUI_SetProperty(s_keymap[i][2], "string", s_keymap[i][s_keyboard_caps]);
				}
				break;
			case STBK_0:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "0");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_1:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "1");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_2:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "2");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_3:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "3");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_4:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "4");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_5:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "5");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_6:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "6");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_7:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "7");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_8:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "8");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			case STBK_9:
				if(strlen(s_full_keyboard_input) < BUF_LEN-1)
				{
					strcat(s_full_keyboard_input, "9");
					GUI_SetProperty(TXT_FULL_KEYBOARD_INPUT, "string", s_full_keyboard_input);
				}
				break;
			default:
				break;
		}
	}

	return EVENT_TRANSFER_STOP;
}

