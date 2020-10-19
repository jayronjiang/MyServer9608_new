#ifndef __INITMODULE_h__
#define __INITMODULE_h__

#include <string>
#include <stdio.h>
#include <stdlib.h>
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
#include "lock.h"
#include "hw_locker.h"
#include "tem_humi.h"
#include "camera.h"
#include "air_condition.h"
#include "io_dev.h"
#include "uart.h"
#include "http.h"
#include "moninterface.h"
#include "camera.h"
#include "http.h"

using namespace std;//引入整个名空间


void initHUAWEIGantry(CabinetClient *pCab);
void initHUAWEIALARM(CabinetClient *pCab);
void initHUAWEIEntity(CfirewallClient *pfw);
void initHUAWEIswitchEntity(CswitchClient *psw);
void init_atlas_struct(CsshClient *pAtlas);
void Init_CANNode(CANNode *pCan);
void init_SPD(SpdClient *pSpd, VMCONTROL_CONFIG *pConf);//初始化SPD
void Init_DO(VMCONTROL_CONFIG *pConf);
int TrapCallBack(string Stroid,int AlarmID,int mgetIndex,unsigned int mRetID);
void HuaweiCallback(void *data, void *userdata);
void LockCallback(uint8_t addr, Lock::Info_S info, void *userdata);
void HWLockCallback(uint8_t addr, HWLock::Info_S info, void *userdata); 
void UartCallback(uint8_t port, uint8_t *buf, uint32_t len, void *userdata);
void AirConditionCallback(AirCondition::AirInfo_S info, void *userdata);
void TemHumiCallback(uint8_t addr,TemHumi::Info_S info,void *userdata);
void IODevCallback(IODev::DevType_EN type,bool sta,void *userdata);
void IPCamCallback(string staCode,string msg,IPCam::State_S state,void *userdata);
void RsuCallback(void *data, void *userdata); 
void canNodeCallback(void *p,void *data,int len);
void MoninterCallback(Moninterface::State_S state,void *userdata);
void initAirConditionInfo(AirCondition::AirInfo_S *Info);
void CameraCallback(Camera::MsgType_EN msg,const char *jpgName,void *userdata);
//void HttpCallback(Http *http, Http::Value_S value, void *userdata);

#endif

