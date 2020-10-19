
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <termios.h>
#include <errno.h>   
#include <limits.h> 
#include <asm/ioctls.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
//#include "comport.h"
//#include "comserver.h"
#include "MyCritical.h"
#include <string>
#include <semaphore.h>  
//#include "Protocol.h"
#include "config.h"
#include "registers.h"
#include "snmp.h"
#include "CabinetClient.h"
#include "lt_state_thread.h"


using namespace std;//寮??ユ?翠釜??绌洪??
CMyCritical ConfigCri;

REMOTE_CONTROL *stuRemote_Ctrl = NULL;	//遥控寄存器结构体
CabinetClient *pCabinetClient[HWSERVER_NUM] = {NULL,NULL};//华为机柜状态
CTrapClient *pCTrapClient = NULL;//华为告警状态
VMCONTROL_STATE VMCtl_State;	//控制器运行状态结构体
VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体

int Writeconfig(void);
int Setconfig(string StrKEY,string StrSetconfig) ;
int WriteWificonfig(void);
int SetWificonfig(string StrKEY,string StrSetconfig);

string getstring(string str,string strkey)
{
  string strget = "";
  int lenpos ;
  lenpos = str.find(strkey) ;
  if(lenpos >= 0)
  {
      str = str.substr(lenpos+strkey.length()) ; 
  }
  else
     return strget ;

  lenpos = str.find('\n') ;
  if(lenpos >= 0)
  {
      str = str.substr(0,lenpos) ; 
      strget = str ;
  }
  else
     return strget ;
  

  return strget ;
}


int GetConfig(void)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
	//CABINETTYPE:作为区分机柜类型，用于编译不同的代码
	//CABINETTYPE  1：华为（包括华为单门 双门等） 5：中兴; 6：金晟安; 7：爱特斯; 8:诺龙; 9：容尊堡; 
				//10:亚邦; 11：艾特网能；12：华软
    #if(CABINETTYPE == 1) //华为
	   pConf->StrVersionNo ="V99.01.02" ;//当前版本号
	   pConf->StrSoftDate="2020-03-31" ;	//当前版本日期
    #elif (CABINETTYPE == 5) //飞达中兴
       pConf->StrVersionNo ="V1.05.25b" ;
   	   pConf->StrSoftDate="2020-07-13" ;	 //当前版本日期
    #elif (CABINETTYPE == 6) //金晟安
       pConf->StrVersionNo ="V1.06.26a" ;//当前版本号 
	   pConf->StrSoftDate="2020-04-13" ;	  //当前版本日期
    #elif (CABINETTYPE == 7) //爱特斯
       pConf->StrVersionNo ="V1.07.22" ;//当前版本号
	   pConf->StrSoftDate="2020-01-09" ;	//当前版本日期
	#elif (CABINETTYPE == 8 || CABINETTYPE == 10) //诺龙/亚邦
	   pConf->StrVersionNo ="V1.08.27" ;//当前版本号
	   pConf->StrSoftDate="2020-07-13" ;   //当前版本日期
	#elif (CABINETTYPE == 9) //容尊
	   pConf->StrVersionNo ="V1.09.26a" ;//当前版本号
	   pConf->StrSoftDate="2020-05-04" ;   //当前版本日期
	#elif (CABINETTYPE == 11) //艾特网能
	   pConf->StrVersionNo ="V1.11.25" ;//当前版本号
	   pConf->StrSoftDate="2020-07-09" ;   //当前版本日期
    #endif


    int i,j,vehplatecnt,vehplate900cnt,rsucnt;
	char key[128],value[50],devicename[128];
	char *strbuf; 
	string strvalue;
	int isize;
	char stripbuf[1501];
    char stripbuf2[1501];

    char strwifibuf[1001]; 
    memset(strwifibuf,0x00,1001) ;

	pConf->user="admin";			//登录用户
	pConf->password="ltyjy";		//密码
	pConf->authority="4";		//用户权限
	pConf->token="mjkzq2019";			//票据
	
    pConf->StrID  = "" ;			//硬件ID
	pConf->StrstationID  = "" ;	//虚拟站编号
	pConf->StrstationName  = "" ;	//虚拟站名

	//参数设置
    pConf->StrNET = "" ;			//网络方式
    pConf->StrDHCP = "";			//是否DHCP
    pConf->StrIP  = "" ;			//IP地址
    pConf->StrMask = "" ;			//子网掩码
    pConf->StrGateway = "";		//网关
    pConf->StrDNS = "";			//DNS地址
    pConf->StrHWServer = "";		//华为服务器地址
	pConf->StrHWGetPasswd = "";	//SNMP GET 密码
	pConf->StrHWSetPasswd = "";	//SNMP SET 密码
	pConf->StrHWServer2 = "";		//金晟安服务器地址2
	pConf->StrHWGetPasswd2 = ""; //金晟安 SNMP GET 密码2
	pConf->StrHWSetPasswd2 = ""; //金晟安SNMP SET 密码2
	pConf->StrServerURL1 = "";		//服务端URL1
	pConf->StrServerURL2 = "";		//服务端URL2
	pConf->StrServerURL3 = "";		//服务端URL3
	pConf->StrServerURL4 = "";		//服务端URL4
	pConf->StrStationURL = "";		//虚拟站端URL
	pConf->StrRSUCount = ""; 		//RSU数量
	pConf->StrRSUType = ""; 		//RSU类型
	for(i=0;i<RSUCTL_NUM;i++)
	{
		pConf->StrRSUIP[i] = "" ;			//RSU IP 地址
		pConf->StrRSUPort[i] = "" ;		//RSU端口
	}
	pConf->StrVehPlateCount = "" ;	//识别仪数量
	for(i=0;i<VEHPLATE_NUM;i++)
	{
		pConf->StrVehPlateIP[i] = "";	//识别仪1IP地址
		pConf->StrVehPlatePort[i] = "";	//识别仪1端口
		pConf->StrVehPlateKey[i] = "";	//识别仪用户名密码
	}
	pConf->StrVehPlate900Count = "" ;	//900识别仪数量
	for(i=0;i<VEHPLATE900_NUM;i++)
	{
		pConf->StrVehPlate900IP[i] = "";	//900识别仪1IP地址
		pConf->StrVehPlate900Port[i] = "";	//900识别仪1端口
		pConf->StrVehPlate900Key[i] = "";	//900识别仪用户名密码
	}
	pConf->StrCAMCount = "" ;	//监控摄像头数量
	for(i=0;i<CAM_NUM;i++)
	{
		pConf->StrCAMIP[i] = "";			//监控摄像头IP地址
		pConf->StrCAMPort[i] = "";		//监控摄像头端口
		pConf->StrCAMKey[i] = ""; //监控摄像头用户名密码
	}

	pConf->StrCabinetType="";		//机柜类型

	pConf->StrFireWareCount = "" ;	//防火墙数量
	for(i=0;i<FIREWARE_NUM;i++)
	{
		pConf->StrFireWareIP[i]="";		  //防火墙IP
		pConf->StrFireWareGetPasswd[i]="";  //防火墙get密码
		pConf->StrFireWareSetPasswd[i]="";  //防火墙set密码
	}
	pConf->StrIPSwitchCount = "" ;	//交换机数量
	for(i=0;i<IPSWITCH_NUM;i++)
	{
		pConf->StrIPSwitchIP[i] ="";//交换机IP
		pConf->StrIPSwitchGetPasswd[i] ="";//交换机get密码
		pConf->StrIPSwitchSetPasswd[i] ="";//交换机set密码
	}
	pConf->StrAtlasType = "" ;	//Atlas类型
	pConf->StrAtlasCount = "" ;	//Atlas数量
	for(i=0;i<ATLAS_NUM;i++)
	{
		pConf->StrAtlasIP[i] =""; //AtlasIP
		pConf->StrAtlasPasswd[i] ="";//Atlas密码
	}
	pConf->StrSPDType = "" ; //防雷器类型
	pConf->StrSPDCount = "" ; //防雷器数量
	for (i = 0; i < (SPD_NUM+RES_NUM); i++)
	{
		pConf->StrSPDIP[i] =""; ;//防雷器IP
		pConf->StrSPDPort[i] ="";//防雷器端口
		pConf->StrSPDAddr[i] ="";//防雷器硬件端口
	}
	pConf->StrSpdRes_Alarm_Value ="";		//接地电阻监测器报警值
	
	for (i = 0; i < SWITCH_COUNT; i++)
	{
		sprintf(key,"do%d_do",i+1);
		pConf->StrDeviceNameSeq[i]=key; //设备名
		sprintf(key,"%d",i+1);
		pConf->StrDoSeq[i] = key; 	//对应DO
		pConf->StrPoweroff_Or_Not[i] = "0";	//市电停电时是否断电
		
		//pConf->StrUnWireDevName[i] = "";
		//pConf->StrUnWireDo[i] = "";
		sprintf(key,"do%d_do",100+i+1);
		pConf->StrUnWireDevName[i] = key;
		sprintf(key,"%d",100+i+1);
		pConf->StrUnWireDo[i] = key;
	}
	
	//门架信息
	pConf->StrFlagNetRoadID = "";	//ETC 门架路网编号
	pConf->StrFlagRoadID = "";		//ETC 门架路段编号
	pConf->StrFlagID = "";			//ETC 门架编号
	pConf->StrPosId = "";			//ETC 门架序号
	pConf->StrDirection = "";		//行车方向
	pConf->StrDirDescription = "";	//行车方向说明
	
//	pConf->StrdeviceType = "";		//设备型号
//	pConf->StrVersionNo = "" ;		//主程序版本号
    pConf->StrWIFIUSER = "";		//WIIFI用户名
    pConf->StrWIFIKEY = "" ;		//WIIFI密码

    pConf->StrLockType = "";		//门锁类型
    pConf->StrLockNum = "" ;		//门锁数量
	for (i = 0; i < LOCK_NUM; i++)
	{
		pConf->StrAdrrLock[i] = "" ;			//门锁地址
	}
	for (i = 0; i < VA_METER_BD_NUM; i++)
	{
		pConf->StrAdrrVAMeter[i] = "" ;		//电压电流传感器地址
	}
	for (i = 0; i < POWER_BD_NUM; i++)
	{
		pConf->StrAdrrPower[i] = "" ;			//电源板的地址
	}
	pConf->StrDoCount="";						//DO 数量

	pConf->StrCpuAlarmValue="20";		//CPU使用率报警阈值
	pConf->StrCpuTempAlarmValue="70";	//CPU温度报警阈值
	pConf->StrMemAlarmValue="80";		//内存使用率报警阈值
	
    ConfigCri.Lock();
    //read config
    FILE* fdd ;
    if((fdd=fopen("/opt/config", "rb"))==NULL)
    {
        printf("read config erro\r\n");
        ConfigCri.UnLock();
        return 1;
    }
	fseek(fdd,0,SEEK_END);
	isize = ftell(fdd);
	strbuf=(char*)malloc(isize);
	fseek(fdd,0,SEEK_SET);
    fread(strbuf,1,isize,fdd);
    fclose(fdd);


    FILE* ipfd ;
    if((ipfd=fopen("/home/root/net/netconfig", "rb"))==NULL)
    {
        printf("read config erro\r\n");
        //ConfigCri.UnLock();
        //return 1;
    }
    else
    {
        fread(stripbuf,1,1500,ipfd);
        fclose(ipfd);
    }


    FILE* ipfd2 ;
    if((ipfd2=fopen("/home/root/net/netconfig2", "rb"))==NULL)
    {
        printf("read config erro\r\n");
        //ConfigCri.UnLock();
       // return 1;
    }
    else
    {
       fread(stripbuf2,1,1500,ipfd2);
       fclose(ipfd2);
    }

    printf("Version:%s\n",pConf->StrVersionNo.c_str()) ;

//    printf("-----config----\n%s\n----end config----\n",strbuf) ;
    pConf->STRCONFIG = strbuf ;

    printf("-----netconfig----\n%s\n----end netconfig----\n",stripbuf) ;
    pConf->STRWIFIWAP = strwifibuf ;
    
    printf("-----netconfig2----\n%s\n----end netconfig2----\n",stripbuf2) ;
  

	string Strkey ;

//	netconfig 读取
	string StrIpConfig = stripbuf;
	Strkey = "IP=";
	pConf->StrIP = getstring(StrIpConfig,Strkey) ;

	Strkey = "Mask=";
	pConf->StrMask = getstring(StrIpConfig,Strkey) ;

	Strkey = "Gateway=";
	pConf->StrGateway = getstring(StrIpConfig,Strkey) ;

	Strkey = "DNS=";
	pConf->StrDNS = getstring(StrIpConfig,Strkey) ;

 //end	  netconfig 读取


//	netconfig2 读取
	string StrIpConfig2 = stripbuf2;
	Strkey = "IP=";
	pConf->StrIP2 = getstring(StrIpConfig2,Strkey) ;

	Strkey = "Mask=";
	pConf->StrMask2 = getstring(StrIpConfig2,Strkey) ;

	Strkey = "Gateway=";
	pConf->StrGateway2 = getstring(StrIpConfig2,Strkey) ;

	Strkey = "DNS=";
	pConf->StrDNS2 = getstring(StrIpConfig2,Strkey) ;

 //end    netconfig2 读取


    string StrConfig = pConf->STRCONFIG ;
    Strkey = "ID=";
    pConf->StrID = getstring(StrConfig,Strkey) ;
	
    Strkey = "CabinetType=";
    pConf->StrCabinetType = getstring(StrConfig,Strkey) ;//机柜类型
	//CABINETTYPE  1：华为（包括华为单门 双门等） 5：中兴; 6：金晟安(双动环); 7：爱特斯; 8:诺龙; 9：容尊堡; 
				//10:亚邦; 11：艾特网能(双动环)；12：华软
	if(pConf->StrCabinetType=="6" || pConf->StrCabinetType=="11")
		pConf->StrHWServerCount = "2";
	else 
	{
		Strkey = "HWServerCount=";
		pConf->StrHWServerCount = getstring(StrConfig,Strkey) ;
		if(pConf->StrHWServerCount =="")
			pConf->StrHWServerCount = "1";
	}	
	if(pConf->StrCabinetType=="3" || pConf->StrCabinetType=="4")
		pConf->StrHWCabinetCount="2";
	else
		pConf->StrHWCabinetCount="1";
    Strkey = "HWServer=";
    pConf->StrHWServer = getstring(StrConfig,Strkey) ;

    Strkey = "HWGetPasswd=";
    pConf->StrHWGetPasswd = getstring(StrConfig,Strkey) ;//SNMP GET 密码

    Strkey = "HWSetPasswd=";
    pConf->StrHWSetPasswd = getstring(StrConfig,Strkey) ;//SNMP SET 密码
    
    Strkey = "HWServer2=";
    pConf->StrHWServer2 = getstring(StrConfig,Strkey) ;//金晟安服务器地址2

    Strkey = "HWGetPasswd2=";
    pConf->StrHWGetPasswd2 = getstring(StrConfig,Strkey) ;//金晟安 SNMP GET 密码

    Strkey = "HWSetPasswd2=";
    pConf->StrHWSetPasswd2 = getstring(StrConfig,Strkey) ;//金晟安 SNMP SET 密码*/

    Strkey = "ServerURL1=";
    pConf->StrServerURL1 = getstring(StrConfig,Strkey) ;

    Strkey = "ServerURL2=";
    pConf->StrServerURL2 = getstring(StrConfig,Strkey) ;

    Strkey = "ServerURL3=";
    pConf->StrServerURL3 = getstring(StrConfig,Strkey) ;

    Strkey = "ServerURL4=";
    pConf->StrServerURL4 = getstring(StrConfig,Strkey);

    Strkey = "StationURL=";
    pConf->StrStationURL = getstring(StrConfig,Strkey) ;//虚拟站端URL

    Strkey = "RSUCount=";
    pConf->StrRSUCount = getstring(StrConfig,Strkey) ;//RSU数量
    if(pConf->StrRSUCount=="")
		pConf->StrRSUCount="0";
	pConf->RSUCount=rsucnt=atoi(pConf->StrRSUCount.c_str());
	if(rsucnt>RSUCTL_NUM)
	{
		sprintf(value,"%d", RSUCTL_NUM) ;
		pConf->StrRSUCount=value;
		rsucnt=RSUCTL_NUM;
	}
	else if(rsucnt<0)
	{
		pConf->StrRSUCount="0";
		rsucnt=0;
	}
    Strkey = "RSUType=";
    pConf->StrRSUType = getstring(StrConfig,Strkey) ;//RSU类型
    if(pConf->StrRSUType=="")
		pConf->StrRSUType="1";

	for(i=0;i<rsucnt;i++)
	{
		sprintf(key,"RSU%dIP=",i+1);
	    pConf->StrRSUIP[i] = getstring(StrConfig,key) ;//RSU IP 地址
	    
//		sprintf(key,"RSU%dPort=",i+1);
//	    pConf->StrRSUPort[i] = getstring(StrConfig,key) ;//RSU端口
		if(pConf->StrRSUType=="1")
			pConf->StrRSUPort[i] = "9528";//门架通用型RSU端口固定9528
		else if(pConf->StrRSUType=="2")
			pConf->StrRSUPort[i] = "7548";//示范工程华为RSU端口固定7548
	}
    Strkey = "VehPlateCount=";
    pConf->StrVehPlateCount = getstring(StrConfig,Strkey) ;//识别仪数量
    if(pConf->StrVehPlateCount=="")
		pConf->StrVehPlateCount="0";
	vehplatecnt=atoi(pConf->StrVehPlateCount.c_str());
	if(vehplatecnt>VEHPLATE_NUM)
	{
		sprintf(value,"%d", VEHPLATE_NUM) ;
		pConf->StrVehPlateCount=value;
		vehplatecnt=VEHPLATE_NUM;
	}
	else if(vehplatecnt<0)
	{
		pConf->StrVehPlateCount="0";
		vehplatecnt=0;
	}
	for(i=0;i<vehplatecnt;i++)
	{
		sprintf(key,"VehPlate%dIP=",i+1);
	    pConf->StrVehPlateIP[i] = getstring(StrConfig,key) ;//识别仪IP地址
	    
		sprintf(key,"VehPlate%dPort=",i+1);
	    pConf->StrVehPlatePort[i] = getstring(StrConfig,key) ;//识别仪端口
	    
//		sprintf(key,"VehPlate%dKey=",i+1);
//	    StrVehPlateKey[i] = getstring(StrConfig,key) ;//识别仪用户名密码
		pConf->StrVehPlateKey[i] = "hdcam:hdcam";//识别仪用户名密码
	}

    Strkey = "VehPlate900Count=";							//900万识别仪数量
    pConf->StrVehPlate900Count = getstring(StrConfig,Strkey) ;//900识别仪数量
    if(pConf->StrVehPlate900Count=="")
		pConf->StrVehPlate900Count="0";
	vehplate900cnt=atoi(pConf->StrVehPlate900Count.c_str());
	if(vehplate900cnt>VEHPLATE900_NUM)
	{
		sprintf(value,"%d", VEHPLATE900_NUM) ;
		pConf->StrVehPlate900Count=value;
		vehplate900cnt=VEHPLATE900_NUM;
	}
	else if(vehplate900cnt<0)
	{
		pConf->StrVehPlate900Count="0";
		vehplate900cnt=0;
	}
	for(i=0;i<VEHPLATE900_NUM;i++)
	{
		sprintf(key,"VehPlate900%dIP=",i+1);
	    pConf->StrVehPlate900IP[i] = getstring(StrConfig,key) ;//900识别仪IP地址
	    
		sprintf(key,"VehPlate900%dPort=",i+1);
	    pConf->StrVehPlate900Port[i] = getstring(StrConfig,key) ;//900识别仪端口
	    
//		sprintf(key,"VehPlate900%dKey=",i+1);
//	    StrVehPlate900Key[i] = getstring(StrConfig,key) ;//900识别仪用户名密码
		sprintf(value,"hdcam:hdcam");
		pConf->StrVehPlate900Key[i] = value ;//900识别仪用户名密码
	}
	
    Strkey = "CAMCount=";
    pConf->StrCAMCount = getstring(StrConfig,Strkey) ;//监控摄像头数量
    if(pConf->StrCAMCount=="")
		pConf->StrCAMCount="0";
	if(atoi(pConf->StrCAMCount.c_str())>CAM_NUM)
	{
		sprintf(value,"%d", CAM_NUM) ;
		pConf->StrCAMCount=value;
	}
	else if(atoi(pConf->StrCAMCount.c_str())<0)
		pConf->StrCAMCount="0";
	for(i=0;i<CAM_NUM;i++)
	{
		sprintf(key,"CAM%dIP=",i+1);
	    pConf->StrCAMIP[i] = getstring(StrConfig,key) ;//监控摄像头IP地址
	    
		sprintf(key,"CAM%dPort=",i+1);
	    pConf->StrCAMPort[i] = getstring(StrConfig,key) ;//监控摄像头端口
	    
		sprintf(key,"CAM%dKey=",i+1);
	    pConf->StrCAMKey[i] = getstring(StrConfig,key) ;//监控摄像头用户名密码
	}
    
    //防火墙配置
    Strkey = "FireWareType=";
    pConf->StrFireWareType = getstring(StrConfig,Strkey) ;//防火墙类型
    if(pConf->StrFireWareType == "")
        pConf->IntFireWareType = 1 ;
    else
        pConf->IntFireWareType = atoi(pConf->StrFireWareType.c_str());
    Strkey = "FireWareCount=";
    pConf->StrFireWareCount = getstring(StrConfig,Strkey) ;//防火墙数量
    if(pConf->StrFireWareCount=="")
		pConf->StrFireWareCount="0";
	if(atoi(pConf->StrFireWareCount.c_str())>FIREWARE_NUM)
	{
		sprintf(value,"%d", FIREWARE_NUM) ;
		pConf->StrFireWareCount=value;
	}
	else if(atoi(pConf->StrFireWareCount.c_str())<0)
		pConf->StrFireWareCount="0";
	for(i=0;i<FIREWARE_NUM;i++)
	{
		sprintf(key,"FireWare%dIP=",i+1);
	    pConf->StrFireWareIP[i] = getstring(StrConfig,key) ;//防火墙IP地址

		sprintf(key,"FireWare%dGetPasswd=",i+1);
	    pConf->StrFireWareGetPasswd[i] = getstring(StrConfig,key) ;//防火墙get密码

		sprintf(key,"FireWare%dSetPasswd=",i+1);
	    pConf->StrFireWareSetPasswd[i] = getstring(StrConfig,key) ;//防火墙set密码
	}
    //交换机配置
    Strkey = "SwitchType=";
    pConf->StrIPSwitchType = getstring(StrConfig,Strkey) ;//交换机类型
    if(pConf->StrIPSwitchType == "")
        pConf->IntIPSwitchType = 1 ;
    else
        pConf->IntIPSwitchType = atoi(pConf->StrIPSwitchType.c_str());
    Strkey = "SwitchCount=";
    pConf->StrIPSwitchCount = getstring(StrConfig,Strkey) ;//交换机数量
    if(pConf->StrIPSwitchCount=="")
		pConf->StrIPSwitchCount="0";
	if(atoi(pConf->StrIPSwitchCount.c_str())>IPSWITCH_NUM)
	{
		sprintf(value,"%d", IPSWITCH_NUM) ;
		pConf->StrIPSwitchCount=value;
	}
	else if(atoi(pConf->StrIPSwitchCount.c_str())<0)
		pConf->StrIPSwitchCount="0";
	for(i=0;i<IPSWITCH_NUM;i++)
	{
		sprintf(key,"Switch%dIP=",i+1);
	    pConf->StrIPSwitchIP[i] = getstring(StrConfig,key) ;//交换机IP地址

		sprintf(key,"Switch%dGetPasswd=",i+1);
	    pConf->StrIPSwitchGetPasswd[i] = getstring(StrConfig,key) ;//交换机get密码

		sprintf(key,"Switch%dSetPasswd=",i+1);
	    pConf->StrIPSwitchSetPasswd[i] = getstring(StrConfig,key) ;//交换机set密码
	}

    //Atlas配置
    Strkey = "AtlasType=";
    pConf->StrAtlasType = getstring(StrConfig,Strkey) ;//Atlas数量
    if(pConf->StrAtlasType=="")
		pConf->StrAtlasType="1";
    Strkey = "AtlasCount=";
    pConf->StrAtlasCount = getstring(StrConfig,Strkey) ;//Atlas数量
    if(pConf->StrAtlasCount=="")
		pConf->StrAtlasCount="0";
	if(atoi(pConf->StrAtlasCount.c_str())>ATLAS_NUM)
	{
		sprintf(value,"%d", ATLAS_NUM) ;
		pConf->StrAtlasCount=value;
	}
	else if(atoi(pConf->StrAtlasCount.c_str())<0)
		pConf->StrAtlasCount="0";
	for(i=0;i<ATLAS_NUM;i++)
	{
		sprintf(key,"Atlas%dIP=",i+1);
	    pConf->StrAtlasIP[i] = getstring(StrConfig,key) ;//AtlasIP地址
		sprintf(key,"Atlas%dPasswd=",i+1);
	    pConf->StrAtlasPasswd[i] = getstring(StrConfig,key) ;//Atlas密码
	    if(pConf->StrAtlasPasswd[i]=="" || pConf->StrAtlasPasswd[i]=="Huawei123")
	    	pConf->StrAtlasPasswd[i] = "admin:Huawei12#$:Huawei@SYS3" ;//Atlas密码
		if(CABINETTYPE == 9) //广深沿江容尊机柜使用研华工控机
			pConf->StrAtlasPasswd[i] = "root:eiec2019:eiec2019" ;//研华工控机密码
	}
    
    //防雷器配置
	Strkey = "SPDType=";
    pConf->StrSPDType = getstring(StrConfig,Strkey);	//PSD类型
	pConf->SPD_Type = atoi(pConf->StrSPDType.c_str());

	Strkey = "SPDCount=";
    pConf->StrSPDCount = getstring(StrConfig,Strkey);	//PSD数量
    if(pConf->StrSPDCount=="")
	{
		pConf->StrSPDCount="0";
	}
	pConf->SPD_num =atoi(pConf->StrSPDCount.c_str());
	if(pConf->SPD_num>SPD_NUM)
	{
		sprintf(value,"%d", SPD_NUM) ;
		pConf->StrSPDCount=value;
		pConf->SPD_num=SPD_NUM;
	}
	else if(pConf->SPD_num<0)
	{
		pConf->StrSPDCount="0";
		pConf->SPD_num=0;
	}
	printf("SPD_type=%d\n\r",pConf->SPD_Type);
	printf("SPD_num=%d\n\r",pConf->SPD_num);

	// 防雷检测
	for(i=0;i<SPD_NUM;i++)
	{
		sprintf(key,"SPD%dAddr=",i+1);
		//Strkey = "SPDAddr=";
		pConf->StrSPDAddr[i] = getstring(StrConfig,key);
		pConf->SPD_Address[i] = atoi(pConf->StrSPDAddr[i].c_str());

		sprintf(key,"SPD%dIP=",i+1);
		//sprintf(key,"SPDIP=");
		pConf->StrSPDIP[i] = getstring(StrConfig,key);			//SPD IP 地址

		//sprintf(key,"SPDPort=");
		sprintf(key,"SPD%dPort=",i+1);
		pConf->StrSPDPort[i] = getstring(StrConfig,key);		//SPD端口
	}
	// 接地电阻只有1个
	Strkey = "SPDResAddr=";
	pConf->StrSPDAddr[SPD_NUM] = getstring(StrConfig,Strkey);
	pConf->SPD_Address[SPD_NUM] = atoi(pConf->StrSPDAddr[SPD_NUM].c_str());

	// 接地电阻如果是网络型的
	sprintf(key,"SPDResIP=");
	pConf->StrSPDIP[SPD_NUM] = getstring(StrConfig,key);			//接地电阻 IP 地址

	sprintf(key,"SPDResPort=");
	pConf->StrSPDPort[SPD_NUM] = getstring(StrConfig,key);			//接地电阻端口

	//门架信息
    Strkey = "FlagNetRoadID=";
    pConf->StrFlagNetRoadID = getstring(StrConfig,Strkey) ;//ETC 门架路网编号
    
    Strkey = "FlagRoadID=";
    pConf->StrFlagRoadID = getstring(StrConfig,Strkey) ;//ETC 门架路段编号
    
    Strkey = "FlagID=";
    pConf->StrFlagID = getstring(StrConfig,Strkey) ;//ETC 门架编号
    
    Strkey = "PosId=";
    pConf->StrPosId = getstring(StrConfig,Strkey) ;//ETC 门架序号
    
    Strkey = "Direction=";
    pConf->StrDirection = getstring(StrConfig,Strkey) ;//行车方向
    
    Strkey = "DirDescription=";
    pConf->StrDirDescription = getstring(StrConfig,Strkey) ;//行车方向说明
	
	/*电子锁的地址配置, 最大支持4把*/
    Strkey = "LockType=";
    pConf->StrLockType = getstring(StrConfig,Strkey);		//门锁类型
    if(pConf->StrLockType=="") pConf->StrLockType="1";		//默认华为锁
    Strkey = "LockNum=";
    pConf->StrLockNum = getstring(StrConfig,Strkey);		//门锁数量
    if(pConf->StrLockNum=="") pConf->StrLockNum="4";		//默认4把
	for (i = 0; i < LOCK_NUM; i++)
	{
		sprintf(key,"LOCKADD%d=",i+1);
		pConf->StrAdrrLock[i] = getstring(StrConfig,key) ;
	}

	/*电压电流传感器的地址配置, 最大支持6个*/
	for (i = 0; i < VA_METER_BD_NUM; i++)
	{
		sprintf(key,"VAMETERADD%d=",i+1);
		pConf->StrAdrrVAMeter[i] = getstring(StrConfig,key);
	}

	/*电源控制板的地址配置, 最大支持3块*/
	for (i = 0; i < POWER_BD_NUM; i++)
	{
		sprintf(key,"POWERBDADD%d=",i+1);
		pConf->StrAdrrPower[i] = getstring(StrConfig,key);
	}

    Strkey = "DO_Count=";
//    pConf->StrDoCount = getstring(StrConfig,Strkey) ;//DO数量
    pConf->StrDoCount = "16" ;//DO数量
	if(atoi(pConf->StrDoCount.c_str())>SWITCH_COUNT)
	{
		sprintf(value,"%d", SWITCH_COUNT) ;
		pConf->StrDoCount=value;
	}

	//DO映射设备，最大支持36路DO
	//车牌识别映射DO
	//RSU映射DO
	for (i = 0; i < RSUCTL_NUM; i++)
	{
		sprintf(key,"RSU%d_DO=",i+1);
		sprintf(devicename,"rsu%d_do",i+1);
		strvalue=getstring(StrConfig,key);
//			printf("config key=%s,strvalue=%s\n",key,strvalue.c_str());
		if(strvalue!="")
		{
			j=atoi(strvalue.c_str())-1;
			if(j<SWITCH_COUNT)
				pConf->StrDeviceNameSeq[j]=devicename; //设备名
			else if(j>=100 && j<100+12)
			{
				pConf->StrUnWireDevName[j-100]=devicename; //没接线设备名
				pConf->StrUnWireDo[j-100]=strvalue;
			}
		}
	}
	for (i = 0; i < VEHPLATE_NUM; i++)
	{
		sprintf(key,"VEHPLATE%d_DO=",i+1);
		sprintf(devicename,"vehplate%d_do",i+1);
		strvalue=getstring(StrConfig,key);
//			printf("config key=%s,strvalue=%s\n",key,strvalue.c_str());
		if(strvalue!="")
		{
			j=atoi(strvalue.c_str())-1;
			if(j<SWITCH_COUNT)
				pConf->StrDeviceNameSeq[j]=devicename; //设备名
			else if(j>=100 && j<100+12)
			{
				pConf->StrUnWireDevName[j-100]=devicename; //没接线设备名
				pConf->StrUnWireDo[j-100]=strvalue;
			}
		}
	}
	for (i = 0; i < VEHPLATE900_NUM; i++)
	{
		sprintf(key,"VEHPLATE900%d_DO=",i+1); 
		sprintf(devicename,"vehplate900%d_do",i+1);
		strvalue=getstring(StrConfig,key);
		printf("config key=%s,strvalue=%s\n",key,strvalue.c_str());
		if(strvalue!="")
		{
			j=atoi(strvalue.c_str())-1;
			if(j<SWITCH_COUNT)
				pConf->StrDeviceNameSeq[j]=devicename; //设备名
			else if(j>=100 && j<100+12)
			{
				pConf->StrUnWireDevName[j-100]=devicename; //没接线设备名
				pConf->StrUnWireDo[j-100]=strvalue;
			}
		}
	}
	for (i = 0; i < CAM_NUM; i++)
	{
		sprintf(key,"CAM%d_DO=",i+1);
		sprintf(devicename,"cam%d_do",i+1);
		strvalue=getstring(StrConfig,key);
		printf("config key=%s,strvalue=%s\n",key,strvalue.c_str());
		if(strvalue!="")
		{
			j=atoi(strvalue.c_str())-1;
			if(j<SWITCH_COUNT)
				pConf->StrDeviceNameSeq[j]=devicename; //设备名
			else if(j>=100 && j<100+12)
			{
				pConf->StrUnWireDevName[j-100]=devicename; //没接线设备名
				pConf->StrUnWireDo[j-100]=strvalue;
			}
		}
	}
	//交换机映射DO
	for (i = 0; i < IPSWITCH_NUM; i++)
	{
		sprintf(key,"IPSWITCH%d_DO=",i+1);
		sprintf(devicename,"ipswitch%d_do",i+1);
		strvalue=getstring(StrConfig,key);
		printf("config key=%s,strvalue=%s\n",key,strvalue.c_str());
		if(strvalue!="")
		{
			j=atoi(strvalue.c_str())-1;
			if(j<SWITCH_COUNT)
				pConf->StrDeviceNameSeq[j]=devicename; //设备名
			else if(j>=100 && j<100+12)
			{
				pConf->StrUnWireDevName[j-100]=devicename; //没接线设备名
				pConf->StrUnWireDo[j-100]=strvalue;
			}
		}
	}
	//防火墙映射DO
	for (i = 0; i < FIREWARE_NUM; i++)
	{
		sprintf(key,"FIREWARE%d_DO=",i+1);
		sprintf(devicename,"fireware%d_do",i+1);
		strvalue=getstring(StrConfig,key);
		printf("config key=%s,strvalue=%s\n",key,strvalue.c_str());
		if(strvalue!="")
		{
			j=atoi(strvalue.c_str())-1;
			if(j<SWITCH_COUNT)
				pConf->StrDeviceNameSeq[j]=devicename; //设备名
			else if(j>=100 && j<100+12)
			{
				pConf->StrUnWireDevName[j-100]=devicename; //没接线设备名
				pConf->StrUnWireDo[j-100]=strvalue;
			}
		}
	}
	//ATLAS映射DO
	for (i = 0; i < ATLAS_NUM; i++)
	{
		sprintf(key,"ATLAS%d_DO=",i+1);
		sprintf(devicename,"atlas%d_do",i+1);
		strvalue=getstring(StrConfig,key);
		printf("config key=%s,strvalue=%s\n",key,strvalue.c_str());
		if(strvalue!="")
		{
			j=atoi(strvalue.c_str())-1;
			if(j<SWITCH_COUNT)
				pConf->StrDeviceNameSeq[j]=devicename; //设备名
			else if(j>=100 && j<100+12)
			{
				pConf->StrUnWireDevName[j-100]=devicename; //没接线设备名
				pConf->StrUnWireDo[j-100]=strvalue;
			}
		}
	}
	//天线头映射DO
	for (i = 0; i < ANTENNA_NUM; i++)
	{
		sprintf(key,"ANTENNA%d_DO=",i+1);
		sprintf(devicename,"antenna%d_do",i+1);
		strvalue=getstring(StrConfig,key);
		printf("config key=%s,strvalue=%s\n",key,strvalue.c_str());
		if(strvalue!="")
		{
			j=atoi(strvalue.c_str())-1;
			if(j<SWITCH_COUNT)
				pConf->StrDeviceNameSeq[j]=devicename; //设备名
			else if(j>=100 && j<100+12)
			{
				pConf->StrUnWireDevName[j-100]=devicename; //没接线设备名
				pConf->StrUnWireDo[j-100]=strvalue;
			}
		}
	}
	//DO市电停电时是否断电
	for (i = 0; i < SWITCH_COUNT; i++)
	{
		sprintf(key,"DO_%d_Poweroff_Or_Not=",i+1);
		strvalue=getstring(StrConfig,key);
		if(strvalue!="")
			pConf->StrPoweroff_Or_Not[i]=strvalue;
	}

	sprintf(key,"CpuAlarmValue=");
	strvalue=getstring(StrConfig,key);
	if(strvalue!="")
		pConf->StrCpuAlarmValue=strvalue;		//CPU使用率报警阈值
	sprintf(key,"CpuTempAlarmValue=");
	strvalue=getstring(StrConfig,key);
	if(strvalue!="")
		pConf->StrCpuTempAlarmValue=strvalue;	//CPU温度报警阈值
	sprintf(key,"MemAlarmValue=");
	strvalue=getstring(StrConfig,key);
	if(strvalue!="")
		pConf->StrMemAlarmValue=strvalue;		//内存使用率报警阈值

	ConfigCri.UnLock();
	free(strbuf);
	return 0 ;
}



int Setconfig(string StrKEY,string StrSetconfig)
{
    string setconfig = ""; 
    string strstart = "";
    string strend = "";
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
    
    ConfigCri.Lock();
    setconfig =  pConf->STRCONFIG ;

    int lenpos ;
    lenpos = setconfig.find(StrKEY) ;
    if(lenpos >= 0)
    {
       strstart = setconfig.substr(0,lenpos+StrKEY.length()) ; 
    }
    else
    {
		printf("Setconfig %s %s\n",StrKEY.c_str(),StrSetconfig.c_str());
		
		pConf->STRCONFIG=pConf->STRCONFIG+StrKEY+StrSetconfig+"\n";
       	ConfigCri.UnLock();
       	return 1 ;
    }
 
    lenpos = setconfig.find('\n',lenpos) ;
    if(lenpos >= 0)
    {
        strend = setconfig.substr(lenpos);
    }
    else
    {
        ConfigCri.UnLock();
        return 1 ;
    }

    pConf->STRCONFIG = strstart + StrSetconfig + strend ;
    ConfigCri.UnLock();

    return 0 ;
}


int OnSpdSetconfigBack(string StrKEY,string StrSetconfig,unsigned int mRetID)
{
   	Setconfig(StrKEY,StrSetconfig);
}

int OnSpdSetWriteconfig(unsigned int mRetID)
{
   	Writeconfig();
}

int Writeconfig(void)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
    ConfigCri.Lock();
    string setconfig  = pConf->STRCONFIG ;
    FILE* fdd ;
    fdd	= fopen("/opt/config", "wb");
    if(fdd == NULL)
    {
       ConfigCri.UnLock();
       return 1 ;
    }
	
//printf("setconfig=%s\r\n",setconfig.c_str() );
    int len = setconfig.length();
    fwrite(setconfig.c_str(),len, 1, fdd);
    fflush(fdd);
    fclose(fdd);
    ConfigCri.UnLock();
    
    return 0 ;

}

int SetWificonfig(string StrKEY,string StrSetconfig)
{
    string setconfig = ""; 
    string strstart = "";
    string strend = "";
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
    
    ConfigCri.Lock();
    setconfig =  pConf->STRWIFIWAP ;

    int lenpos ;
    lenpos = setconfig.find(StrKEY) ;
    if(lenpos >= 0)
    {
       strstart = setconfig.substr(0,lenpos+StrKEY.length()) ; 
    }
    else
    {
       ConfigCri.UnLock();
       return 1 ;
    }
 
    lenpos = setconfig.find('\n',lenpos) ;
    if(lenpos >= 0)
    {
        strend = setconfig.substr(lenpos);
    }
    else
    {
        ConfigCri.UnLock();
        return 1 ;
    }

    pConf->STRWIFIWAP = strstart + StrSetconfig + strend ;
    ConfigCri.UnLock();

    return 0 ;
}

int WriteWificonfig(void)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
    ConfigCri.Lock();
    string setconfig  = pConf->STRWIFIWAP ;
    FILE* fdd ;
    fdd	= fopen("/opt/wpa_supplicant.conf", "wb");
    if(fdd == NULL)
    {
       ConfigCri.UnLock();
       return 1 ;
    }

    int len = setconfig.length();
    fwrite(setconfig.c_str(),len, 1, fdd);
    fflush(fdd);
    fclose(fdd);
    ConfigCri.UnLock();
    
    return 0 ;

}


//单独写网络配置文件
int WriteNetconfig(char *configbuf,int configlen)
{
    int ret = 0;
    ConfigCri.Lock();
    FILE* fdd ;
    fdd	= fopen("/opt/netconfig", "wb");
    if(fdd == NULL)
    {
       ConfigCri.UnLock();
       return 1 ;
    }

    ret = fwrite(configbuf,configlen, 1, fdd);
    fflush(fdd);
    fclose(fdd);
    ConfigCri.UnLock();

    if(ret > 0)
    {
        //覆盖/home/root/net/netconfig
        system("wr cp /opt/netconfig /home/root/net/");
    }


    return 0 ;

}


int WriteNetconfig2(char *configbuf,int configlen)
{
    int ret = 0;
    ConfigCri.Lock();
    FILE* fdd ;
    fdd	= fopen("/opt/netconfig2", "wb");
    if(fdd == NULL)
    {
       ConfigCri.UnLock();
       return 1 ;
    }

    ret = fwrite(configbuf,configlen, 1, fdd);
    fflush(fdd);
    fclose(fdd);
    ConfigCri.UnLock();

    if(ret > 0)
    {
        //覆盖/home/root/net/netconfig2
        system("wr cp /opt/netconfig2 /home/root/net/");
    }


    return 0 ;

}







