/**
 *
 * @file        cascam_public.h
 * @brief
 * @version     1.1.0
 * @date        08/25/2015 09:47:49 AM
 * @author      B.Z., 88156088@nationalchip.com
 *
 */
#include "cascam_types.h"
#include "cas/cascam_wanfa.h"
#include "cas/cascam_tnt.h"
#include "cas/cascam_nstv.h"
#include "cas/cascam_dhd.h"
// TODO: need to add 
typedef enum _CASCAMCASEnum{
    CASCAM_WANFA = 0,
    CASCAM_TNT,
    CASCAM_CMB,
    CASCAM_NSTV,
    CASCAM_DHD,
    CASCAM_PRIVATE,
    CASCAM_TOTAL,
}CASCAMCASEnum;
#define CASCAM_MAX_CAS CASCAM_TOTAL

typedef struct _CASCAMInitParaClass{
   unsigned int CASFlag;
   unsigned int vcc_pole;// for smartcard
   unsigned int detect_pole;// for smartcard
}CASCAMInitParaClass;

// cas data struct
typedef struct _CASCAMDataGetClass{
   unsigned char cas_flag;//from the enum "_CASCAMCASEnum"
   CASCAMCASGetDataTypeEnum main_id;// public id
   unsigned char sub_id;//private or public id
   void*         data_buf;
   unsigned int  buf_len;// data_buf
}CASCAMDataGetClass;

typedef struct _CASCAMDataSetClass{
   unsigned char cas_flag;//from the enum "_CASCAMCASEnum"
   CASCAMCASSetDataTypeEnum main_id;// public id
   unsigned char sub_id;// private or public id
   void*         data_buf;
   unsigned int  buf_len;// data_buf
}CASCAMDataSetClass;

// osd screen display
typedef enum _CASCAMOSDDialogTypeEnum{
    CASCAM_DLG_1,
    CASCAM_DLG_2,
    CASCAM_DLG_3,
    CASCAM_DLG_4,
    CASCAM_DLG_5,
    CASCAM_DLG_6,
    CASCAM_DLG_7,
    CASCAM_DLG_8,
    CASCAM_DLG_9,
}CASCAMOSDDialogTypeEnum;

typedef struct _CASCAMOSDEventClass{
    char type;// 0: osd; 1:...
    void *data;
}CASCAMOSDEventClass;

int CASCAM_Change_Program(CASCAMProgramParaClass *pProgPara);
int CASCAM_Release_Demux(CASCAMDemuxFlagClass *pFlags);
int CASCAM_Start_Demux(CASCAMDemuxFlagClass *pFlags, unsigned short pid);
int CASCAM_Change_Frequence(void);
int CASCAM_Init(CASCAMInitParaClass Paras);
int CASCAM_Register_CAS(CASCAMControlBlockClass *CAS);
int CASCAM_Get_CAS_ID(unsigned int **ppID, unsigned char *pCount);
int CASCAM_Is_Vaild_CAS(unsigned short CASID);
int CASCAM_Get_Property(CASCAMDataGetClass *GetProperty);
int CASCAM_Set_Property(CASCAMDataSetClass *SetProperty);
int CASCAM_Do_Event(CASCAMOSDEventClass *para);
int CASCAM_Set_Zone(double zone);
void CASCAM_Osd_Clean(void);

