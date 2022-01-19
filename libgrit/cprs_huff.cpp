#include "cprs.h"

extern "C"
{
    #include "cue_huffman.h"
}

uint huffgba_compress(RECORD *dst, const RECORD *src, int)
{
    if(dst==NULL || src==NULL || src->data==NULL)
        return 0;

    int srcS= src->width*src->height;
    int dstS;
    unsigned char *result= HUF_Code(src->data, srcS, &dstS, CMD_CODE_20);

    if(result==NULL)
        return 0;

    dst->data= (BYTE*)malloc(dstS);
    memcpy(dst->data, result, dstS);
    free(result);

    dst->width= 1;
    dst->height= dstS;

    return dstS;
}
