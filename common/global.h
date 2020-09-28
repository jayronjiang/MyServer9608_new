#ifndef __GLOBAL_h__
#define __GLOBAL_h__

#include <string>
#include <time.h>
#include "CabinetClient.h"
#include "CfirewallClient.h"
#include "CswitchClient.h"
#include "tsPanel.h"
#include "ipcam.h"
#include "CsshClient.h"
#include "comport.h"
#include "canNode.h"
#include "server.h"
#include "SpdClient.h"
#include "rsu.h"

using namespace std;//引入整个名空间

#define LOCKER_CLOSED			0
#define LOCKER_OPEN				1

// 设备是否断电的阈值
#define POWER_DOWN_VALUE		(24.0f)

#define BOX_NULL_VALUE  2147483647

void IPGetFromDevice(uint8_t seq,char *ip);

// 取CAN控制板的电压值
float voltTrueGetFromDev(uint8_t seq);

// 取CAN控制板的电压值
float ampTrueGetFromDev(uint8_t seq);

// 该机柜还是在线?
// seq: 0:设备柜
// 		1:电池柜
uint16_t linkStGetFrombox(uint8_t seq);

// 该设备离线还是在线?
// 1：在线，2：离线
uint16_t linkStGetFromDevice(uint8_t seq);

// 该设备离线还是在线?
// 1：有电，2：没电
uint16_t powerStGetFromDevice(uint8_t seq);

// 该通道是否配置?
bool isChannelNull(uint8_t seq);

// 单柜还是双柜?返回柜子个数
uint8_t getBoxNum(void);

// 从配置文件中读取CAN控制器版本号,只读地址为1的控制器
string getCanVersion(uint8_t seq);

// 获取机柜n的IP地址
string IPGetFromBox(uint8_t seq);

// 获取该机柜是离线还是在线?
string linkStringGetFromBox(uint8_t seq);

void GetIPinfo(IPInfo *ipInfo);

void GetIPinfo2(IPInfo *ipInfo);

// 取8个字节的ID号
unsigned long long IDgetFromConfig(void);

// seq:DO序号, DeviceName：返回的设备名称，ip:返回的ip地址,port:返回的端口号
void IPgetFromDevice(uint8_t seq,string& DeviceName, string& ip, string& port);

void VAgetFromDevice(uint8_t seq, string& volt, string& amp);

uint16_t DoorStatusFromLocker(void);
extern unsigned long GetTickCount(); //返回秒



void initHUAWEIGantry(CabinetClient *pCab);
void initHUAWEIALARM(CabinetClient *pCab);
void initHUAWEIEntity(CfirewallClient *pfw);
void initHUAWEIswitchEntity(CswitchClient *psw);
void init_atlas_struct(CsshClient *pAtlas);
void Init_CANNode(CANNode *pCan);
void init_SPD(SpdClient *pSpd, VMCONTROL_CONFIG *pConf);//初始化SPD
void Init_DO(VMCONTROL_CONFIG *pConf);
int TrapCallBack(string Stroid,int AlarmID,int mgetIndex,unsigned int mRetID);


#endif
