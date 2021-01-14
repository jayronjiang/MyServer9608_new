#ifndef __CFIREWALLCLIENT_H__
#define __CFIREWALLCLIENT_H__

#include <string> 
#include <pthread.h>
#include "WalkClient.h"

using namespace std; 


    typedef enum {
        //防火墙
        hwEntityCpuUCheck = 10000,   //查询
        hwEntityCpuUsage = 10001,    // CPU
        hwEntityMemUsage = 10002,    //内存使用率
        hwEntityTemperature = 10003, //温度
        hwEntityDescr = 10004,       //接口查询
        hwEntityOperStatus = 10005,  //接口状态查询
        hwEntityInOctets = 10006,    //总字节数
        hwEntityInErrors = 10007,    //出错数
        hwEntityOutOctets = 10008,   //总字节数
        hwEntityOutErrors = 10009,   //出错数
        hwEntityDevModel = 10010,    //型号

        //迪普防火墙
        dpEntityCpuUCheck = 12000,   //查询
        dpEntityCpuUsage = 12001,    // CPU
        dpEntityMemUsage = 12002,    //内存使用率
        dpEntityTemperature = 12003, //温度
        dpEntityDescr = 12004,       //接口查询
        dpEntityOperStatus = 12005,  //接口状态查询
        dpEntityInOctets = 12006,    //总字节数
        dpEntityInErrors = 12007,    //出错数
        dpEntityOutOctets = 12008,   //总字节数
        dpEntityOutErrors = 12009,   //出错数
        dpEntityDevModel = 12010,    //型号

        //深信服防火墙
        sfEntityCpuUCheck = 14000,   //查询
        sfEntityCpuUsage = 14001,    // CPU
        sfEntityMemUsage = 14002,    //内存使用率
        sfEntityTemperature = 14003, //温度
        sfEntityDescr = 14004,       //接口查询
        sfEntityOperStatus = 14005,  //接口状态查询
        sfEntityInOctets = 14006,    //总字节数
        sfEntityInErrors = 14007,    //出错数
        sfEntityOutOctets = 14008,   //总字节数
        sfEntityOutErrors = 14009,   //出错数
        sfEntityDevModel = 14010,    //型号

        //山石网科防火墙(HillStone) 2020-12-04新增
        hsEntityCpuUCheck = 16000,   //查询
        hsEntityCpuUsage = 16001,    // CPU
        hsEntityMemUsage = 16002,    //内存使用率
        hsEntityTemperature = 16003, //温度
        hsEntityDescr = 16004,       //接口查询
        hsEntityOperStatus = 16005,  //接口状态查询
        hsEntityInOctets = 16006,    //总字节数
        hsEntityInErrors = 16007,    //出错数
        hsEntityOutOctets = 16008,   //总字节数
        hsEntityOutErrors = 16009,   //出错数
        hsEntityDevModel = 16010,    //型号
		
		//中兴防火墙(HillStone) 2020-01-04新增
        zxEntityCpuUCheck = 17000,   //查询
        zxEntityCpuUsage = 17001,    // CPU
        zxEntityMemUsage = 17002,    //内存使用率
        zxEntityTemperature = 17003, //温度
        zxEntityDescr = 17004,       //接口查询
        zxEntityOperStatus = 17005,  //接口状态查询
        zxEntityInOctets = 17006,    //总字节数
        zxEntityInErrors = 17007,    //出错数
        zxEntityOutOctets = 17008,   //总字节数
        zxEntityOutErrors = 17009,   //出错数
        zxEntityDevModel = 17010,    //型号
		
    } EntityType_EN;
	
	
	    typedef struct {

        //防火墙
        unsigned long hwEntityTimeStamp;        //防火墙获取时间戳
        bool hwEntityLinked;                    //连接状态
        string strhwEntityCpuUsage;             // CPU
        string strhwEntityMemUsage;             //内存使用率
        string strhwEntityTemperature;          //温度
        unsigned long hwEntityTimeStamp1;       //防火墙获取时间戳
        bool hwEntityLinked1;                   //连接状态
        string strhwEntityCpuUsage1;            // CPU
        string strhwEntityMemUsage1;            //内存使用率
        string strhwEntityTemperature1;         //温度
                                                //交换机
        unsigned long hwswitchEntityTimeStamp;  //交换机获取时间戳
        bool hwswitchEntityLinked;              //连接状态
        string strhwswitchEntityCpuUsage;       // CPU
        string strhwswitchEntityMemUsage;       //内存使用率
        string strhwswitchEntityTemperature;    //温度
        unsigned long hwswitchEntityTimeStamp1; //交换机获取时间戳
        bool hwswitchEntityLinked1;             //连接状态
        string strhwswitchEntityCpuUsage1;      // CPU
        string strhwswitchEntityMemUsage1;      //内存使用率
        string strhwswitchEntityTemperature1;   //温度

        string strhwEntityFactory;   //生产商
        string strhwEntityDevModel;  //设备型号
        string strhwEntityFactory1;  //生产商
        string strhwEntityDevModel1; //设备型号

        string strhwswitchEntityFactory;   //生产商
        string strhwswitchEntityDevModel;  //设备型号
        string strhwswitchEntityFactory1;  //生产商
        string strhwswitchEntityDevModel1; //设备型号

    } Value_S;
	


    typedef struct {
        int Descr;
        string type;
        string state;
        string inoctets;
        string inerrors;
        string outoctets;
        string outerrors;
    } Data_S;
	
	
class CfirewallClient
{
	
public:
       CfirewallClient(void);
       ~CfirewallClient(void);
        
       int SetIntervalTime(int mSetIntervalTime);
       int SendOid(int entityType, int nIndex);
       int Start(void);

private: 
       pthread_t tid; 
       void run(void);
       static void *CfirewallSendthread(void *args);
       static int OnfirewallRecvBack(int mgetindx, string getsp, string getspInt, int Intstrtype, string mstrip, unsigned int mRetID) ;
     
public:
       CWalkClient mCWalkClient;
       pthread_mutex_t dataMutex ;
       pthread_mutex_t IntervalTimeMutex ;
       int mIntervalTime ;     

       string StrHWServer;
       string StrHWGetPasswd ;
       string StrHWSetPasswd ;

       Value_S HUAWEIDevValue;
	   
	   
	int IntFireWareType; //防火墙类型 1：华为,2：迪普，3：深信服 ,4:山石网科
       int IntIPSwitchType; //交换机类型  1：华为,2：华三
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
       // TFIRESWITCH mTFIRESWITCH[32];
       // TFIRESWITCH mTFIRESWITCH1[32];
       // string strswitchjson;
       // string strswitchjson1;

       //防火墙网络数据
       Data_S mTFIREWALL[32];
       Data_S mTFIREWALL1[32];
       string strfirewalljson;
       string strfirewalljson1;



  
       void myprintf(char* str);
       void WriteLog(char* str);
       int SendWalkSnmpOid_IPSwitch(string mSnmpOid);

       

};




















#endif



