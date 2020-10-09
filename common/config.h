#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string.h>
#include "registers.h"

using namespace std; 

//CABINETTYPE:作为区分机柜类型，用于编译不同的代码
//CABINETTYPE  1：华为（包括华为单门 双门等） 5：中兴; 6：金晟安; 7：爱特斯; 8:诺龙; 9：容尊堡; 
			//10:亚邦; 11：艾特网能；12：华软；13：利通
#define CABINETTYPE  1

int GetConfig(void);
int WriteNetconfig(char *configbuf,int configlen);
int WriteNetconfig2(char *configbuf,int configlen);
int OnSpdSetconfigBack(string StrKEY,string StrSetconfig,unsigned int mRetID);
int OnSpdSetWriteconfig(unsigned int mRetID);


#endif
