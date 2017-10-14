#ifndef APP_BOOK_H
#define APP_BOOK_H
#include "app.h"
#include "app_utility.h"

#define RADIO_REC_BOOK
//#undef RADIO_REC_BOOK

#define APP_BOOK_NUM  32
#define BOOK_PROGRAM_PVR BOOK_TYPE_1

typedef struct
{
	int prog_id;
	time_t duration;
}BookProgStruct;

typedef enum 
{
	BOOKTYPE_POWOFF,
	BOOKTYPE_PVR,
	BOOKTYPE_PLAY,
}BookType;

typedef struct app_book AppBook;
struct app_book
{
	int32_t (*get)(GxBookGet*);
	status_t (*creat)(GxBook*);
	status_t (*modify)(GxBook*);
	status_t (*remove)(GxBook*);
	status_t (*invalid_remove)(void);
	status_t (*exec)(GxBook*);
	time_t (*get_sleep_time)(time_t);
	WeekDay (*mode2wday)(uint32_t);
	uint32_t (*wday2mode)(WeekDay);
	void (*enable)(void);
	void (*disable)(time_t); //0, forever; 1,ignor sec
};
WndStatus book_popdlg_create(BookType book_type);
extern AppBook g_AppBook;

#endif 