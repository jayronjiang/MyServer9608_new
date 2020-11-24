#ifndef __CABINETCLIENT_H__
#define __CABINETCLIENT_H__

#include <string> 
#include <pthread.h>
#include "snmp.h"
#include "WalkClient.h"

using namespace std; 


#pragma pack(push, 1)
class CabinetClient
{
public:
       CabinetClient(void);
       ~CabinetClient(void);
        
       int SetIntervalTime(int mSetIntervalTime);
       int SendCabinetOid(EM_HUAWEIGantry mEM_HUAWEIGantry,int nIndex);
       int Start(void);

private: 

       pthread_t tid; 
	   pthread_mutex_t SetsnmpoidMutex ;
       void Cabinetrun(void);
       static void *CabinetSendthread(void *args);
	   

     
public:
       CWalkClient mCWalkClient;
       pthread_mutex_t CabinetdataMutex ;
       pthread_mutex_t IntervalTimeMutex ;
       int mIntervalTime ;     

       string StrHWServer;
       string StrHWGetPasswd ;
       string StrHWSetPasswd ;

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

#pragma pack(pop)



#endif



