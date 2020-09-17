
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

extern DEVICE_PARAMS *stuDev_Param[POWER_BD_NUM];		//装置参数寄存器
extern REMOTE_CONTROL *stuRemote_Ctrl;	//遥控寄存器结构体
extern CabinetClient *pCabinetClient;//华为机柜状态
extern VMCONTROL_STATE VMCtl_State;	//控制器运行状态结构体
extern SPD_PARAMS *stuSpd_Param;		//防雷器结构体
extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体

CfirewallClient *pCfirewallClient[FIREWARE_NUM];	//防火墙对象
CswitchClient *pCswitchClient[IPSWITCH_NUM];		//交换机对象
RSU *pCRSU[RSUCTL_NUM];;					//RSU对象
IPCam *pCVehplate[VEHPLATE_NUM];			//300万车牌识别仪对象
IPCam *pCVehplate900[VEHPLATE900_NUM];		//900万全景摄像机对象
tsPanel *pCPanel;	//液晶屏对象
CsshClient *pCsshClient[ATLAS_NUM];			//ATLAS对象
CANNode *pCOsCan = NULL;		//Can对象


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


void RsuCallback(RSU::DataType_EN type, void *data, void *userdata) 
{

    RSU *pRSU = (RSU *)userdata ;

    uint16_t i,j;

    if(data == NULL)
        return;

    printf("callback\n");

    /* 心跳信息 */
    switch (type) {
    case RSU::heartbeat: {
        RSU::RsuControler_S *ctrler = (RSU::RsuControler_S *)data;
        RSU::CtrlerSta_S *sta = ctrler->ctrlSta;

        printf("HeartBeat: timestamp = %ld\n",ctrler->timestamp);
        for (i = 0; i < RSU_CONTROLER_NUM; i++) {
            printf("ctrl.sta[%d]  sta = %X psamNum = %d  ", i, sta[i].state, sta[i].psamNum);
            printf("psamSta :{ ");
            for (j = 0; j < 12; j++)
                printf(" [%X %X %X] ", sta[i].psamSta[j].channel, sta[i].psamSta[j].status, sta[i].psamSta[j].auth);
            printf(" }\n");
        }

        printf("ant = %d   workAnt = %d  ", ctrler->totalAnt, ctrler->workingAnt);
        printf("antInfo :{ ");
        for (j = 0; j < RSU_CTRL_ANT_NUM; j++)
            printf(" [%X %X %X %X] ", ctrler->antInfo[j].id, ctrler->antInfo[j].sta, ctrler->antInfo[j].channel, ctrler->antInfo[j].power);
        printf(" }\n");

    } break;
    /* 状态信息 */
    case RSU::stateInfo: {
        RSU::RsuInfo_S *info = (RSU::RsuInfo_S *)data;
        uint8_t *id;

        printf("StateInfo : state = %X  psma num = %d ",info->state,info->PsamNum);
        printf("psamInfo :{ ");
        for (j = 0; j < RSU_PSMA_TERMINAL_NUM; j++){
            id = info->psamInfo[j].id;
            printf(" [%X %X %X %X%X%X%X%X%X] \n", info->psamInfo[j].channel,info->psamInfo[j].version,info->psamInfo[j].auth,id[0],id[1],id[2],id[3],id[4],id[5]);
        }
        printf(" }\n");
        printf("algId: %X  manuID: %X \n",info->algId,info->manuID);
        printf("RSU:id: %X%X%X  ver: %X%X \n",info->id[0],info->id[1],info->id[2],info->ver[0],info->ver[1]);
        printf("workSta: %X flagId: %X%X%X \n",info->workSta,info->flagId[0],info->flagId[1],info->flagId[2]);
        printf("reserved: %X %X %X %X \n",info->reserved[0],info->reserved[1],info->reserved[2],info->reserved[3]);

    } break;
    /* 复位信息 */
    case RSU::rstInfo: {
        RSU::RsuRst_S *reset = (RSU::RsuRst_S *)data;


    } break;
    }
}

void IPCamCallback(string staCode,string msg,IPCam::State_S state,void *userdata)
{
    IPCam *pIPCam = (IPCam *)userdata ;
    printf("staCode = %s\n",staCode.c_str());
    printf("message = %s\n",msg.c_str());
    printf("linked = %s\n",state.linked ? "true" : "false");
    printf("timestamp = %ld\n",state.timestamp);
    
    printf("ip = %s\n",state.ip.c_str());
    printf("factoryid = %s\n",state.factoryid.c_str());
    printf("errcode = %s\n",state.errcode.c_str());
    printf("devicemodel = %s\n",state.devicemodel.c_str());
    printf("softversion = %s\n",state.softversion.c_str());
    printf("statustime = %s\n",state.statustime.c_str());
    printf("filllight = %s\n",state.filllight.c_str());
    printf("temperature = %s\n",state.temperature.c_str());
    printf("picstateid = %s\n",state.picstateid.c_str());
    printf("gantryid = %s\n",state.gantryid.c_str());
    printf("statetime = %s\n",state.statetime.c_str());
    printf("overstockImageJourCount = %s\n",state.overstockImageJourCount.c_str());
    printf("overstockImageCount = %s\n",state.overstockImageCount.c_str());
    printf("cameranum = %s\n",state.cameranum.c_str());
    printf("lanenum = %s\n",state.lanenum.c_str());
    printf("connectstatus = %s\n",state.connectstatus.c_str());
    printf("workstatus = %s\n",state.workstatus.c_str());
    printf("lightworkstatus = %s\n",state.lightworkstatus.c_str());
    printf("recognitionrate = %s\n",state.recognitionrate.c_str());
    printf("hardwareversion = %s\n",state.hardwareversion.c_str());
    printf("softwareversion = %s\n",state.softwareversion.c_str());
    printf("runningtime = %s\n",state.runningtime.c_str());
    printf("brand = %s\n",state.brand.c_str());
    printf("devicetype = %s\n",state.devicetype.c_str());
    printf("statuscode = %s\n",state.statuscode.c_str());
    printf("statusmsg = %s\n",state.statusmsg.c_str());
}

void HttpCallback(long code,char *data,uint32_t dLen,void *userdata)
{
    printf("\n*********** HTTP CALLBACK **********************\n");
    printf("CODE = %ld  DATA length = %d\n\n",code,dLen);
    printf("%s \r\n",data);
}

// p指针需要传进this,否则无法处理node中的数据
void canNodeCallback(void *p,void *data,int len)
{
	CANNode *pcan = (CANNode *)p;
	can_frame *c_data = (can_frame *)data;
	uint16_t amp =0, volt = 0;
	uint8_t temp = 0, flag = 0, oa_alarm = 0, version = 0;
	uint16_t seq = 0;
	uint16_t *pointer = &volt;
	uint8_t *buf = &c_data->data[0];

	//预留36路
	if (c_data->can_id > 0)
	{
		DEBUG_PRINTF("init: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\r\n",c_data->data[0],c_data->data[1],\
			c_data->data[2],c_data->data[3],c_data->data[4],c_data->data[5],c_data->data[6],c_data->data[7]);
		
		seq = c_data->can_id-1;
		flag = c_data->data[0];
		temp = c_data->data[1];
		pointer = &volt;
		char_to_int(buf+2, pointer);

		pointer = &amp;
		char_to_int(buf+4, pointer);

		oa_alarm = c_data->data[6];
		version = c_data->data[7];

		// 协议转换
		pcan->canNode[seq].address = seq+1;
		pcan->canNode[seq].TimeStamp = timestamp_get();
		pcan->canNode[seq].isConnect = true;

		pcan->canNode[seq].phase.flag = flag;
		pcan->canNode[seq].phase.temp = (int8_t)temp;
		pcan->canNode[seq].phase.vln = (float)volt/100;
		pcan->canNode[seq].phase.amp = (float)amp/1000;
		pcan->canNode[seq].phase.alarm_threshold = oa_alarm*50;
		pcan->canNode[seq].phase.version = pcan->int8_to_string(version);

		DEBUG_PRINTF("recv: addr=%d, flag=%02x, temp=%d, volt=%f, amp=%f, oa=%d, version=%s\r\n",pcan->canNode[seq].address,pcan->canNode[seq].phase.flag,\
			pcan->canNode[seq].phase.temp,pcan->canNode[seq].phase.vln,pcan->canNode[seq].phase.amp,pcan->canNode[seq].phase.alarm_threshold,pcan->canNode[seq].phase.version.c_str());
	}
	//return 0;
}



int main(void)
{
	int i,j;
	unsigned int pos_cnt = 0;
	unsigned int temp = 0;
	char str[256];
	string ip;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
	//初始化看门狗
//	WatchDogInit();

	WriteLog("now start program!\n");
	//读设置文件
	GetConfig();
	
	//防雷器结构体
	stuSpd_Param = (SPD_PARAMS*)malloc(sizeof(SPD_PARAMS));
	memset(stuSpd_Param,0,sizeof(SPD_PARAMS));

//	lockerDataInit(true);

	/////////////////  电源控制板配置开始	/////////////////////////////////////////////
	//装置参数寄存器,分为电源板和IO板
	temp = 0;	//统计到底有几个电源板
	for (i = 0; i < POWER_BD_NUM; i++)
	{
		stuDev_Param[i] = (DEVICE_PARAMS*)malloc(sizeof(DEVICE_PARAMS));
		memset(stuDev_Param[i],0,sizeof(DEVICE_PARAMS));
		/*配置文件中是否有配置*/
		if (pConf->StrAdrrPower[i].length() != 0)
		{
			stuDev_Param[i]->Address = atoi(pConf->StrAdrrPower[i].c_str());
//			Rs485_table_set(POWER_BD_1+i, ENABLE,pos_cnt+actual_locker_num, stuDev_Param[i]->Address);
			pos_cnt++;
			temp++;
		}
		else
		{
			stuDev_Param[i] = NULL;	// 防止为野指针
//			Rs485_table_set(POWER_BD_1+i, DISABLE,NULL_VAR, NULL_VAR);
		}
	}
	/*如果配置表中没有配置，则默认配置1块电源板，地址为71*/
	if (temp == 0)
	{
		stuDev_Param[0] = (DEVICE_PARAMS*)malloc(sizeof(DEVICE_PARAMS));
		memset(stuDev_Param[0],0,sizeof(DEVICE_PARAMS));
		stuDev_Param[0]->Address = POWER_CTRL_ADDR_1;
//		Rs485_table_set(POWER_BD_1, ENABLE,pos_cnt+actual_locker_num, stuDev_Param[0]->Address);
		pos_cnt++;
		temp++;
	}
	/////////////////  电源控制板配置结束	/////////////////////////////////////////////


	//遥控设备结构体
	stuRemote_Ctrl = (REMOTE_CONTROL*)malloc(sizeof(REMOTE_CONTROL));
	
	/////////////////  DO配置表开始/////////////////////////////////////////////
	temp = 0;	//统计到底有几个DO
	for (i = 0; i < SWITCH_COUNT; i++)
	{
		/*配置文件中是否有配置*/
		if (pConf->StrDoSeq[i].length() != 0)
		{
			temp++; // 表明配置文件中有配置
			pConf->DoSeq[i] = atoi(pConf->StrDoSeq[i].c_str());
			if(pConf->DoSeq[i] > 0)
			{
				pConf->DoSeq[i]--; 		// 配置文件是从1~12, 标号是要减1
			}
		}
		else
		{
			pConf->DoSeq[i] = NULL;	// 未配置,不使用
		}
	}
	// 如果都没有配置，就按DO顺序进行默认配置
	if (temp == 0)
	{
		//printf("temp=0\r\n");
		for (i = 0; i < SWITCH_COUNT; i++)
		{
			pConf->DoSeq[i] = i;
		}
	}
	/////////////////  DO配置表结束/////////////////////////////////////////////
	//初始化http服务端
	Init_HttpServer();
	
	//初始化服务器线程
	init_TCPServer();
	usleep(500000); //delay 500ms
	write(WDTfd, "\0", 1);
	
	WalksnmpInit();

	//机柜
	pCabinetClient = new CabinetClient();
    initHUAWEIGantry(pCabinetClient);
	initHUAWEIALARM(pCabinetClient);
	pCabinetClient->StrHWServer = pConf->StrHWServer;
	pCabinetClient->StrHWGetPasswd = pConf->StrHWGetPasswd;
	pCabinetClient->StrHWSetPasswd = pConf->StrHWSetPasswd;
	pCabinetClient->Start();

	//防火墙 
	for(i=0;i<atoi(pConf->StrFireWareCount.c_str());i++)
   	{
		pCfirewallClient[i] = new CfirewallClient();
		initHUAWEIEntity(pCfirewallClient[i]);
		pCfirewallClient[i]->IntFireWareType = pConf->IntFireWareType;//防火墙类型 1：华为,2：迪普，3：深信服
		pCfirewallClient[i]->StrFireWareCount = pConf->StrFireWareCount; //防火墙数量
		pCfirewallClient[i]->StrFireWareIP[0] = pConf->StrFireWareIP[i];
		pCfirewallClient[i]->StrFireWareGetPasswd[0] = pConf->StrFireWareGetPasswd[i];
		pCfirewallClient[i]->StrFireWareSetPasswd[0] = pConf->StrFireWareSetPasswd[i];
		pCfirewallClient[i]->Start();
	}


	//交换机
	for(i=0;i<atoi(pConf->StrIPSwitchCount.c_str());i++)
   	{
		pCswitchClient[i] = new CswitchClient();
		initHUAWEIswitchEntity(pCswitchClient[i]);
		pCswitchClient[i]->IntIPSwitchType = pConf->IntIPSwitchType;//交换机类型  1：华为,2：华三
		pCswitchClient[i]->StrIPSwitchCount = pConf->StrIPSwitchCount; //交换机数量
		pCswitchClient[i]->StrIPSwitchIP[0] = pConf->StrIPSwitchIP[i];
		pCswitchClient[i]->StrIPSwitchGetPasswd[0] = pConf->StrIPSwitchSetPasswd[i];
		pCswitchClient[i]->StrIPSwitchSetPasswd[0] = pConf->StrIPSwitchGetPasswd[i];
		pCswitchClient[i]->Start();
	}

   	//RSU
   	for(i=0;i<atoi(pConf->StrRSUCount.c_str());i++)
   	{
	    pCRSU[i]  = new RSU(pConf->StrRSUIP[i].c_str(),pConf->StrRSUPort[i].c_str());

	    pCRSU[i]->setCallback(RsuCallback,pCRSU[i]);
   	}

	//车牌识别仪
   	for(i=0;i<atoi(pConf->StrVehPlateCount.c_str());i++)
   	{
   		pCVehplate[i] = NULL;
		
		ip=pConf->StrVehPlateIP[i].substr(7,pConf->StrVehPlateIP[i].size()-7);//去掉"http://"
	    pCVehplate[i] = new IPCam(ip,pConf->StrVehPlatePort[i],pConf->StrVehPlateKey[i]);
		if(pCVehplate[i] == NULL)
		{
			sprintf(str,"车牌识别仪 %d %s 初始化失败！\n",i,ip.c_str());
			WriteLog(str);
			continue;
		}
	    pCVehplate[i]->setCallback(IPCamCallback,pCVehplate[i]);
   	}

	//900万全景车牌识别仪
   	for(i=0;i<atoi(pConf->StrVehPlate900Count.c_str());i++)
   	{
		ip=pConf->StrVehPlate900IP[i].substr(7,pConf->StrVehPlate900IP[i].size()-7);//去掉"http://"
	    pCVehplate900[i] = new IPCam(ip,pConf->StrVehPlate900Port[i],pConf->StrVehPlate900Key[i]);
		if(pCVehplate900[i] == NULL)
		{
			sprintf(str,"900万全景车牌识别仪 %d %s 初始化失败！\n",i,ip.c_str());
			WriteLog(str);
			continue;
		}
	    pCVehplate900[i]->setCallback(IPCamCallback,pCVehplate900[i]);
   	}

	//Atlas
   	for(i=0;i<atoi(pConf->StrAtlasCount.c_str());i++)
   	{
		pCsshClient[i] = new CsshClient();
		pCsshClient[i]->mStrAtlasIP = pConf->StrAtlasIP[i];
		pCsshClient[i]->mStrAtlasPasswd = pConf->StrAtlasPasswd[i];
		pCsshClient[i]->Start();
   	}

	//初始化液晶屏
	pCPanel =new tsPanel(pCabinetClient,&VMCtl_Config);	

	//初始化Can
    pCOsCan =  new CANNode((char *)"can0",CAN_500K,0,0x000,0xF00,0);
	pCOsCan->setCallback(canNodeCallback,NULL);
	
    //初始化利通控制器状态获取线程
    init_lt_status();

	//启动WebServer
    init_WebServer();

	//启动WebSocket
	init_WebSocket();

	//处理重启
	init_DealDoReset();
	
	while(1)
	{
		sleep(5);

		//printf("GetTickCount=0x%x\n",GetTickCount());
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



