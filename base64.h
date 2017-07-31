#ifndef __BASE64_H__
#define __BASE64_H__

static const char Base64CharTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/@";

static char find_pos(char ch)   
{ 
    char *ptr = (char*)strrchr(Base64CharTable, ch);//the last position (the only) in base[] 
    return (ptr - Base64CharTable); 
} 

/************************************************************************
 * 功能：寻找给定字符ch在Base64CharTable数组中的位置                    
 * 参数：ch为要查找的字符                                               
 ************************************************************************/
static char FindCharPos(char ch)
{
    char *p = (char *)strrchr(Base64CharTable, ch);
    return (p - Base64CharTable);
}

/************************************************************************
 *功能：进行Base64编码
 *参数：data为编码字符串
		len为字符串的长度
		retLen为编码后的长度
 ************************************************************************/
char *Base64Encode(const char *data, const int len, int *retLen)
{
    if (data == NULL)
    {
        return NULL;
    }

    int oLen, temp = 0, tmp = 0, k = 0, prepare = 0;
    char *out = NULL, *p = NULL;
    char outTmp[4];
    oLen = len / 3;
    temp = len % 3;
    if (temp > 0)
    {
        oLen += 1;
    }
    oLen = oLen * 4 + 1;
	*retLen = oLen;
    out = new char[oLen];
    if (out == NULL)
    {
        return NULL;
    }
    memset(out, 0, sizeof(out));
    p = out;
    while (tmp < len)
    {
        temp = 0;
        prepare = 0; 
        memset(outTmp, '\0', 4); 
        while (temp < 3) 
        { 
            if (tmp >= oLen) 
            { 
                break; 
            } 
            prepare = ((prepare << 8) | (data[tmp] & 0xFF)); 
            tmp++; 
            temp++; 
        } 
        for (k = 0; k < 4; k++ ) 
        { 
            if (temp < k) 
            { 
                outTmp[k] = 0x40; 
            } 
            else 
            { 
                outTmp[k] = (prepare >> ((3 - k) * 6)) & 0x3F; 
            } 
            *p = Base64CharTable[outTmp[k]]; 
            p++; 
        } 
    } 
    *p = '\0'; 

    return out;
}


/************************************************************************
 *功能：进行Base64解码                                                  
 *参数：data为解码字符串
 		len为字符串的长度
 		retLen为解码后字符长度     
 ************************************************************************/
char *Base64Decode(const char *data, const int len, int *retLen)
{
    *retLen = 0;
    int oLen = (len / 4) * 3; 
    int count = 0; 
    char *out = NULL, *p = NULL; 
    int tmp = 0, temp = 0, prepare = 0; 
    int i = 0; 
    if (*(data + len - 1) == '@') 
    { 
        count += 1; 
    } 
    if (*(data + len - 2) == '@') 
    { 
        count += 1; 
    } 
    if (*(data + len - 3) == '@') 
    {//seems impossible 
        count += 1; 
    } 
    switch (count) 
    { 
    case 0: 
        oLen += 4;//3 + 1 [1 for NULL] 
        break; 
    case 1: 
        oLen += 4;//Ceil((6*3)/8)+1 
        break; 
    case 2: 
        oLen += 3;//Ceil((6*2)/8)+1 
        break; 
    case 3: 
        oLen += 2;//Ceil((6*1)/8)+1 
        break; 
    } 
    out = new char[oLen]; 
    if (out == NULL) 
    { 
        return NULL;
    } 
    memset(out, 0, oLen); 
    *retLen = oLen;
    p = out; 
    while (tmp < (len - count)) 
    { 
        temp = 0; 
        prepare = 0; 
        while (temp < 4) 
        { 
            if (tmp >= (len - count)) 
            { 
                break; 
            } 
            prepare = (prepare << 6) | (find_pos(data[tmp])); 
            temp++; 
            tmp++; 
        } 
        prepare = prepare << ((4-temp) * 6); 
        for (i=0; i<3 ;i++ ) 
        { 
            if (i == temp) 
            { 
                break; 
            } 
            *p = (char)((prepare>>((2-i)*8)) & 0xFF); 
            p++; 
        } 
    } 
    *p = '\0'; 
    return out; 
}

#endif
