#ifndef __TEA_h__
#define __TEA_h__

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include "MyCritical.h"
#include "global.h"

/*结构联合声明短整型*/
typedef union int_union
{
	uint16_t i;
	uint8_t b[2];
}INTEGER_UNION;

/*结构联合声明长整型*/
typedef union long_union
{
	uint32_t i;
	uint8_t b[4];
}LONG_UNION;
#define BIT(i)	(1<<(i))	// 32位平台32位

#define LBIT(i)	(1ULL<<(i))	// 要注意32位平台LONG也是32位
#define BITS_MSK_GET(bitsoff, bits) (((0x01ULL<<(bits))-1) <<(bitsoff)) // 获取从bitsoff 开始共bits位全1掩码


//CRC
unsigned short GetCrc(unsigned char *buf,int len) ;
//加密函数
void encrypt (uint32_t* v) ;
//解密函数
void decrypt (uint32_t* v) ;

unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen);
void CalulateCRCbySoft(unsigned char *pucData,unsigned char wLength,unsigned char *pOutData);
unsigned char CRC_sum(unsigned char *puchMsg , unsigned short usDataLen);
int GbkToUtf8(char *str_str, size_t src_len, char *dst_str, size_t dst_len);
int Utf8ToGbk(char *src_str, size_t src_len, char *dst_str, size_t dst_len);
UINT32 timestamp_get(void);
UINT32 timestamp_delta(UINT32 const timestamp);


#endif
