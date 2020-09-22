#ifndef __DEBUG_H__
#define __DEBUG_H__

// 打印宏定义
#define __DEBUG

#ifdef __DEBUG
	#define DEBUG_PRINTF(...)  printf(__VA_ARGS__)
#else
	#define DEBUG_PRINTF(...)
#endif


// 字符串宏定义

#define STR(x) 		#x 
#define TO_STR(x) STR(x)

#endif