#ifdef ECOS_OS
#include "app_config.h"
#include "frontend/gx_frontend_init.hxx"
#include "gxcore_bsp.h"
#include "app_bsp.h"
/* ------------------------- Main Chip ----------------------------------- */
#ifdef GX3201
CHIP_GX3201
#define CHIP gx3201
#endif
#ifdef GX3211
CHIP_GX3211
#define CHIP gx3211
#endif

#ifdef GX6605S
CHIP_GX6605S
#define CHIP gx6605s 
#endif

/* ------------------------- Board support device ------------------------ */
MOD_WDT
MOD_IRR
MOD_SCI
MOD_SCPU
//MOD_MMC
MOD_AV(NULL)
MOD_UART
//MOD_NORFLASH
MOD_SPIFLASH
MOD_FLASHIO
//MOD_NANDFLASH
MOD_USB
MOD_MTC
#if NETWORK_SUPPORT
MOD_NET
#if WIFI_SUPPORT
MOD_WIFI
#endif
//MOD_ETH
#if MOD_3G_SUPPORT
MOD_USB3G
#endif
#endif
//MOD_HDMI
MOD_PANEL
MOD_I2C
MOD_GPIO
//#if IPTV_SUPPORT
MOD_OTP
//#endif
/* ------------------------------ Video & Audio Codec -------------------- */
//CODEC_AC3
//CODEC_DOLBY51
//CODEC_AVSA
//CODEC_AVSV
//CODEC_DRA
//CODEC_H263
CODEC_MPEG212A(CHIP)
CODEC_MPEG4_AAC(CHIP)
CODEC_DRA(CHIP)
CODEC_VIDEO(CHIP)
//CODEC_DOLBY(CHIP)
//CODEC_OGG
//CODEC_RA_AAC
//CODEC_RA_RA8LBR
//CODEC_RV

/* ------------------------- Support filesystem -------------------------- */
MOD_FAT
MOD_NTFS
//MOD_JFFS2
MOD_CAMFS
MOD_MINIFS
MOD_ROMFS
MOD_RAMFS

/* -------------------------- Frontend ----------------------------------- */

// new hardware: GX6601MPW_NRE_LOFP128_AV2012_ONBOARD_V1.1, 2013_04_20
//MOD_113x(1,"|4:2:0x80:41:0:0xC0:&0:1:7:0")//6601
//MOD_113x(1,"|4:2:0x80:41:0:0xc0:&1:1:7:0:0")//6602
//MOD_113x(1,"|4:2:0x80:41:0:0xC6:&0:1:7:0")//6602
//MOD_113x(1,"|4:2:0x80:41:0:0xC6:&0:1:7:0:0")//6601 box//
//MOD_113x(1,"|4:2:0x80:55:0:0x18:&0:1:7:0:0")//6601e RDA5815M
//MOD_113x(1,"|4:2:0x80:41:0:0xC6:&1:0:7:0:1")//GX6601 Starton
//MOD_113x(2,"|4:2:0x80:41:0:0xc0:&1:1:7:0:0|4:1:0xd0:41:1:0xc0:&1:1:2:0:0")
//MOD_113x(1,"|4:2:0x80:41:0:0xC6:&1:0:7:0:1")//GX6601 Starton
//MOD_113x(1,"|4:2:0x80:41:0:0xC0:&0:1:7:0:0")//GX6601 NextStar
//MOD_113x(2,"|4:2:0x80:41:0:0xc6:&1:1:7:0|4:1:0xd0:41:1:0xc6:&1:1:2:0")//GX6602 NRE PUBLIC
//MOD_113x(1,"|4:2:0x80:41:0:0xC0:&1:1:7:0")//GX6601 New Public Board
//MOD_113x(1,"|4:2:0x80:54:0:0xc2:&1:1:7:1")//6601_mtv600
//MOD_113x(1,"|4:2:0x80:41:0:0xC0:&1:1:7:0")//6601
//MOD_113x(2,"|4:0:0xd2:41:0:0xc0:&1:1:4:1|4:0:0xc2:41:0:0xc0:&1:1:6:1")
// new demo
//MOD_113x(2,"|4:0:0xd2:41:0:0xc6:&0:1:6:0|4:0:0xd0:41:0:0xc0:&0:1:6:0")
//MOD_113x(1,"|4:0:0xc2:41:0:0xc0:&1:1:6:1")
//MOD_113x(1,"|4:0:0xd2:41:0:0xc0:&1:1:4:1")
//MOD_1131(1,"|4:0:0xd2:41:0:0xc0:&1:1")
//MOD_1132(1,"|5:0:0xc2:41:0:0xc0:&1:1")
//MOD_1131(1,"|4:0:0xd0:41:0:0xc6:&1:1")
//MOD_1131(1,"|4:0:0xd2:41:0:0xc6:")
//MOD_1131(2,"|4:0:0xd2:41:0:0xc6:|4:1:0xd0:41:1:0xc6:")
//MOD_GX1201
//MOD_GX1501

/* -------------------------- Language Package --------------------------- */
LANG_US
LANG_UTF8
//LANG_SIMP_CHINESE
//LANG_TRAD_CHINESE
//LANG_KOREAN
//LANG_JAPANESE
//LANG_ARABIC_O
//LANG_ARABIC_W
//LANG_GREEK_O
//LANG_GREEK_W
//LANG_CENT_EUR
//LANG_BALTIC_O
//LANG_BALTIC_W
//LANG_MULTI_LANTIN1
//LANG_LATIN2_O
//LANG_LATIN1_W
//LANG_CYRILLIC_O
//LANG_CYRILLIC_W
//LANG_RUSSIAN_O
//LANG_TURKISH_O
//LANG_TURKISH_W
//LANG_MULTI_LATIN1_EUR
//LANG_HEBREW_O
//LANG_HEBREW_W
//LANG_THAI
//LANG_VIETNAM
#endif

