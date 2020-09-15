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
#define TO_STR(x) #x
#define STR(x) TO_STR(x)

#endif