#ifndef __C_GPIO_H__
#define __C_GPIO_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>


#define GPIO_IN  0
#define GPIO_OUT 1

#define GPIO_EDGE_NONE		0
#define GPIO_EDGE_RISING	1
#define GPIO_EDGE_FAILING	2
#define GPIO_EDGE_BOTH		3


// PIN值的算法是 GPIO组别*32+pin,比如GPIO3.20,则定义：
// #define GPIO_320	116
class CCGPIO
{
public:
	CCGPIO(void);
	CCGPIO(int pin,int dir,int edge);
	~CCGPIO(void);

	// 回调函数
	typedef void (*Callback)(void *p,void *data,int flag);
	void setCallback(Callback cb,void *userdata);

	// 公共函数声明,初始化设置的函数
	int gpio_export(int pin);
	int gpio_unexport(int pin);
	int gpio_direction(int pin, int dir);
	int gpio_edge(int pin, int edge);
	
	// 外部主要调用这2个函数
	int gpio_write(int pin, int value);
	int gpio_read(int pin);
      
	int gpio_fd;
	int pinname;
	// 返回的值
	bool fliped;			// 变位标志位
	uint32_t timestamp;		// 变位时间戳
	int value;
private:
	pthread_t gtid;
	Callback callback;
	void *userdata;
	static void *GPIOPollThread(void *arg);

	struct pollfd fds[1];
};

#endif

