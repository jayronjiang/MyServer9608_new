#ifndef __CANPORT_H__
#define __CANPORT_H__

#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/raw.h>
#include <stdint.h>


// 定义平常的波特率
#define CAN_1000K	1000000
#define CAN_500K	500000
#define CAN_250K	250000
#define CAN_125K	125000
#define CAN_50K		50000


#define CAN0_UP 	"ifconfig can0 up"//打开CAN0
#define CAN0_DOWN 	"ifconfig can0 down"//关闭CAN0
#define CAN0_RATE1000K 	"/sbin/ip link set can0 type can bitrate 1000000"//将CAN0波特率设置为1M bps
#define CAN0_RATE500K 	"/sbin/ip link set can0 type can bitrate 500000"//将CAN0波特率设置为500000 bps
#define CAN0_RATE250K 	"/sbin/ip link set can0 type can bitrate 250000"//将CAN0波特率设置为500000 bps
#define CAN0_RATE125K 	"/sbin/ip link set can0 type can bitrate 125000"//将CAN0波特率设置为125000 bps
#define CAN0_RATE50K 	"/sbin/ip link set can0 type can bitrate 50000"//将CAN0波特率设置为50000 bps


#define CAN1_UP 	"ifconfig can1 up"//打开CAN1
#define CAN1_DOWN 	"ifconfig can1 down"//关闭CAN1
#define CAN1_RATE1000K 	"/sbin/ip link set can1 type can bitrate 1000000"//将CAN1波特率设置为1M bps
#define CAN1_RATE500K 	"/sbin/ip link set can1 type can bitrate 500000"//将CAN1波特率设置为500000 bps
#define CAN1_RATE250K 	"/sbin/ip link set can1 type can bitrate 250000"//将CAN1波特率设置为500000 bps
#define CAN1_RATE125K 	"/sbin/ip link set can1 type can bitrate 125000"//将CAN1波特率设置为125000 bps
#define CAN1_RATE50K 	"/sbin/ip link set can1 type can bitrate 50000"//将CAN1波特率设置为50000 bps

#define FILTER_NUM		2		// 目前最多使用2组过滤器

class CCanPort
{
public:
		// 未指定任何参数，构造对象后需要设置filter,loopback,并open一次
	   CCanPort(void);
	   // 一次性需要can口名称,波特率,过滤器组:0/1,过滤器id，过滤器掩码, 回环设置:0:关闭，1:打开
       CCanPort(char *cCanName,int Baud,int filter_id,uint32_t can_id, uint32_t can_mask,int loop_back);
       ~CCanPort(void);
      
       int openCan(char *cCanName,int Baud);
	   int closeCan(void);
       int Send(unsigned char *buf,int len);
	   void filterSet(int filter_id,uint32_t can_id, uint32_t can_mask);

       int fd ;
       unsigned short int canlen;
	   unsigned char *m_canbuf;
       unsigned char RecvStart ;
	   
	   struct can_filter filter[FILTER_NUM];	// CAN过滤器,2组
	   int loopback; 				// 回环功能 表示关闭, 1 表示开启( 默认)
};

#endif
