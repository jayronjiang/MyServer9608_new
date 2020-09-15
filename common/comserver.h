#ifndef __COMSERVER_H__
#define __COMSERVER_H__

#include "modbus.h"
#include "registers.h"
#include "jsonPackage.h"
//#include "HttpPost.h"

#define FRAME_HEAD_1		0x5A
#define FRAME_HEAD_2		0xA5
#define CMD_WRITE			0x82
#define CMD_READ			0x83
// 数据变量的地址先定义为0xDFF0, 是IP地址, 加上门架号,4个字,从0xDFEC~0xDFEF
// 温度1 ->0xDFE8,2->0xDFE9,湿度1->0xDFEA,湿度2->0xDFEB
#define VAR_REG_ADD			0xDFE8
#define TIME_REG_ADD		0x0010
#define LETTER_REG_ADD		0xB1C0
// 文本形式的FLAGID,增加一个新的地址，从而兼容以前老的触摸屏代码
#define FLAGID_CHAR_REG_ADD		0xB300


#define MSG_SEND_INTERVAL	400000		//400ms
#define WAIT_SECOND_2		(2000000/MSG_SEND_INTERVAL)
#define WAIT_SECOND_4		(4000000/MSG_SEND_INTERVAL)		// 不用错开,同时置位下一个周期会处理
#define WAIT_SECOND_5		(5000000/MSG_SEND_INTERVAL)		// 不用错开,同时置位下一个周期会处理
#define WAIT_SECOND_6		(6000000/MSG_SEND_INTERVAL)



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
#define WRITE_LETTER_MSG		0x08		// 修改前面的G/S
#define WRITE_FLAGID_CHAR_MSG		0x09	// 修改文本形式的门架号



#define SYS_CFG_ADDR			0x80
#define LED_CFG_ADDR			0x82

#define LIGHT_100_ON			100		// 背光亮度100%
#define LIGHT_0_OFF				0		// 背光亮度0%
#define SLEEP_ENTER_TIM			3000	// 进入背光时间,3000*10ms=30s


// 232轮询设置枚举,可扩展
typedef enum
{
	SCREEN_TIME_SET = 0,
	VAR_SET,
	BACKLIGHT_EN_SET,	// 使能背光休眠
	BACKLIGHT_CFG_SET,	// 设置背光休眠时间
	BACKLIGHT_ON_SET,	// 打开背光
	BACKLIGHT_OFF_SET,	// 关闭背光
	ERR_CHECK,
	LETTER_SET,
	FLAG_STRING_SET,	// 文本式的门架号ID

	SCREEN_SET_NUM
}SREEN_SET_LIST;



//消息等待枚举
typedef enum
{
	WAIT_COM_NONE = 0,
	WAIT_TIME_MSG,
	WAIT_VAR_MSG,
	WAIT_BACKLIGHT_EN_MSG,
	WAIT_BACKLIGHT_CFG_MSG,
	WAIT_BACKLIGHT_ON_MSG,
	WAIT_BACKLIGHT_OFF_MSG,
	WAIT_LETTER_MSG,
	WAIT_FLAGID_STRING_MSG,

	WAIT_MSG_COM_NUM
}WAIT_MSG_COM_LIST;



void cominit(void) ;

//向串口屏发送数据
int SendCom2WriteReg(UINT16 Addr, UINT8 Func);
UINT8 message_pack(UINT16 address,UINT8 msg_type,UINT8 *buf);
void ScreenFlagSet(SREEN_SET_LIST sFlag);


#endif
