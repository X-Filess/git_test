
extern int cdcas_check_need_verify(char *name);
extern int cdcas_verify_sign(char *buf, unsigned int size);

int ca_check_need_verify(char *name)
{
    int ret = 0;
    if(name)
    {
        ret = cdcas_check_need_verify(name);
    }
    return ret;
}

int ca_verify_sign(char *buf, unsigned int size)
{
    int ret = 0;
    if((buf) && (size > 0))
    {
        ret = cdcas_verify_sign(buf, size);
    }
    return ret;
}

