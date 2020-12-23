#ifndef __CABINETCLIENT_H__
#define __CABINETCLIENT_H__

#include <string> 
#include <pthread.h>
#include "snmp.h"
#include "WalkClient.h"
#include "supervision_hr.h"


using namespace std; 

//华软的盈创力和的动环
typedef enum
{
	hrtemperatureHighThreshold=30000,  //温度报警上限
	hrtemperatureLowThreshold=30001,   //温度报警下限
	hrEnvTemperature=30002,             //环境温度值
	hrhumidityHighThreshold=30003,      //湿度报警上限
	hrhumidityLowThreshold=30004,       //湿度报警下限
	hrEnvHumidity=30005,                //环境湿度值
    hrSmokeSensorStatus=30006,			//烟雾传感器状态
	hrWaterSensorStatus=30007,			//水浸传感器状态
	hrDoor1SensorStatus=30008,			//前门磁传感器状态
	hrDoor2SensorStatus=30009			//后门磁传感器状态
	
}EM_HRGantry;

class CabinetClient
{
public:
       CabinetClient(void);
       ~CabinetClient(void);
        
       int SetIntervalTime(int mSetIntervalTime);
       int SendCabinetOid(int mEM_HUAWEIGantry,int nIndex,string mStrHWServer,string mStrHWGetPasswd);
       int Start(void);

private: 

       pthread_t tid; 
	   pthread_mutex_t SetsnmpoidMutex ;
       void hwCabinetrun(void);
	   void hrCabinetrun(void);
       static void *CabinetSendthread(void *args);
	   

     
public:
       int mCabinetType; //机柜类型 1:华为, 2: 华软
       CWalkClient mCWalkClient;
       pthread_mutex_t CabinetdataMutex ;
       pthread_mutex_t IntervalTimeMutex ;
       int mIntervalTime ;     

       string StrHWServer;
       string StrHWGetPasswd ;
       string StrHWSetPasswd ;
	   
	   //华软的空调和门锁
	   SupervisionHR *pSupervisionHR;
	   
	   //电源和电池的动环IP,只有华软的电源柜才需要设置,其它的柜不要设置
	   string StrPowerServer;
       string StrPowerGetPasswd ;
       string StrPowerSetPasswd ;

       THUAWEIGantry HUAWEIDevValue;
       THUAWEIALARM HUAWEIDevAlarm;
       //空调地址
       unsigned int hwAirAddrbuf[2];
       //温适度地址
       unsigned int hwTemAddrbuf[2];
       //电池地址
       unsigned int hwAcbAddrbuf[10] = {0,0,0,0,0,0,0,0,0,0};
       unsigned char mAcbIndex = 0 ;

       int sethwDcAirPowerOffTempPointCtlIndex[2];
       int sethwDcAirPowerOnTempPointCtlIndex[2];
       int sethwSetEnvTempUpperLimitIndex[2];
       int sethwSetEnvTempLowerLimitIndex[2];
       int sethwSetEnvHumidityUpperLimitIndex[2];
       int sethwSetEnvHumidityLowerLimitIndex[2];
       int sethwDcAirCtrlModeIndex[2];
       int sethwCoolingDevicesModeIndex;
       int sethwCtrlSmokeResetIndex;
       int sethwSetAcsUpperVoltLimitIndex;
       int sethwSetAcsLowerVoltLimitIndex;
       int sethwSetDcsUpperVoltLimitIndex;
       int sethwSetDcsLowerVoltLimitIndex;
       int sethwCtrlMonEquipResetIndex;

       unsigned long GetTickCount() ;
       void myprintf(char* str);
       void WriteLog(char* str);
       int SendWalkSnmpOid_IPSwitch(string mSnmpOid);
	   int SnmpSetOid(EM_HUAWEIGantry mEM_HUAWEIGantry,string mIntValue,int mIndex);

       

};



typedef int (*TrapAlarmBack)(string Stroid,int AlarmID,int mgetIndex,unsigned int mRetID);


class CTrapClient
{
public:
       CTrapClient(CabinetClient *pmCabinetClient);
       ~CTrapClient(void);
	   
	   unsigned int m_RetID;
	   TrapAlarmBack m_TrapAlarmBack;
	   int SetTrapAlarmBack(TrapAlarmBack mTrapAlarmBack,unsigned int mretID);
       int Start(void);
       int Trapsnmp_input(int op, void *vsession, int reqid, void *vpdu, void *magic);
	   
private: 

       CabinetClient *pCabinetClient;
	   int GetAlarmID(char* sp);
	   int DealAlarm(string Stroid,int AlarmID,int mgetIndex);
	   void initHUAWEIALARM(void);
	   
	   int netsnmp_running ;
	   static void *snmptrapthread(void*arg);
	   void *snmptrapthreadRun(void);
	   void snmptrapd_main_loop(void); 
	   void *snmptrapd_add_session(void *t);
	   
};




#endif



