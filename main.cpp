
#include <stdio.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
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
#include <string>
#include "main.h"

#define WDT "/dev/watchdog"

using namespace std; 

extern REMOTE_CONTROL *stuRemote_Ctrl;	//遥控寄存器结构体
extern CabinetClient *pCabinetClient[HWSERVER_NUM];//华为机柜状态
extern CTrapClient *pCTrapClient;//华为告警状态
extern VMCONTROL_STATE VMCtl_State;	//控制器运行状态结构体
extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体

CfirewallClient *pCfirewallClient[FIREWARE_NUM] = {NULL,NULL};	//防火墙对象
CswitchClient *pCswitchClient[IPSWITCH_NUM] = {NULL,NULL};		//交换机对象
Artc *pCArtRSU[RSUCTL_NUM] = {NULL,NULL};;					//RSU对象
Huawei *pCHWRSU[RSUCTL_NUM] = {NULL,NULL};;					//RSU对象
IPCam *pCVehplate[VEHPLATE_NUM];			//300万车牌识别仪对象
IPCam *pCVehplate900[VEHPLATE900_NUM] = {NULL,NULL,NULL};		//900万全景摄像机对象
Camera *pCCam[CAM_NUM]= {NULL,NULL,NULL,NULL}	;						//机柜监控摄像头
tsPanel *pCPanel = NULL;	//液晶屏对象
CsshClient *pCsshClient[ATLAS_NUM] = {NULL,NULL};			//ATLAS对象
CANNode *pCOsCan = NULL;		//Can对象
SpdClient *pCSpdClent = NULL;		//SPD防雷器
Lock *pCLock[LOCK_NUM] = {NULL,NULL,NULL,NULL};					//生久门锁对象
HWLock *pHWCLock[LOCK_NUM] = {NULL,NULL,NULL,NULL};				//华为门锁对象
TemHumi *pCTemHumi;//[TEMHUMI_NUM];// = {NULL,NULL};;					//温湿度对象
AirCondition *pCAirCondition;//[AIRCON_NUM]= {NULL};;			//空调
IODev *pCIOdev = NULL;				//IO
Moninterface *Moninter;						//工控机监控
AirCondition::AirInfo_S AirCondInfo;		//直流空调结构体
TemHumi::Info_S TemHumiInfo;				//温湿度结构体
Lock::Info_S LockInfo[LOCK_NUM];			//电子锁结构体
CallBackTimeStamp CBTimeStamp;				//外设采集回调时间戳

//#define CARD_NUM		5	// 暂时为5张卡
//Lock *pCLock[LOCKER_MAX_NUM] = {NULL,NULL,NULL,NULL};


void WriteLog(char* str);

int WDTfd = -1;

void WatchDogInit(void)
{
    //int WDTfd = -1;
    int timeout;

    WDTfd = open(WDT, O_WRONLY);
    if (WDTfd == -1)
    {
        printf("fail to open "WDT "!\n");
    }
    printf(WDT " is opened!\n");

    timeout = 120;
     printf("set timeout was is %d seconds\n", timeout);

    ioctl(WDTfd, WDIOC_SETTIMEOUT, &timeout);

    ioctl(WDTfd, WDIOC_GETTIMEOUT, &timeout);
    printf("The timeout was is %d seconds\n", timeout);
    write(WDTfd, "\0", 1);

}




int main(void)
{
	int i,j;
	unsigned int pos_cnt = 0;
	unsigned int temp = 0;
	char str[256],dir[20];
	string ip;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
	//初始化看门狗
	WatchDogInit();

	WriteLog("now start program!\n");
	//读设置文件
	GetConfig();

	//遥控设备结构体
	stuRemote_Ctrl = (REMOTE_CONTROL*)malloc(sizeof(REMOTE_CONTROL));
	
	Init_DO(pConf);
	WalksnmpInit();
	//CABINETTYPE  1：华为（包括华为单门 双门等） 5：中兴; 6：金晟安; 7：爱特斯; 8:诺龙; 9：容尊堡; 
				//10:亚邦; 11：艾特网能；12：华软；13：利通
	//机柜
	for(i=0;i<HWSERVER_NUM;i++)
	{
		pCabinetClient[i] = new CabinetClient();
	    initHUAWEIGantry(pCabinetClient[i]);
		initHUAWEIALARM(pCabinetClient[i]);
		if(atoi(pConf->StrCabinetType.c_str())>=1 && atoi(pConf->StrCabinetType.c_str())<=4) //华为机柜
		{
			pCabinetClient[i]->StrHWServer = pConf->StrHWServer;
			pCabinetClient[i]->StrHWGetPasswd = pConf->StrHWGetPasswd;
			pCabinetClient[i]->StrHWSetPasswd = pConf->StrHWSetPasswd;
			if(i<atoi(pConf->StrHWServerCount.c_str()))
				pCabinetClient[i]->Start();
		}
	}
	//华为告警状态
	pCTrapClient = new CTrapClient(pCabinetClient[0]);
	pCTrapClient->SetTrapAlarmBack(TrapCallBack,(unsigned int)pCTrapClient);
	pCTrapClient->Start();

	if(pConf->StrCabinetType=="13") //利通机柜
	{
		//温湿度：传参为485地址 
/*		for(i=0;i<TEMHUMI_NUM;i++)
		{
			uint8_t addr=(i==0?TEMHUMI_ADDRESS_1:TEMHUMI_ADDRESS_2);
			pCTemHumi[i] = new TemHumi(addr);
			pCTemHumi[i]->setCallback(TemHumiCallback, pCTemHumi[i]);
		}*/
		pCTemHumi = new TemHumi(0x7);
		pCTemHumi->setCallback(TemHumiCallback, pCTemHumi);

		/* 空调：传参为485地址 */
/*		for(i=0;i<AIRCON_NUM;i++)
		{
		    pCAirCondition[i]=new AirCondition(0x6);
		    pCAirCondition[i]->setCallback(AirConditionCallback,NULL);
		    pCAirCondition[i]->config(AirCondition::Cfg_Refrigeration,20);
		}*/
	    pCAirCondition=new AirCondition(0x6); 
	    pCAirCondition->setCallback(AirConditionCallback,NULL);
	    pCAirCondition->config(AirCondition::Cfg_Refrigeration,20);
		initAirConditionInfo(&AirCondInfo);

		/* 工控机监控:传参为目标设备IP地址 */
//		Moninter = new Moninterface(pConf->StrAtlasIP[0].c_str());	// ("128.8.82.160");
//		Moninter->setCallback(MoninterCallback,NULL);

		/* 摄像头，传参为摄像头IP地址以及 IMA_SAVE_PATH_ROOT 下图片文件夹名 */
		for(i=0;i<CAM_NUM;i++)
		{
			sprintf(dir,"%d",i); 
			string url="http://"+pConf->StrCAMIP[i]+"/onvif-http/snapshot?Profile_1";
			pCCam[i]=new Camera(url,dir);
			pCCam[i]->setCallback(CameraCallback, pCCam[i]);
		}

		sleep(2);
		//IO
	    pCIOdev = IODev::getInstance();
	    pCIOdev->setCallback(IODevCallback,NULL);
	}

	for(i=0;i<LOCK_NUM;i++)
	{
		if(pConf->StrLockType=="1")
		{
			//华为电子锁对象
			pHWCLock[i] = new HWLock(atoi(pConf->StrAdrrLock[i].c_str()));
			pHWCLock[i]->setCallback(HWLockCallback, pHWCLock[i]);
		}
		else if(pConf->StrLockType=="2")
		{
			//生久电子锁对象
			pCLock[i] = new Lock(atoi(pConf->StrAdrrLock[i].c_str()));
			pCLock[i]->setCallback(LockCallback, pCLock[i]);
		}
	}
	
	//防火墙 
	for(i=0;i<FIREWARE_NUM;i++)
   	{
		pCfirewallClient[i] = new CfirewallClient();
		initHUAWEIEntity(pCfirewallClient[i]);
		pCfirewallClient[i]->IntFireWareType = pConf->IntFireWareType;//防火墙类型 1：华为,2：迪普，3：深信服
		pCfirewallClient[i]->StrFireWareCount = pConf->StrFireWareCount; //防火墙数量
		pCfirewallClient[i]->StrFireWareIP[0] = pConf->StrFireWareIP[i];
		pCfirewallClient[i]->StrFireWareGetPasswd[0] = pConf->StrFireWareGetPasswd[i];
		pCfirewallClient[i]->StrFireWareSetPasswd[0] = pConf->StrFireWareSetPasswd[i];
		if(i<atoi(pConf->StrFireWareCount.c_str()))
			pCfirewallClient[i]->Start();
	}


	//交换机
	for(i=0;i<IPSWITCH_NUM;i++)
   	{
		pCswitchClient[i] = new CswitchClient();
		initHUAWEIswitchEntity(pCswitchClient[i]);
		pCswitchClient[i]->IntIPSwitchType = pConf->IntIPSwitchType;//交换机类型  1：华为,2：华三
		pCswitchClient[i]->StrIPSwitchCount = pConf->StrIPSwitchCount; //交换机数量
		pCswitchClient[i]->StrIPSwitchIP[0] = pConf->StrIPSwitchIP[i];
		pCswitchClient[i]->StrIPSwitchGetPasswd[0] = pConf->StrIPSwitchSetPasswd[i];
		pCswitchClient[i]->StrIPSwitchSetPasswd[0] = pConf->StrIPSwitchGetPasswd[i];
		if(i<atoi(pConf->StrIPSwitchCount.c_str()))
			pCswitchClient[i]->Start();
	}

   	//RSU
   	for(i=0;i<RSUCTL_NUM;i++)
   	{
   		if(pConf->StrRSUType=="1")	//门架通用型RSU
   		{
		    pCArtRSU[i]  = new Artc(pConf->StrRSUIP[i].c_str(),pConf->StrRSUPort[i].c_str());
			pCArtRSU[i]->setCallback(RsuCallback,pCArtRSU[i]);
   		}
		else if(pConf->StrRSUType=="2")	//华为RSU
		{
printf("start rsu %s\r\n",pConf->StrIP.c_str());
			pCHWRSU[i]  = new Huawei(pConf->StrIP.c_str(),pConf->StrRSUPort[i],"/tr069");
			pCHWRSU[i]->setCallback(RsuCallback,pCHWRSU[i]);
		}
   	} 

	//车牌识别仪
   	for(i=0;i<VEHPLATE_NUM;i++)
   	{
   		if(pConf->StrVehPlateIP[i].size()>=7)
			ip=pConf->StrVehPlateIP[i].substr(7,pConf->StrVehPlateIP[i].size()-7);//去掉"http://"
		else 
			ip=pConf->StrVehPlateIP[i];
	    pCVehplate[i] = new IPCam(ip,pConf->StrVehPlatePort[i],pConf->StrVehPlateKey[i]);
	    pCVehplate[i]->setCallback(IPCamCallback,pCVehplate[i]);
   	}

	//900万全景车牌识别仪
   	for(i=0;i<VEHPLATE900_NUM;i++)
   	{
   		if(pConf->StrVehPlate900IP[i].size()>=7)
			ip=pConf->StrVehPlate900IP[i].substr(7,pConf->StrVehPlate900IP[i].size()-7);//去掉"http://"
		else 
			ip=pConf->StrVehPlate900IP[i];
	    pCVehplate900[i] = new IPCam(ip,pConf->StrVehPlate900Port[i],pConf->StrVehPlate900Key[i]);
	    pCVehplate900[i]->setCallback(IPCamCallback,pCVehplate900[i]);
   	}

	//Atlas
   	for(i=0;i<ATLAS_NUM;i++)
   	{
		pCsshClient[i] = new CsshClient();
		pCsshClient[i]->mStrAtlasIP = pConf->StrAtlasIP[i];
		pCsshClient[i]->mStrAtlasPasswd = pConf->StrAtlasPasswd[i];
		if(i<atoi(pConf->StrAtlasCount.c_str()))
			pCsshClient[i]->Start();
   	}

	//初始化动力控制板
    pCOsCan =  new CANNode((char *)"can0",CAN_500K,0,0x000,0xF00,0);
	pCOsCan->setCallback(canNodeCallback,NULL);

	//初始化液晶屏
	pCPanel =new tsPanel(&pCabinetClient[0],&VMCtl_Config);
	// 配置动环和机柜的数量逻辑，如果是1个动环，1/2个机柜，则配置为Box_Config(0,0)
	// 如果是2个动环，则配置为Box_Config(0,1)
	pCPanel->Box_Config(0,0);
	pCPanel->time_interval = 2;		// 800ms刷新
		
	//初始化SPD
	pCSpdClent = new SpdClient();
    init_SPD(pCSpdClent, pConf);

	usleep(500000); //delay 500ms
	write(WDTfd, "\0", 1);
	
    //初始化利通控制器状态获取线程
    init_lt_status();

	//启动WebServer
    init_WebServer();

	//启动WebSocket
	init_WebSocket();

	//处理重启
	init_DealDoReset();
	//初始化http服务端
	Init_HttpServer();
	
	//初始化服务器线程
	init_TCPServer();

	//初始化利通后台推送服务
	init_LTKJ_DataPost();
	while(1)
	{
		sleep(5);

		write(WDTfd, "\0", 1);
		
		printf("GetTickCount=0x%x\n",GetTickCount());
	}

   

   return 0 ;
}

void WriteLog(char* str)
 {
	 char sDateTime[30],stmp[10];
	 FILE *fpLog;
	 string exePath,filename;
	 exePath="logs";
	 if(access(exePath.c_str(),0) == -1)
	 	mkdir(exePath.c_str(),0755);
	 
	 time_t nSeconds;
	 struct tm * pTM;
	 
	 time(&nSeconds);
	 pTM = localtime(&nSeconds);
	 
	 //判断后一天文件是否存在，存在就先删除
	 sprintf(stmp,"%d",pTM->tm_mday+1);   
	 filename=exePath+"/"+stmp+".txt";
	 if((access(filename.c_str(),F_OK))!=-1)   
	 {	 
		 printf("%s 存在\n",filename.c_str());
		 remove(filename.c_str());
	 }		 
	 if(pTM->tm_mday>=30)
	 {
		 filename=exePath+"/1.txt";
		 if((access(filename.c_str(),F_OK))!=-1)   
		 {	 
			 printf("%s 存在\n",filename.c_str());
			 remove(filename.c_str());
		 }		 
	 }
	 
	 //系统日期和时间,格式: yyyymmddHHMMSS 
	 sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
			 pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
			 pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
	 
/**/	 sprintf(stmp,"%d",pTM->tm_mday);	 
	 filename=exePath+"/"+stmp+".txt";
	 fpLog = fopen(filename.c_str(), "a");
	 
	 fseek(fpLog, 0, SEEK_END);
	 fprintf(fpLog, "%s->%s", sDateTime,str);
	 printf("%s-->%s",sDateTime,str);	 
	 
	 fclose(fpLog);/**/
 }
 
void myprintf(char* str)
  {
	  char sDateTime[30],stmp[10];
	  time_t nSeconds;
	  struct tm * pTM;
	  
	  time(&nSeconds);
	  pTM = localtime(&nSeconds);
	  
	  //系统日期和时间,格式: yyyymmddHHMMSS 
	  sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
			  pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
			  pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
	  
	  printf("%s-->%s",sDateTime,str);	  
  }



