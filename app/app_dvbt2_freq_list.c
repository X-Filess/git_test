#include <gxtype.h>
#include "app.h"
#include "app_dvbt2_search.h"
#if (DEMOD_DVB_T > 0)
static classDefaultFreqCh english_freq_channel[]=
{
    {"5",177.5,1},	//the first one as main freq is set to nit search when auto scan.by dugan
    {"6",184.5,1},
    {"7",191.5,1},
    {"8",198.5,1},
    {"9",205.5,1},
    {"10",212.5,1},
    {"11",219.5,1},
    {"12",226.5,1},
    {"21",474.0,0},
    {"22",482.0,0},
    {"23",490.0,0},
    {"24",498.0,0},
    {"25",506.0,0},
    {"26",514.0,0},
    {"27",522.0,0},
    {"28",530.0,0},
    {"29",538.0,0},
    {"30",546.0,0},
    {"31",554.0,0},
    {"32",562.0,0},
    {"33",570.0,0},
    {"34",578.0,0},
    {"35",586.0,0},
    {"36",594.0,0},
    {"37",602.0,0},
    {"38",610.0,0},
    {"39",618.0,0},
    {"40",626.0,0},
    {"41",634.0,0},
    {"42",642.0,0},
    {"43",650.0,0},
    {"44",658.0,0},
    {"45",666.0,0},
    {"46",674.0,0},
    {"47",682.0,0},
    {"48",690.0,0},
    {"49",698.0,0},
    {"50",706.0,0},
    {"51",714.0,0},
    {"52",722.0,0},
    {"53",730.0,0},
    {"54",738.0,0},
    {"55",746.0,0},
    {"56",754.0,0},
    {"57",762.0,0},
    {"58",770.0,0},
    {"59",778.0,0},
    {"60",786.0,0},
    {"61",794.0,0},
    {"62",802.0,0},
    {"63",810.0,0},
    {"64",818.0,0},
    {"65",826.0,0},
    {"66",834.0,0},
    {"67",842.0,0},
    {"68",850.0,0},
    {"69",858.0,0},
};
#define ENG_CH_NUM	(sizeof(english_freq_channel)/sizeof(classDefaultFreqCh))

static classDefaultFreqCh italy_freq_channel[]=
{
    {"D",177.5,1},	//the first one as main freq is set to nit search when auto scan.by dugan
    {"D1",184.5,1},
    {"E",186.0,1},
    {"E1",191.5,1},
    {"F",194.5,1},
    {"F1",198.5,1},
    {"G",203.5,1},
    {"G1",205.5,1},
    {"H",212.5,1},
    {"H1",219.5,1},
    {"H2",226.5,1},
    {"21",474.0,0},
    {"22",482.0,0},
    {"23",490.0,0},
    {"24",498.0,0},
    {"25",506.0,0},
    {"26",514.0,0},
    {"27",522.0,0},
    {"28",530.0,0},
    {"29",538.0,0},
    {"30",546.0,0},
    {"31",554.0,0},
    {"32",562.0,0},
    {"33",570.0,0},
    {"34",578.0,0},
    {"35",586.0,0},
    {"36",594.0,0},
    {"37",602.0,0},
    {"38",610.0,0},
    {"39",618.0,0},
    {"40",626.0,0},
    {"41",634.0,0},
    {"42",642.0,0},
    {"43",650.0,0},
    {"44",658.0,0},
    {"45",666.0,0},
    {"46",674.0,0},
    {"47",682.0,0},
    {"48",690.0,0},
    {"49",698.0,0},
    {"50",706.0,0},
    {"51",714.0,0},
    {"52",722.0,0},
    {"53",730.0,0},
    {"54",738.0,0},
    {"55",746.0,0},
    {"56",754.0,0},
    {"57",762.0,0},
    {"58",770.0,0},
    {"59",7780,0},
    {"60",786.0,0},
    {"61",794.0,0},
    {"62",802.0,0},
    {"63",810.0,0},
    {"64",818.0,0},
    {"65",826.0,0},
    {"66",834.0,0},
    {"67",842.0,0},
    {"68",850.0,0},
    {"69",858.0,0},
};
#define ITALY_CH_NUM	(sizeof(italy_freq_channel)/sizeof(classDefaultFreqCh))

static classDefaultFreqCh test_freq_channel[]=
{
    {"21",474.0,0},	//the first one as main freq is set to nit search when auto scan.by dugan
    {"68",850.0,0},
};
#define TEST_CH_NUM	(sizeof(test_freq_channel)/sizeof(classDefaultFreqCh))

static classDefaultFreqCh *freq_list[ITEM_COUNTRY_MAX] =
{
    english_freq_channel,
    italy_freq_channel,
    test_freq_channel,
};
static int freq_list_ch_num[ITEM_COUNTRY_MAX] =
{
    ENG_CH_NUM,
    ITALY_CH_NUM,
    TEST_CH_NUM,
};
static char country_name[ITEM_COUNTRY_MAX][MAX_COUNTRY_NAME_LEN] =
{
    "England",
    "Italy",
    "test",
};

int AppDvbt2GetFreqList(classDefaultFreqCh **fch)
{	
    int32_t s_country = -1;	
    int num = 0;
    //GxBus_ConfigGetInt(OSD_LANG_KEY, &lang, OSD_LANG);//
    GxBus_ConfigGetInt(T2_COUNTRY, &s_country, T2_COUNTRY_VALUE);
    if(s_country >= ITEM_COUNTRY_MAX)
    {
        *fch = freq_list[ITEM_COUNTRY_ENG];
        num = freq_list_ch_num[ITEM_COUNTRY_ENG];
    }

    *fch  = freq_list[s_country];//for test
    num = freq_list_ch_num[s_country];

    return num;
}
void AppDvbt2SetCountry(int s_country)
{
    GxBus_ConfigSetInt(T2_COUNTRY, s_country);
}
int AppDvbt2GetCountry()
{
    int32_t s_country = -1;	
    GxBus_ConfigGetInt(T2_COUNTRY, &s_country, T2_COUNTRY_VALUE);
    return s_country;
}

char * AppDvbt2GetCountryName(int s_country)
{
    return country_name[s_country];
}
int AppDvbt2GetCountryNum(void)
{
    return ITEM_COUNTRY_MAX;
}

#endif
