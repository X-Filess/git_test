
void  cascam_mjd_to_ymd(unsigned int   mjd,
                            unsigned short   *year,
                            unsigned char  *month,
                            unsigned char  *day,
                            unsigned char  *weekDay)
{
	unsigned int  uMjd;
	unsigned int  y, m, k;


	uMjd = (unsigned int)mjd;

	if( uMjd < 15079 ) { /* MJD Lower than 1900/3/1 ? */
		uMjd += 0x10000; /* Adjust MJD */
	}

	y = ( uMjd * 100 - 1507820 ) / 36525;					/* Calculate Y', M' */
	m = ( uMjd * 10000 - 149561000 - ( ( y * 36525 ) / 100 ) * 10000 ) / 306001;

	*day = (unsigned char)( uMjd - 14956 - ( ( y * 36525 ) / 100 ) - ( ( m * 306001 ) / 10000 ) );
	/* Calculate Day */
	k = ( ( m == 14 ) || ( m == 15 ) ) ? 1 : 0;/* If M'=14 or M'=15 then K=1 else K=0 */

	*year = (unsigned short )( y + k ) + 1900;	/* Calculate Year */
	*month = (unsigned char)( m - 1 - k * 12 );	/* Calculate Month */

	*weekDay = (unsigned char)( ( ( uMjd + 2 ) % 7 ) + 1 ); /* Calculate Week Day */
	if (*weekDay >= 7)
	{
		*weekDay = *weekDay % 7;
	}
}

void  cascam_ymd_to_mjd(unsigned int   *mjd,
                            unsigned short   year,
                            unsigned char  month,
                            unsigned char  day)
{
	unsigned int  uMjd;
	unsigned int  l;

	if( month == 1 || month ==2)
	{
		l = 1;
	}
	else
	{
		l = 0;
	}

	uMjd = 14956 + day + (unsigned int)((year - l)*36525/100) + (unsigned int)((month + 1 + l*12) * 306001/10000);
	*mjd = (unsigned short )uMjd;
}

//
void  cascam_bcd_to_abc(unsigned int   bcd,
                            unsigned short *a,
                            unsigned char  *b,
                            unsigned char  *c)
{
    *a = (((bcd >> 28) & 0xf)*1000) 
            + (((bcd >> 24) & 0xf)*100)
            + (((bcd >> 20) & 0xf)*10)
            + ((bcd >> 16) & 0xf);

    *b = (((bcd >> 12) & 0xf)*10) + ((bcd >> 8) & 0xf);

    *c = (((bcd >> 4) & 0xf)*10) + ((bcd >> 0) & 0xf);
}

//
void  cascam_abc_to_bcd(unsigned int   *bcd,
                            unsigned short a,
                            unsigned char  b,
                            unsigned char  c)
{
    unsigned short bcd_high_word = 0;
    unsigned short bcd_low_word  = 0;

    bcd_high_word = (((a/1000) & 0xf) << 12) 
                    | ((((a/100)%10) & 0xf) << 8)
                    | ((((a/10)%10) & 0xf) << 4)
                    | ((a%10) & 0xf);
    bcd_low_word = (((b/10) & 0xf) << 12)
                    | (((b%10) & 0xf) << 8)
                    | (((c/10) & 0xf) << 4)
                    | ((c%10) & 0xf);
    *bcd = (bcd_high_word << 16) | bcd_low_word;
}


