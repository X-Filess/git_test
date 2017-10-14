/**
 *
 * @file        cascam_platform.h
 * @brief
 * @version     1.1.0
 * @date        08/25/2015 09:47:49 AM
 * @author      B.Z., 88156088@nationalchip.com
 *
 */
#include "cascam_types.h"
// demux
int cascam_release_demux(CASCAMDemuxFlagClass *pFlag);
int cascam_start_demux(CASCAMDemuxFlagClass *pFlag, unsigned short pid);

int cascam_is_vaild_cas(unsigned short CASID);

// time convert func
void  cascam_mjd_to_ymd(unsigned int   mjd,
                            unsigned short   *year,
                            unsigned char  *month,
                            unsigned char  *day,
                            unsigned char  *weekDay);

void  cascam_ymd_to_mjd(unsigned int   *mjd,
                            unsigned short   year,
                            unsigned char  month,
                            unsigned char  day);

void  cascam_bcd_to_abc(unsigned int   bcd,
                            unsigned short *a,
                            unsigned char  *b,
                            unsigned char  *c);

void  cascam_abc_to_bcd(unsigned int   *bcd,
                            unsigned short a,
                            unsigned char  b,
                            unsigned char  c);

