#include <iostream>
#include <cstring>
#include <map>
#include <stdio.h>
#include "registers.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include "snmp.h"
#include "rsu.h"
#include "cJSON.h"
#include "CabinetClient.h"
#include "lt_state_thread.h"
#include "CsshClient.h"
#include "CfirewallClient.h"
#include "CswitchClient.h"
#include "ipcam.h"
#include "global.h"
#include "canNode.h"
#include "SpdClient.h"

bool SetjsonAuthorityStr(int messagetype,char *json, int *len);//0 登录控制器
void SetjsonIPStr(int messagetype,string &mstrjson);//5 读取/设置网口1
bool jsonstrIpInfoReader(char* jsonstr, int len);//8 IP地址
//bool jsonStrSpdWriter(int messagetype,char *pstrUpsPam, char *json, int *len);//11 防雷器寄存器参数
bool jsonStrVMCtlParamWriter(int messagetype,char *pstrDevInfo, string &mstrjson);//12 控制器参数->JSON字符串
bool jsonStrVMCtlParamWriterXY(int messagetype,char *pstrVMCtl, string &mstrjson);//12 控制器参数->JSON字符串(新粤)
bool jsonstrVmCtlParamReader(char* jsonstr, int len, UINT8 *pstPam);//12 JSON字符串->控制器参数
bool jsonstrVmCtlParamReaderXY(char* jsonstr, int len, UINT8 *pstPam);//12 JSON字符串->JSON字符串(新粤)
bool jsonStrRsuWriterXY(int messagetype, string &mstrjson);//14 RSU天线参数
void SetjsonIPSwitchStatusStr(int messagetype,string &mstrjson);//16交换机状态参数
bool jsonStrVehPlateWriter(int messagetype, string &mstrjson);//15 车牌识别仪参数
bool jsonstrRCtrlReader(char* jsonstr, int len, UINT8 *pstuRCtrl);//18 json解析到结构体
//void SetjsonTableStr(char* table, string &mstrjson);
//bool jsonstrRCtrlReader(string jsonstr, UINT8 *pstuRCtrl);//18 json解析到结构体
bool jsonStrSwitchStatusWriter(int messagetype, string &mstrjson);	//19回路电压电流开关状态
bool jsonStrSwitchStatusWriterXY(int messagetype, string &mstrjson);	//19回路电压电流开关状态
bool jsonStrHWCabinetWriter(int messagetype,char *pstPam, string &mstrjson);				//20 华为机柜状态
void SetjsonDealLockerStr(int messagetype,UINT32 cardid,UINT16 lockaddr,string &mstrjson);		//22 门禁开关锁请求					//22 门禁开关锁请求
void SetjsonDealLockerStr64(int messagetype,UINT64 cardid,UINT16 lockaddr,string &mstrjson);
void SetjsonFireWallStatusStr(int messagetype,string &mstrjson);			//23防火墙状态
void SetjsonAtlasStatusStr(int messagetype,string &mstrjson);	//24 ATLAS状态
bool jsonStrReader(char* jsonstrin, int lenin, char* jsonstrout, int *lenout);
bool SetjsonReceiveOKStr(int messagetype,char *json, int *len);
bool jsonIPCamReader(char* jsonstr, int len);
bool jsonIPCam900Reader(char* jsonstr, int len,int mIndex);
void SetjsonIP2Str(int messagetype,string &mstrjson);//25 读取/设置网口2
void SetjsonSpdAIStatusStr(int messagetype,string &mstrjson);	//27 防雷器参数
bool jsonstrSPDReader(char* jsonstr, int len, UINT8 *pstuRCtrl);//27防雷器参数json解析到结构体
void SetjsonSpdResStatusStr(int messagetype,string &mstrjson);//28 接地电阻参数
bool jsonStrVehPlate900Writer(int messagetype, string &mstrjson);	//29 300万全景车牌识别参数
bool jsonStrVMCtrlStateWriter(int messagetype, string &mstrjson);//30控制器运行状态


