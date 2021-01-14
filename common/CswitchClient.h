#ifndef __CSWITCHCLIENT_H__
#define __CSWITCHCLIENT_H__

#include <string> 
#include <pthread.h>
//#include "snmp.h"
#include "WalkClient.h"

using namespace std; 


typedef struct
{
  int Descr;
  string type;
  string state;
  string inoctets;
  string inerrors;
  string outoctets;
  string outerrors;


}TSWITCH;


#pragma pack(push, 1)

typedef struct
{
    //交换机
	unsigned long hwswitchEntityTimeStamp; 		//交换机获取时间戳
	bool hwswitchEntityLinked;					//连接状态
    string strhwswitchEntityCpuUsage;          //CPU 
    string strhwswitchEntityMemUsage;          //内存使用率
    string strhwswitchEntityTemperature;       //温度
	unsigned long hwswitchEntityTimeStamp1; 		//交换机获取时间戳
	bool hwswitchEntityLinked1;					//连接状态
    string strhwswitchEntityCpuUsage1;          //CPU
    string strhwswitchEntityMemUsage1;          //内存使用率
    string strhwswitchEntityTemperature1;       //温度

    string strhwEntityFactory;                	//生产商
    string strhwEntityDevModel;                //设备型号
    string strhwEntityFactory1;                	//生产商
    string strhwEntityDevModel1;                //设备型号

    string strhwswitchEntityFactory;            //生产商
    string strhwswitchEntityDevModel;           //设备型号
    string strhwswitchEntityFactory1;            //生产商
    string strhwswitchEntityDevModel1;           //设备型号
	
}TSWITCHVALUE;

#pragma pack(pop)


class CswitchClient
{
public:
       CswitchClient(void);
       ~CswitchClient(void);
        
       int SetIntervalTime(int mSetIntervalTime);
       int SendOid(int mEM_HUAWEIGantry,int nIndex);
       int Start(void);

private: 
       pthread_t tid; 
       void run(void);
       static void *Sendthread(void *args);
       
     
public:
       CWalkClient mCWalkClient;
       pthread_mutex_t dataMutex ;
       pthread_mutex_t IntervalTimeMutex ;
       int mIntervalTime ;     

       string StrHWServer;
       string StrHWGetPasswd ;
       string StrHWSetPasswd ;

       TSWITCHVALUE HUAWEIDevValue;
       //THUAWEIGantry HUAWEIDevValue;
       //THUAWEIALARM HUAWEIDevAlarm;
	   
	   
	   int IntFireWareType = 1; //防火墙类型 1：华为,2：迪普，3：深信服
       int IntIPSwitchType = 1; //交换机类型  1：华为,2：华三 3：三旺(2020-10-22新增) 4:奥德梅斯 5:中兴捷迅(2021-1-5新增)
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
       TSWITCH mTFIRESWITCH[32];
       TSWITCH mTFIRESWITCH1[32];
       string strswitchjson = "";
       string strswitchjson1 = "";
       //防火墙网络数据
       TSWITCH mTFIREWALL[32];
       TSWITCH mTFIREWALL1[32];
       string strfirewalljson = "";
       string strfirewalljson1 = "";
  
       void myprintf(char* str);
       void WriteLog(char* str);
       int SendWalkSnmpOid_IPSwitch(string mSnmpOid);

       

};




















#endif



