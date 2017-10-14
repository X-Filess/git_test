#include "cdca_verify.h"
#include "cdca_key2.inc"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char* thiz_verity_name[] = {
    "OTA",
    "KERNEL",
    "ROOT",
};

int cdcas_check_need_verify(char *name)
{
    int i = 0;
    int count = sizeof(thiz_verity_name)/sizeof(char*);
    for(i = 0; i < count; i++)
    {
        if(0 == strncmp(name, thiz_verity_name[i], sizeof(thiz_verity_name[i])))
        {
            return 1;
        }
    }
    return 0;
}

int cdcas_verify_sign(char *buf, unsigned int size)
{
    int ret = 0;
    char *p = malloc(size);
    if(p)
    {
        // partion struct: image data + signature(256Byte) + head v3(16Byte)
        // CDCA need struct: signature(256Byte) + head v3(16Bytes) + image data
        memcpy(p, (buf + size - 272), 272);
        memcpy((p + 272), buf, (size - 272));
#if 0
        {
            int i = 0;
            printf("\033[36morg data\n");
            for(i = 0; i < size; i++)
            {
                printf("%02x ", buf[i]);
                if(((i+1)%16) == 0)
                    printf("\n");
            }
            printf("\033[0m");
            printf("\n\033[35mmodify data\n");
            for(i = 0; i < size; i++)
            {
                printf("%02x ", p[i]);
                if(((i+1)%16) == 0)
                    printf("\n");
            }
            printf("\033[0m");

        }
#endif
        printf("\nCA VERIFY, size = %x, before: %02x, %02x, %02x, %02x; after: %02x, %02x, %02x, %02x\n", size, buf[0], buf[1], buf[2], buf[3], p[0], p[1], p[2], p[3]);
        if(RE_OK != (ret = CDCA_Verify_One((const unsigned char*)p, size)))
        {
            free(p);
            printf("\n\033[34mVerify failed, %s, %d, ret = %d\033[0m\n", __func__, __LINE__, ret);
            return -1;
        }
        else
        {
            free(p);
            printf("\n\033[34mVerify success, %s, %d\033[0m\n", __func__, __LINE__);
            return 0;
        }
    }
    return -1;
}
