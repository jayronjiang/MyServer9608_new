//___base_64.cpp
/*base_64.cpp文件*/
#include <iostream>
#include <string>
#include <cstring>
#include "base_64.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>


using namespace std;

std::string Base64::Encode(const unsigned char * str,int bytes) {
    int num = 0,bin = 0,i;
    std::string _encode_result;
    const unsigned char * current;
    current = str;
    while(bytes > 2) {
        _encode_result += _base64_table[current[0] >> 2];
        _encode_result += _base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
        _encode_result += _base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
        _encode_result += _base64_table[current[2] & 0x3f];

        current += 3;
        bytes -= 3;
    }
    if(bytes > 0)
    {
        _encode_result += _base64_table[current[0] >> 2];
        if(bytes%3 == 1) {
            _encode_result += _base64_table[(current[0] & 0x03) << 4];
            _encode_result += "==";
        } else if(bytes%3 == 2) {
            _encode_result += _base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
            _encode_result += _base64_table[(current[1] & 0x0f) << 2];
            _encode_result += "=";
        }
    }
    return _encode_result;
}


// 这个和类里面的解码函数的区别是，这个是输出HEX串,类里那个是输出string,用处不同
void base64_2_Hex_decode(const char *code, int lpBufLen,unsigned char *outstr)
{
//根据base64表，以字符找到对应的十进制数据
    int table[]={0,0,0,0,0,0,0,0,0,0,0,0,
             0,0,0,0,0,0,0,0,0,0,0,0,
             0,0,0,0,0,0,0,0,0,0,0,0,
             0,0,0,0,0,0,0,62,0,0,0,
             63,52,53,54,55,56,57,58,
             59,60,61,0,0,0,0,0,0,0,0,
             1,2,3,4,5,6,7,8,9,10,11,12,
             13,14,15,16,17,18,19,20,21,
             22,23,24,25,0,0,0,0,0,0,26,
             27,28,29,30,31,32,33,34,35,
             36,37,38,39,40,41,42,43,44,
             45,46,47,48,49,50,51
               };
    long len;
    long str_len;
    unsigned char *res = outstr;
    int i,j;

	//计算解码后的字符串长度
    len=strlen((char*)code);
	//printf("code_len = %d\n\r",len);
	//判断编码后的字符串后是否有=
    if(strstr((char*)code,"=="))
        {str_len=len/4*3-2;}
    else if(strstr((char*)code,"="))	// =就是结束符
        {str_len=len/4*3-1;}
    else
        {str_len=len/4*3;}

	// 改成形参后这句没意义了
    if (NULL != lpBufLen) {
        lpBufLen = str_len;
    }

    //res=(unsigned char*)malloc(sizeof(unsigned char)*str_len+1);//不能每次分配而不释放

	if (len < BASE64_HEX_LEN)	// 防止异常操作
	{
		//以4个字符为一位进行解码
	    for(i=0,j=0;i < len-2;j+=3,i+=4)
	    {
	        res[j]=((unsigned char)table[code[i]])<<2 | (((unsigned char)table[code[i+1]])>>4); //取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的后2位进行组合
	        res[j+1]=(((unsigned char)table[code[i+1]])<<4) | (((unsigned char)table[code[i+2]])>>2); //取出第二个字符对应base64表的十进制数的后4位与第三个字符对应bas464表的十进制数的后4位进行组合
	        res[j+2]=(((unsigned char)table[code[i+2]])<<6) | ((unsigned char)table[code[i+3]]); //取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合
			//printf("resdata1 = %02x resdata2 = %02x resdata3 = %02x\n\r",res[j],res[j+1],res[j+2]);
		}
		res[str_len]='\0';
	}
}

std::string Base64::Decode(const char *str,int length) {
       //解码表
    const char DecodeTable[] =
    {
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
        -2,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
        -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
    };
    int bin = 0,i=0,pos=0;
    std::string _decode_result;
    const char *current = str;
    char ch;
	char temp[100]= {0,0,0,};
    while( (ch = *current++) != '\0' && length-- > 0 )
    {
        if (ch == base64_pad) { // 当前一个字符是“=”号
            /*
            先说明一个概念：在解码时，4个字符为一组进行一轮字符匹配。
            两个条件：
                1、如果某一轮匹配的第二个是“=”且第三个字符不是“=”，说明这个带解析字符串不合法，直接返回空
                2、如果当前“=”不是第二个字符，且后面的字符只包含空白符，则说明这个这个条件合法，可以继续。
            */
            if (*current != '=' && (i % 4) == 1) {
                return NULL;
            }
            continue;
        }
        ch = DecodeTable[ch];
        //这个很重要，用来过滤所有不合法的字符
        if (ch < 0 ) { /* a space or some other separator character, we simply skip over */
            continue;
        }
        switch(i % 4)
        {
            case 0:
                bin = ch << 2;
                break;
            case 1:
                bin |= ch >> 4;
                //_decode_result += bin;
                sprintf(temp,"%02x",bin);
				_decode_result += temp;
                bin = ( ch & 0x0f ) << 4;
                break;
            case 2:
                bin |= ch >> 2;
                //_decode_result += bin;
                sprintf(temp,"%02x",bin);
				_decode_result += temp;
                bin = ( ch & 0x03 ) << 6;
                break;
            case 3:
                bin |= ch;
				sprintf(temp,"%02x",bin);
				_decode_result += temp;
                //_decode_result += bin;
                break;
        }
        i++;
    }
    return _decode_result;
}




//int main()
int Base64Init(void)
{
    unsigned char str[] = "这是一条测试数据：http://img.v.cmcm.com/7dd6e6d7-0c2d-5a58-a072-71f828b94cbc_crop_216x150.jpg";
    string normal,encoded;
    int i,len = sizeof(str)-1;
    Base64 *base = new Base64();
    encoded = base->Encode(str,len);
    cout << "base64 encode : " << encoded << endl;
    len = encoded.length();
    const char * str2 = encoded.c_str();
    normal = base->Decode(str2,len);
    cout << "base64 decode : " << normal <<endl;
    return 0;
}









