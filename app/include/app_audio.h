#ifndef __APP_AUDIO_H__
#define __APP_AUDIO_H__

#include "service/gxplayer.h"
#include "module/si/si_pmt.h"

typedef struct
{
	uint32_t id;
	char *name;
	AudioCodecType audiotype;
}AudioList_t;

typedef struct
{
	AudioList_t *audioLangList;
	uint32_t audioLangCnt;
}AudioLang_t;

void app_track_set(PmtInfo* audio);
void app_audio_free(void);
AudioLang_t* app_get_audio_lang(void);
void app_audio_lang_free(AudioLang_t *audio_lang);

#endif
