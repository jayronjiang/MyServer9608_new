#ifndef __CANNODE_H__
#define __CANNODE_H__

#include <stdint.h>
#include <string>
#include <pthread.h>
#include "tea.h"
#include "canport.h"
#include "arpa/inet.h"

using namespace std;


#define PHASE_MAX_NUM	16
#define CLOSED			0xFF01		// 这里CLOSED是断开
#define OPEN			0xFF00

// DO控制
#define DO_SET			0xA0
#define A_SET			0xA1	// 过流阈值设置

#define CAN_DISC_TIMEOUT	(3)	// 3s钟未收到信息，则判断为断线

/*电源 控制定义*/
typedef enum
{
	POWER_1_CTRL_CLOSE = 0,		//0,分闸
	POWER_1_CTRL_OPEN,			// 合闸
	POWER_2_CTRL_CLOSE,
	POWER_2_CTRL_OPEN,
	POWER_3_CTRL_CLOSE,
	POWER_3_CTRL_OPEN,
	POWER_4_CTRL_CLOSE,
	POWER_4_CTRL_OPEN,
	POWER_5_CTRL_CLOSE,
	POWER_5_CTRL_OPEN,
	POWER_6_CTRL_CLOSE,
	POWER_6_CTRL_OPEN,

	POWER_7_CTRL_CLOSE,		// 12
	POWER_7_CTRL_OPEN,
	POWER_8_CTRL_CLOSE,
	POWER_8_CTRL_OPEN,
	POWER_9_CTRL_CLOSE,
	POWER_9_CTRL_OPEN,
	POWER_10_CTRL_CLOSE,
	POWER_10_CTRL_OPEN,
	POWER_11_CTRL_CLOSE,
	POWER_11_CTRL_OPEN,
	POWER_12_CTRL_CLOSE,
	POWER_12_CTRL_OPEN,

	POWER_13_CTRL_CLOSE,	// 24
	POWER_13_CTRL_OPEN,
	POWER_14_CTRL_CLOSE,
	POWER_14_CTRL_OPEN,
	POWER_15_CTRL_CLOSE,
	POWER_15_CTRL_OPEN,
	POWER_16_CTRL_CLOSE,
	POWER_16_CTRL_OPEN,

	POWER_CTRL_NUM			//32
}POWER_CTRL_LIST;


class CANNode
{
public:
	typedef struct va_protocol_struct
	{
		uint8_t flag;
		int8_t temp;
		float vln;		// 电压
		float amp;		// 电流
		uint16_t alarm_threshold;	// 过流阈值
		string version;
	}VA_PROTOCOL_PARAMS;
	typedef struct va_meter_struct
	{
		uint16_t address;
		VA_PROTOCOL_PARAMS phase;
		uint64_t TimeStamp;		//获取时间戳
		bool isConnect;
	}VA_METER_PARAMS;

public:
	// 未指定任何参数，构造对象后需要设置filter,loopback,并open一次
	CANNode(void);
	// 一次性需要can口名称,波特率,过滤器组:0/1,过滤器id，过滤器掩码, 回环设置:0:关闭，1:打开
	CANNode(char *cCanName,int Baud,int filter_id,uint32_t can_id, uint32_t can_mask,int loop_back);
	void CANThreadNew(void);
	~CANNode(void);

	// 回调函数
	typedef void (*Callback)(void *p,void *data,int len);
	void setCallback(Callback cb,void *userdata);

	// 设置指定地址的DO控制字
	void canContrlFlagSet(uint16_t seq,uint16_t type);
	// 清除指定地址的DO控制字
	void canContrlFlagClear(uint16_t seq,uint16_t type);
	// 设置指定地址的过流阈值控制字
	void oaFlagSet(uint16_t seq);
	// 清除指定地址的过流阈值控制字
	void oaFlagClear(uint16_t seq);
	void oaAlarmValueSet(uint16_t seq,uint16_t value);
	string int8_to_string(uint8_t data);
	
	
	CCanPort *mCanPort = NULL;	// 需要调用CAN口
	VA_METER_PARAMS canNode[PHASE_MAX_NUM];

	// 这3个参数也可以放在private里面
	uint64_t can_control_flag;
	uint32_t oa_set_flag;	// 16路过流阈值
	uint16_t oa_alarm_value[PHASE_MAX_NUM];
	   
private:
	pthread_t gtid;
	pthread_t stid;
	/* 状态获取时间戳 */
	Callback callback;
	void *userdata;

	// 这些参数构造函数要初始化
	uint64_t timestamp[PHASE_MAX_NUM];
	CMyCritical ComCri;

private:
	static void *NetCanNodeGetThread(void *arg);
	static void *NetCanNodeSendThread(void *arg);

	void disconnctProcess(void);
	void handle_err_frame(const struct can_frame *fr);
	int DealCanData(can_frame *c_data);
	void CanDataPack(uint16_t id,uint16_t type,can_frame *s_data, uint16_t cmd);
	int can_ctrl_process(uint64_t *pctrl_flag);
	int can_oaset_process(uint32_t *pctrl_flag);
	void print_frame(struct can_frame *fr);
};

#endif

