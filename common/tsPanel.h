#ifndef __TS_PANEL_H__
#define __TS_PANEL_H__

#include <stdint.h>
#include <string>
#include <pthread.h>
#include "tea.h"
#include "comport.h"
#include "arpa/inet.h"
#include "global.h"
#include "CabinetClient.h"
#include "registers.h"



using namespace std;

class tsPanel
{

public:
// 轮询枚举,可扩展
	typedef enum
	{
		SCREEN_TIME_SET = 0,
		BACKLIGHT_EN_SET,	// 使能背光休眠
		BACKLIGHT_CFG_SET,	// 设置背光休眠时间
		BACKLIGHT_ON_SET,	// 打开背光
		BACKLIGHT_OFF_SET,	// 关闭背光
		VAR_SET,
		FLAG_STRING_SET,	// 文本式的门架号ID
		DEV_PAGE_1_0_SET,
		DEV_PAGE_1_1_SET,
		DEV_PAGE_2_0_SET,
		DEV_PAGE_2_1_SET,
		DEV_PAGE_3_0_SET,
		DEV_PAGE_3_1_SET,
		DEV_PAGE_4_0_SET,
		DEV_PAGE_4_1_SET,

		SCREEN_SET_NUM
	}SREEN_SET_LIST;

#if 0
	typedef struct
	{
	    char ip[20];
	    char submask[20];
	    char gateway_addr[20];
	    char dns[20];
	}IPInfo;
#endif
public:
	tsPanel(CabinetClient *pCab,VMCONTROL_CONFIG *pConfig);
	~tsPanel(void);

	typedef void (*Callback)(void *userdata,int len);
	void setCallback(Callback cb,void *userdata);
	void ScreenFlagSet(SREEN_SET_LIST sFlag);
	void disconnctProcess(void);
	void CabClientSet(CabinetClient *pCab);
	void VM_ConfigSet(VMCONTROL_CONFIG *pConfig);
	
	CComPort *mComPort;	// 需要调用串口
	CMyCritical ComCri;
	bool isConnect;
	in_addr IPaddr[2];
	uint16_t time_interval;
	// 初始化要传参
	CabinetClient *tsCabClient;//华为机柜状态
	VMCONTROL_CONFIG *tsVM_Config;	//控制器配置信息结构体
	   
private:
	pthread_t gtid;
	pthread_t stid;
	/* 状态获取时间戳 */
	Callback callback;
	void *userdata;

	// 这些参数构造函数要初始化
	uint64_t timestamp;
	uint32_t screen_poll_flag;

private:
    static void *NetPanelGetThread(void *arg);
    static void *NetPanelSendThread(void *arg);
	uint8_t tsPanelBoxValuePack(uint8_t *pbuf,uint16_t addr);
	void IP1getFromConfig(void);
	void IP2getFromConfig(void);
	uint16_t Get_3PartString(char *pbuf,string str_init,uint16_t TEXT_LEN);
	uint16_t Get_AsciiString(char *pbuf,string str_init,uint16_t TEXT_LEN);
	uint8_t linkValueTrans(bool linked);
	string tsPanelDevSeqFunc(uint8_t seq);
	uint8_t tsPanelVarMsgStringPack(uint8_t *pbuf,uint16_t addr);
	uint8_t tsPanelRTCStringPack(uint8_t *pbuf,uint16_t addr);
	uint8_t tsPanelBackLightEnablePack(uint8_t *pbuf,uint16_t addr);
	uint8_t tsPanelBackLightCfgPack(uint8_t *pbuf,uint16_t addr);
	uint8_t tsPanelBackLightOnPack(uint8_t *pbuf,uint16_t addr);
	uint8_t tsPanelBackLightOffPack(uint8_t *pbuf,uint16_t addr);
	uint8_t tsPanelDevListPack(uint8_t page,uint8_t *pbuf,uint16_t addr);
	void backlightLinkedToLocker(void);
	uint16_t GetSpaceString(char *pbuf,string str_init,uint16_t TEXT_LEN,uint16_t n_space);
	uint8_t tsMessagePack(uint16_t address,uint8_t msg_type,uint8_t *buf);
	void TouchScreenWrite(uint16_t Addr, uint8_t Func);
};

// 断线时间判断

#define FRAME_HEAD_1		0x5A
#define FRAME_HEAD_2		0xA5
#define CMD_WRITE			0x82
#define CMD_READ			0x83
// 数据变量的地址先定义为0xE000, 是IP地址, 2个IP地址加设备ID
#define VAR_REG_ADD			0xB000
#define TIME_REG_ADD		0x009C
//#define LETTER_REG_ADD		0xB1C0
// 文本形式的FLAGID,增加一个新的地址，从而兼容以前老的触摸屏代码
#define FLAGID_BOX_REG_ADD		0xC000


#define DEV_PAGE_REG_ADD		0xD000



#define MSG_SEND_INTERVAL	400000		//400ms
#define WAIT_SECOND_2		(2000000/MSG_SEND_INTERVAL)
#define WAIT_SECOND_3		(3000000/MSG_SEND_INTERVAL)		// 不用错开,同时置位下一个周期会处理
#define WAIT_SECOND_4		(4000000/MSG_SEND_INTERVAL)		// 不用错开,同时置位下一个周期会处理
#define WAIT_SECOND_5		(5000000/MSG_SEND_INTERVAL)		// 不用错开,同时置位下一个周期会处理
#define WAIT_SECOND_6		(6000000/MSG_SEND_INTERVAL)
#define WAIT_SECOND_20		(20000000/MSG_SEND_INTERVAL)
#define WAIT_SECOND_30		(30000000/MSG_SEND_INTERVAL)


#define PANEL_DISC_TIMEOUT	(15)		// 15s


/*写数据的回复buf的顺序定义*/
// 5A A5 03 82 4F 4B
#define BUF_HEAD1			0
#define BUF_HEAD2			1
#define BUF_LENTH			2
#define BUF_CMD				3

// 信息打包类型
#define NOT_USED_MSG		0xFF		// 未使用
#define TRANS_MSG			0x00		// 透传命令
#define WRITE_VAR_MSG		0x01		// 写命令
#define WRITE_TIME_MSG		0x02		// 写时间命令
#define READ_MSG			0x03		// 读命令
#define BACKLIGHT_EN_MSG		0x04	// 背光使能
#define BACKLIGHT_CFG_MSG		0x05	// 背光配置
#define BACKLIGHT_ON_MSG		0x06	// 背光打开
#define BACKLIGHT_OFF_MSG		0x07	// 背光关闭
//#define WRITE_LETTER_MSG		0x08		// 修改前面的G/S
#define WRITE_FLAG_BOX_MSG		0x09	// 修改文本形式的门架号

#define WRITE_DEV_PAGE1_0_MSG		0x10	// 更新设备列表信息
#define WRITE_DEV_PAGE1_1_MSG		0x11	// 更新设备列表信息
#define WRITE_DEV_PAGE2_0_MSG		0x12	// 更新设备列表信息
#define WRITE_DEV_PAGE2_1_MSG		0x13	// 更新设备列表信息
#define WRITE_DEV_PAGE3_0_MSG		0x14	// 更新设备列表信息
#define WRITE_DEV_PAGE3_1_MSG		0x15	// 更新设备列表信息
#define WRITE_DEV_PAGE4_0_MSG		0x16	// 更新设备列表信息
#define WRITE_DEV_PAGE4_1_MSG		0x17	// 更新设备列表信息



#define SYS_CFG_ADDR			0x80
#define LED_CFG_ADDR			0x82

#define LIGHT_100_ON			100		// 背光亮度100%
#define LIGHT_0_OFF				0		// 背光亮度0%
#define SLEEP_ENTER_TIM			3000	// 进入背光时间,3000*10ms=30s

#define MAX_CHANNEL				16	// 最大支持16路的通道显示

#endif

