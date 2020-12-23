#ifndef __CFIREWALLCLIENT_H__
#define __CFIREWALLCLIENT_H__

#include <string> 
#include <pthread.h>
#include "snmp.h"
#include "WalkClient.h"

using namespace std; 


class CfirewallClient
{
public:
       CfirewallClient(void);
       ~CfirewallClient(void);
        
       int SetIntervalTime(int mSetIntervalTime);
       int SendOid(EM_HUAWEIGantry mEM_HUAWEIGantry,int nIndex);
       int Start(void);

private: 
       pthread_t tid; 
       void run(void);
       static void *CfirewallSendthread(void *args);
       
     
public:
       CWalkClient mCWalkClient;
       pthread_mutex_t dataMutex ;
       pthread_mutex_t IntervalTimeMutex ;
       int mIntervalTime ;     

       string StrHWServer;
       string StrHWGetPasswd ;
       string StrHWSetPasswd ;

       THUAWEIGantry HUAWEIDevValue;
       THUAWEIALARM HUAWEIDevAlarm;
	   
	   
	   int IntFireWareType = 1; //防火墙类型 1：华为,2：迪普，3：深信服 ,4:山石网科
       int IntIPSwitchType = 1; //交换机类型  1：华为,2：华三
       string StrFireWareCount;	//防火墙数量
       string StrFireWareIP[4];         //防火墙IP
       string StrFireWareGetPasswd[4];  //防火墙get密码
       string StrFireWareSetPasswd[4];  //防火墙set密码
       string StrIPSwitchCount;	//交换机数量
       string StrIPSwitchIP[4] ;//交换机IP
       string StrIPSwitchGetPasswd[4] ;//交换机get密码
       string StrIPSwitchSetPasswd[4] ;//交换机set密码
       string StrDeviceNameSeq[4];	//设备名的配置

       //交换机网络数据
       TFIRESWITCH mTFIRESWITCH[32];
       TFIRESWITCH mTFIRESWITCH1[32];
       string strswitchjson = "";
       string strswitchjson1 = "";
       //防火墙网络数据
       TFIRESWITCH mTFIREWALL[32];
       TFIRESWITCH mTFIREWALL1[32];
       string strfirewalljson = "";
       string strfirewalljson1 = "";



  
       void myprintf(char* str);
       void WriteLog(char* str);
       int SendWalkSnmpOid_IPSwitch(string mSnmpOid);

       

};




















#endif



