#include "jsonPackage.h"
//#include "IpcamServer.h"
#include "comserver.h"
//#include "rs485server.h"

extern REMOTE_CONTROL *stuRemote_Ctrl;	//遥控寄存器结构体
extern CabinetClient *pCabinetClient[HWSERVER_NUM];//华为机柜状态
extern VMCONTROL_STATE VMCtl_State;	//控制器运行状态结构体
//extern ATLAS_STATE stuAtlasState[ATLAS_NUM]; //Atlas状态
extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体
extern CANNode *pCOsCan;		//Can对象
extern SpdClient *pCSpdClent;		//SPD防雷器

extern int Writeconfig(void);
//extern void GetConfig(VMCONTROL_CONFIG *vmctrl_param);
extern void RemoteControl(UINT8* pRCtrl);
extern int Setconfig(string StrKEY,string StrSetconfig);
extern void SetIPinfo(IPInfo *ipInfo);
extern void SetIPinfo2(IPInfo *ipInfo);
// 获取电子锁的开关状态信息
extern UINT16 LockerStatusGet(unsigned char seq);
extern unsigned long GetTickCount(); 
extern void WriteLog(char* str);
extern void SetIPinfo();
extern void SetIPinfo2();

//extern TIPcamState mTIPcamState[VEHPLATE_NUM];
//extern TIPcamState mTIPcam900State[VEHPLATE900_NUM];

extern CfirewallClient *pCfirewallClient[FIREWARE_NUM];
extern CswitchClient *pCswitchClient[IPSWITCH_NUM];
extern Artc *pCArtRSU[RSUCTL_NUM];;					//RSU对象
extern Huawei *pCHWRSU[RSUCTL_NUM];;					//RSU对象
extern IPCam *pCVehplate[VEHPLATE_NUM];
extern IPCam *pCVehplate900[VEHPLATE900_NUM];
extern CsshClient *pCsshClient[ATLAS_NUM];			//ATLAS对象

bool isIPAddressValid(const char* pszIPAddr)
{
    if (!pszIPAddr) return false; //若pszIPAddr为空
    char IP1[100],cIP[4];
    int len = strlen(pszIPAddr);
    int i = 0,j=len-1;
    int k, m = 0,n=0,num=0;
    //去除首尾空格(取出从i-1到j+1之间的字符):
    while (pszIPAddr[i++] == ' ');
    while (pszIPAddr[j--] == ' ');

    for (k = i-1; k <= j+1; k++)
    {
        IP1[m++] = *(pszIPAddr + k);
    }
    IP1[m] = '\0';

    char *p = IP1;

    while (*p!= '\0')
    {
        if (*p == ' ' || *p<'0' || *p>'9') return false;
        cIP[n++] = *p; //保存每个子段的第一个字符，用于之后判断该子段是否为0开头

        int sum = 0;  //sum为每一子段的数值，应在0到255之间  
        while (*p != '.'&&*p != '\0')
        {  
          if (*p == ' ' || *p<'0' || *p>'9') return false;  
          sum = sum * 10 + *p-48;  //每一子段字符串转化为整数  
          p++;
        }  
        if (*p == '.') {  
            if ((*(p - 1) >= '0'&&*(p - 1) <= '9') && (*(p + 1) >= '0'&&*(p + 1) <= '9'))//判断"."前后是否有数字，若无，则为无效IP，如“1.1.127.”  
                num++;  //记录“.”出现的次数，不能大于3
            else
                return false;
        };
        if ((sum > 255) || (sum > 0 && cIP[0] =='0')||num>3) return false;//若子段的值>255或为0开头的非0子段或“.”的数目>3，则为无效IP
  
        if (*p != '\0') p++;  
        n = 0;
    }  
    if (num != 3) return false;  
    return true;
}  

bool jsonStrReader(char* jsonstrin, int lenin, char* jsonstrout, int *lenout)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	REMOTE_CONTROL *pRCtrl;
	int messagetype=-1;
	int valueint;
	int opt=SFLAG_READ;
	string mstrdata;

	cJSON *json=0, *jsonkey=0, *jsonvalue=0;
	//解析数据包
	json = cJSON_Parse(jsonstrin);
	if(json==0) return false;

	jsonkey = cJSON_GetObjectItem(json, "messagetype");
	if(jsonkey!=0)				
	{
		messagetype = jsonkey->valueint;
	}
	jsonkey = cJSON_GetObjectItem(json, "opt");
	if(jsonkey!=0)				
	{
		opt = jsonkey->valueint;
	}
printf("jsonStrReader messagetype=%d\n",messagetype);
	switch (messagetype)
	{
		case NETCMD_LOGIN:			//0 用户登录
			pRCtrl=stuRemote_Ctrl;
			memset(pRCtrl,0,sizeof(REMOTE_CONTROL));
			jsonstrRCtrlReader(jsonstrin,lenin,(UINT8 *)pRCtrl);//将json字符串转换成结构体
			memset(jsonstrout,0,JSON_LEN);
			*lenout=0;
			SetjsonAuthorityStr(messagetype,jsonstrout,lenout);
			break;
		case NETCMD_CONFIG_NETWORK:		//5 读取/设置网口1
			if(opt==SFLAG_READ)
			{
				SetjsonIPStr(messagetype,mstrdata);
				*lenout = mstrdata.size();
				memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			}
			else if(opt==SFLAG_WRITE)
			{
				string iptmp,masktmp,gatewaytmp;
				iptmp=pConf->StrIP;masktmp=pConf->StrMask;gatewaytmp=pConf->StrGateway;//保存原IP设置
				jsonstrIpInfoReader(jsonstrin,lenin);//将json字符串转换成结构体
				SetjsonReceiveOKStr(messagetype,jsonstrout,lenout);
				SetIPinfo();
				if(iptmp!=pConf->StrIP || masktmp!=pConf->StrMask || gatewaytmp!=pConf->StrGateway)	//IP设置更改
					stuRemote_Ctrl->SysReset=SYSRESET; 	//等待重启
			}
			break;
		case NETCMD_SEND_DEV_PARAM: 			//12 控制器参数
			if(opt==SFLAG_READ)
			{
				//GetConfig(&vmctrl_param);
				jsonStrVMCtlParamWriterXY(messagetype,(char*)&VMCtl_Config,mstrdata);
				*lenout = mstrdata.size();
				memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			}
			else if(opt==SFLAG_WRITE)
			{
				jsonstrVmCtlParamReaderXY(jsonstrin,lenin,(UINT8*)&VMCtl_Config);//将json字符串转换成结构体
				SetjsonReceiveOKStr(messagetype,jsonstrout,lenout);
			}
			break;
		case NETCMD_SEND_RSU_PARAM: 			//14 RSU天线参数
			memset(jsonstrout,0,JSON_LEN);
			jsonStrRsuWriterXY(messagetype,mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		case NETCMD_SEND_VEHPLATE_PARAM: 			//15 车牌识别参数
			memset(jsonstrout,0,JSON_LEN);
			jsonStrVehPlateWriter(messagetype,mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		case NETCMD_SEND_SWITCH_INFO: 			//16 交换机状态
			memset(jsonstrout,0,JSON_LEN);
			SetjsonIPSwitchStatusStr(messagetype,mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		case NETCMD_FLAGRUNSTATUS:			//17 门架运行状态
			SetjsonReceiveOKStr(messagetype,jsonstrout,lenout);
			break;
		case NETCMD_REMOTE_CONTROL: 			//18 遥控设备
		case NETCMD_HWCABINET_PARMSET: 			//21 华为机柜参数设置
		case NETCMD_DEAL_LOCKER:			//22  门禁开关锁请求
			pRCtrl=stuRemote_Ctrl;
			memset(pRCtrl,0,sizeof(REMOTE_CONTROL));
			jsonstrRCtrlReader(jsonstrin,lenin,(UINT8 *)pRCtrl);//将json字符串转换成结构体
			RemoteControl((UINT8*)pRCtrl);
			memset(jsonstrout,0,JSON_LEN);
			*lenout=0;
			SetjsonReceiveOKStr(messagetype,jsonstrout,lenout);
			break;
		case NETCMD_SWITCH_STATUS:			//19 回路开关状态
			memset(jsonstrout,0,JSON_LEN);
			jsonStrSwitchStatusWriterXY(messagetype,mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		case NETCMD_HWCABINET_STATUS:			//20  华为机柜状态
			memset(jsonstrout,0,JSON_LEN);
			jsonStrHWCabinetWriter(messagetype,(char*)pCabinetClient[0],mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		case NETCMD_SEND_FIREWALL_INFO:			//23 防火墙状态
			memset(jsonstrout,0,JSON_LEN);
			SetjsonFireWallStatusStr(messagetype,mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		case NETCMD_SEND_ATLAS_INFO: 		//24 ATLAS状态
			memset(jsonstrout,0,JSON_LEN);
			SetjsonAtlasStatusStr(messagetype,mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		case NETCMD_CONFIG_NETWORK2: 	//25 读取/设置网口2
			if(opt==SFLAG_READ)
			{
				SetjsonIP2Str(messagetype,mstrdata);
				*lenout = mstrdata.size();
				memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			}
			else if(opt==SFLAG_WRITE)
			{
				string iptmp,masktmp,gatewaytmp;
				iptmp=pConf->StrIP2;masktmp=pConf->StrMask2;gatewaytmp=pConf->StrGateway2;//保存原IP设置
				jsonstrIpInfoReader(jsonstrin,lenin);//将json字符串转换成结构体
				SetIPinfo2();
				SetjsonReceiveOKStr(messagetype,jsonstrout,lenout);
				if(iptmp!=pConf->StrIP2 || masktmp!=pConf->StrMask2 || gatewaytmp!=pConf->StrGateway2)	//IP设置更改
					stuRemote_Ctrl->SysReset=SYSRESET; 	//等待重启
			}
			break;
		case NETCMD_SEND_SPD_AI_PARAM:	//27 防雷器参数
			if(opt==SFLAG_READ)
			{
				SetjsonSpdAIStatusStr(messagetype,mstrdata);
				*lenout = mstrdata.size();
				memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			}
			else if(opt==SFLAG_WRITE)
			{
				pRCtrl=stuRemote_Ctrl;
				memset(pRCtrl,0,sizeof(REMOTE_CONTROL));
				jsonstrSPDReader(jsonstrin,lenin,(UINT8 *)pRCtrl);//将json字符串转换成结构体
				RemoteControl((UINT8*)pRCtrl);
				memset(jsonstrout,0,JSON_LEN);
				*lenout=0;
				SetjsonReceiveOKStr(messagetype,jsonstrout,lenout);
			}
			break;
		case NETCMD_SEND_SPD_RES_PARAM:	//28 接地电阻参数
			if(opt==SFLAG_READ)
			{
				SetjsonSpdResStatusStr(messagetype,mstrdata);
				*lenout = mstrdata.size();
				memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			}
			break;
		case NETCMD_SEND_VEHPLATE900_PARAM: 			//29 300万全景车牌识别参数
			memset(jsonstrout,0,JSON_LEN);
			jsonStrVehPlate900Writer(messagetype,mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		case NETCMD_SEND_VMCTRL_STATE: 			//30 控制器运行状态
			memset(jsonstrout,0,JSON_LEN);
			jsonStrVMCtrlStateWriter(messagetype,mstrdata);
			*lenout = mstrdata.size();
			memcpy(jsonstrout,mstrdata.c_str(),mstrdata.size());
			break;
		default:
			break;
	
	}
	return true;
}




bool jsonstrRCtrlReader(char* jsonstr, int len, UINT8 *pstuRCtrl)
{
	printf("%s \t\n",jsonstr);
	
	REMOTE_CONTROL *pRCtrl=(REMOTE_CONTROL *)pstuRCtrl;
	CabinetClient *hwDev=pCabinetClient[0];//华为机柜状态
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	SpdClient *pSpd=pCSpdClent;		//SPD防雷器
	int i;
	int valueint,arraysize;
	char key[50],value[128];
	cJSON *json=0, *jsonkey=0, *jsonvalue=0, *jsonlist=0, *jsonitem=0;
	//解析数据包
	json = cJSON_Parse(jsonstr);
	if(json==0) return false;

	int cabineid=0,operate=0;
	memset(pRCtrl,ACT_HOLD,sizeof(REMOTE_CONTROL));
	pRCtrl->hwsetenvtemplowerlimit[0]=ACT_HOLD_FF;	//环境温度告警下限255:保持；-20-20（有效）；-20（缺省值）
	pRCtrl->hwsetenvtemplowerlimit[1]=ACT_HOLD_FF;	//环境温度告警下限255:保持；-20-20（有效）；-20（缺省值）
	pRCtrl->hwsetenvhumidityupperlimit[0]=ACT_HOLD_FF;	//环境湿度告警上限 255:保持；0-100（有效）；95（缺省值）
	pRCtrl->hwsetenvhumidityupperlimit[1]=ACT_HOLD_FF;	//环境湿度告警上限 255:保持；0-100（有效）；95（缺省值）
	pRCtrl->hwsetenvhumiditylowerlimit[0]=ACT_HOLD_FF;	//环境湿度告警下限 255:保持；0-100（有效）；5（缺省值）
	pRCtrl->hwsetenvhumiditylowerlimit[1]=ACT_HOLD_FF;	//环境湿度告警下限 255:保持；0-100（有效）；5（缺省值）
	pRCtrl->hwdcairpowerontemppoint[0]=ACT_HOLD_FF;		//空调开机温度点 255:保持； -20-80（有效）；45(缺省值)
	pRCtrl->hwdcairpowerontemppoint[1]=ACT_HOLD_FF;		//空调开机温度点 255:保持； -20-80（有效）；45(缺省值)
	pRCtrl->hwdcairpowerofftemppoint[0]=ACT_HOLD_FF;		//空调关机温度点  		  255:保持； -20-80（有效）；37(缺省值)
	pRCtrl->hwdcairpowerofftemppoint[1]=ACT_HOLD_FF;		//空调关机温度点  		  255:保持； -20-80（有效）；37(缺省值)
	sprintf(pRCtrl->systemtime,"");						//设置控制器时间

	jsonkey = cJSON_GetObjectItem(json, "sysreset");	//系统重启
	if(jsonkey!=0)
		pRCtrl->SysReset = (jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint);
	
	jsonkey = cJSON_GetObjectItem(json, "setsystemtime");
	if(jsonkey!=0)	sprintf(pRCtrl->systemtime,"%s",jsonkey->valuestring);			//设置控制器时间

	//控制单板复位 0：保持；1：热复位；
	jsonkey = cJSON_GetObjectItem(json, "hwctrlmonequipreset");
	if(jsonkey!=0)	
	{
		if(jsonkey->type==cJSON_String)
			pRCtrl->hwctrlmonequipreset=atoi(jsonkey->valuestring);
		else if(jsonkey->type==cJSON_Int)
			pRCtrl->hwctrlmonequipreset=jsonkey->valueint;
	}
	//AC过压点设置 0:保持；50-600（有效）；280（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetacsuppervoltlimit");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetAcsUpperVoltLimit) 
			pRCtrl->hwsetacsuppervoltlimit=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetAcsUpperVoltLimit.c_str()))
			pRCtrl->hwsetacsuppervoltlimit=jsonkey->valueint;	
	}		
	//AC欠压点设置 0:保持；50-600（有效）；180（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetacslowervoltlimit");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetAcsLowerVoltLimit) 
			pRCtrl->hwsetacslowervoltlimit=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetAcsLowerVoltLimit.c_str()))
			pRCtrl->hwsetacslowervoltlimit=jsonkey->valueint;	
	}
	//设置DC过压点 0:保持；53-600（有效）；58（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetdcsuppervoltlimit");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && atoi(jsonkey->valuestring)!=atoi(hwDev->HUAWEIDevValue.strhwSetDcsUpperVoltLimit.c_str()))
			pRCtrl->hwsetdcsuppervoltlimit=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetDcsUpperVoltLimit.c_str())) 
			pRCtrl->hwsetdcsuppervoltlimit=jsonkey->valueint;	
	}
	//设置DC欠压点 0:保持；35 - 57（有效）；45（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetdcslowervoltlimit");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && atoi(jsonkey->valuestring)!=atoi(hwDev->HUAWEIDevValue.strhwSetDcsLowerVoltLimit.c_str()))
			pRCtrl->hwsetdcslowervoltlimit=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetDcsLowerVoltLimit.c_str())) 
			pRCtrl->hwsetdcslowervoltlimit=jsonkey->valueint;	
	}
	//环境温度告警上限 0:保持；25-80（有效）；55（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetenvtempupperlimit");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetEnvTempUpperLimit[0]) 
			pRCtrl->hwsetenvtempupperlimit[0]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetEnvTempUpperLimit[0].c_str())) 
			pRCtrl->hwsetenvtempupperlimit[0]=jsonkey->valueint;	
	}
	//环境温度告警上限 0:保持；25-80（有效）；55（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetenvtempupperlimit2");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetEnvTempUpperLimit[1]) 
			pRCtrl->hwsetenvtempupperlimit[1]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetEnvTempUpperLimit[1].c_str())) 
			pRCtrl->hwsetenvtempupperlimit[1]=jsonkey->valueint;	
	}
	//环境温度告警下限255:保持；-20-20（有效）；-20（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetenvtemplowerlimit");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetEnvTempLowerLimit[0]) 
			pRCtrl->hwsetenvtemplowerlimit[0]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetEnvTempLowerLimit[0].c_str())) 
			pRCtrl->hwsetenvtemplowerlimit[0]=jsonkey->valueint;	
	}
	//环境温度告警下限255:保持；-20-20（有效）；-20（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetenvtemplowerlimit2");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetEnvTempLowerLimit[1]) 
			pRCtrl->hwsetenvtemplowerlimit[1]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetEnvTempLowerLimit[1].c_str())) 
			pRCtrl->hwsetenvtemplowerlimit[1]=jsonkey->valueint;	
	}
	//环境湿度告警上限 255:保持；0-100（有效）；95（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetenvhumidityupperlimit");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[0]) 
			pRCtrl->hwsetenvhumidityupperlimit[0]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[0].c_str())) 
			pRCtrl->hwsetenvhumidityupperlimit[0]=jsonkey->valueint;	
	}
	//环境湿度告警上限 255:保持；0-100（有效）；95（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetenvhumidityupperlimit2");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[1]) 
			pRCtrl->hwsetenvhumidityupperlimit[1]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[1].c_str())) 
			pRCtrl->hwsetenvhumidityupperlimit[1]=jsonkey->valueint;	
	}
	//环境湿度告警下限 255:保持；0-100（有效）；5（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetenvhumiditylowerlimit");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[0]) 
			pRCtrl->hwsetenvhumiditylowerlimit[0]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[0].c_str())) 
			pRCtrl->hwsetenvhumiditylowerlimit[0]=jsonkey->valueint;	
	}
	//环境湿度告警下限 255:保持；0-100（有效）；5（缺省值）
	jsonkey = cJSON_GetObjectItem(json, "hwsetenvhumiditylowerlimit2");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[1]) 
			pRCtrl->hwsetenvhumiditylowerlimit[1]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[1].c_str())) 
			pRCtrl->hwsetenvhumiditylowerlimit[1]=jsonkey->valueint;	
	}
	//温控模式				0：保持；1：纯风扇模式；2：纯空调模式；3：智能模式；
	jsonkey = cJSON_GetObjectItem(json, "hwcoolingdevicesmode");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwCoolingDevicesMode) 
			pRCtrl->hwcoolingdevicesmode=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwCoolingDevicesMode.c_str())) 
			pRCtrl->hwcoolingdevicesmode=jsonkey->valueint;	
	}
	//空调开机温度点 255:保持； -20-80（有效）；45(缺省值)
	jsonkey = cJSON_GetObjectItem(json, "hwdcairpowerontemppoint");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwDcAirPowerOnTempPoint[0]) 
			pRCtrl->hwdcairpowerontemppoint[0]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwDcAirPowerOnTempPoint[0].c_str())) 
			pRCtrl->hwdcairpowerontemppoint[0]=jsonkey->valueint;	
	}

	//空调开机温度点 255:保持； -20-80（有效）；45(缺省值)
	jsonkey = cJSON_GetObjectItem(json, "hwdcairpowerontemppoint2");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwDcAirPowerOnTempPoint[1]) 
			pRCtrl->hwdcairpowerontemppoint[1]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwDcAirPowerOnTempPoint[1].c_str())) 
			pRCtrl->hwdcairpowerontemppoint[1]=jsonkey->valueint;	
	}
	//空调关机温度点 		  255:保持； -20-80（有效）；37(缺省值)
	jsonkey = cJSON_GetObjectItem(json, "hwdcairpowerofftemppoint");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwDcAirPowerOffTempPoint[0]) 
			pRCtrl->hwdcairpowerofftemppoint[0]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwDcAirPowerOffTempPoint[0].c_str())) 
			pRCtrl->hwdcairpowerofftemppoint[0]=jsonkey->valueint;	
	}
	//空调关机温度点 		  255:保持； -20-80（有效）；37(缺省值)
	jsonkey = cJSON_GetObjectItem(json, "hwdcairpowerofftemppoint2");
	if(jsonkey!=0)
	{
		if(jsonkey->type==cJSON_String && jsonkey->valuestring!=hwDev->HUAWEIDevValue.strhwDcAirPowerOffTempPoint[1]) 
			pRCtrl->hwdcairpowerofftemppoint[1]=atoi(jsonkey->valuestring);	
		else if(jsonkey->type==cJSON_Int && jsonkey->valueint!=atoi(hwDev->HUAWEIDevValue.strhwDcAirPowerOffTempPoint[1].c_str())) 
			pRCtrl->hwdcairpowerofftemppoint[1]=jsonkey->valueint;	
	}
	//控制烟感复位 0：保持；1：不需复位；2：复位
	jsonkey = cJSON_GetObjectItem(json, "hwctrlsmokereset");
	if(jsonkey!=0) 
		pRCtrl->hwctrlsmokereset[0] = (jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint);

	//控制烟感复位 0：保持；1：不需复位；2：复位
	jsonkey = cJSON_GetObjectItem(json, "hwctrlsmokereset2");
	if(jsonkey!=0) 
		pRCtrl->hwctrlsmokereset[1] = (jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint);

	//电子门锁id
	jsonkey = cJSON_GetObjectItem(json, "cabineid");
	if(jsonkey!=0) 
		cabineid = (jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint);

	//电子门锁操作
	jsonkey = cJSON_GetObjectItem(json, "operate");
	if(jsonkey!=0) 
		operate = (jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint);

	//登录用户
	jsonkey = cJSON_GetObjectItem(json, "user");
	if(jsonkey!=0) 
		pConf->user=jsonkey->valuestring;	

	//密码
	jsonkey = cJSON_GetObjectItem(json, "password");
	if(jsonkey!=0) 
		pConf->password=jsonkey->valuestring;	

	//do
	for(i=0;i<SWITCH_COUNT;i++)
	{
		sprintf(key,"do%d",i+1);
		
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0) 
			pRCtrl->doseq[i]=atoi(jsonkey->valuestring); 
	}

	//报警值修改
	jsonkey = cJSON_GetObjectItem(json, "alarm_value");
	if(jsonkey!=0) 
	{
		if(jsonkey->type==cJSON_String && pSpd->stuSpd_Param->rSPD_res.alarm_value!=atoi(jsonkey->valuestring))
			pSpd->stuSpd_Param->rSPD_res.alarm_value=atoi(jsonkey->valuestring); 
		else if(jsonkey->type==cJSON_Int && pSpd->stuSpd_Param->rSPD_res.alarm_value!=jsonkey->valueint)
			pSpd->stuSpd_Param->rSPD_res.alarm_value=jsonkey->valueint;

	}

	//修改设备id
	jsonkey = cJSON_GetObjectItem(json, "id");
	if(jsonkey!=0) 
	{
		if(jsonkey->type==cJSON_String && pSpd->stuSpd_Param->rSPD_res.id!=atoi(jsonkey->valuestring))
			pSpd->stuSpd_Param->rSPD_res.id=atoi(jsonkey->valuestring); 
		else if(jsonkey->type==cJSON_Int && pSpd->stuSpd_Param->rSPD_res.id!=jsonkey->valueint)
			pSpd->stuSpd_Param->rSPD_res.id=jsonkey->valueint; 

	}

	//SPD 列表
    jsonlist = cJSON_GetObjectItem(json, "spdlist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//雷击计数清零
                jsonkey=cJSON_GetObjectItem(jsonitem,"clearcounter");
                if(jsonkey != NULL)
					pRCtrl->DO_spdcnt_clear[i]=(jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint); 
            	//总雷击计数清0
                jsonkey=cJSON_GetObjectItem(jsonitem,"cleartotalcounter");
                if(jsonkey != NULL)
					pRCtrl->DO_totalspdcnt_clear[i]=(jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint); 
            	//雷击时间清0
                jsonkey=cJSON_GetObjectItem(jsonitem,"strucktimerecclear");
                if(jsonkey != NULL)
					pRCtrl->DO_psdtime_clear[i]=(jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint); 
            	//在线时间清0
                jsonkey=cJSON_GetObjectItem(jsonitem,"onlinetimeclear");
                if(jsonkey != NULL)
					pRCtrl->DO_daytime_clear[i]=(jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint); 
            	//漏电流报警阈值
                jsonkey=cJSON_GetObjectItem(jsonitem,"leak_alarm_threshold");
                if(jsonkey != NULL)
					pRCtrl->spdleak_alarm_threshold[i]=(jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint); 
            	//外接漏电流控制
                jsonkey=cJSON_GetObjectItem(jsonitem,"extleakcurrctrl");
                if(jsonkey != NULL)
					pRCtrl->DO_leak_type[i]=(jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint); 
            	//防雷器设备地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"modbus_addr");
                if(jsonkey != NULL)
					pRCtrl->spd_modbus_addr[i]=(jsonkey->type==cJSON_String?atoi(jsonkey->valuestring):jsonkey->valueint); 
            }
        }
    }
	//printf("\n");
	if(cabineid==1 && operate==ACT_UNLOCK) pRCtrl->FrontDoorCtrl=ACT_UNLOCK;//前门电子门锁 0：保持 1：关锁：2：开锁；3无权限
	if(cabineid==2 && operate==ACT_UNLOCK) pRCtrl->BackDoorCtrl=ACT_UNLOCK; 	//后门电子门锁 0：保持 1：关锁：2：开锁；3无权限
	if(cabineid==3 && operate==ACT_UNLOCK) pRCtrl->SideDoorCtrl=ACT_UNLOCK; 		//侧门电子门锁 0：保持 1：关锁：2：开锁；3无权限
	if(cabineid==4 && operate==ACT_UNLOCK) pRCtrl->RightSideDoorCtrl=ACT_UNLOCK; 		//侧门电子门锁 0：保持 1：关锁：2：开锁；3无权限

	return true;
}

bool jsonstrVmCtlParamReader(char* jsonstr, int len, UINT8 *pstPam)
{
	printf("%s \t\n",jsonstr);
	
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	SpdClient *pSpd=pCSpdClent;		//SPD防雷器
	char key[50],value[128],devicename[50];
	int valueint,arraysize;
	cJSON *json=0, *jsonkey=0, *jsonvalue=0, *jsonlist=0, *jsonitem=0, *jsondeviceModel=0;
	int i,j,k,vpcount;
	bool locker_changed = false;
	char deal_do[SWITCH_COUNT];//处理do标记;
	FDATA dummy;
	//解析数据包
	json = cJSON_Parse(jsonstr);
	if(json==0) return false;
	

	//门架信息
	jsonkey = cJSON_GetObjectItem(json, "cabinettype");
	if(jsonkey!=0)				//机柜类型	1：华为双机柜双开门；2：华为双机柜单开门；3：华为单机柜双开门；4：华为单机柜单开门
	{
		if(jsonkey->valuestring!=pConf->StrCabinetType)
		{
			pConf->StrCabinetType=jsonkey->valuestring;
			Setconfig("CabinetType=",pConf->StrCabinetType.c_str());
		}
	}
	jsonkey = cJSON_GetObjectItem(json, "flagnetroadid");
	if(jsonkey!=0)				//ETC 门架路网编号
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFlagNetRoadID)
		{
			pConf->StrFlagNetRoadID=value;
			Setconfig("FlagNetRoadID=",value);
		}
	}
	//ETC 门架路段编号
	jsonkey = cJSON_GetObjectItem(json, "flagroadid");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFlagRoadID)
		{
			pConf->StrFlagRoadID=value;
			Setconfig("FlagRoadID=",value);
		}
	}
	
	//ETC 门架编号
	jsonkey = cJSON_GetObjectItem(json, "flagid");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFlagID)
		{
			// 更新屏幕上的字母
//			ScreenFlagSet(LETTER_SET);
			pConf->StrFlagID=value;
			Setconfig("FlagID=",value);
		}
	}

	//ETC 门架序号
	jsonkey = cJSON_GetObjectItem(json, "posid");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrPosId)
		{
			pConf->StrPosId=value;
			Setconfig("PosId=",value);
		}
	}
	
	//行车方向
	jsonkey = cJSON_GetObjectItem(json, "direction");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrDirection)
		{
			pConf->StrDirection=value;
			Setconfig("Direction=",value);
		}
	}
	
	//行车方向说明
	jsonkey = cJSON_GetObjectItem(json, "dirdescription");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrDirDescription)
		{
			pConf->StrDirDescription=value;
			Setconfig("DirDescription=",value);
		}
	}
	
	//参数设置
	//华为动环服务器个数
	jsonkey = cJSON_GetObjectItem(json, "hwservercount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWServerCount)
		{
			pConf->StrHWServerCount=value;
			Setconfig("HWServerCount=",value);
		}
	}
	//华为服务器地址
	jsonkey = cJSON_GetObjectItem(json, "hwserver");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWServer)
		{
			pConf->StrHWServer=value;
			Setconfig("HWServer=",value);
//			ClearvecWalkSnmp();
		}
	}
	//SNMP GET 密码
	jsonkey = cJSON_GetObjectItem(json, "hwgetpasswd");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWGetPasswd)
		{
			pConf->StrHWGetPasswd=value;
			Setconfig("HWGetPasswd=",value);
//			ClearvecWalkSnmp();
		}
	}
	//SNMP SET 密码
	jsonkey = cJSON_GetObjectItem(json, "hwsetpasswd");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWSetPasswd)
		{
			pConf->StrHWSetPasswd=value;
			Setconfig("HWSetPasswd=",value);
//			ClearvecWalkSnmp();
		}
	}

	//金晟安服务器地址
	jsonkey = cJSON_GetObjectItem(json, "hwserver2");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWServer2)
		{
			pConf->StrHWServer2=value;
			Setconfig("HWServer2=",value);
//			ClearvecWalkSnmp();
		}
	}

	//金晟安 SNMP GET 密码
	jsonkey = cJSON_GetObjectItem(json, "hwgetpasswd2");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWGetPasswd2)
		{
			pConf->StrHWGetPasswd2=value;
			Setconfig("HWGetPasswd2=",value);
//			ClearvecWalkSnmp();
		}
	}

	//金晟安 SNMP SET 密码
	jsonkey = cJSON_GetObjectItem(json, "hwsetpasswd2");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWSetPasswd2)
		{
			pConf->StrHWSetPasswd2=value;
			Setconfig("HWSetPasswd2=",value);
//			ClearvecWalkSnmp();
		}
	}

	//服务器1推送地址
	jsonkey = cJSON_GetObjectItem(json, "serverurl1");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrServerURL1)
		{
			pConf->StrServerURL1=value;
			Setconfig("ServerURL1=",value);
		}
	}

	//服务器2推送地址
	jsonkey = cJSON_GetObjectItem(json, "serverurl2");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrServerURL2)
		{
			pConf->StrServerURL2=value;
			Setconfig("ServerURL2=",value);
		}
	}

	//服务器3推送地址
	jsonkey = cJSON_GetObjectItem(json, "serverurl3");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrServerURL3)
		{
			pConf->StrServerURL3=value;
			Setconfig("ServerURL3=",value);
		}
	}

	//门锁4推送地址
	jsonkey = cJSON_GetObjectItem(json, "serverurl4");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrServerURL4)
		{
			pConf->StrServerURL4=value;
			Setconfig("ServerURL4=",value);
		}
	}

	//控制器接收地址
	jsonkey = cJSON_GetObjectItem(json, "stationurl");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrStationURL)
		{
			pConf->StrStationURL=value;
			Setconfig("StationURL=",value);
		}
	}

	//RSU控制器数量
	jsonkey = cJSON_GetObjectItem(json, "rsucount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrRSUCount && atoi(value)>=0 && atoi(value)<=RSUCTL_NUM)
		{
			pConf->StrRSUCount=value;
			Setconfig("RSUCount=",value);
		}
	}

	//RSU控制器类型
	jsonkey = cJSON_GetObjectItem(json, "rsutype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrRSUType)
		{
			pConf->StrRSUType=value;
			Setconfig("RSUType=",value);
		}
	}
	
	for(i=0;i<RSUCTL_NUM;i++)
	{		
		sprintf(key,"rsu%dip",i+1);//RSUIP地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrRSUIP[i])
			{
				pConf->StrRSUIP[i]=value;
				sprintf(key,"RSU%dIP=",i+1);//RSUIP地址
				Setconfig(key,value);
			}
		}
	}
	
	//识别仪数量
	jsonkey = cJSON_GetObjectItem(json, "vehplatecount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrVehPlateCount && atoi(value)>=0 && atoi(value)<=VEHPLATE_NUM)
		{
			pConf->StrVehPlateCount=value;
			Setconfig("VehPlateCount=",value);
		}
	}
	
	for(i=0;i<VEHPLATE_NUM;i++)
	{
		sprintf(key,"vehplate%dip",i+1);//识别仪IP地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrVehPlateIP[i])
			{
				pConf->StrVehPlateIP[i]=value;
				sprintf(key,"VehPlate%dIP=",i+1);
				Setconfig(key,value);
			}
		}
		
		sprintf(key,"vehplate%dport",i+1);//识别仪端口
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrVehPlatePort[i])
			{
				pConf->StrVehPlatePort[i]=value;
				sprintf(key,"VehPlate%dPort=",i+1);
				Setconfig(key,value);
			}
		}
	}
	
	//900识别仪数量	
	jsonkey = cJSON_GetObjectItem(json, "vehplate900count");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrVehPlate900Count && atoi(value)>=0 && atoi(value)<=VEHPLATE900_NUM)
		{
			pConf->StrVehPlate900Count=value;
			Setconfig("VehPlate900Count=",value);
		}
	}
	for(i=0;i<VEHPLATE900_NUM;i++)
	{
		sprintf(key,"vehplate900%dip",i+1);//900识别仪IP地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrVehPlate900IP[i])
			{
				pConf->StrVehPlate900IP[i]=value;
				sprintf(key,"VehPlate900%dIP=",i+1);
				Setconfig(key,value);
			}
		}
		sprintf(key,"vehplate900%dport",i+1);//900识别仪端口
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrVehPlate900Port[i])
			{
				pConf->StrVehPlate900Port[i]=value;
				sprintf(key,"VehPlate900%dPort=",i+1);
				Setconfig(key,value);
			}
		}
	}
	
	//监控摄像头数量
	jsonkey = cJSON_GetObjectItem(json, "camcount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrCAMCount && atoi(value)>=0 && atoi(value)<=CAM_NUM)
		{
			pConf->StrCAMCount=value;
			Setconfig("CAMCount=",value);
		}
	}
	for(i=0;i<CAM_NUM;i++)
	{
		sprintf(key,"cam%dip",i+1);//监控摄像头IP地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrCAMIP[i])
			{
				pConf->StrCAMIP[i]=value;
				sprintf(key,"CAM%dIP=",i+1);
				Setconfig(key,value);
			}
		}
		sprintf(key,"cam%dport",i+1);//监控摄像头端口
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrCAMPort[i])
			{
				pConf->StrCAMPort[i]=value;
				sprintf(key,"CAM%dPort=",i+1);
				Setconfig(key,value);
			}
		}
		sprintf(key,"cam%dkey",i+1);//监控摄像头用户名密码
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrCAMKey[i])
			{
				pConf->StrCAMKey[i]=value;
				sprintf(key,"CAM%dKey=",i+1);
				Setconfig(key,value);
			}
		}
	}
	
	//交换机数量
	jsonkey = cJSON_GetObjectItem(json, "ipswitchcount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrIPSwitchCount && atoi(value)>=0 && atoi(value)<=IPSWITCH_NUM)
		{
			pConf->StrIPSwitchCount=value;
			Setconfig("SwitchCount=",value);
		}
	}
	//交换机类型
	jsonkey = cJSON_GetObjectItem(json, "ipswitchtype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrIPSwitchType)
		{
			pConf->StrIPSwitchType=value;
			pConf->IntIPSwitchType=atoi(value);
			Setconfig("SwitchType=",value);
		}
	}
	
	for(i=0;i<IPSWITCH_NUM;i++)
	{
		sprintf(key,"ipswitch%dip",i+1);//交换机IP地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrIPSwitchIP[i])
			{
				pConf->StrIPSwitchIP[i]=value;
				sprintf(key,"Switch%dIP=",i+1);
				Setconfig(key,value);
//				ClearvecWalkSnmp();
			}
		}
		sprintf(key,"ipswitch%dgetpasswd",i+1);//交换机get密码
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrIPSwitchGetPasswd[i])
			{
				pConf->StrIPSwitchGetPasswd[i]=value;
				sprintf(key,"Switch%dGetPasswd=",i+1);
				Setconfig(key,value);
//				ClearvecWalkSnmp();
			}
		}
		sprintf(key,"ipswitch%dsetpasswd",i+1);//交换机set密码
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrIPSwitchSetPasswd[i])
			{
				pConf->StrIPSwitchSetPasswd[i]=value;
				sprintf(key,"Switch%dSetPasswd=",i+1);
				Setconfig(key,value);
//				ClearvecWalkSnmp();
			}
		}
	}
	
	//防火墙数量
	jsonkey = cJSON_GetObjectItem(json, "firewarecount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFireWareCount && atoi(value)>=0 && atoi(value)<=FIREWARE_NUM)
		{
			pConf->StrFireWareCount=value;
			Setconfig("FireWareCount=",value);
		}
	}
	//防火墙类型
	jsonkey = cJSON_GetObjectItem(json, "firewaretype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFireWareType)
		{
			pConf->StrFireWareType=value;
			pConf->IntFireWareType=atoi(value);
			Setconfig("FireWareType=",value);
		}
	}
	for(i=0;i<FIREWARE_NUM;i++)
	{
		sprintf(key,"fireware%dip",i+1);//防火墙IP地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrFireWareIP[i])
			{
				pConf->StrFireWareIP[i]=value;
				sprintf(key,"FireWare%dIP=",i+1);
				Setconfig(key,value);
//				ClearvecWalkSnmp();
			}
		}
		sprintf(key,"fireware%dgetpasswd",i+1);//防火墙get密码
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrFireWareGetPasswd[i])
			{
				pConf->StrFireWareGetPasswd[i]=value;
				sprintf(key,"FireWare%dGetPasswd=",i+1);
				Setconfig(key,value);
//				ClearvecWalkSnmp();
			}
		}
		sprintf(key,"fireware%dsetpasswd",i+1);//防火墙set密码
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrFireWareSetPasswd[i])
			{
				pConf->StrFireWareSetPasswd[i]=value;
				sprintf(key,"FireWare%dSetPasswd=",i+1);
				Setconfig(key,value);
//				ClearvecWalkSnmp();
			}
		}
	}
	
	//Atlas类型
	jsonkey = cJSON_GetObjectItem(json, "atlastype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrAtlasType)
		{
			pConf->StrAtlasType=value;
			Setconfig("AtlasType=",value);
		}
	}
	//Atlas数量
	jsonkey = cJSON_GetObjectItem(json, "atlascount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrAtlasCount && atoi(value)>=0 && atoi(value)<=ATLAS_NUM)
		{
			pConf->StrAtlasCount=value;
			Setconfig("AtlasCount=",value);
		}
	}
	for(i=0;i<ATLAS_NUM;i++)
	{
		sprintf(key,"atlas%dip",i+1);//AtlasIP地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrAtlasIP[i])
			{
				pConf->StrAtlasIP[i]=value;
				sprintf(key,"Atlas%dIP=",i+1);
				Setconfig(key,value);
			}
		}
		sprintf(key,"atlas%dpasswd",i+1);//Atlas密码
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			if(jsonkey->valuestring=="")//如果是空值，恢复成默认值
				sprintf(value,"admin:Huawei12#$:Huawei@SYS3");
			else
				sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrAtlasPasswd[i])
			{
				pConf->StrAtlasPasswd[i]=value;
				sprintf(key,"Atlas%dPasswd=",i+1);
				Setconfig(key,value);
			}
		}
	}
	
	//防雷器SPD数量
	jsonkey = cJSON_GetObjectItem(json, "spdcount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDCount && atoi(value)>=0 && atoi(value)<=SPD_NUM)
		{
			pConf->SPD_num =atoi(value);
			pConf->StrSPDCount=value;
			Setconfig("SPDCount=",value);
			// 数量改变，全部初始化
			for(i=0;i<SPD_NUM+RES_NUM;i++)
			{
				pConf->HZ_reset_pre[i] = true;
			}
		}
	}
	//防雷器类型
	jsonkey = cJSON_GetObjectItem(json, "spdtype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDType)
		{
			pConf->SPD_Type = atoi(value);
			pConf->StrSPDType=value;
			Setconfig("SPDType=",value);
			// 类型改变，全部初始化
			for(i=0;i<SPD_NUM+RES_NUM;i++)
			{
				pConf->HZ_reset_pre[i] = true;
			}
		}
	}
	for(i=0;i<SPD_NUM;i++)
	{
		sprintf(key,"spd%dip",i+1);//SPDIP地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrSPDIP[i])
			{
				pConf->StrSPDIP[i]=value;
				sprintf(key,"SPD%dIP=",i+1);
				Setconfig(key,value);
				pConf->HZ_reset_pre[i] = true;
			}
		}
		sprintf(key,"spd%dport",i+1);//SPD端口
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrSPDPort[i])
			{
				pConf->StrSPDPort[i]=value;
				sprintf(key,"SPD%dPort=",i+1);
				Setconfig(key,value);
				pConf->HZ_reset_pre[i] = true;
			}
		}
		sprintf(key,"spd%daddr",i+1);//SPD硬件地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrSPDAddr[i])
			{
				pConf->SPD_Address[i] = atoi(value);
				pConf->StrSPDAddr[i]=value;
				sprintf(key,"SPD%dAddr=",i+1);
				Setconfig(key,value);
				pConf->HZ_reset_pre[i] = true;
			}
		}
	}
	//防雷器接地电阻IP
	jsonkey = cJSON_GetObjectItem(json, "spdresip");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDIP[SPD_NUM])
		{
			pConf->StrSPDIP[SPD_NUM]=value;
			sprintf(key,"SPDResIP=");
			Setconfig(key,value);
			pConf->HZ_reset_pre[SPD_NUM] = true;
		}
	}
	//防雷器接地电阻端口
	jsonkey = cJSON_GetObjectItem(json, "spdresport");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDPort[SPD_NUM])
		{
			pConf->StrSPDPort[SPD_NUM]=value;
			sprintf(key,"SPDResPort=");
			Setconfig(key,value);
			pConf->HZ_reset_pre[SPD_NUM] = true;
		}
	}
	//防雷器接地电阻地址
	jsonkey = cJSON_GetObjectItem(json, "spdresid");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDAddr[SPD_NUM])
		{
			pConf->SPD_Address[SPD_NUM] = atoi(value);
			pConf->StrSPDAddr[SPD_NUM]=value;
			sprintf(key,"SPDResAddr=");
			Setconfig(key,value);
			pConf->HZ_reset_pre[SPD_NUM] = true;
			// 不要去修改具体的硬件设备本身的地址，我们只是对接协议，不改地址
//			pSpd->Ex_SPD_Set_Process(SPD_RES_SET,RES_ID_ADDR,dummy,pConf->SPD_Address[SPD_NUM]);
		}
	}
	
	for(i=0;i<LOCK_NUM;i++)
	{
		sprintf(key,"adrrlock%d",i+1);//门锁地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrAdrrLock[i])
			{
				pConf->StrAdrrLock[i]=value;
				sprintf(key,"LOCKADD%d=",i+1);
				Setconfig(key,value);
				locker_changed = true;	// 锁的配置发生变化
			}
		}
	}	
	for(i=0;i<POWER_BD_NUM;i++)
	{
		sprintf(key,"poweraddr%d",i+1);//电源板地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrAdrrPower[i])
			{
				pConf->StrAdrrPower[i]=value;
				sprintf(key,"POWERBDADD%d=",i+1);
				Setconfig(key,value);
			}
		}
	}	
	//硬件ID
	jsonkey = cJSON_GetObjectItem(json, "hardwareid");
	if(jsonkey!=0)						 
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrID)
		{
			pConf->StrID=value;
			Setconfig("ID=",value);
		}
	}
	for(i=0;i<VA_METER_BD_NUM;i++)
	{
		sprintf(key,"adrrvameter%d",i+1);//电能表地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrAdrrVAMeter[i])
			{
				pConf->StrAdrrVAMeter[i]=value;
				sprintf(key,"VAMETERADDR%d=",i+1);
				Setconfig(key,value);
			}
		}
	}
	//CPU使用率报警阈值
	jsonkey = cJSON_GetObjectItem(json, "cpualarmvalue");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrCpuAlarmValue)
		{
			pConf->StrCpuAlarmValue=value;
			Setconfig("CpuAlarmValue=",value);
		}
	}
	//CPU温度报警阈值
	jsonkey = cJSON_GetObjectItem(json, "cputempalarmvalue");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrCpuTempAlarmValue)
		{
			pConf->StrCpuTempAlarmValue=value;
			Setconfig("CpuTempAlarmValue=",value);
		}
	}
	//内存使用率报警阈值
	jsonkey = cJSON_GetObjectItem(json, "memalarmvalue");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrMemAlarmValue)
		{
			pConf->StrMemAlarmValue=value;
			Setconfig("MemAlarmValue=",value);
		}
	}
	//do数量
	jsonkey = cJSON_GetObjectItem(json, "do_count");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrDoCount && atoi(value)>=0 && atoi(value)<=SWITCH_COUNT)
		{
			pConf->StrDoCount=value;
			Setconfig("DO_Count=",value);
		}
	}
	
	for(i=0;i<RSUCTL_NUM;i++)
	{
		sprintf(devicename,"rsu%d_do",i+1);
		jsonkey = cJSON_GetObjectItem(json, devicename);
		if(jsonkey!=0)			//有这个设备	
		{
			sprintf(value,"%s",jsonkey->valuestring);
			j=atoi(value)-1;
			deal_do[j]=1;//处理do标记
			if(j>=0 && j<SWITCH_COUNT)
			{
				if(pConf->StrDeviceNameSeq[j]!=devicename) //设备改变
				{
					string strold=pConf->StrDeviceNameSeq[j];
					pConf->StrDeviceNameSeq[j]=devicename;
					string strnew=pConf->StrDeviceNameSeq[j];
					transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
					strnew=strnew+"=";
					Setconfig(strnew.c_str(),value);//DO映射
					
					if(strold=="")		//原来没有设备，添加do映射
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
					}
					transform(strold.begin(), strold.end(), strold.begin(), ::toupper);
					strold=strold+"=";
					Setconfig(strold.c_str(),"");//清除原来DO映射
				}
			}
			else if(j==-1) //do改为无设备
			{
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrDeviceNameSeq[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						sprintf(key,"do%d_do",k+1);
						pConf->StrDeviceNameSeq[k]=key; //设备名
						sprintf(key,"%d",k+1);
						pConf->StrDoSeq[k] = key;	//对应DO
					}
				}
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrUnWireDevName[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						pConf->StrUnWireDevName[k]=""; //设备名
						pConf->StrUnWireDo[k] = "";	//对应DO
					}
				}
			}
			else if(j>=100 && j<100+SWITCH_COUNT)
			{
				string strnew=devicename;
				transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
				strnew=strnew+"=";
				Setconfig(strnew.c_str(),value);//DO映射
				
				pConf->StrUnWireDevName[j-100]=devicename;
				pConf->StrUnWireDo[j-100] = value; //对应DO
			}
		}
	}
	for(i=0;i<VEHPLATE_NUM;i++)
	{
		sprintf(devicename,"vehplate%d_do",i+1);
		jsonkey = cJSON_GetObjectItem(json, devicename);
		if(jsonkey!=0)			//有这个设备	
		{
			sprintf(value,"%s",jsonkey->valuestring);
			j=atoi(value)-1;
			deal_do[j]=1;//处理do标记
//printf("value=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",atoi(value),StrDeviceNameSeq[i].c_str(),StrDoSeq[i].c_str(),devicename,value);
			if(j>=0 && j<SWITCH_COUNT)
			{
				if(pConf->StrDeviceNameSeq[j]!=devicename) 	//do改设备
				{
					string strold=pConf->StrDeviceNameSeq[j];
					pConf->StrDeviceNameSeq[j]=devicename;
					string strnew=pConf->StrDeviceNameSeq[j];
					transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
					strnew=strnew+"=";
					Setconfig(strnew.c_str(),value);//DO映射
					
					if(strold=="")		//原来没有设备，添加do映射
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
					}
					transform(strold.begin(), strold.end(), strold.begin(), ::toupper);
					strold=strold+"=";
					Setconfig(strold.c_str(),"");//清除原来DO映射
				}
			}
			else if(j==-1) //do改为无设备
			{
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrDeviceNameSeq[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						sprintf(key,"do%d_do",k+1);
						pConf->StrDeviceNameSeq[k]=key; //设备名
						sprintf(key,"%d",k+1);
						pConf->StrDoSeq[k] = key;	//对应DO
					}
				}
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrUnWireDevName[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						pConf->StrUnWireDevName[k]=""; //设备名
						pConf->StrUnWireDo[k] = "";	//对应DO
					}
				}
			}
			else if(j>=100 && j<100+SWITCH_COUNT)
			{
				string strnew=devicename;
				transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
				strnew=strnew+"=";
				Setconfig(strnew.c_str(),value);//DO映射
				
				pConf->StrUnWireDevName[j-100]=devicename;
				pConf->StrUnWireDo[j-100] = value; //对应DO
			}
		}
	}
	for(i=0;i<VEHPLATE900_NUM;i++)
	{
		sprintf(devicename,"vehplate900%d_do",i+1);
		jsonkey = cJSON_GetObjectItem(json, devicename);
		if(jsonkey!=0)			//有这个设备	
		{
			sprintf(value,"%s",jsonkey->valuestring);
			j=atoi(value)-1;
			deal_do[j]=1;//处理do标记
			if(j>=0 && j<SWITCH_COUNT)
			{
				printf("olddev:%s,newdev:%s\n",pConf->StrDeviceNameSeq[j].c_str(),devicename);
				if(pConf->StrDeviceNameSeq[j]!=devicename)
				{
					string strold=pConf->StrDeviceNameSeq[j];
					pConf->StrDeviceNameSeq[j]=devicename;
					string strnew=pConf->StrDeviceNameSeq[j];
					transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
					strnew=strnew+"=";
					Setconfig(strnew.c_str(),value);//DO映射
					
					if(strold=="")		//原来没有设备，添加do映射
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
					}
					transform(strold.begin(), strold.end(), strold.begin(), ::toupper);
					strold=strold+"=";
					Setconfig(strold.c_str(),"");//清除原来DO映射
				}
			}
			else if(j==-1) //do改为无设备
			{
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrDeviceNameSeq[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						sprintf(key,"do%d_do",k+1);
						pConf->StrDeviceNameSeq[k]=key; //设备名
						sprintf(key,"%d",k+1);
						pConf->StrDoSeq[k] = key;	//对应DO
					}
				}
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrUnWireDevName[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						pConf->StrUnWireDevName[k]=""; //设备名
						pConf->StrUnWireDo[k] = "";	//对应DO
					}
				}
			}
			else if(j>=100 && j<100+SWITCH_COUNT)
			{
				string strnew=devicename;
				transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
				strnew=strnew+"=";
				Setconfig(strnew.c_str(),value);//DO映射
				
				pConf->StrUnWireDevName[j-100]=devicename;
				pConf->StrUnWireDo[j-100] = value; //对应DO
			}
		}
	}
	for(i=0;i<CAM_NUM;i++)
	{
		sprintf(devicename,"cam%d_do",i+1);
		jsonkey = cJSON_GetObjectItem(json, devicename);
		if(jsonkey!=0)			//有这个设备	
		{
			sprintf(value,"%s",jsonkey->valuestring);
			j=atoi(value)-1;
			deal_do[j]=1;//处理do标记
			if(j>=0 && j<SWITCH_COUNT)
			{
				if(pConf->StrDeviceNameSeq[j]!=devicename)
				{
					string strold=pConf->StrDeviceNameSeq[j];
					pConf->StrDeviceNameSeq[j]=devicename;
					string strnew=pConf->StrDeviceNameSeq[j];
					transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
					strnew=strnew+"=";
					Setconfig(strnew.c_str(),value);//DO映射
					
					if(strold=="")		//原来没有设备，添加do映射
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
					}
					transform(strold.begin(), strold.end(), strold.begin(), ::toupper);
					strold=strold+"=";
					Setconfig(strold.c_str(),"");//清除原来DO映射
				}
			}
			else if(j==-1) //do改为无设备
			{
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrDeviceNameSeq[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						sprintf(key,"do%d_do",k+1);
						pConf->StrDeviceNameSeq[k]=key; //设备名
						sprintf(key,"%d",k+1);
						pConf->StrDoSeq[k] = key;	//对应DO
					}
				}
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrUnWireDevName[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						pConf->StrUnWireDevName[k]=""; //设备名
						pConf->StrUnWireDo[k] = "";	//对应DO
					}
				}
			}
			else if(j>=100 && j<100+SWITCH_COUNT)
			{
				string strnew=devicename;
				transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
				strnew=strnew+"=";
				Setconfig(strnew.c_str(),value);//DO映射
				
				pConf->StrUnWireDevName[j-100]=devicename;
				pConf->StrUnWireDo[j-100] = value; //对应DO
			}
		}
	}
	for(i=0;i<FIREWARE_NUM;i++)
	{
		sprintf(devicename,"fireware%d_do",i+1);
		jsonkey = cJSON_GetObjectItem(json, devicename);
		if(jsonkey!=0)			//有这个设备	
		{
			sprintf(value,"%s",jsonkey->valuestring);
			j=atoi(value)-1;
			deal_do[j]=1;//处理do标记
			if(j>=0 && j<SWITCH_COUNT)
			{
				if(pConf->StrDeviceNameSeq[j]!=devicename)
				{
					string strold=pConf->StrDeviceNameSeq[j];
					pConf->StrDeviceNameSeq[j]=devicename;
					string strnew=pConf->StrDeviceNameSeq[j];
					transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
					strnew=strnew+"=";
					Setconfig(strnew.c_str(),value);//DO映射
					
					if(strold=="")		//原来没有设备，添加do映射
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
					}
					transform(strold.begin(), strold.end(), strold.begin(), ::toupper);
					strold=strold+"=";
					Setconfig(strold.c_str(),"");//清除原来DO映射
				}
			}
			else if(j==-1) //do改为无设备
			{
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrDeviceNameSeq[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						sprintf(key,"do%d_do",k+1);
						pConf->StrDeviceNameSeq[k]=key; //设备名
						sprintf(key,"%d",k+1);
						pConf->StrDoSeq[k] = key;	//对应DO
					}
				}
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrUnWireDevName[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						pConf->StrUnWireDevName[k]=""; //设备名
						pConf->StrUnWireDo[k] = "";	//对应DO
					}
				}
			}
			else if(j>=100 && j<100+SWITCH_COUNT)
			{
				string strnew=devicename;
				transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
				strnew=strnew+"=";
				Setconfig(strnew.c_str(),value);//DO映射
				
				pConf->StrUnWireDevName[j-100]=devicename;
				pConf->StrUnWireDo[j-100] = value; //对应DO
			}
		}
	}
	for(i=0;i<IPSWITCH_NUM;i++)
	{
		sprintf(devicename,"ipswitch%d_do",i+1);
		jsonkey = cJSON_GetObjectItem(json, devicename);
		if(jsonkey!=0)			//有这个设备	
		{
			sprintf(value,"%s",jsonkey->valuestring);
			j=atoi(value)-1;
			deal_do[j]=1;//处理do标记
			if(j>=0 && j<SWITCH_COUNT)
			{
				if(pConf->StrDeviceNameSeq[j]!=devicename)
				{
					string strold=pConf->StrDeviceNameSeq[j];
					pConf->StrDeviceNameSeq[j]=devicename;
					string strnew=pConf->StrDeviceNameSeq[j];
					transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
					strnew=strnew+"=";
					Setconfig(strnew.c_str(),value);//DO映射
					
					if(strold=="")		//原来没有设备，添加do映射
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
					}
					transform(strold.begin(), strold.end(), strold.begin(), ::toupper);
					strold=strold+"=";
					Setconfig(strold.c_str(),"");//清除原来DO映射
				}
			}
			else if(j==-1) //do改为无设备
			{
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrDeviceNameSeq[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						sprintf(key,"do%d_do",k+1);
						pConf->StrDeviceNameSeq[k]=key; //设备名
						sprintf(key,"%d",k+1);
						pConf->StrDoSeq[k] = key;	//对应DO
					}
				}
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrUnWireDevName[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						pConf->StrUnWireDevName[k]=""; //设备名
						pConf->StrUnWireDo[k] = "";	//对应DO
					}
				}
			}
			else if(j>=100 && j<100+SWITCH_COUNT)
			{
				string strnew=devicename;
				transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
				strnew=strnew+"=";
				Setconfig(strnew.c_str(),value);//DO映射
				
				pConf->StrUnWireDevName[j-100]=devicename;
				pConf->StrUnWireDo[j-100] = value; //对应DO
			}
		}
	}
	for(i=0;i<ATLAS_NUM;i++)
	{
		sprintf(devicename,"atlas%d_do",i+1);
		jsonkey = cJSON_GetObjectItem(json, devicename);
		if(jsonkey!=0)			//有这个设备	
		{
			sprintf(value,"%s",jsonkey->valuestring);
			j=atoi(value)-1;
			deal_do[j]=1;//处理do标记
			if(j>=0 && j<SWITCH_COUNT)
			{
				if(pConf->StrDeviceNameSeq[j]!=devicename)
				{
					string strold=pConf->StrDeviceNameSeq[j];
					pConf->StrDeviceNameSeq[j]=devicename;
					string strnew=pConf->StrDeviceNameSeq[j];
					transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
					strnew=strnew+"=";
					Setconfig(strnew.c_str(),value);//DO映射
					
					if(strold=="")		//原来没有设备，添加do映射
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
					}
					transform(strold.begin(), strold.end(), strold.begin(), ::toupper);
					strold=strold+"=";
					Setconfig(strold.c_str(),"");//清除原来DO映射
				}
			}
			else if(j==-1) //do改为无设备
			{
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrDeviceNameSeq[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						sprintf(key,"do%d_do",k+1);
						pConf->StrDeviceNameSeq[k]=key; //设备名
						sprintf(key,"%d",k+1);
						pConf->StrDoSeq[k] = key;	//对应DO
					}
				}
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrUnWireDevName[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						pConf->StrUnWireDevName[k]=""; //设备名
						pConf->StrUnWireDo[k] = "";	//对应DO
					}
				}
			}
			else if(j>=100 && j<100+SWITCH_COUNT)
			{
				string strnew=devicename;
				transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
				strnew=strnew+"=";
				Setconfig(strnew.c_str(),value);//DO映射
				
				pConf->StrUnWireDevName[j-100]=devicename;
				pConf->StrUnWireDo[j-100] = value; //对应DO
			}
		}
	}
	for(i=0;i<ANTENNA_NUM;i++)
	{
		sprintf(devicename,"antenna%d_do",i+1);
		jsonkey = cJSON_GetObjectItem(json, devicename);
		if(jsonkey!=0)			//有这个设备	
		{
			sprintf(value,"%s",jsonkey->valuestring);
			j=atoi(value)-1;
			deal_do[j]=1;//处理do标记
			if(j>=0 && j<SWITCH_COUNT)
			{
				if(pConf->StrDeviceNameSeq[j]!=devicename)
				{
					string strold=pConf->StrDeviceNameSeq[j];
					pConf->StrDeviceNameSeq[j]=devicename;
					string strnew=pConf->StrDeviceNameSeq[j];
					transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
					strnew=strnew+"=";
					Setconfig(strnew.c_str(),value);//DO映射
					
					if(strold=="")		//原来没有设备，添加do映射
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
					}
					transform(strold.begin(), strold.end(), strold.begin(), ::toupper);
					strold=strold+"=";
					Setconfig(strold.c_str(),"");//清除原来DO映射
				}
			}
			else if(j==-1) //do改为无设备
			{
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrDeviceNameSeq[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						sprintf(key,"do%d_do",k+1);
						pConf->StrDeviceNameSeq[k]=key; //设备名
						sprintf(key,"%d",k+1);
						pConf->StrDoSeq[k] = key;	//对应DO
					}
				}
				for(k=0;k<SWITCH_COUNT;k++)
				{
					string strnew=devicename;
					if(strnew==pConf->StrUnWireDevName[k])
					{
//printf("k=%d,olddev=%s,do=%s,newdev=%s,do=%s\n",k,StrDeviceNameSeq[k].c_str(),StrDoSeq[k].c_str(),strnew.c_str(),value);
						transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
						strnew=strnew+"=";
						Setconfig(strnew.c_str(),value);//DO映射

						pConf->StrUnWireDevName[k]=""; //设备名
						pConf->StrUnWireDo[k] = "";	//对应DO
					}
				}
			}
			else if(j>=100 && j<100+SWITCH_COUNT)
			{
				string strnew=devicename;
				transform(strnew.begin(), strnew.end(), strnew.begin(), ::toupper);
				strnew=strnew+"=";
				Setconfig(strnew.c_str(),value);//DO映射
				
				pConf->StrUnWireDevName[j-100]=devicename;
				pConf->StrUnWireDo[j-100] = value; //对应DO
			}
		}
	}
	
	// 统一处理防雷接地初始化标志
	for(i=0;i<SPD_NUM+RES_NUM;i++)
	{
		if (pConf->SPD_Type == TYPE_HUAZI)
		{
			if (pConf->HZ_reset_pre[i] == true)
			{
				pConf->HZ_reset_pre[i] = false;
				pConf->HZ_reset_flag[i] = true;
			}
		}
	}
	// 没有配置的都置空
	pConf->SPD_num=atoi(pConf->StrSPDCount.c_str());
	if (pConf->SPD_num < SPD_NUM)
	{
		for (i=pConf->SPD_num;i<SPD_NUM;i++)
		{
			pConf->StrSPDIP[i] =""; ;//防雷器IP
			sprintf(key,"SPD%dIP=",i+1);
			Setconfig(key,pConf->StrSPDIP[i]);
			pConf->StrSPDPort[i] ="";//防雷器端口
			sprintf(key,"SPD%dPort=",i+1);
			Setconfig(key,pConf->StrSPDPort[i]);
			pConf->StrSPDAddr[i] ="";//防雷器硬件端口
			sprintf(key,"SPD%dAddr=",i+1);
			Setconfig(key,pConf->StrSPDAddr[i]);
		}
	}
#if 0
	if (locker_changed)
	{
		locker_changed = false;
		// 锁的参数重新初始化
		lockerDataInit(false);
	}
	printf("\n");
#endif	
	Writeconfig();
	
	return true;
}


bool jsonstrVmCtlParamReaderXY(char* jsonstr, int len, UINT8 *pstPam)
{
//	printf("%s \t\n",jsonstr);
	//json是json对象指针,json_name是 name对象的指针,json_age是age对象的指针
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	SpdClient *pSpd=pCSpdClent;		//SPD防雷器
	char key[50],value[128],keytmp[50],valuetmp[50];
	int valueint,arraysize;
	FDATA dummy;
	bool locker_changed = false;
    string name,ip,port,skey,getpasswd,setpasswd,passwd,deviceType,deviceTypeNo;

	cJSON *json=0, *jsonkey=0, *jsonvalue=0, *jsonlist=0, *jsonitem=0, *jsondeviceModel=0;
	int i,vpcount;
	//解析数据包
	json = cJSON_Parse(jsonstr);
	if(json==0) return false;

	//门架信息
	jsonkey = cJSON_GetObjectItem(json, "cabinettype");
	if(jsonkey!=0)				//机柜类型	1：华为双机柜双开门；2：华为双机柜单开门；3：华为单机柜双开门；4：华为单机柜单开门
	{
		if(jsonkey->type==cJSON_Int)
			sprintf(value,"%d",jsonkey->valueint);
		else if(jsonkey->type==cJSON_String)
			sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrCabinetType)
		{
			pConf->StrCabinetType=value;
			Setconfig("CabinetType=",value);
		}
	}
	jsonkey = cJSON_GetObjectItem(json,"flagnetroadid" );
	if(jsonkey!=0)				//ETC 门架路网编号
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFlagNetRoadID)
		{
			pConf->StrFlagNetRoadID=value;
			Setconfig("FlagNetRoadID=",value);
		}
	}
	//ETC 门架路段编号
	jsonkey = cJSON_GetObjectItem(json, "flagroadid");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFlagRoadID)
		{
			pConf->StrFlagRoadID=value;
			Setconfig("FlagRoadID=",value);
		}
	}
	
	//ETC 门架编号
	jsonkey = cJSON_GetObjectItem(json, "flagid");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFlagID)
		{
			pConf->StrFlagID=value;
			#if 0
			// 更新屏幕上的字母
			ScreenFlagSet(LETTER_SET);
			#endif
			Setconfig("FlagID=",value);
		}
	}

	//ETC 门架序号
	jsonkey = cJSON_GetObjectItem(json, "posid");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrPosId)
		{
			pConf->StrPosId=value;
			Setconfig("PosId=",value);
		}
	}
	
	//行车方向
	jsonkey = cJSON_GetObjectItem(json,"direction" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrDirection)
		{
			pConf->StrDirection=value;
			Setconfig("Direction=",value);
		}
	}
	
	//行车方向说明
	jsonkey = cJSON_GetObjectItem(json,"dirdescription" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrDirDescription)
		{
			pConf->StrDirDescription=value;
			Setconfig("DirDescription=",value);
		}
	}
	
	//参数设置
	//华为动环服务器数量
	jsonkey = cJSON_GetObjectItem(json,"hwservercount" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWServerCount)
		{
			pConf->StrHWServerCount=value;
			Setconfig("HWServerCount=",value);
		}
	}
	
	//华为服务器地址
	jsonkey = cJSON_GetObjectItem(json,"hwserver" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWServer)
		{
			pConf->StrHWServer=value;
			Setconfig("HWServer=",value);
		}
	}
	
	//SNMP GET 密码
	jsonkey = cJSON_GetObjectItem(json, "hwgetpasswd");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWGetPasswd)
		{
			pConf->StrHWGetPasswd=value;
			Setconfig("HWGetPasswd=",value);		
		}
	}
	
	//SNMP SET 密码
	jsonkey = cJSON_GetObjectItem(json, "hwsetpasswd");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWSetPasswd)
		{
			pConf->StrHWSetPasswd=value;
			Setconfig("HWSetPasswd=",value);		
		}
	}
	
	//华为服务器地址2
	jsonkey = cJSON_GetObjectItem(json, "hwserver2");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWServer2)
		{
			pConf->StrHWServer2=value;
			Setconfig("HWServer2=",value);
		}
	}
	
	//SNMP GET 密码
	jsonkey = cJSON_GetObjectItem(json, "hwgetpasswd2");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWGetPasswd2)
		{
			pConf->StrHWGetPasswd2=value;
			Setconfig("HWGetPasswd2=",value);		
		}
	}
	
	//SNMP SET 密码
	jsonkey = cJSON_GetObjectItem(json, "hwsetpasswd2");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrHWSetPasswd2)
		{
			pConf->StrHWSetPasswd2=value;
			Setconfig("HWSetPasswd2=",value);		
		}
	}
	
	//服务器推送地址列表
    jsonlist = cJSON_GetObjectItem(json, "serverurllist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
				jsonkey=cJSON_GetObjectItem(jsonitem,"url");
				if(jsonkey != NULL)
				{
					sprintf(value,"%s",jsonkey->valuestring);
					//服务器1推送地址
					if(i==0 && value!=pConf->StrServerURL1)
					{
						pConf->StrServerURL1=value;
						sprintf(key,"ServerURL1=",i+1);
						Setconfig(key,value);
					}
					//服务器2推送地址
					if(i==1 && value!=pConf->StrServerURL2)
					{
						pConf->StrServerURL2=value;
						sprintf(key,"ServerURL2=",i+1);//RSUIP地址
						Setconfig(key,value);
					}
					//服务器3推送地址
					if(i==2 && value!=pConf->StrServerURL3)
					{
						pConf->StrServerURL3=value;
						sprintf(key,"ServerURL3=",i+1);//RSUIP地址
						Setconfig(key,value);
					}
					//门锁4推送地址
					if(i==3 && value!=pConf->StrServerURL4)
					{
						pConf->StrServerURL4=value;
						sprintf(key,"ServerURL4=",i+1);//RSUIP地址
						Setconfig(key,value);
					}
				}
            }
        }
    }
	
	//控制器接收地址
	jsonkey = cJSON_GetObjectItem(json, "stationurl");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrStationURL)
		{
			pConf->StrStationURL=value;
			Setconfig("StationURL=",value);
		}
	}
	
	//RSU控制器数量
	jsonkey = cJSON_GetObjectItem(json, "rsucount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrRSUCount && atoi(value)>=0 && atoi(value)<=RSUCTL_NUM)
		{
			pConf->StrRSUCount=value;
			Setconfig("RSUCount=",value);
		}
	}
	
	//RSU控制器类型
	jsonkey = cJSON_GetObjectItem(json, "rsutype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrRSUType)
		{
			pConf->StrRSUType=value;
			Setconfig("RSUType=",value);
		}
	}
	
	//RSU 列表
    jsonlist = cJSON_GetObjectItem(json, "rsulist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//RSUIP地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"ip");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrRSUIP[i])
					{
						pConf->StrRSUIP[i]=value;
						sprintf(key,"RSU%dIP=",i+1);//RSUIP地址
						Setconfig(key,value);
					}
                }
            }
        }
    }
	
	//识别仪数量
	jsonkey = cJSON_GetObjectItem(json, "vehplatecount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrVehPlateCount && atoi(value)>=0 && atoi(value)<=VEHPLATE_NUM)
		{
			pConf->StrVehPlateCount=value; 
			Setconfig("VehPlateCount=",value);
		}
	}
	
	//识别仪列表
    jsonlist = cJSON_GetObjectItem(json, "vehplatelist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//IP地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"ip");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					sprintf(valuetmp,"http://%s",value);
					if(valuetmp!=pConf->StrVehPlateIP[i])
					{
						pConf->StrVehPlateIP[i]=valuetmp; 
						sprintf(key,"VehPlate%dIP=",i+1);
						Setconfig(key,valuetmp);
					}
                }
				//端口
                jsonkey=cJSON_GetObjectItem(jsonitem,"port");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrVehPlatePort[i])
					{
						pConf->StrVehPlatePort[i]=value;
						sprintf(key,"VehPlate%dPort=",i+1);
						Setconfig(key,value);
					}
                }
            }
        }
    }
	
	//900识别仪数量
	jsonkey = cJSON_GetObjectItem(json, "vehplate900count");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrVehPlate900Count && atoi(value)>=0 && atoi(value)<=VEHPLATE900_NUM)
		{
			pConf->StrVehPlate900Count=value;	
			Setconfig("VehPlate900Count=",value);
		}
	}
	
	//900识别仪列表
    jsonlist = cJSON_GetObjectItem(json, "vehplate900list");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize && i<VEHPLATE900_NUM;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//IP地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"ip");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					sprintf(valuetmp,"http://%s",value);
					if(valuetmp!=pConf->StrVehPlate900IP[i])
					{
						pConf->StrVehPlate900IP[i]=valuetmp; 
						sprintf(key,"VehPlate900%dIP=",i+1);
						Setconfig(key,valuetmp);
					}
                }
				//端口
                jsonkey=cJSON_GetObjectItem(jsonitem,"port");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrVehPlate900Port[i])
					{
						pConf->StrVehPlate900Port[i]=value;
						sprintf(key,"VehPlate900%dPort=",i+1);
						Setconfig(key,value);
					}
                }
            }
        }
    }
	
	//监控摄像头数量
	jsonkey = cJSON_GetObjectItem(json, "camcount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrCAMCount && atoi(value)>=0 && atoi(value)<=CAM_NUM)
		{
			pConf->StrCAMCount=value;	
			Setconfig("CAMCount=",value);
		}
	}
	
	//监控摄像头列表
    jsonlist = cJSON_GetObjectItem(json, "camlist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize && i<CAM_NUM;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//IP地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"ip");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrCAMIP[i])
					{
						pConf->StrCAMIP[i]=value;	
						sprintf(key,"CAM%dIP=",i+1);
						Setconfig(key,value);
					}
                }
				//端口
                jsonkey=cJSON_GetObjectItem(jsonitem,"port");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrCAMPort[i])
					{
						pConf->StrCAMPort[i]=value;
						sprintf(key,"CAM%dPort=",i+1);
						Setconfig(key,value);
					}
                }
				//监控摄像头用户名密码
                jsonkey=cJSON_GetObjectItem(jsonitem,"key");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrCAMKey[i])
					{
						pConf->StrCAMKey[i]=value;
						sprintf(key,"CAM%dKey=",i+1);
						Setconfig(key,value);
					}
                }
            }
        }
    }
	
	//交换机类型
	jsonkey = cJSON_GetObjectItem(json, "ipswitchtype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrIPSwitchType)
		{
			pConf->StrIPSwitchType=value; 
			pConf->IntIPSwitchType=atoi(value);
			Setconfig("SwitchType=",value);
		}
	}
	
	//交换机数量
	jsonkey = cJSON_GetObjectItem(json, "ipswitchcount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrIPSwitchCount && atoi(value)>=0 && atoi(value)<=IPSWITCH_NUM)
		{
			pConf->StrIPSwitchCount=value; 
			Setconfig("SwitchCount=",value);
		}
	}
	
	//交换机列表
    jsonlist = cJSON_GetObjectItem(json, "ipswitchlist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize && i<IPSWITCH_NUM;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//IP地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"ip");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrIPSwitchIP[i])
					{
						pConf->StrIPSwitchIP[i]=value; 
						sprintf(key,"Switch%dIP=",i+1);
						Setconfig(key,value);
					}
                }
				//交换机get密码
                jsonkey=cJSON_GetObjectItem(jsonitem,"getpasswd");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrIPSwitchGetPasswd[i])
					{
						pConf->StrIPSwitchGetPasswd[i]=value;
						sprintf(key,"Switch%dGetPasswd=",i+1);
						Setconfig(key,value);
					}
                }
				//交换机set密码
                jsonkey=cJSON_GetObjectItem(jsonitem,"setpasswd");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrIPSwitchSetPasswd[i])
					{
						pConf->StrIPSwitchSetPasswd[i]=value;
						sprintf(key,"Switch%dSetPasswd=",i+1);
						Setconfig(key,value);
					}
                }
            }
        }
    }
	
	//防火墙类型
	jsonkey = cJSON_GetObjectItem(json, "firewaretype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFireWareType)
		{
			pConf->StrFireWareType=value; 
			pConf->IntFireWareType=atoi(value);
			Setconfig("FireWareType=",value);
		}
	}
	
	//防火墙数量
	jsonkey = cJSON_GetObjectItem(json, "firewarecount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrFireWareCount && atoi(value)>=0 && atoi(value)<=FIREWARE_NUM)
		{
			pConf->StrFireWareCount=value; 
			Setconfig("FireWareCount=",value);
		}
	}
	
	//防火墙列表
    jsonlist = cJSON_GetObjectItem(json, "firewarelist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize && i<FIREWARE_NUM;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//IP地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"ip");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrFireWareIP[i])
					{
						pConf->StrFireWareIP[i]=value; 
						sprintf(key,"FireWare%dIP=",i+1);
						Setconfig(key,value);
					}
                }
				//防火墙get密码
                jsonkey=cJSON_GetObjectItem(jsonitem,"getpasswd");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrFireWareGetPasswd[i])
					{
						pConf->StrFireWareGetPasswd[i]=value;
						sprintf(key,"FireWare%dGetPasswd=",i+1);
						Setconfig(key,value);
					}
                }
				//防火墙set密码
                jsonkey=cJSON_GetObjectItem(jsonitem,"setpasswd");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrFireWareSetPasswd[i])
					{
						pConf->StrFireWareSetPasswd[i]=value;
						sprintf(key,"FireWare%dSetPasswd=",i+1);
						Setconfig(key,value);
					}
                }
            }
        }
    }
	
	//Atlas类型
	jsonkey = cJSON_GetObjectItem(json, "atlastype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrAtlasType)
		{
			pConf->StrAtlasType=value;	
			Setconfig("AtlasType=",value);
		}
	}
	
	//Atlas数量
	jsonkey = cJSON_GetObjectItem(json, "atlascount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrAtlasCount && atoi(value)>=0 && atoi(value)<=ATLAS_NUM)
		{
			pConf->StrAtlasCount=value;	
			Setconfig("AtlasCount=",value);
		}
	}
	
	//Atlas列表
    jsonlist = cJSON_GetObjectItem(json, "atlaslist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize && i<ATLAS_NUM;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//Atlas地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"ip");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrAtlasIP[i])
					{
						pConf->StrAtlasIP[i]=value; 
						sprintf(key,"Atlas%dIP=",i+1);
						Setconfig(key,value);
					}
                }
				//Atlas密码
                jsonkey=cJSON_GetObjectItem(jsonitem,"key");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrAtlasPasswd[i])
					{
						pConf->StrAtlasPasswd[i]=value;
						sprintf(key,"Atlas%dPasswd=",i+1);
						Setconfig(key,value);
					}
                }
            }
        }
    }
	//防雷器SPD数量
	jsonkey = cJSON_GetObjectItem(json, "spdcount");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDCount && atoi(value)>=0 && atoi(value)<=SPD_NUM)
		{
			pConf->StrSPDCount=value;	
			pConf->SPD_num =atoi(value);
			Setconfig("SPDCount=",value);
			// 数量改变，全部初始化
			for(i=0;i<SPD_NUM+RES_NUM;i++)
			{
				pConf->HZ_reset_pre[i] = true;
			}
		}
	}
	
	//防雷器SPD类型
	jsonkey = cJSON_GetObjectItem(json, "spdtype");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDType)
		{
			pConf->StrSPDType=value;	
			pConf->SPD_Type = atoi(value);
			Setconfig("SPDType=",value);
			// 类型改变，全部初始化
			for(i=0;i<SPD_NUM+RES_NUM;i++)
			{
				pConf->HZ_reset_pre[i] = true;
			}
		}
	}
	
	//防雷器列表
    jsonlist = cJSON_GetObjectItem(json, "spdlist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//防雷器ip地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"ip");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrSPDIP[i])
					{
						pConf->StrSPDIP[i]=value; 
						sprintf(key,"SPD%dIP=",i+1);
						Setconfig(key,value);
						pConf->HZ_reset_pre[i] = true;
					}
                }
            	//防雷器端口
                jsonkey=cJSON_GetObjectItem(jsonitem,"port");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrSPDPort[i])
					{
						pConf->StrSPDPort[i]=value; 
						sprintf(key,"SPD%dPort=",i+1);
						Setconfig(key,value);
						pConf->HZ_reset_pre[i] = true;
					}
                }
            	//防雷器设备地址
                jsonkey=cJSON_GetObjectItem(jsonitem,"addr");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					if(value!=pConf->StrSPDAddr[i])
					{
						pConf->StrSPDAddr[i]=value; 
						pConf->SPD_Address[i] = atoi(value);
						sprintf(key,"SPD%dAddr=",i+1);
						Setconfig(key,value);
						pConf->HZ_reset_pre[i] = true;
					}
                }
            }
        }
    }
	
	//接地检测器地址
	jsonkey = cJSON_GetObjectItem(json, "spdresip");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDIP[SPD_NUM])
		{
			pConf->StrSPDIP[SPD_NUM]=value;	
			Setconfig("SPDResIP=",value);
			pConf->HZ_reset_pre[SPD_NUM] = true;
		}
	}
	//接地检测器端口
	jsonkey = cJSON_GetObjectItem(json, "spdresport");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDPort[SPD_NUM])
		{
			pConf->StrSPDPort[SPD_NUM]=value;	
			Setconfig("SPDResPort=",value);
			pConf->HZ_reset_pre[SPD_NUM] = true;
		}
	}

	//接地检测器id
	jsonkey = cJSON_GetObjectItem(json, "spdresid");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrSPDAddr[SPD_NUM])
		{
			pConf->StrSPDAddr[SPD_NUM]=value;	
			pConf->SPD_Address[SPD_NUM] = atoi(value);
			Setconfig("SPDResPort=",value);
			pConf->HZ_reset_pre[SPD_NUM] = true;
//			pSpd->Ex_SPD_Set_Process(SPD_RES_SET,RES_ID_ADDR,dummy,pConf->SPD_Address[SPD_NUM]);
		}
	}
	
	//接地电阻报警值
	jsonkey = cJSON_GetObjectItem(json, "spdres_alarm_value");
	if(jsonkey!=0)				
	{
		UINT16 addr_ref=1;
		sprintf(value,"%s",jsonkey->valuestring);
		if(atoi(value)!=pSpd->stuSpd_Param->rSPD_res.alarm_value)
		{
			pSpd->stuSpd_Param->rSPD_res.alarm_value=atoi(value);
			printf("spdres报警值修改=%s\n",value);
			//spdres更改报警值
			if (pConf->SPD_Type == TYPE_LEIXUN)
			{
				addr_ref = RES_ALARM_ADDR;
			}
			// 广西宽永KY0M其实没有接地电阻，但也保留
			else if ((pConf->SPD_Type == TYPE_KY) ||(pConf->SPD_Type == TYPE_KY0M))
			{
				addr_ref = KY_RES_ALARM_ADDR;
			}
			pSpd->Ex_SPD_Set_Process(0,SPD_RES_SET,addr_ref,dummy,atoi(value));
		}
	}
	//do数量
	jsonkey = cJSON_GetObjectItem(json, "do_count");
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrDoCount && atoi(value)>=0 && atoi(value)<=SWITCH_COUNT)
		{
			pConf->StrDoCount=value;	
			Setconfig("DO_Count=",value);
		}
	}
		
	//do列表
    jsonlist = cJSON_GetObjectItem(json, "dolist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//设备名
                jsonkey=cJSON_GetObjectItem(jsonitem,"name");
                if(jsonkey != NULL)
                {
					sprintf(keytmp,"%s",jsonkey->valuestring);
                }
            	//do值
                jsonkey=cJSON_GetObjectItem(jsonitem,"value");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
                }
            	//do市电停电时是否断电
                jsonkey=cJSON_GetObjectItem(jsonitem,"poweroff_or_not");
                if(jsonkey != NULL)
                {
					sprintf(valuetmp,"%s",jsonkey->valuestring);
                }
            }

            //deviceModel
            jsondeviceModel = cJSON_GetObjectItem(jsonitem, "deviceModel");
            if(jsondeviceModel!=0)
            {
				name="";ip="";port="";skey="";getpasswd="";setpasswd="";passwd="";deviceType="";deviceTypeNo="";
                //设备名
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"name");
                if(jsonkey != NULL)	name=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"ip");
                if(jsonkey != NULL)	ip=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"port");
                if(jsonkey != NULL)	port=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"key");
                if(jsonkey != NULL)	skey=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"getpasswd");
                if(jsonkey != NULL)	getpasswd=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"setpasswd");
                if(jsonkey != NULL)	setpasswd=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"passwd");
                if(jsonkey != NULL)	passwd=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"deviceType");
                if(jsonkey != NULL)	deviceType=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"deviceTypeNo");
                if(jsonkey != NULL)	deviceTypeNo=jsonkey->valuestring;
            }
			
			printf("%s %s %s %s %s %s %s %s %s %s %s %s\n",keytmp,value,valuetmp,name.c_str(),ip.c_str(),port.c_str(),skey.c_str(),getpasswd.c_str(),setpasswd.c_str(),passwd.c_str(),deviceType.c_str(),deviceTypeNo.c_str());

			if(pConf->StrPoweroff_Or_Not[i]!=valuetmp)
			{
				pConf->StrPoweroff_Or_Not[i]=valuetmp;	
				sprintf(key,"DO_%d_Poweroff_Or_Not=",i+1);
				Setconfig(key,valuetmp);
			}

			//更新DO对应的设备名称和端口号
			bool found=false;
			for(int j=0;j<SWITCH_COUNT;j++)
			{
				if(keytmp==pConf->StrDeviceNameSeq[j])
				{   
					found=true;
					if(pConf->StrDoSeq[j]!=value)
					{
						pConf->StrDoSeq[j]=value;
						pConf->DoSeq[j]=atoi(value);
						string stmp=pConf->StrDeviceNameSeq[j];
						transform(stmp.begin(), stmp.end(), stmp.begin(), ::toupper);
						stmp=stmp+"=";
						Setconfig(stmp.c_str(),value);//DO映射
					}
				}
			}
			if(found==false)		//匹配不到该设备名，增加一个do
			{
				int pos=atoi(value);
				pConf->StrDoSeq[pos-1]=value;
				pConf->DoSeq[pos-1]=atoi(value);
				pConf->StrDeviceNameSeq[pos-1]=keytmp;
				string stmp=keytmp;
				transform(stmp.begin(), stmp.end(), stmp.begin(), ::toupper);
				stmp=stmp+"=";
				Setconfig(stmp.c_str(),value);//DO映射
			}
				
			int no=atoi(deviceTypeNo.c_str())-1;
			//更新RSU设备IP
			if(deviceType=="rsu" && ip!=pConf->StrRSUIP[no])
			{
				
				pConf->StrRSUIP[no]=ip;
				sprintf(key,"RSU%dIP=",no+1);//RSUIP地址
				Setconfig(key,ip.c_str());
			}

			//更新车牌识别仪设备信息
			if(deviceType=="vehplate")
			{
				//更新车牌识别仪设备IP
				sprintf(value,"http://%s",ip.c_str());
				if(value!=pConf->StrVehPlateIP[no])
				{
					pConf->StrVehPlateIP[i]=value; 
					sprintf(key,"VehPlate%dIP=",no+1);
					Setconfig(key,value);
				}
				//更新车牌识别仪设备端口
				if(port!=pConf->StrVehPlatePort[no])
				{
					pConf->StrVehPlatePort[no]=port;
					sprintf(key,"VehPlate%dPort=",no+1);
					Setconfig(key,port.c_str());
				}
				//更新车牌识别仪设备端口
				if(skey!=pConf->StrVehPlateKey[no])
				{
					pConf->StrVehPlateKey[no]=skey;
					//sprintf(key,"VehPlate%dKey=",no+1);
					//Setconfig(key,port.c_str());
				}
			}
			
			//更新900万全景车牌识别仪设备信息
			if(deviceType=="vehplate900")
			{
				//更新900万全景车牌识别仪设备IP
				sprintf(value,"http://%s",ip.c_str());
				if(value!=pConf->StrVehPlate900IP[no])
				{
					pConf->StrVehPlate900IP[i]=value; 
					sprintf(key,"VehPlate900%dIP=",no+1);
					Setconfig(key,value);
				}
				//更新900万全景车牌识别仪设备端口
				if(port!=pConf->StrVehPlate900Port[no])
				{
					pConf->StrVehPlate900Port[no]=port;
					sprintf(key,"VehPlate900%dPort=",no+1);
					Setconfig(key,port.c_str());
				}
				//更新900万全景车牌识别仪设备端口
				if(skey!=pConf->StrVehPlate900Key[no])
				{
					pConf->StrVehPlate900Key[no]=skey;
					//sprintf(key,"VehPlate%dKey=",no+1);
					//Setconfig(key,port.c_str());
				}
			}
			
			//更新监控摄像机设备信息
			if(deviceType=="cam")
			{
				//更新IP
				if(ip!=pConf->StrCAMIP[no])
				{
					pConf->StrCAMIP[i]=ip; 
					sprintf(key,"CAM%dIP=",no+1);
					Setconfig(key,ip.c_str());
				}
				//更新端口
				if(port!=pConf->StrCAMPort[no])
				{
					pConf->StrCAMPort[no]=port;
					sprintf(key,"CAM%dPort=",no+1);
					Setconfig(key,port.c_str());
				}
				//更新端口
				if(skey!=pConf->StrCAMKey[no])
				{
					pConf->StrCAMKey[no]=skey;
					sprintf(key,"CAM%dKey=",no+1);
					Setconfig(key,port.c_str());
				}
			}
			
			//更新atlas信息
			if(deviceType=="atlas")
            {
            	//Atlas地址
				if(ip!=pConf->StrAtlasIP[no])
				{
					pConf->StrAtlasIP[no]=ip; 
					sprintf(key,"Atlas%dIP=",no+1);
					Setconfig(key,ip.c_str());
				}
				//Atlas密码
				if(skey!=pConf->StrAtlasPasswd[no])
				{
					pConf->StrAtlasPasswd[no]=skey;
					sprintf(key,"Atlas%dPasswd=",no+1);
					Setconfig(key,skey.c_str());
				}
            }
			
			//更新交换机信息
			if(deviceType=="ipswitch")
            {
            	//交换机地址
				if(ip!=pConf->StrIPSwitchIP[no])
				{
					pConf->StrIPSwitchIP[no]=ip; 
					sprintf(key,"Switch%dIP=",no+1);
					Setconfig(key,ip.c_str());
				}
				//交换机get密码
				if(getpasswd!=pConf->StrIPSwitchGetPasswd[no])
				{
					pConf->StrIPSwitchGetPasswd[no]=getpasswd;
					sprintf(key,"Switch%dGetPasswd=",no+1);
					Setconfig(key,getpasswd.c_str());
				}
				//交换机set密码
				if(setpasswd!=pConf->StrIPSwitchSetPasswd[no])
				{
					pConf->StrIPSwitchSetPasswd[no]=setpasswd;
					sprintf(key,"Switch%dSetPasswd=",no+1);
					Setconfig(key,setpasswd.c_str());
				}
            }
			
			//更新防火墙信息
			if(deviceType=="fireware")
            {
            	//IP地址
				if(ip!=pConf->StrFireWareIP[no])
				{
					pConf->StrFireWareIP[no]=ip; 
					sprintf(key,"FireWare%dIP=",no+1);
					Setconfig(key,ip.c_str());
				}
				//防火墙get密码
				if(getpasswd!=pConf->StrFireWareGetPasswd[no])
				{
					pConf->StrFireWareGetPasswd[no]=getpasswd;
					sprintf(key,"FireWare%dGetPasswd=",no+1);
					Setconfig(key,getpasswd.c_str());
				}
				//防火墙set密码
				if(setpasswd!=pConf->StrFireWareSetPasswd[no])
				{
					pConf->StrFireWareSetPasswd[no]=setpasswd;
					sprintf(key,"FireWare%dSetPasswd=",no+1);
					Setconfig(key,setpasswd.c_str());
				}
            }
        }
    }
	
	//无接配电层设备列表
    jsonlist = cJSON_GetObjectItem(json, "unwire_dolist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//设备名
                jsonkey=cJSON_GetObjectItem(jsonitem,"name");
                if(jsonkey != NULL)
                {
					sprintf(keytmp,"%s",jsonkey->valuestring);
                }
            	//do值
                jsonkey=cJSON_GetObjectItem(jsonitem,"value");
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
                }
            }

            //deviceModel
            jsondeviceModel = cJSON_GetObjectItem(jsonitem, "deviceModel");
            if(jsondeviceModel!=0)
            {
				name="";ip="";port="";skey="";getpasswd="";setpasswd="";passwd="";deviceType="";deviceTypeNo="";
                //设备名
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"name");
                if(jsonkey != NULL)	name=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"ip");
                if(jsonkey != NULL)	ip=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"port");
                if(jsonkey != NULL)	port=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"key");
                if(jsonkey != NULL)	skey=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"getpasswd");
                if(jsonkey != NULL)	getpasswd=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"setpasswd");
                if(jsonkey != NULL)	setpasswd=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"passwd");
                if(jsonkey != NULL)	passwd=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"deviceType");
                if(jsonkey != NULL)	deviceType=jsonkey->valuestring;
                jsonkey=cJSON_GetObjectItem(jsondeviceModel,"deviceTypeNo");
                if(jsonkey != NULL)	deviceTypeNo=jsonkey->valuestring;
            }
			printf("%s %s %s %s %s %s %s %s %s %s %s %s\n",keytmp,value,valuetmp,name.c_str(),ip.c_str(),port.c_str(),skey.c_str(),getpasswd.c_str(),setpasswd.c_str(),passwd.c_str(),deviceType.c_str(),deviceTypeNo.c_str());

			bool found=false;
			for(int j=0;j<UNWIRE_SWITCH_COUNT;j++)
			{
				if(keytmp==pConf->StrUnWireDevName[j])
				{   
					found=true;
					if(pConf->StrUnWireDo[j]!=value)
					{
						string stmp=pConf->StrUnWireDevName[j];
						transform(stmp.begin(), stmp.end(), stmp.begin(), ::toupper);
						stmp=stmp+"=";
						Setconfig(stmp.c_str(),value);//DO映射

						//调换设备名称
						pConf->StrUnWireDevName[atoi(value)-101]=pConf->StrUnWireDevName[j];
						sprintf(key,"do%d_do",100+j+1);
						pConf->StrUnWireDevName[j] = key;
					}
				}
			}
			if(found==false)		//匹配不到该设备名，增加一个do
			{
				int pos=atoi(value);
				pConf->StrUnWireDo[pos-1]=value;
				pConf->StrUnWireDevName[pos-1]=keytmp;
				string stmp=keytmp;
				transform(stmp.begin(), stmp.end(), stmp.begin(), ::toupper);
				stmp=stmp+"=";
				Setconfig(stmp.c_str(),value);//DO映射
			}

			int no=atoi(deviceTypeNo.c_str())-1;
			//更新RSU设备IP
			if(deviceType=="rsu" && ip!=pConf->StrRSUIP[no])
			{
				
				pConf->StrRSUIP[no]=ip;
				sprintf(key,"RSU%dIP=",no+1);//RSUIP地址
				Setconfig(key,ip.c_str());
			}

			//更新车牌识别仪设备信息
			if(deviceType=="vehplate")
			{
				//更新车牌识别仪设备IP
				sprintf(value,"http://%s",ip.c_str());
				if(value!=pConf->StrVehPlateIP[no])
				{
					pConf->StrVehPlateIP[i]=value; 
					sprintf(key,"VehPlate%dIP=",no+1);
					Setconfig(key,value);
				}
				//更新车牌识别仪设备端口
				if(port!=pConf->StrVehPlatePort[no])
				{
					pConf->StrVehPlatePort[no]=port;
					sprintf(key,"VehPlate%dPort=",no+1);
					Setconfig(key,port.c_str());
				}
				//更新车牌识别仪设备端口
				if(skey!=pConf->StrVehPlateKey[no])
				{
					pConf->StrVehPlateKey[no]=skey;
					//sprintf(key,"VehPlate%dKey=",no+1);
					//Setconfig(key,port.c_str());
				}
			}
			
			//更新900万全景车牌识别仪设备信息
			if(deviceType=="vehplate900")
			{
				//更新900万全景车牌识别仪设备IP
				sprintf(value,"http://%s",ip.c_str());
				if(value!=pConf->StrVehPlate900IP[no])
				{
					pConf->StrVehPlate900IP[i]=value; 
					sprintf(key,"VehPlate900%dIP=",no+1);
					Setconfig(key,value);
				}
				//更新900万全景车牌识别仪设备端口
				if(port!=pConf->StrVehPlate900Port[no])
				{
					pConf->StrVehPlate900Port[no]=port;
					sprintf(key,"VehPlate900%dPort=",no+1);
					Setconfig(key,port.c_str());
				}
				//更新900万全景车牌识别仪设备端口
				if(skey!=pConf->StrVehPlate900Key[no])
				{
					pConf->StrVehPlate900Key[no]=skey;
					//sprintf(key,"VehPlate%dKey=",no+1);
					//Setconfig(key,port.c_str());
				}
			}
			
			//更新监控摄像机设备信息
			if(deviceType=="cam")
			{
				//更新IP
				if(ip!=pConf->StrCAMIP[no])
				{
					pConf->StrCAMIP[i]=ip; 
					sprintf(key,"CAM%dIP=",no+1);
					Setconfig(key,ip.c_str());
				}
				//更新端口
				if(port!=pConf->StrCAMPort[no])
				{
					pConf->StrCAMPort[no]=port;
					sprintf(key,"CAM%dPort=",no+1);
					Setconfig(key,port.c_str());
				}
				//更新端口
				if(skey!=pConf->StrCAMKey[no])
				{
					pConf->StrCAMKey[no]=skey;
					sprintf(key,"CAM%dKey=",no+1);
					Setconfig(key,port.c_str());
				}
			}
			
			//更新atlas信息
			if(deviceType=="atlas")
            {
            	//Atlas地址
				if(ip!=pConf->StrAtlasIP[no])
				{
					pConf->StrAtlasIP[no]=ip; 
					sprintf(key,"Atlas%dIP=",no+1);
					Setconfig(key,ip.c_str());
				}
				//Atlas密码
				if(passwd!=pConf->StrAtlasPasswd[no])
				{
					pConf->StrAtlasPasswd[no]=passwd;
					sprintf(key,"Atlas%dPasswd=",no+1);
					Setconfig(key,passwd.c_str());
				}
            }
			
			//更新交换机信息
			if(deviceType=="ipswitch")
            {
            	//交换机地址
				if(ip!=pConf->StrIPSwitchIP[no])
				{
					pConf->StrIPSwitchIP[no]=ip; 
					sprintf(key,"Switch%dIP=",no+1);
					Setconfig(key,ip.c_str());
				}
				//交换机get密码
				if(getpasswd!=pConf->StrIPSwitchGetPasswd[no])
				{
					pConf->StrIPSwitchGetPasswd[no]=getpasswd;
					sprintf(key,"Switch%dGetPasswd=",no+1);
					Setconfig(key,getpasswd.c_str());
				}
				//交换机set密码
				if(setpasswd!=pConf->StrIPSwitchSetPasswd[no])
				{
					pConf->StrIPSwitchSetPasswd[no]=setpasswd;
					sprintf(key,"Switch%dSetPasswd=",no+1);
					Setconfig(key,setpasswd.c_str());
				}
            }
			
			//更新防火墙信息
			if(deviceType=="fireware")
            {
            	//IP地址
				if(ip!=pConf->StrFireWareIP[no])
				{
					pConf->StrFireWareIP[no]=ip; 
					sprintf(key,"FireWare%dIP=",no+1);
					Setconfig(key,ip.c_str());
				}
				//防火墙get密码
				if(getpasswd!=pConf->StrFireWareGetPasswd[no])
				{
					pConf->StrFireWareGetPasswd[no]=getpasswd;
					sprintf(key,"FireWare%dGetPasswd=",no+1);
					Setconfig(key,getpasswd.c_str());
				}
				//防火墙set密码
				if(setpasswd!=pConf->StrFireWareSetPasswd[no])
				{
					pConf->StrFireWareSetPasswd[no]=setpasswd;
					sprintf(key,"FireWare%dSetPasswd=",no+1);
					Setconfig(key,setpasswd.c_str());
				}
            }
        }
    }
	// 统一处理防雷接地初始化标志
	for(i=0;i<SPD_NUM+RES_NUM;i++)
	{
		if (pConf->SPD_Type == TYPE_HUAZI)
		{
			if (pConf->HZ_reset_pre[i] == true)
			{
				pConf->HZ_reset_pre[i] = false;
				pConf->HZ_reset_flag[i] = true;
			}
		}
	}
	
	// 没有配置的都置空
	if (pConf->SPD_num == 1)
	{
		for (i=1;i<SPD_NUM;i++)
		{
			pConf->StrSPDIP[i] =""; ;//防雷器IP
			sprintf(key,"SPD%dIP=",i+1);
			Setconfig(key,pConf->StrSPDIP[i]);
			pConf->StrSPDPort[i] ="";//防雷器端口
			sprintf(key,"SPD%dPort=",i+1);
			Setconfig(key,pConf->StrSPDPort[i]);
			pConf->StrSPDAddr[i] ="";//防雷器硬件端口
			sprintf(key,"SPD%dAddr=",i+1);
			Setconfig(key,pConf->StrSPDAddr[i]);
		}
		Setconfig("SPDResIP=","");
		Setconfig("SPDResPort=","");
	}
	for(i=0;i<LOCK_NUM;i++)
	{
		sprintf(key,"adrrlock%d",i+1);//门锁地址
		jsonkey = cJSON_GetObjectItem(json, key);
		if(jsonkey!=0)				
		{
			sprintf(value,"%s",jsonkey->valuestring);
			if(value!=pConf->StrAdrrLock[i])
			{
				pConf->StrAdrrLock[i]=value;	
				sprintf(key,"LOCKADD%d=",i+1);
				Setconfig(key,value);
				locker_changed = true;	// 锁的配置发生变化
			}
		}
	}	
	
	sprintf(key,"hardwareid");	//硬件ID
	jsonkey = cJSON_GetObjectItem(json, key);
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrID)
		{
			pConf->StrID=value;	
			Setconfig("ID=",value);
		}
	}
	
	sprintf(key,"cpualarmvalue");	//CPU使用率报警阈值
	jsonkey = cJSON_GetObjectItem(json, key);
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrCpuAlarmValue)
		{
			pConf->StrCpuAlarmValue=value;	
			Setconfig("CpuAlarmValue=",value);
		}
	}
	
	sprintf(key,"cputempalarmvalue");	//CPU温度报警阈值
	jsonkey = cJSON_GetObjectItem(json, key);
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrCpuTempAlarmValue)
		{
			pConf->StrCpuTempAlarmValue=value;	
			Setconfig("CpuTempAlarmValue=",value);
		}
	}
	
	sprintf(key,"memalarmvalue");	//内存使用率报警阈值
	jsonkey = cJSON_GetObjectItem(json, key);
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrMemAlarmValue)
		{
			pConf->StrMemAlarmValue=value;	
			Setconfig("MemAlarmValue=",value);
		}
	}
#if 0
	if (locker_changed)
	{
		locker_changed = false;
		// 锁的参数重新初始化
		lockerDataInit(false);
	}
#endif
	Writeconfig();
	
	return true;
}

bool jsonstrIpInfoReader(char* jsonstr, int len)
{
	printf("%s \t\n",jsonstr);
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	char key[50],value[128];
	cJSON *json=0, *jsonkey=0, *jsonvalue=0;
	//解析数据包
	json = cJSON_Parse(jsonstr);
	if(json==0) return false;
	jsonkey = cJSON_GetObjectItem(json,"ipaddr" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrIP && isIPAddressValid(value))
		{
			pConf->StrIP=value;
		}
	}
	jsonkey = cJSON_GetObjectItem(json,"mask" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrMask && isIPAddressValid(value))
		{
			pConf->StrMask=value;
		}
	}
	jsonkey = cJSON_GetObjectItem(json,"gateway" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrGateway && isIPAddressValid(value))
		{
			pConf->StrGateway=value;
		}
	}
	jsonkey = cJSON_GetObjectItem(json,"dns" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrDNS && isIPAddressValid(value))
		{
			pConf->StrDNS=value;
		}
	}
	
	jsonkey = cJSON_GetObjectItem(json,"ipaddr2" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrIP2 && isIPAddressValid(value))
		{
			pConf->StrIP2=value;
		}
	}
	jsonkey = cJSON_GetObjectItem(json,"mask2" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrMask2 && isIPAddressValid(value))
		{
			pConf->StrMask2=value;
		}
	}
	jsonkey = cJSON_GetObjectItem(json,"gateway2" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrGateway2 && isIPAddressValid(value))
		{
			pConf->StrGateway2=value;
		}
	}
	jsonkey = cJSON_GetObjectItem(json,"dns2" );
	if(jsonkey!=0)				
	{
		sprintf(value,"%s",jsonkey->valuestring);
		if(value!=pConf->StrDNS2 && isIPAddressValid(value))
		{
			pConf->StrDNS2=value;
		}
	}
	printf("\n");
	return true;
}


bool jsonStrVMCtlParamWriter(int messagetype,char *pstrVMCtl,string &mstrjson)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	SpdClient *pSpd=pCSpdClent;		//SPD防雷器
	int vehplatecnt,vehplate900cnt,rsucnt;
	
	char str[100],sDateTime[30];
	int i,j,CabinetType; 
	static int recordno=0;
	
    time_t nSeconds;
    struct tm * pTM;
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
    
    //门架信息
	strJson = strJson + "\"flagnetroadid\":\""+ pConf->StrFlagNetRoadID +"\",\n";	//ETC 门架路网编号
	strJson = strJson + "\"flagroadid\":\""+ pConf->StrFlagRoadID +"\",\n";	//ETC 门架路段编号
	strJson = strJson + "\"flagid\":\""+ pConf->StrFlagID +"\",\n";	//ETC 门架编号
	strJson = strJson + "\"posid\":\""+ pConf->StrPosId +"\",\n";	//ETC 门架序号
	strJson = strJson + "\"direction\":\""+ pConf->StrDirection +"\",\n";	//行车方向
	strJson = strJson + "\"dirdescription\":\""+ pConf->StrDirDescription +"\",\n";	//行车方向说明

    //IP 地址
	strJson = strJson + "\"ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"mask\":\""+ pConf->StrMask +"\",\n";	//子网掩码
	strJson = strJson + "\"gateway\":\""+ pConf->StrGateway +"\",\n";	//网关
	strJson = strJson + "\"dns\":\""+ pConf->StrDNS +"\",\n";	//DNS地址
	
	strJson = strJson + "\"ipaddr2\":\""+ pConf->StrIP2 +"\",\n";	//IP地址
	strJson = strJson + "\"mask2\":\""+ pConf->StrMask2 +"\",\n";	//子网掩码
	strJson = strJson + "\"gateway2\":\""+ pConf->StrGateway2 +"\",\n";	//网关
	strJson = strJson + "\"dns2\":\""+ pConf->StrDNS2 +"\",\n";	//DNS地址
	
    //参数设置;		
	strJson = strJson + "\"hwservercount\":\""+ pConf->StrHWServerCount +"\",\n";	//华为动环服务器个数
	strJson = strJson + "\"hwserver\":\""+ pConf->StrHWServer +"\",\n";	//华为服务器IP地址
	strJson = strJson + "\"hwgetpasswd\":\""+ pConf->StrHWGetPasswd +"\",\n";	//SNMP GET 密码
	strJson = strJson + "\"hwsetpasswd\":\""+ pConf->StrHWSetPasswd +"\",\n";	//SNMP SET 密码

	CabinetType=atoi(pConf->StrCabinetType.c_str());
	if(CabinetType==6 || CabinetType==11)//金晟安 或者 艾特网能
	{
		strJson = strJson + "\"hwserver2\":\""+ pConf->StrHWServer2 +"\",\n";	//华为服务器IP地址
		strJson = strJson + "\"hwgetpasswd2\":\""+ pConf->StrHWGetPasswd2 +"\",\n";	//SNMP GET 密码
		strJson = strJson + "\"hwsetpasswd2\":\""+ pConf->StrHWSetPasswd2 +"\",\n";	//SNMP SET 密码
	}
	strJson = strJson + "\"serverurl1\":\""+ pConf->StrServerURL1 +"\",\n";	//服务器1推送地址
	strJson = strJson + "\"serverurl2\":\""+ pConf->StrServerURL2 +"\",\n";	//服务器2推送地址
	strJson = strJson + "\"serverurl3\":\""+ pConf->StrServerURL3 +"\",\n";	//服务器3推送地址
	strJson = strJson + "\"serverurl4\":\""+ pConf->StrServerURL4 +"\",\n";	//服务器4推送地址
	strJson = strJson + "\"stationurl\":\""+ pConf->StrStationURL +"\",\n";	//控制器接收地址
	strJson = strJson + "\"rsucount\":\""+ pConf->StrRSUCount +"\",\n";	//RSU数量
	strJson = strJson + "\"rsutype\":\""+ pConf->StrRSUType +"\",\n";	//RSU类型
	rsucnt=atoi(pConf->StrRSUCount.c_str());
	for(i=0;i<RSUCTL_NUM;i++)
	{
		sprintf(str,"\"rsu%dip\":\"%s\",\n",i+1,pConf->StrRSUIP[i].c_str()); strJson = strJson + str;//RSUIP地址
		sprintf(str,"\"rsu%dport\":\"%s\",\n",i+1,pConf->StrRSUPort[i].c_str()); strJson = strJson + str;//RSU端口
	}
	strJson = strJson + "\"vehplatecount\":\""+ pConf->StrVehPlateCount +"\",\n";	//识别仪数量
	vehplatecnt=atoi(pConf->StrVehPlateCount.c_str());
	for(i=0;i<VEHPLATE_NUM;i++)
	{
		sprintf(str,"\"vehplate%dip\":\"%s\",\n",i+1,pConf->StrVehPlateIP[i].c_str()); strJson = strJson + str;//识别仪地址
		sprintf(str,"\"vehplate%dport\":\"%s\",\n",i+1,pConf->StrVehPlatePort[i].c_str()); strJson = strJson + str;//识别仪端口
		sprintf(str,"\"vehplate%dkey\":\"%s\",\n",i+1,pConf->StrVehPlateKey[i].c_str()); strJson = strJson + str;//识别仪用户名密码
	}
	strJson = strJson + "\"vehplate900count\":\""+ pConf->StrVehPlate900Count +"\",\n";	//900识别仪数量
	vehplate900cnt=atoi(pConf->StrVehPlate900Count.c_str());
	for(i=0;i<VEHPLATE900_NUM;i++)
	{
		sprintf(str,"\"vehplate900%dip\":\"%s\",\n",i+1,pConf->StrVehPlate900IP[i].c_str()); strJson = strJson + str;//900识别仪地址
		sprintf(str,"\"vehplate900%dport\":\"%s\",\n",i+1,pConf->StrVehPlate900Port[i].c_str()); strJson = strJson + str;//900识别仪端口
		sprintf(str,"\"vehplate900%dkey\":\"%s\",\n",i+1,pConf->StrVehPlate900Key[i].c_str()); strJson = strJson + str;//900识别仪用户名密码
	}
	strJson = strJson + "\"camcount\":\""+ pConf->StrCAMCount +"\",\n";	//监控摄像头数量
	for(i=0;i<CAM_NUM;i++)
	{
		sprintf(str,"\"cam%dip\":\"%s\",\n",i+1,pConf->StrCAMIP[i].c_str()); 
		strJson = strJson + str;//监控摄像头IP地址
		sprintf(str,"\"cam%dport\":\"%s\",\n",i+1,pConf->StrCAMPort[i].c_str()); //监控摄像头端口
		strJson = strJson + str;
		sprintf(str,"\"cam%dkey\":\"%s\",\n",i+1,pConf->StrCAMKey[i].c_str()); //监控摄像头用户名密码
		strJson = strJson + str;
	}
	
	strJson = strJson + "\"firewarecount\":\""+ pConf->StrFireWareCount +"\",\n";	//防火墙数量
	sprintf(str,"\"firewaretype\":\"%d\",\n",pConf->IntFireWareType); strJson = strJson + str;//防火墙类型
	for(i=0;i<FIREWARE_NUM;i++)
	{
		sprintf(str,"\"fireware%dip\":\"%s\",\n",i+1,pConf->StrFireWareIP[i].c_str()); //防火墙IP
		strJson = strJson + str;
		sprintf(str,"\"fireware%dgetpasswd\":\"%s\",\n",i+1,pConf->StrFireWareGetPasswd[i].c_str()); //防火墙get密码
		strJson = strJson + str;
		sprintf(str,"\"fireware%dsetpasswd\":\"%s\",\n",i+1,pConf->StrFireWareSetPasswd[i].c_str()); //防火墙set密码
		strJson = strJson + str;
	}
	strJson = strJson + "\"ipswitchcount\":\""+ pConf->StrIPSwitchCount +"\",\n";	//交换机数量
	sprintf(str,"\"ipswitchtype\":\"%d\",\n",pConf->IntIPSwitchType); strJson = strJson + str;//交换机类型
	for(i=0;i<IPSWITCH_NUM;i++)
	{
		sprintf(str,"\"ipswitch%dip\":\"%s\",\n",i+1,pConf->StrIPSwitchIP[i].c_str()); //交换机IP
		strJson = strJson + str;
		sprintf(str,"\"ipswitch%dgetpasswd\":\"%s\",\n",i+1,pConf->StrIPSwitchGetPasswd[i].c_str()); //交换机get密码
		strJson = strJson + str;
		sprintf(str,"\"ipswitch%dsetpasswd\":\"%s\",\n",i+1,pConf->StrIPSwitchSetPasswd[i].c_str()); //交换机set密码
		strJson = strJson + str;
	}
	//Atlas
	strJson = strJson + "\"atlastype\":\""+ pConf->StrAtlasType +"\",\n";	//Atlas类型
	strJson = strJson + "\"atlascount\":\""+ pConf->StrAtlasCount +"\",\n";	//Atlas数量
	for(i=0;i<ATLAS_NUM;i++)
	{
		sprintf(str,"\"atlas%dip\":\"%s\",\n",i+1,pConf->StrAtlasIP[i].c_str()); //AtlasIP
		strJson = strJson + str;
		sprintf(str,"\"atlas%dpasswd\":\"%s\",\n",i+1,pConf->StrAtlasPasswd[i].c_str()); //Atlas密码
		strJson = strJson + str;
	}
	//防雷器
	strJson = strJson + "\"spdcount\":\""+ pConf->StrSPDCount +"\",\n";	//防雷器数量
	strJson = strJson + "\"spdtype\":\""+ pConf->StrSPDType +"\",\n";	//防雷器类型
	if(pConf->StrSPDType=="1")
		strJson = strJson + "\"spdfactory\":\"雷迅\",\n";	//防雷器厂商
	else if(pConf->StrSPDType=="2")
		strJson = strJson + "\"spdfactory\":\"华咨圣泰\",\n";	//防雷器厂商
	else
		strJson = strJson + "\"spdfactory\":\"\",\n";	//防雷器厂商
	for(i=0;i<SPD_NUM;i++)
	{
		sprintf(str,"\"spd%dip\":\"%s\",\n",i+1,pConf->StrSPDIP[i].c_str()); //防雷器IP
		strJson = strJson + str;
		sprintf(str,"\"spd%dport\":\"%s\",\n",i+1,pConf->StrSPDPort[i].c_str()); //防雷器端口
		strJson = strJson + str;
		sprintf(str,"\"spd%daddr\":\"%s\",\n",i+1,pConf->StrSPDAddr[i].c_str()); //防雷器设备地址
		strJson = strJson + str;
	}
	sprintf(str,"\"spdresip\":\"%s\",\n",pConf->StrSPDIP[SPD_NUM].c_str()); //接地电阻IP
	strJson = strJson + str;
	sprintf(str,"\"spdresport\":\"%s\",\n",pConf->StrSPDPort[SPD_NUM].c_str()); //接地电阻端口
	strJson = strJson + str;
	sprintf(str,"\"spdresid\":\"%s\",\n",pConf->StrSPDAddr[SPD_NUM].c_str()); //接地电阻设备地址
	strJson = strJson + str;
	if(pSpd!=NULL && pSpd->stuSpd_Param!=NULL)
	{
		sprintf(str,"\"spdres_alarm_value\":\"%d\",\n",pSpd->stuSpd_Param->rSPD_res.alarm_value); //接地电阻报警值
		strJson = strJson + str;
	}
	for(i=0;i<LOCK_NUM;i++)
	{
		sprintf(str,"\"adrrlock%d\":\"%s\",\n",i+1,pConf->StrAdrrLock[i].c_str()); strJson = strJson + str;//门锁的地址
	}
	for(i=0;i<VA_METER_BD_NUM;i++)
	{
		sprintf(str,"\"adrrvameter%d\":\"%s\",\n",i+1,pConf->StrAdrrVAMeter[i].c_str()); strJson = strJson + str;//电压电流传感器的地址
	}
	for(i=0;i<POWER_BD_NUM;i++)
	{
		sprintf(str,"\"poweraddr%d\":\"%s\",\n",i+1,pConf->StrAdrrPower[i].c_str()); strJson = strJson + str;//电源板1的地址
	}
	strJson = strJson + "\"do_count\":\""+ pConf->StrDoCount +"\",\n";	//do数量
	for(i=0;i<SWITCH_COUNT;i++)
	{
		sprintf(str,"\"%s\":\"%s\",\n",pConf->StrDeviceNameSeq[i].c_str(),pConf->StrDoSeq[i].c_str()); strJson = strJson + str;//设备映射DO
	}
	for(i=0;i<UNWIRE_SWITCH_COUNT;i++)
	{
		if(pConf->StrUnWireDevName[i]!="" && pConf->StrUnWireDo[i]!="")
		{sprintf(str,"\"%s\":\"%s\",\n",pConf->StrUnWireDevName[i].c_str(),pConf->StrUnWireDo[i].c_str()); strJson = strJson + str;}//没接线设备映射DO
	}
	
	strJson = strJson + "\"devicetype\":\""+ pConf->StrdeviceType +"\",\n";	//设备型号900~919
	strJson = strJson + "\"hardwareid\":\""+ pConf->StrID +"\",\n";	//硬件ID
	strJson = strJson + "\"softversion\":\""+ pConf->StrVersionNo +"\",\n";	//主程序版本号920
	
	strJson = strJson + "\"cpualarmvalue\":\""+ pConf->StrCpuAlarmValue +"\",\n";	//CPU使用率报警阈值
	strJson = strJson + "\"cputempalarmvalue\":\""+ pConf->StrCpuTempAlarmValue +"\",\n";	//CPU温度报警阈值
	strJson = strJson + "\"memalarmvalue\":\""+ pConf->StrMemAlarmValue +"\",\n";	//内存使用率报警阈值
	
	strJson = strJson + "\"secsoftversion1\":\""+ getCanVersion(1) +"\",\n";	//CAN控制器版本号
	strJson = strJson + "\"secsoftversion2\":\""+ pConf->StrsecSoftVersion[1] +"\",\n";	//副版本号
	strJson = strJson + "\"secsoftversion3\":\""+ pConf->StrsecSoftVersion[2] +"\",\n";	//副版本号
	strJson = strJson + "\"softdate\":\""+ pConf->StrSoftDate +"\"\n";	//版本日期
	
    strJson +=  "}";

    mstrjson = strJson;
	return true;
}


bool jsonStrVMCtlParamWriterXY(int messagetype,char *pstrVMCtl, string &mstrjson)
{
printf("jsonStrVMCtlParamWriterXY 开始\r\n");
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	SpdClient *pSpd=pCSpdClent;		//SPD防雷器
	int vehplatecnt,vehplate900cnt,rsucnt;
	
	char str[100],sDateTime[30];
	int i,j; 
	static int recordno=0;
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
    
    //门架信息
	strJson = strJson + "\"flagnetroadid\":\""+ pConf->StrFlagNetRoadID +"\",\n";	//ETC 门架路网编号
	strJson = strJson + "\"flagroadid\":\""+ pConf->StrFlagRoadID +"\",\n";	//ETC 门架路段编号
	strJson = strJson + "\"flagid\":\""+ pConf->StrFlagID +"\",\n";	//ETC 门架编号
	strJson = strJson + "\"posid\":\""+ pConf->StrPosId +"\",\n";	//ETC 门架序号
	strJson = strJson + "\"direction\":\""+ pConf->StrDirection +"\",\n";	//行车方向
	strJson = strJson + "\"dirdescription\":\""+ pConf->StrDirDescription +"\",\n";	//行车方向说明

	strJson = strJson + "\"iplist\": [\n";		//ip列表
	//IP 地址
    strJson +=  "{\n";
	sprintf(str,"\"name\":\"lan1\",\n"); strJson = strJson + str;//名称
	strJson = strJson + "\"ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"mask\":\""+ pConf->StrMask +"\",\n";	//子网掩码
	strJson = strJson + "\"gateway\":\""+ pConf->StrGateway +"\",\n";	//网关
	strJson = strJson + "\"dns\":\""+ pConf->StrDNS +"\"\n";	//DNS地址
    strJson +=  "},\n";
	
    strJson +=  "{\n";
	sprintf(str,"\"name\":\"lan2\",\n"); strJson = strJson + str;//名称
	strJson = strJson + "\"ipaddr\":\""+ pConf->StrIP2 +"\",\n";	//IP地址
	strJson = strJson + "\"mask\":\""+ pConf->StrMask2 +"\",\n";	//子网掩码
	strJson = strJson + "\"gateway\":\""+ pConf->StrGateway2 +"\",\n";	//网关
	strJson = strJson + "\"dns\":\""+ pConf->StrDNS2 +"\"\n";	//DNS地址
    strJson +=  "}\n";
    strJson +=  "],\n";
	
	//参数设置; 	
	strJson = strJson + "\"hwservercount\":\""+ pConf->StrHWServerCount +"\",\n";	//华为动环服务器数量
	strJson = strJson + "\"hwserver\":\""+ pConf->StrHWServer +"\",\n";	//华为服务器IP地址
	strJson = strJson + "\"hwgetpasswd\":\""+ pConf->StrHWGetPasswd +"\",\n";	//SNMP GET 密码
	strJson = strJson + "\"hwsetpasswd\":\""+ pConf->StrHWSetPasswd +"\",\n";	//SNMP SET 密码
	switch(atoi(pConf->StrCabinetType.c_str()))
	{
		case 1:
		case 2:
		case 3:
		case 4:
			strJson = strJson + "\"cabinetfactroy\":\"华为\",\n";	//机柜厂商
			break;
		case 5:
			strJson = strJson + "\"cabinetfactroy\":\"中兴\",\n"; //机柜厂商
			break;
		case 6:
			strJson = strJson + "\"cabinetfactroy\":\"金晟安\",\n"; //机柜厂商
			break;
		case 7:
			strJson = strJson + "\"cabinetfactroy\":\"爱特思\",\n"; //机柜厂商
			break;
		case 8:
			strJson = strJson + "\"cabinetfactroy\":\"诺龙\",\n"; //机柜厂商
			break;
		case 9:
			strJson = strJson + "\"cabinetfactroy\":\"容尊\",\n"; //机柜厂商
			break;
		case 10:
			strJson = strJson + "\"cabinetfactroy\":\"亚邦\",\n"; //机柜厂商
			break;
		case 11:
			strJson = strJson + "\"cabinetfactroy\":\"华软\",\n"; //机柜厂商
			break;
	}
	
	strJson = strJson + "\"hwserver2\":\""+ pConf->StrHWServer2 +"\",\n";	//华为服务器IP地址
	strJson = strJson + "\"hwgetpasswd2\":\""+ pConf->StrHWGetPasswd2 +"\",\n";	//SNMP GET 密码
	strJson = strJson + "\"hwsetpasswd2\":\""+ pConf->StrHWSetPasswd2 +"\",\n";	//SNMP SET 密码
	
	strJson = strJson + "\"serverurllist\": [\n";		//url列表
	for(i=0;i<4;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"serverurl%d\",\n",i+1); strJson = strJson + str;//名称
		if(i==0)
			strJson = strJson + "\"url\":\""+ pConf->StrServerURL1 +"\"\n";	//服务器1推送地址
		if(i==1)
			strJson = strJson + "\"url\":\""+ pConf->StrServerURL2 +"\"\n";	//服务器2推送地址
		if(i==2)
			strJson = strJson + "\"url\":\""+ pConf->StrServerURL3 +"\"\n";	//服务器3推送地址
		if(i==3)
			strJson = strJson + "\"url\":\""+ pConf->StrServerURL4 +"\"\n";	//服务器4推送地址
		
		if(i<3)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	strJson = strJson + "\"rsucount\":\""+ pConf->StrRSUCount +"\",\n";	//RSU数量
	strJson = strJson + "\"rsutype\":\""+ pConf->StrRSUType +"\",\n";	//RSU类型
	rsucnt=atoi(pConf->StrRSUCount.c_str());
	strJson = strJson + "\"rsulist\": [\n";		//rsu列表
	for(i=0;i<RSUCTL_NUM;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"rsu%d\",\n",i+1); strJson = strJson + str;//名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrRSUIP[i].c_str()); strJson = strJson + str;//RSUIP地址
		sprintf(str,"\"port\":\"%s\"\n",pConf->StrRSUPort[i].c_str()); strJson = strJson + str;//RSU端口
		if(i<RSUCTL_NUM-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	strJson = strJson + "\"vehplatecount\":\""+ pConf->StrVehPlateCount +"\",\n";	//识别仪数量
	vehplatecnt=atoi(pConf->StrVehPlateCount.c_str());
	strJson = strJson + "\"vehplatelist\": [\n";		//vehplate列表
	for(i=0;i<VEHPLATE_NUM;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"vehplate%d\",\n",i+1); strJson = strJson + str;//名称
		if(pConf->StrVehPlateIP[i]!="" && pConf->StrVehPlateIP[i].length()>7)
		{
			sprintf(str,"\"ip\":\"%s\",\n",pConf->StrVehPlateIP[i].c_str()+7); strJson = strJson + str;//识别仪地址
		}
		else
		{
			sprintf(str,"\"ip\":\"\",\n"); strJson = strJson + str;//识别仪地址
		}
		sprintf(str,"\"port\":\"%s\",\n",pConf->StrVehPlatePort[i].c_str()); strJson = strJson + str;//识别仪端口
		sprintf(str,"\"key\":\"%s\"\n",pConf->StrVehPlateKey[i].c_str()); strJson = strJson + str;//识别仪用户名密码
		if(i<VEHPLATE_NUM-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	strJson = strJson + "\"vehplate900count\":\""+ pConf->StrVehPlate900Count +"\",\n";	//900识别仪数量
	vehplate900cnt=atoi(pConf->StrVehPlate900Count.c_str());
	strJson = strJson + "\"vehplate900list\": [\n";		//vehplate900列表
	for(i=0;i<VEHPLATE900_NUM;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"vehplate900%d\",\n",i+1); strJson = strJson + str;//名称
		if(pConf->StrVehPlate900IP[i]!="" && pConf->StrVehPlate900IP[i].length()>7)
		{
			sprintf(str,"\"ip\":\"%s\",\n",pConf->StrVehPlate900IP[i].c_str()+7); strJson = strJson + str;//900识别仪地址
		}
		else
		{
			sprintf(str,"\"ip\":\"\",\n"); strJson = strJson + str;//900识别仪地址
		}
		sprintf(str,"\"port\":\"%s\",\n",pConf->StrVehPlate900Port[i].c_str()); strJson = strJson + str;//900识别仪端口
		sprintf(str,"\"key\":\"%s\"\n",pConf->StrVehPlate900Key[i].c_str()); strJson = strJson + str;//900识别仪用户名密码
		if(i<VEHPLATE900_NUM-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	strJson = strJson + "\"camcount\":\""+ pConf->StrCAMCount +"\",\n";	//监控摄像头数量
	strJson = strJson + "\"camlist\": [\n";		//cam列表
	for(i=0;i<CAM_NUM;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"cam%d\",\n",i+1); strJson = strJson + str;//名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrCAMIP[i].c_str());strJson = strJson + str;//监控摄像头IP地址
		sprintf(str,"\"port\":\"%s\",\n",pConf->StrCAMPort[i].c_str()); strJson = strJson + str;//监控摄像头端口
		sprintf(str,"\"key\":\"%s\"\n",pConf->StrCAMKey[i].c_str()); strJson = strJson + str;//监控摄像头用户名密码
		if(i<CAM_NUM-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	sprintf(str,"\"firewaretype\":\"%d\",\n",pConf->IntFireWareType); strJson = strJson + str;//防火墙类型
	strJson = strJson + "\"firewarecount\":\""+ pConf->StrFireWareCount +"\",\n";	//防火墙数量
	strJson = strJson + "\"firewarelist\": [\n";		//防火墙列表
	for(i=0;i<FIREWARE_NUM;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"fireware%d\",\n",i+1); strJson = strJson + str;//名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrFireWareIP[i].c_str());strJson = strJson + str; //防火墙IP
		sprintf(str,"\"getpasswd\":\"%s\",\n",pConf->StrFireWareGetPasswd[i].c_str()); strJson = strJson + str;//防火墙get密码
		sprintf(str,"\"setpasswd\":\"%s\"\n",pConf->StrFireWareSetPasswd[i].c_str()); strJson = strJson + str;//防火墙set密码
		if(i<FIREWARE_NUM-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	sprintf(str,"\"ipswitchtype\":\"%d\",\n",pConf->IntIPSwitchType); strJson = strJson + str;//交换机类型
	strJson = strJson + "\"ipswitchcount\":\""+ pConf->StrIPSwitchCount +"\",\n";	//交换机数量
	strJson = strJson + "\"ipswitchlist\": [\n";		//交换机列表
	for(i=0;i<IPSWITCH_NUM;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"ipswitch%d\",\n",i+1); strJson = strJson + str;//名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrIPSwitchIP[i].c_str()); strJson = strJson + str;//交换机IP
		sprintf(str,"\"getpasswd\":\"%s\",\n",pConf->StrIPSwitchGetPasswd[i].c_str()); strJson = strJson + str;//交换机get密码
		sprintf(str,"\"setpasswd\":\"%s\"\n",pConf->StrIPSwitchSetPasswd[i].c_str()); strJson = strJson + str;//交换机set密码
		if(i<IPSWITCH_NUM-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	strJson = strJson + "\"atlastype\":\""+ pConf->StrAtlasType +"\",\n";	//atlas类型
	strJson = strJson + "\"atlascount\":\""+ pConf->StrAtlasCount +"\",\n";	//atlas数量
	strJson = strJson + "\"atlaslist\": [\n";		//atlas列表
	for(i=0;i<ATLAS_NUM;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"atlas%d\",\n",i+1); strJson = strJson + str;//名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrAtlasIP[i].c_str()); strJson = strJson + str;//atlasIP
		sprintf(str,"\"key\":\"%s\",\n",pConf->StrAtlasPasswd[i].c_str()); strJson = strJson + str;//atlas密码
		sprintf(str,"\"passwd\":\"%s\"\n",pConf->StrAtlasPasswd[i].c_str()); strJson = strJson + str;//atlas密码
		if(i<ATLAS_NUM-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	strJson = strJson + "\"spdcount\":\""+ pConf->StrSPDCount +"\",\n";	//防雷器数量
	strJson = strJson + "\"spdtype\":\""+ pConf->StrSPDType +"\",\n";	//防雷器类型
	if(pConf->StrSPDType=="1")
		strJson = strJson + "\"spdfactory\":\"雷迅\",\n";	//防雷器厂商
	else if(pConf->StrSPDType=="2")
		strJson = strJson + "\"spdfactory\":\"华咨圣泰\",\n";	//防雷器厂商
	else
		strJson = strJson + "\"spdfactory\":\"\",\n";	//防雷器厂商
	strJson = strJson + "\"spdlist\": [\n";		//防雷器列表
	for(i=0;i<SPD_NUM;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"spd%d\",\n",i+1); strJson = strJson + str;//名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrSPDIP[i].c_str()); strJson = strJson + str;//防雷器IP
		sprintf(str,"\"port\":\"%s\",\n",pConf->StrSPDPort[i].c_str()); strJson = strJson + str;//防雷器端口
		sprintf(str,"\"addr\":\"%s\"\n",pConf->StrSPDAddr[i].c_str()); strJson = strJson + str;//设备地址
		if(i<SPD_NUM-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	sprintf(str,"\"spdresip\":\"%s\",\n",pConf->StrSPDIP[SPD_NUM].c_str()); //接地电阻IP
	strJson = strJson + str;
	sprintf(str,"\"spdresport\":\"%s\",\n",pConf->StrSPDPort[SPD_NUM].c_str()); //接地电阻端口
	strJson = strJson + str;
	sprintf(str,"\"spdresid\":\"%s\",\n",pConf->StrSPDAddr[SPD_NUM].c_str()); //接地电阻设备地址
	strJson = strJson + str;
	if(pSpd!=NULL && pSpd->stuSpd_Param!=NULL)
	{
		sprintf(str,"\"spdres_alarm_value\":\"%d\",\n",pSpd->stuSpd_Param->rSPD_res.alarm_value); //接地电阻报警值
		strJson = strJson + str;
	}
	strJson = strJson + "\"do_count\":\""+ pConf->StrDoCount +"\",\n"; //do数量
	strJson = strJson + "\"dolist\": [\n";		//do映射列表
	for(i=0;i<atoi(pConf->StrDoCount.c_str());i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"%s\",\n",pConf->StrDeviceNameSeq[i].c_str()); strJson = strJson + str;//名称
		sprintf(str,"\"value\":\"%s\",\n",pConf->StrDoSeq[i].c_str()); strJson = strJson + str;//设备映射DO
		sprintf(str,"\"poweroff_or_not\":\"%s\"\n",pConf->StrPoweroff_Or_Not[i].c_str()); strJson = strJson + str;//市电停电时是否断电
		if(i<atoi(pConf->StrDoCount.c_str())-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	sprintf(str,"\"unwire_do_count\":\"%d\",\n",UNWIRE_SWITCH_COUNT); strJson = strJson + str;//无接do数量
	strJson = strJson + "\"unwire_dolist\": [\n";		//无接do映射列表
	for(i=0;i<UNWIRE_SWITCH_COUNT;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"name\":\"%s\",\n",pConf->StrUnWireDevName[i].c_str()); strJson = strJson + str;//名称
		sprintf(str,"\"value\":\"%s\"\n",pConf->StrUnWireDo[i].c_str()); strJson = strJson + str;//设备映射DO
		if(i<UNWIRE_SWITCH_COUNT-1)
			strJson +=	"},\n";
		else
			strJson +=	"}\n";
	}
	strJson +=	"],\n";
	
	for(i=0;i<LOCK_NUM;i++)
	{
		sprintf(str,"\"adrrlock%d\":\"%s\",\n",i+1,pConf->StrAdrrLock[i].c_str()); strJson = strJson + str;//门锁的地址
	}
	for(i=0;i<POWER_BD_NUM;i++)
	{
		sprintf(str,"\"poweraddr%d\":\"%s\",\n",i+1,pConf->StrAdrrPower[i].c_str()); strJson = strJson + str;//电源板1的地址
	}

	strJson = strJson + "\"devicetype\":\""+ pConf->StrdeviceType +"\",\n";	//设备型号900~919
	strJson = strJson + "\"hardwareid\":\""+ pConf->StrID +"\",\n";	//硬件ID
	strJson = strJson + "\"softversion\":\""+ pConf->StrVersionNo +"\",\n";	//主程序版本号920
	
	strJson = strJson + "\"cpualarmvalue\":\""+ pConf->StrCpuAlarmValue +"\",\n";	//CPU使用率报警阈值
	strJson = strJson + "\"cputempalarmvalue\":\""+ pConf->StrCpuTempAlarmValue +"\",\n";	//CPU温度报警阈值
	strJson = strJson + "\"memalarmvalue\":\""+ pConf->StrMemAlarmValue +"\",\n";	//内存使用率报警阈值
	
	strJson = strJson + "\"secsoftversion1\":\""+ getCanVersion(1) +"\",\n";	//副版本号
	strJson = strJson + "\"secsoftversion2\":\""+ pConf->StrsecSoftVersion[1] +"\",\n";	//副版本号
	strJson = strJson + "\"secsoftversion3\":\""+ pConf->StrsecSoftVersion[2] +"\",\n";	//副版本号
	strJson = strJson + "\"softdate\":\""+ pConf->StrSoftDate +"\"\n"; //版本日期
	
	strJson +=	"}";

    mstrjson = strJson;
printf("jsonStrVMCtlParamWriterXY 结束\r\n");
	return true;
}

bool jsonStrRsuWriterXY(int messagetype, string &mstrjson)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	CANNode *pCan=pCOsCan;					//Can对象
	char str[100],sDateTime[30];
	int i,j,rsucnt; 
	Artc *pRsu;
	Huawei *pHWRsu;
	Artc::RsuInfo_S ArtcState;
	Huawei::RsuInfo_S HWState;
	
	time_t nSeconds;
	struct tm * pTM;
	
	time(&nSeconds);
	pTM = localtime(&nSeconds);

	//系统日期和时间,格式: yyyymmddHHMMSS 
	sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
			pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
			pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
	strJson +=	"{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n"; //IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	sprintf(str,"\"rsucnt\":%s,\n",pConf->StrRSUCount.c_str()); strJson = strJson + str;//RSU数量
	sprintf(str,"\"rsutype\":%s,\n",pConf->StrRSUType.c_str()); strJson = strJson + str;//RSU类型
	rsucnt=atoi(pConf->StrRSUCount.c_str());
	strJson = strJson + "\"rsulist\": [\n"; 		//rsu列表
	if(pConf->StrRSUType=="1")			//门架通用型RSU
	{
		for(i=0;i<rsucnt;i++)
		{
			strJson = strJson + "{\n";	
			
			sprintf(str,"\"id\":\"%d\",\n",i+1); strJson = strJson + str;//RSU编号
			sprintf(str,"\"name\":\"rsu%d\",\n",i+1); strJson = strJson + str;//设备名称
			strJson = strJson + "\"ip\":\""+ pConf->StrRSUIP[i] +"\",\n";	//RSUIP地址
			sprintf(str,"\"port\":\"%s\",\n",pConf->StrRSUPort[i].c_str()); strJson = strJson + str;//RSU端口

			pRsu=pCArtRSU[i];
			if(pRsu!=NULL)
			{
				ArtcState=pRsu->getRsuInfo();
				if(ArtcState.controler.linked && GetTickCount()-ArtcState.controler.timestamp<1*60) //1分钟没更新，默认没连接
				{
					sprintf(str,"\"isonline\":\"%d\",\n",ArtcState.controler.linked); strJson = strJson + str;//连接状态
					if(pCan!=NULL)
					{
						sprintf(str,"\"volt\":%.1f,\n",pCan->canNode[i].phase.vln);strJson = strJson + str;	//电压 
						sprintf(str,"\"amp\":%.3f,\n",pCan->canNode[i].phase.amp); strJson = strJson + str;//电流
					}
					sprintf(str,"\"controlstatus\":%d,\n",ArtcState.controler.ctrlSta[0].state);strJson = strJson + str;//控制器1状态;00:表示正常 ，否则表示异常

					sprintf(str,"\"algid\":%d,\n",ArtcState.sta.algId); strJson = strJson + str;//算法标识，默认填写00H
					sprintf(str,"\"manuid\":\"%02x\",\n",ArtcState.sta.manuID); strJson = strJson + str;//路侧单元厂商代码
					if(ArtcState.sta.manuID==0xa8)
						{sprintf(str,"\"manufactory\":\"爱特斯\",\n"); strJson = strJson + str;}//路侧单元厂商
					else if(ArtcState.sta.manuID==0xa2)
						{sprintf(str,"\"manufactory\":\"万集\",\n"); strJson = strJson + str;}//路侧单元厂商
					else if(ArtcState.sta.manuID==0xa0)
						{sprintf(str,"\"manufactory\":\"金溢\",\n"); strJson = strJson + str;}//路侧单元厂商
					else if(ArtcState.sta.manuID==0xa1)
						{sprintf(str,"\"manufactory\":\"成谷\",\n"); strJson = strJson + str;}//路侧单元厂商
					sprintf(str,"\"controlid\":\"%02x%02x%02x\",\n",ArtcState.sta.id[0],ArtcState.sta.id[1],ArtcState.sta.id[2]); strJson = strJson + str;//路侧单元编号
					sprintf(str,"\"softwareversion\":\"%02x%02x\",\n",ArtcState.sta.ver[0],ArtcState.sta.ver[1]); strJson = strJson + str;//路侧单元软件版本号
					sprintf(str,"\"workstatus\":%d,\n",ArtcState.sta.workSta);strJson = strJson + str;	//工作模式返回状态，默认填写00H
					sprintf(str,"\"flagid\":\"%02x%02x%02x\",\n",ArtcState.sta.flagId[0],ArtcState.sta.flagId[1],ArtcState.sta.flagId[2]); strJson = strJson + str;//ETC门架编号（由C0 帧中获取,失败填充00H
					if(ArtcState.sta.PsamNum>0)
						{sprintf(str,"\"psamcount\":%d,\n",ArtcState.sta.PsamNum); strJson = strJson + str;}//PSAM数量
					else
						{sprintf(str,"\"psamcount\":%d\n",ArtcState.sta.PsamNum); strJson = strJson + str;}//PSAM数量
					if(ArtcState.sta.PsamNum>0)
					{
						strJson = strJson + "\"psamlist\": [\n";		//psam列表
						for(j=0;j<ArtcState.sta.PsamNum;j++)
						{
							strJson = strJson + "{\n";	
							sprintf(str,"\"id\": \"%d\",\n", ArtcState.sta.psamInfo[j].channel);strJson += str;		//PSAM卡插槽号
							sprintf(str,"\"name\": \"psam%d\",\n", j+1);strJson += str;		//名称
							sprintf(str,"\"psamid\":\"%02x%02x%02x%02x%02x%02x\",\n",ArtcState.sta.psamInfo[j].id[0],
								ArtcState.sta.psamInfo[j].id[1],ArtcState.sta.psamInfo[j].id[2],ArtcState.sta.psamInfo[j].id[3],
								ArtcState.sta.psamInfo[j].id[4],ArtcState.sta.psamInfo[j].id[5]);
							strJson = strJson + str; // PSAM信息
							sprintf(str,"\"status\": \"%d\"\n", ArtcState.sta.psamInfo[j].auth);strJson += str;		//1字节PSAM授权状态00H已授权01H未授权
							if(j==ArtcState.sta.PsamNum-1)
								strJson = strJson + "}\n";
							else
								strJson = strJson + "},\n";
						}
						strJson = strJson + "]\n";
					}
				}
				else
				{
					if(pCan!=NULL)
					{
						sprintf(str,"\"volt\":%.1f,\n",pCan->canNode[i].phase.vln);strJson = strJson + str;	//电压 
						sprintf(str,"\"amp\":%.3f,\n",pCan->canNode[i].phase.amp); strJson = strJson + str;//电流
					}
					ArtcState.controler.linked=false;
					ArtcState.controler.totalAnt=0;
					sprintf(str,"\"isonline\":\"%d\"\n",ArtcState.controler.linked); strJson = strJson + str;//连接状态
				}
			}
			else
			{
				sprintf(str,"\"isonline\":\"0\"\n"); strJson = strJson + str;//连接状态
			}
			if(i==rsucnt-1)
				strJson = strJson + "}\n";
			else
				strJson = strJson + "},\n";
		}
		strJson = strJson + "],\n";
		if(ArtcState.controler.totalAnt>0)
		{
			sprintf(str,"\"antennacount\":%d,\n",ArtcState.controler.totalAnt);strJson = strJson + str; //天线数量
			strJson = strJson + "\"antennalist\": [\n"; 	//天线头列表
			for(j=0;j<ArtcState.controler.totalAnt;j++)
			{
				strJson = strJson + "{\n";	
				sprintf(str,"\"id\": \"%d\",\n", j+1);strJson += str;		//天线编号
				sprintf(str,"\"name\": \"antenna%d\",\n", j+1);strJson += str;		//名称
				sprintf(str,"\"status\": %d,\n", ArtcState.controler.antInfo[j].sta);strJson += str;		//38 天线i 控制状态
				sprintf(str,"\"power\": %d,\n", ArtcState.controler.antInfo[j].power);strJson += str; //39 天线i 功率
				sprintf(str,"\"channel\": %d\n", ArtcState.controler.antInfo[j].channel);strJson += str; //40 天线i 信道号
				if(j==ArtcState.controler.totalAnt-1)
					strJson = strJson + "}\n";
				else
					strJson = strJson + "},\n";
			}
			strJson = strJson + "]\n";
		}
		else
		{
			sprintf(str,"\"antennacount\":%d\n",ArtcState.controler.totalAnt);strJson = strJson + str;	//天线数量
		}
	}
	else if(pConf->StrRSUType=="2")		//华为RSU
	{
		for(i=0;i<rsucnt;i++)
		{
			strJson = strJson + "{\n";	
			
			sprintf(str,"\"id\":\"%d\",\n",i+1); strJson = strJson + str;//RSU编号
			sprintf(str,"\"name\":\"rsu%d\",\n",i+1); strJson = strJson + str;//设备名称
			strJson = strJson + "\"ip\":\""+ pConf->StrRSUIP[i] +"\",\n";	//RSUIP地址
			sprintf(str,"\"port\":\"%s\",\n",pConf->StrRSUPort[i].c_str()); strJson = strJson + str;//RSU端口
	
			pHWRsu=pCHWRSU[i];
			if(pHWRsu!=NULL)
			{
				HWState=pHWRsu->getRsuInfo();
				if(HWState.linked && GetTickCount()-HWState.timestamp<1*60) //1分钟没更新，默认没连接
				{
					sprintf(str,"\"isonline\":\"%d\",\n",HWState.linked); strJson = strJson + str;//连接状态
					if(pCan!=NULL)
					{
						sprintf(str,"\"volt\":%.3f,\n",pCan->canNode[i].phase.vln);strJson = strJson + str;	//电压 
						sprintf(str,"\"amp\":%.3f,\n",pCan->canNode[i].phase.amp); strJson = strJson + str;//电流
					}
					sprintf(str,"\"manufactory\":\"华为\",\n"); strJson = strJson + str;//路侧单元厂商
					sprintf(str,"\"controlstatus\":%d,\n",atoi(HWState.status.c_str()));strJson = strJson + str;//控制器1状态;00:表示正常 ，否则表示异常
		
					sprintf(str,"\"cpurate\":\"%s\",\n",HWState.cpuRate.c_str()); strJson = strJson + str;//Cpu占用率
					sprintf(str,"\"memrate\":\"%s\",\n",HWState.memRate.c_str()); strJson = strJson + str;//内存占用率
					sprintf(str,"\"temperture\":\"%s\",\n",HWState.temperture.c_str()); strJson = strJson + str;//温度
					sprintf(str,"\"controlid\":\"%s\",\n",HWState.seriNum.c_str()); strJson = strJson + str;//序列号
					sprintf(str,"\"softwareversion\":\"%s\",\n",HWState.softVer.c_str()); strJson = strJson + str;//路侧单元软件版本号
					sprintf(str,"\"hardwareversion\":\"%s\",\n",HWState.hardVer.c_str()); strJson = strJson + str;//硬件版本
					sprintf(str,"\"longitude\":\"%s\",\n",HWState.longitude.c_str());strJson = strJson + str; //设备位置经度
					sprintf(str,"\"latitude\":\"%s\"\n",HWState.latitude.c_str()); strJson = strJson + str;//设备位置纬度
				}
				else
				{
					HWState.linked=false;
					sprintf(str,"\"isonline\":\"%d\"\n",HWState.linked); strJson = strJson + str;//连接状态
				}
			}
			else
			{
				sprintf(str,"\"isonline\":\"0\"\n"); strJson = strJson + str;//连接状态
			}
			if(i==rsucnt-1)
				strJson = strJson + "}\n";
			else
				strJson = strJson + "},\n";
		}
		strJson = strJson + "]\n";
	}
	
	strJson = strJson + "}\n";

	mstrjson=strJson;
	return true;
}


bool jsonStrVehPlateWriter(int messagetype, string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j; 
	int vehplatecnt;
	IPCam *pCVP=NULL;
	IPCam::State_S State;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
	time_t nSeconds;
	struct tm * pTM;
	
	time(&nSeconds);
	pTM = localtime(&nSeconds);

	//系统日期和时间,格式: yyyymmddHHMMSS 
	sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
			pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
			pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;

	strJson +=	"{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n"; //IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	// 
	strJson = strJson + "\"vehplatecnt\":\""+ pConf->StrVehPlateCount +"\",\n";	//车牌识别数量
	vehplatecnt=atoi(pConf->StrVehPlateCount.c_str());
	strJson = strJson + "\"vehplatelist\":[\n";
	for(i=0;i<vehplatecnt;i++)
	{
		strJson = strJson + "{\n";
	
		sprintf(str,"\"name\":\"vehplate%d\",\n",i+1);strJson+=str;	//摄相机名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrVehPlateIP[i].c_str());strJson+=str;	//摄相机IP
		sprintf(str,"\"port\":\"%s\",\n",pConf->StrVehPlatePort[i].c_str());strJson+=str;	//识别仪端口
		sprintf(str,"\"key\":\"%s\",\n",pConf->StrVehPlateKey[i].c_str());strJson+=str;	//用户名密码
		strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n"; //时间

		pCVP=pCVehplate[i];
		if(pCVP!=NULL)
		{
			State = pCVP->getState();
			if(State.linked && GetTickCount()-State.timestamp<5*60)//5分钟没更新，恢复默认
			{
				sprintf(str,"\"isonline\":\"%d\",\n",State.linked); strJson = strJson + str;//连接状态
				sprintf(str,"\"picstateid\":%s,\n",State.picstateid.c_str());strJson+=str;	//流水号
				sprintf(str,"\"gantryid\":%s,\n",State.gantryid.c_str());strJson+=str;	//门架编号,全网唯一编号
				sprintf(str,"\"statetime\":%s,\n",State.statetime.c_str());strJson+=str	;	//状态采集时间
				sprintf(str,"\"overstockimagejourcount\":%s,\n",State.overstockImageJourCount.c_str());strJson+=str;	//积压图片流水数
				sprintf(str,"\"overstockimagecount\":%s,\n",State.overstockImageCount.c_str());strJson+=str;	//积压图片数
				sprintf(str,"\"cameranum\":%s,\n",State.cameranum.c_str());strJson+=str;	//相机编号（101~299）
				sprintf(str,"\"lanenum\":%s,\n",State.lanenum.c_str());strJson+=str;	//车道编号
				sprintf(str,"\"connectstatus\":%s,\n",State.connectstatus.c_str());strJson+=str;	//连接状态
				sprintf(str,"\"workstatus\":%s,\n",State.workstatus.c_str());strJson+=str;	//工作状态
				sprintf(str,"\"lightworkstatus\":%s,\n",State.lightworkstatus.c_str());strJson+=str;	//补光灯的工作状态
				sprintf(str,"\"recognitionrate\":%s,\n",State.recognitionrate.c_str());strJson+=str;	//识别成功率
				sprintf(str,"\"hardwareversion\":%s,\n",State.hardwareversion.c_str());strJson+=str;//固件版本
				sprintf(str,"\"softwareversion\":%s,\n",State.softwareversion.c_str());strJson+=str;	//软件版本
				sprintf(str,"\"runningtime\":%s,\n",State.runningtime.c_str());strJson+=str;	//设备从开机到现在的运行时间（秒）
				sprintf(str,"\"brand\":%s,\n",State.brand.c_str());strJson+=str;	//厂商型号
				sprintf(str,"\"devicetype\":%s,\n",State.devicetype.c_str());strJson+=str;	//设备型号
				sprintf(str,"\"statuscode\":%s,\n",State.statuscode.c_str());strJson+=str;	//状态码,详见附录A3 0-正常；其他由厂商自定义
				sprintf(str,"\"statusmsg\":%s\n",State.statusmsg.c_str());strJson+=str;	//状态描述 由厂商自定义,最大长度256 例如：正常
			}
			else
			{
				sprintf(str,"\"isonline\":\"%d\"\n",State.linked); strJson = strJson + str;//连接状态
			}
		}
		else
		{
			sprintf(str,"\"isonline\":\"0\"\n"); strJson = strJson + str;//连接状态
		}
		if(i==vehplatecnt-1)
			strJson = strJson + "}\n";
		else
			strJson = strJson + "},\n";
	}
	strJson = strJson + "]\n";
	
	strJson +=	"}\n\n\0\0";

	mstrjson=strJson;
	return true;
}

bool jsonStrVehPlate900Writer(int messagetype, string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j; 
	int vehplate900cnt;
	IPCam *pCVP=NULL;
	IPCam::State_S State;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
	time_t nSeconds;
	struct tm * pTM;
	
	time(&nSeconds);
	pTM = localtime(&nSeconds);

	//系统日期和时间,格式: yyyymmddHHMMSS 
	sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
			pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
			pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;

	strJson +=	"{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n"; //IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	// 
	strJson = strJson + "\"vehplate900cnt\":\""+ pConf->StrVehPlate900Count +"\",\n";	//车牌识别数量
	vehplate900cnt=atoi(pConf->StrVehPlate900Count.c_str());
	strJson = strJson + "\"vehplate900list\":[\n";
	for(i=0;i<vehplate900cnt;i++)
	{
		strJson = strJson + "{\n";
	
		sprintf(str,"\"name\":\"vehplate900%d\",\n",i+1);strJson+=str;	//摄相机名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrVehPlate900IP[i].c_str());strJson+=str;	//摄相机IP
		sprintf(str,"\"port\":\"%s\",\n",pConf->StrVehPlate900Port[i].c_str());strJson+=str;	//识别仪端口
		sprintf(str,"\"key\":\"%s\",\n",pConf->StrVehPlate900Key[i].c_str());strJson+=str;	//用户名密码
		strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n"; //时间

		pCVP=pCVehplate900[i];
		if(pCVP!=NULL)
		{
			State = pCVP->getState();
			if(State.linked && GetTickCount()-State.timestamp<5*60)//5分钟没更新，恢复默认
			{
				sprintf(str,"\"isonline\":\"%d\",\n",State.linked); strJson = strJson + str;//连接状态
				sprintf(str,"\"picstateid\":%s,\n",State.picstateid.c_str());strJson+=str;	//流水号
				sprintf(str,"\"gantryid\":%s,\n",State.gantryid.c_str());strJson+=str;	//门架编号,全网唯一编号
				sprintf(str,"\"statetime\":%s,\n",State.statetime.c_str());strJson+=str	;	//状态采集时间
				sprintf(str,"\"overstockimagejourCount\":%s,\n",State.overstockImageJourCount.c_str());strJson+=str;	//积压图片流水数
				sprintf(str,"\"overstockimagecount\":%s,\n",State.overstockImageCount.c_str());strJson+=str;	//积压图片数
				sprintf(str,"\"cameranum\":%s,\n",State.cameranum.c_str());strJson+=str;	//相机编号（101~299）
				sprintf(str,"\"lanenum\":%s,\n",State.lanenum.c_str());strJson+=str;	//车道编号
				sprintf(str,"\"connectstatus\":%s,\n",State.connectstatus.c_str());strJson+=str;	//连接状态
				sprintf(str,"\"workstatus\":%s,\n",State.workstatus.c_str());strJson+=str;	//工作状态
				sprintf(str,"\"lightworkstatus\":%s,\n",State.lightworkstatus.c_str());strJson+=str;	//补光灯的工作状态
				sprintf(str,"\"recognitionrate\":%s,\n",State.recognitionrate.c_str());strJson+=str;	//识别成功率
				sprintf(str,"\"hardwareversion\":%s,\n",State.hardwareversion.c_str());strJson+=str;//固件版本
				sprintf(str,"\"softwareversion\":%s,\n",State.softwareversion.c_str());strJson+=str;	//软件版本
				sprintf(str,"\"runningtime\":%s,\n",State.runningtime.c_str());strJson+=str;	//设备从开机到现在的运行时间（秒）
				sprintf(str,"\"brand\":%s,\n",State.brand.c_str());strJson+=str;	//厂商型号
				sprintf(str,"\"devicetype\":%s,\n",State.devicetype.c_str());strJson+=str;	//设备型号
				sprintf(str,"\"statuscode\":%s,\n",State.statuscode.c_str());strJson+=str;	//状态码,详见附录A3 0-正常；其他由厂商自定义
				sprintf(str,"\"statusmsg\":%s\n",State.statusmsg.c_str());strJson+=str;	//状态描述 由厂商自定义,最大长度256 例如：正常
			}
			else
			{
				sprintf(str,"\"isonline\":\"%d\"\n",State.linked); strJson = strJson + str;//连接状态
			}
		}
		else
		{
			sprintf(str,"\"isonline\":\"0\"\n"); strJson = strJson + str;//连接状态
		}
		if(i==vehplate900cnt-1)
			strJson = strJson + "}\n";
		else
			strJson = strJson + "},\n";
	}
	strJson = strJson + "]\n";
	
	strJson +=	"}\n\n\0\0";

	mstrjson=strJson;
	return true;
}

bool jsonStrVMCtrlStateWriter(int messagetype, string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j; 
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	VMCONTROL_STATE *pSta=&VMCtl_State; //控制器运行状态结构体
	
	time_t nSeconds;
	struct tm * pTM;
	
	time(&nSeconds);
	pTM = localtime(&nSeconds);

	//系统日期和时间,格式: yyyymmddHHMMSS 
	sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
			pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
			pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;

	strJson +=	"{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n"; //IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型

//printf("VMCtrlState now=0x%x, TimeStamp=0x%x, ticks=%d\n",GetTickCount(),pSta->TimeStamp,GetTickCount()-pSta->TimeStamp);
	if(pSta->Linked && GetTickCount()-pSta->TimeStamp>2*60)//如果大于2分钟没更新，认为没连接
	{
		init_lt_state_struct();
char msg[256];
sprintf(msg,"VMCtrlState now=0x%x last=0x%x \r\n",GetTickCount(),pSta->TimeStamp);
WriteLog(msg);
	}
	strJson = strJson + "\"hostname\":\""+ pSta->strhostname +"\",\n";	//主机名称
	strJson = strJson + "\"cpurate\":\""+ pSta->strcpuRate +"\",\n";	//CPU占用率
	strJson = strJson + "\"cputemp\":\""+ pSta->strcpuTemp +"\",\n";	//CPU温度
	strJson = strJson + "\"mentotal\":\""+ pSta->strmenTotal +"\",\n";	//内存总数
	strJson = strJson + "\"menused\":\""+ pSta->strmenUsed +"\",\n";	//已使用内存
	strJson = strJson + "\"menrate\":\""+ pSta->strmenRate +"\",\n";	//内存使用率
	strJson = strJson + "\"zimagever\":\""+ pSta->strzimageVer +"\",\n";	//内核版本
	strJson = strJson + "\"zimagedate\":\""+ pSta->strzimageDate +"\",\n";	//内核日期

	strJson = strJson + "\"cpualarm\":\""+ pSta->strCpuAlarm +"\",\n";		//CPU使用率报警
	strJson = strJson + "\"cputempalarm\":\""+ pSta->strCpuTempAlarm +"\",\n";		//CPU温度报警
	strJson = strJson + "\"memalarm\":\""+ pSta->strMemAlarm +"\",\n";		//内存使用率报警
	strJson = strJson + "\"softrate\":\""+ pSta->strSoftRate +"\",\n";		//主程序CPU占用率
	strJson = strJson + "\"softalarm\":\""+ pSta->strSoftAlarm +"\",\n";	//主程序运行异常报警
	strJson = strJson + "\"ksoftirqdrate\":\""+ pSta->strKsoftirqdRate +"\",\n";	//ksoftirqdrateCPU占用率
	strJson = strJson + "\"kworkerrate\":\""+ pSta->strkworkerRate +"\",\n";	//kworkerCPU占用率
	strJson = strJson + "\"lan2broadcastalarm\":\""+ pSta->strLan2BroadcastAlarm +"\",\n";	//网口2广播风暴报警
	strJson = strJson + "\"lan2broadcastalarmmesg\":\""+ pSta->strLan2BroadcastAlarmMesg +"\",\n";	//网口2广播风暴报警信息
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\"\n";	//时间
	strJson +=	"}\n\0";

	mstrjson=strJson;
	return true;
}
	
	
	
bool SetjsonAuthorityStr(int messagetype,char *json, int *len)
{
	char str[100],sDateTime[30];
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
    strJson +=  "{\n";
	sprintf(str,"%d",messagetype);strJson = strJson + "\"messagetype\":"+ str +",\n";	//消息类型				
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	if(pConf->user=="admin")
	{
		if(pConf->password=="lt1" || pConf->password=="admin")
		{
			strJson = strJson + "\"code\": 0,\n";	//操作码
			strJson = strJson + "\"describe\": \"\",\n"; //操作码
			strJson = strJson + "\"authority\": 1,\n";	//用户权限
			strJson = strJson + "\"token\":\""+ pConf->token +"\",\n"; //票据
		}
		else if(pConf->password=="lt12")
		{
			strJson = strJson + "\"code\": 0,\n";	//操作码
			strJson = strJson + "\"describe\": \"\",\n"; //操作码
			strJson = strJson + "\"authority\": 2,\n";	//用户权限
			strJson = strJson + "\"token\":\""+ pConf->token +"\",\n"; //票据
		}
		else if(pConf->password=="lt123" || pConf->password=="ltyjy")
		{
			strJson = strJson + "\"code\": 0,\n";	//操作码
			strJson = strJson + "\"describe\": \"\",\n"; //操作码
			strJson = strJson + "\"authority\": 3,\n";	//用户权限
			strJson = strJson + "\"token\":\""+ pConf->token +"\",\n"; //票据
		}
		else
		{
			strJson = strJson + "\"code\": 1,\n";	//操作码
			strJson = strJson + "\"describe\": \"密码不正确\",\n"; //操作码
			strJson = strJson + "\"authority\": 0,\n";	//用户权限
			strJson = strJson + "\"token\":\""+ pConf->token +"\",\n"; //票据
		}
	}
	else 
	{
		strJson = strJson + "\"code\": 1,\n";	//操作码
		strJson = strJson + "\"describe\": \"用户没授权\",\n"; //操作码
		strJson = strJson + "\"authority\": 0,\n";	//用户权限
		strJson = strJson + "\"token\": \"\",\n";	//票据
	}
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\"\n";	//时间

	strJson +=	"}\n\n\0\0";

	*len=strJson.length();
	memcpy(json,(char*)strJson.c_str(),*len);

	return true;
}

	
		
bool SetjsonReceiveOKStr(int messagetype,char *json, int *len)
{
	char str[100],sDateTime[30];
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
    strJson +=  "{\n";
	sprintf(str,"%d",messagetype);strJson = strJson + "\"messagetype\":"+ str +",\n";	//消息类型				
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"code\": 0,\n";	//操作码
	strJson = strJson + "\"describe\": \"\",\n";	//操作码
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	strJson = strJson + "\"receive\": \"OK\"\n";	

	strJson +=	"}\n\n\0\0";

//	printf("the json len= %d out = %s\n",strJson.length(), strJson.c_str());
	*len=strJson.length();
	memcpy(json,(char*)strJson.c_str(),*len);

	return true;
}

//19回路电压电流开关状态
bool jsonStrSwitchStatusWriter(int messagetype, string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j; 
	static int recordno=0;
	int va_meter_bd,phase,docount;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	CANNode *pCan=pCOsCan;					//Can对象
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	
	docount=atoi(pConf->StrDoCount.c_str());
	if(pCan!=NULL)
	{
		for(i=0;i<docount;i++) //开关数量
		{
			sprintf(str,"\"do%d_isconnect\":%d,\n",i+1,pCan->canNode[i].isConnect); strJson = strJson + str;//连接状态
			if(pCan->canNode[i].phase.vln<24.0)
				sprintf(str,"\"do%d_status\":0,\n",i+1); //断电
			else 
				sprintf(str,"\"do%d_status\":1,\n",i+1); //通电
			strJson = strJson + str;
			sprintf(str,"\"do%d_vol\":%.3f,\n",i+1,pCan->canNode[i].phase.vln); strJson = strJson + str;//电压
			if(i==docount-1)
			{
				sprintf(str,"\"do%d_amp\":%.3f\n",i+1,pCan->canNode[i].phase.amp); strJson = strJson + str;//电流
			}
			else
			{
				sprintf(str,"\"do%d_amp\":%.3f,\n",i+1,pCan->canNode[i].phase.amp); strJson = strJson + str;//电流
			}
		}
	}

	strJson +=	"}\n\n\0\0";

	mstrjson=strJson;
	return true;
}


//19回路电压电流开关状态-新粤
bool jsonStrSwitchStatusWriterXY(int messagetype, string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j,docount; 
	int va_meter_bd,phase;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	CANNode *pCan=pCOsCan;					//Can对象
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	
	strJson = strJson + "\"do_count\": \"" + pConf->StrDoCount + "\",\n";	//do数量
	strJson = strJson + "\"dolist\": [\n"; 		//do列表

	docount=atoi(pConf->StrDoCount.c_str());
	for(i=0;i<docount;i++)
	{
		strJson = strJson + "{\n";		
		for(j=0;j<docount;j++)
		{
			if(atoi(pConf->StrDoSeq[j].c_str())==i+1)
				break;
		}
		strJson = strJson + "\"name\": \"" + pConf->StrDeviceNameSeq[j] + "\",\n";	//设备名称
		if(pCan!=NULL)
		{
			sprintf(str,"\"isconnect\":%d,\n",pCan->canNode[i].isConnect); strJson = strJson + str;//连接状态
			if(pCan->canNode[i].phase.vln<24.0)
				sprintf(str,"\"status\":0,\n"); //断电
			else 
				sprintf(str,"\"status\":1,\n"); //通电
			strJson = strJson + str;
			sprintf(str,"\"volt\":%.1f,\n",pCan->canNode[i].phase.vln); strJson = strJson + str;//电压
			sprintf(str,"\"amp\":%.3f\n",pCan->canNode[i].phase.amp); strJson = strJson + str;//电流
		}
		else
		{
			sprintf(str,"\"isconnect\":0\n"); strJson = strJson + str;//连接状态
		}
		if(i==docount-1)
			strJson = strJson + "}\n";
		else
			strJson = strJson + "},\n";
	}
	strJson = strJson + "]\n";
	strJson = strJson + "}\n";

	mstrjson=strJson;

	return true;
}


bool jsonStrHWCabinetWriter(int messagetype,char *pstPam, string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j,status; 
	static int recordno=0;
	CabinetClient *pCab=(CabinetClient *)pstPam;//华为机柜状态
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型

    pthread_mutex_lock(&pCab->CabinetdataMutex);
	if(pCab->HUAWEIDevValue.hwLinked && GetTickCount()-pCab->HUAWEIDevValue.hwTimeStamp<15*60)//超过15分钟没更新，认为没有连接
	{
	//华为机柜字段
	#if(CABINETTYPE>=1 && CABINETTYPE<=4)
		sprintf(str,"\"isonline\":\"%d\",\n",pCab->HUAWEIDevValue.hwLinked); strJson = strJson + str;//连接状态
		sprintf(str,"\"acbgroupbatonline\":\"%d\",\n",pCab->HUAWEIDevValue.AcbGroupBatOnline); strJson = strJson + str;//锂电池组是否在线
		sprintf(str,"\"dcaironline\":\"%d\",\n",pCab->HUAWEIDevValue.DcAirOnline); strJson = strJson + str;//直流空调是否在线（设备柜）
		sprintf(str,"\"dcaironline2\":\"%d\",\n",pCab->HUAWEIDevValue.DcAirOnline2); strJson = strJson + str;//直流空调是否在线（电池柜）
		sprintf(str,"\"temhumonline\":\"%d\",\n",pCab->HUAWEIDevValue.TemHumOnline); strJson = strJson + str;//温湿度传感器是否在线（设备柜）
		sprintf(str,"\"temhumonline2\":\"%d\",\n",pCab->HUAWEIDevValue.TemHumOnline2); strJson = strJson + str;//温湿度传感器是否在线（电池柜）

	    strJson = strJson + "\"hwacbgroupbatvolt\": " + pCab->HUAWEIDevValue.strhwAcbGroupBatVolt.c_str() + ",\n";	//锂电电池电压 214
	    strJson = strJson + "\"hwacbgroupbatcurr\": " + pCab->HUAWEIDevValue.strhwAcbGroupBatCurr.c_str() + ",\n";	//锂电电池电流 215
	    strJson = strJson + "\"hwacbgrouptotalcapacity\": " + pCab->HUAWEIDevValue.strhwAcbGroupTotalCapacity.c_str() + ",\n";	//锂电电池总容量 216
	    strJson = strJson + "\"hwacbgrouptotalremaincapacity\": " + pCab->HUAWEIDevValue.strhwAcbGroupTotalRemainCapacity.c_str() + ",\n";	//锂电电池剩余容量 217
	    strJson = strJson + "\"hwacbgroupbackuptime\": " + pCab->HUAWEIDevValue.strhwAcbGroupBackupTime.c_str() + ",\n";	//电池备电时长 218
	    strJson = strJson + "\"hwacbgroupbatsoh\": " + pCab->HUAWEIDevValue.strhwAcbGroupBatSoh.c_str() + ",\n";	//电池SOH 219
	    strJson = strJson + "\"hwaporablvoltage\": " + pCab->HUAWEIDevValue.strhwApOrAblVoltage.c_str() + ",\n";	//A/AB电压 220
	    strJson = strJson + "\"hwbporbclvoltage\": " + pCab->HUAWEIDevValue.strhwBpOrBclVoltage.c_str() + ",\n";	//B/BC电压 221
	    strJson = strJson + "\"hwcporcalvoltage\": " + pCab->HUAWEIDevValue.strhwCpOrCalVoltage.c_str() + ",\n";	//C/CA电压 222
	    strJson = strJson + "\"hwaphasecurrent\": " + pCab->HUAWEIDevValue.strhwAphaseCurrent.c_str() + ",\n";	//A相电流 223
	    strJson = strJson + "\"hwbphasecurrent\": " + pCab->HUAWEIDevValue.strhwBphaseCurrent.c_str() + ",\n";	//B相电流 224
	    strJson = strJson + "\"hwcphasecurrent\": " + pCab->HUAWEIDevValue.strhwCphaseCurrent.c_str() + ",\n";	//C相电流 225
	    strJson = strJson + "\"hwdcoutputvoltage\": " + pCab->HUAWEIDevValue.strhwDcOutputVoltage.c_str() + ",\n";	//DC输出电压 226
	    strJson = strJson + "\"hwdcoutputcurrent\": " + pCab->HUAWEIDevValue.strhwDcOutputCurrent.c_str() + ",\n";	//DC输出电流 227
	    strJson = strJson + "\"hwenvtemperature\": " + pCab->HUAWEIDevValue.strhwEnvTemperature[0].c_str() + ",\n";	//环境温度值 228
	    strJson = strJson + "\"hwenvtemperature2\": " + pCab->HUAWEIDevValue.strhwEnvTemperature[1].c_str() + ",\n";	//环境温度值 228
	    strJson = strJson + "\"hwenvhumidity\": " + pCab->HUAWEIDevValue.strhwEnvHumidity[0].c_str() + ",\n";	//环境湿度值 229
	    strJson = strJson + "\"hwenvhumidity2\": " + pCab->HUAWEIDevValue.strhwEnvHumidity[1].c_str() + ",\n";	//环境湿度值 229
	//    strJson = strJson + "\"hwdcairctrlmode\": " + pCab->HUAWEIDevValue.strhwDcAirCtrlMode[0].c_str() + ",\n";	//空调控制模式 230
	//    strJson = strJson + "\"hwdcairctrlmode2\": " + pCab->HUAWEIDevValue.strhwDcAirCtrlMode[1].c_str() + ",\n";	//空调控制模式 230
	    strJson = strJson + "\"hwdcairrunstatus\": " + pCab->HUAWEIDevValue.strhwDcAirRunStatus[0].c_str() + ",\n";	//空调运行状态 231
	    strJson = strJson + "\"hwdcairrunstatus2\": " + pCab->HUAWEIDevValue.strhwDcAirRunStatus[1].c_str() + ",\n";	//空调运行状态 231
	    strJson = strJson + "\"hwdcaircompressorrunstatus\": " + pCab->HUAWEIDevValue.strhwDcAirCompressorRunStatus[0].c_str() + ",\n";	//空调压缩机运行状态 232
	    strJson = strJson + "\"hwdcaircompressorrunstatus2\": " + pCab->HUAWEIDevValue.strhwDcAirCompressorRunStatus[1].c_str() + ",\n";	//空调压缩机运行状态 232
	    strJson = strJson + "\"hwdcairinnrfanspeed\": " + pCab->HUAWEIDevValue.strhwDcAirInnrFanSpeed[0].c_str() + ",\n";	//空调内机转速 233
	    strJson = strJson + "\"hwdcairinnrfanspeed2\": " + pCab->HUAWEIDevValue.strhwDcAirInnrFanSpeed[1].c_str() + ",\n";	//空调内机转速 233
	    strJson = strJson + "\"hwdcairouterfanspeed\": " + pCab->HUAWEIDevValue.strhwDcAirOuterFanSpeed[0].c_str() + ",\n";	//空调外风机转速 234
	    strJson = strJson + "\"hwdcairouterfanspeed2\": " + pCab->HUAWEIDevValue.strhwDcAirOuterFanSpeed[1].c_str() + ",\n";	//空调外风机转速 234
	    strJson = strJson + "\"hwdcaircompressorruntime\": " + pCab->HUAWEIDevValue.strhwDcAirCompressorRunTime[0].c_str() + ",\n";	//空调压缩机运行时间 235
	    strJson = strJson + "\"hwdcaircompressorruntime2\": " + pCab->HUAWEIDevValue.strhwDcAirCompressorRunTime[1].c_str() + ",\n";	//空调压缩机运行时间 235
	    strJson = strJson + "\"hwdcairenterchanneltemp\": " + pCab->HUAWEIDevValue.strhwDcAirEnterChannelTemp[0].c_str() + ",\n";	//空调回风口温度 236
	    strJson = strJson + "\"hwdcairenterchanneltemp2\": " + pCab->HUAWEIDevValue.strhwDcAirEnterChannelTemp[1].c_str() + ",\n";	//空调回风口温度 236
	    strJson = strJson + "\"hwdcairpowerontemppoint\": " + pCab->HUAWEIDevValue.strhwDcAirPowerOnTempPoint[0].c_str() + ",\n";	//空调开机温度点 237
	    strJson = strJson + "\"hwdcairpowerontemppoint2\": " + pCab->HUAWEIDevValue.strhwDcAirPowerOnTempPoint[1].c_str() + ",\n";	//空调开机温度点 237
	    strJson = strJson + "\"hwdcairpowerofftemppoint\": " + pCab->HUAWEIDevValue.strhwDcAirPowerOffTempPoint[0].c_str() + ",\n";	//空调关机温度点 238
	    strJson = strJson + "\"hwdcairpowerofftemppoint2\": " + pCab->HUAWEIDevValue.strhwDcAirPowerOffTempPoint[1].c_str() + ",\n";	//空调关机温度点 238
	    strJson = strJson + "\"hwenvtempalarmtraps\": " + pCab->HUAWEIDevAlarm.hwEnvTempAlarmTraps.c_str() + ",\n";	//设备柜环境温度告警 239
	    strJson = strJson + "\"hwenvtempalarmtraps2\": " + pCab->HUAWEIDevAlarm.hwEnvTempAlarmTraps2.c_str() + ",\n";	//电池柜环境温度告警 239
	    strJson = strJson + "\"hwenvhumialarmresumetraps\": " + pCab->HUAWEIDevAlarm.hwEnvHumiAlarmTraps.c_str() + ",\n";	//设备柜环境湿度告警 240
	    strJson = strJson + "\"hwenvhumialarmresumetraps2\": " + pCab->HUAWEIDevAlarm.hwEnvHumiAlarmTraps2.c_str() + ",\n";	//电池柜环境湿度告警 240
	    strJson = strJson + "\"hwdooralarmtraps\": " + pCab->HUAWEIDevAlarm.hwDoorAlarmTraps.c_str() + ",\n";	//门禁告警(设备柜) 241
	    strJson = strJson + "\"hwdooralarmtraps2\": " + pCab->HUAWEIDevAlarm.hwDoorAlarmTraps2.c_str() + ",\n";	//门禁告警（电池柜） 241
	    strJson = strJson + "\"hwwateralarmtraps\": " + pCab->HUAWEIDevAlarm.hwWaterAlarmTraps.c_str() + ",\n";	//水浸告警 242
	    strJson = strJson + "\"hwwateralarmtraps2\": " + pCab->HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str() + ",\n";	//水浸告警 242
	    strJson = strJson + "\"hwsmokealarmtraps\": " + pCab->HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str() + ",\n";	//烟雾告警 243
	    strJson = strJson + "\"hwsmokealarmtraps2\": " + pCab->HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str() + ",\n";	//烟雾告警 243
	    strJson = strJson + "\"hwair_cond_infan_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_infan_alarm.c_str() + ",\n";	//空调内风机故障 245
	    strJson = strJson + "\"hwair_cond_infan_alarm2\": " + pCab->HUAWEIDevAlarm.hwair_cond_infan_alarm2.c_str() + ",\n";	//空调内风机故障 245
	    strJson = strJson + "\"hwair_cond_outfan_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_outfan_alarm.c_str() + ",\n";	//空调外风机故障 246
	    strJson = strJson + "\"hwair_cond_outfan_alarm2\": " + pCab->HUAWEIDevAlarm.hwair_cond_outfan_alarm2.c_str() + ",\n";	//空调外风机故障 246
	    strJson = strJson + "\"hwair_cond_comp_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_comp_alarm.c_str() + ",\n";	//空调压缩机故障 247
	    strJson = strJson + "\"hwair_cond_comp_alarm2\": " + pCab->HUAWEIDevAlarm.hwair_cond_comp_alarm2.c_str() + ",\n";	//空调压缩机故障 247
	    strJson = strJson + "\"hwair_cond_return_port_sensor_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_return_port_sensor_alarm.c_str() + ",\n";	//空调回风口传感器故障 248
	    strJson = strJson + "\"hwair_cond_return_port_sensor_alarm2\": " + pCab->HUAWEIDevAlarm.hwair_cond_return_port_sensor_alarm2.c_str() + ",\n";	//空调回风口传感器故障 248
	    strJson = strJson + "\"hwair_cond_evap_freezing_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_evap_freezing_alarm.c_str() + ",\n";	//空调蒸发器冻结 249
	    strJson = strJson + "\"hwair_cond_evap_freezing_alarm2\": " + pCab->HUAWEIDevAlarm.hwair_cond_evap_freezing_alarm2.c_str() + ",\n";	//空调蒸发器冻结 249
	    strJson = strJson + "\"hwair_cond_freq_high_press_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_freq_high_press_alarm.c_str() + ",\n";	//空调频繁高压力 250
	    strJson = strJson + "\"hwair_cond_freq_high_press_alarm2\": " + pCab->HUAWEIDevAlarm.hwair_cond_freq_high_press_alarm2.c_str() + ",\n";	//空调频繁高压力 250
	    strJson = strJson + "\"hwair_cond_comm_fail_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_comm_fail_alarm.c_str() + ",\n";	//空调通信失败告警 251
	    strJson = strJson + "\"hwair_cond_comm_fail_alarm2\": " + pCab->HUAWEIDevAlarm.hwair_cond_comm_fail_alarm2.c_str() + ",\n";	//空调通信失败告警 251
		//防雷器告警
	    strJson = strJson + "\"hwacspdalarmtraps\": " + pCab->HUAWEIDevAlarm.hwACSpdAlarmTraps.c_str() + ",\n";	//交流防雷器故障
	    strJson = strJson + "\"hwdcspdalarmtraps\": " + pCab->HUAWEIDevAlarm.hwDCSpdAlarmTraps.c_str() + ",\n";	//直流防雷器故障
		//新增加的状态
		//设备信息 
	    strJson = strJson + "\"hwmonequipsoftwareversion\": \"" + pCab->HUAWEIDevValue.strhwMonEquipSoftwareVersion.c_str() + "\",\n";	//软件版本
	    strJson = strJson + "\"hwmonequipmanufacturer\": \"" + pCab->HUAWEIDevValue.strhwMonEquipManufacturer.c_str() + "\",\n";	//设备生产商
		//锂电(新增加)
	    strJson = strJson + "\"hwacbgrouptemperature\": " + pCab->HUAWEIDevValue.strhwAcbGroupTemperature.c_str() + ",\n";	//电池温度
	    strJson = strJson + "\"hwacbgroupovercurthr\": " + pCab->HUAWEIDevValue.strhwAcbGroupOverCurThr.c_str() + ",\n";	//充电过流告警点
	    strJson = strJson + "\"hwacbgrouphightempthr\": " + pCab->HUAWEIDevValue.strhwAcbGroupHighTempThr.c_str() + ",\n";	//高温告警点
	    strJson = strJson + "\"hwacbgrouplowtempth\": " + pCab->HUAWEIDevValue.strhwAcbGroupLowTempTh.c_str() + ",\n";	//低温告警点
	    strJson = strJson + "\"hwacbgroupdodtoacidbattery\": " + pCab->HUAWEIDevValue.strhwAcbGroupDodToAcidBattery.c_str() + ",\n";	//锂电放电DOD
		//开关电源(新增加)
	    strJson = strJson + "\"hwsetacsuppervoltlimit\": " + pCab->HUAWEIDevValue.strhwSetAcsUpperVoltLimit.c_str() + ",\n";	//AC过压点设置
	    strJson = strJson + "\"hwsetacslowervoltlimit\": " + pCab->HUAWEIDevValue.strhwSetAcsLowerVoltLimit.c_str() + ",\n";	//AC欠压点设置
	    strJson = strJson + "\"hwsetdcsuppervoltlimit\": " + pCab->HUAWEIDevValue.strhwSetDcsUpperVoltLimit.c_str() + ",\n";	//设置DC过压点
	    strJson = strJson + "\"hwsetdcslowervoltlimit\": " + pCab->HUAWEIDevValue.strhwSetDcsLowerVoltLimit.c_str() + ",\n";	//设置DC欠压点
	    strJson = strJson + "\"hwsetlvdvoltage\": " + pCab->HUAWEIDevValue.strhwSetLvdVoltage.c_str() + ",\n";	//设置LVD电压
		//环境传感器(新增加)
	    strJson = strJson + "\"hwsetenvtempupperlimit\": " + pCab->HUAWEIDevValue.strhwSetEnvTempUpperLimit[0].c_str() + ",\n";	//环境温度告警上限
	    strJson = strJson + "\"hwsetenvtempupperlimit2\": " + pCab->HUAWEIDevValue.strhwSetEnvTempUpperLimit[1].c_str() + ",\n";	//环境温度告警上限
	    strJson = strJson + "\"hwsetenvtemplowerlimit\": " + pCab->HUAWEIDevValue.strhwSetEnvTempLowerLimit[0].c_str() + ",\n";	//环境温度告警下限
	    strJson = strJson + "\"hwsetenvtemplowerlimit2\": " + pCab->HUAWEIDevValue.strhwSetEnvTempLowerLimit[1].c_str() + ",\n";	//环境温度告警下限
	    strJson = strJson + "\"hwsetenvhumidityupperlimit\": " + pCab->HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[0].c_str() + ",\n";	//环境湿度告警上限
	    strJson = strJson + "\"hwsetenvhumidityupperlimit2\": " + pCab->HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[1].c_str() + ",\n";	//环境湿度告警上限
	    strJson = strJson + "\"hwsetenvhumiditylowerlimit\": " + pCab->HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[0].c_str() + ",\n";	//环境湿度告警下限
	    strJson = strJson + "\"hwsetenvhumiditylowerlimit2\": " + pCab->HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[1].c_str() + ",\n";	//环境湿度告警下限
		//直流空调(新增加)
	    strJson = strJson + "\"hwdcairruntime\": " + pCab->HUAWEIDevValue.strhwDcAirRunTime[0].c_str() + ",\n";	//设备柜空调运行时间
	    strJson = strJson + "\"hwdcairruntime2\": " + pCab->HUAWEIDevValue.strhwDcAirRunTime[1].c_str() + ",\n";	//电池柜空调运行时间
	    strJson = strJson + "\"hwcoolingdevicesmode\": " + pCab->HUAWEIDevValue.strhwCoolingDevicesMode.c_str() + ",\n";	//温控模式
		//供配电
		//电源告警
		sprintf(str,"\"hwacinputfailalarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcInputFailAlarm.c_str());strJson += str;		 //交流电源输入停电告警
		sprintf(str,"\"hwacinputl1failalarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcInputL1FailAlarm.c_str());strJson += str;		 //交流电源输入L1	相缺相告警
		sprintf(str,"\"hwacinputl2failalarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcInputL2FailAlarm.c_str());strJson += str;		 //交流电源输入L2	相缺相告警
		sprintf(str,"\"hwacinputl3failalarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcInputL3FailAlarm.c_str());strJson += str;		 //交流电源输入L3	相缺相告警
		sprintf(str,"\"hwdcvoltalarmtraps\": %s,\n", pCab->HUAWEIDevAlarm.hwDcVoltAlarmTraps.c_str());strJson += str;		 //直流电源输出告警
		sprintf(str,"\"hwloadlvdalarmtraps\": %s,\n", pCab->HUAWEIDevAlarm.hwLoadLvdAlarmTraps.c_str());strJson += str;		 //LLVD1下电告警
		//锂电池告警
		sprintf(str,"\"hwacbgroup_comm_fail_alarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_comm_fail_alarm.c_str());strJson += str;		 //所有锂电通信失败
		sprintf(str,"\"hwacbgroup_discharge_alarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_discharge_alarm.c_str());strJson += str;		 //电池放电告警
		sprintf(str,"\"hwacbgroup_charge_overcurrent_alarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_charge_overcurrent_alarm.c_str());strJson += str;		 //电池充电过流
		sprintf(str,"\"hwacbgroup_temphigh_alarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_temphigh_alarm.c_str());strJson += str;		 //电池温度高
		sprintf(str,"\"hwacbgroup_templow_alarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_templow_alarm.c_str());strJson += str;		 //电池温度低
		sprintf(str,"\"hwacbgroup_poweroff_alarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_poweroff_alarm.c_str());strJson += str;		 //电池下电
		sprintf(str,"\"hwacbgroup_fusebreak_alarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_fusebreak_alarm.c_str());strJson += str;		 //电池熔丝断
		sprintf(str,"\"hwacbgroup_moduleloss_alarm\": %s,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_moduleloss_alarm.c_str());strJson += str;		 //模块丢失
		//2019-08-20新增
		sprintf(str,"\"hwacbgroupbatrunningstate\": %s,\n", pCab->HUAWEIDevValue.strhwAcbGroupBatRunningState.c_str());strJson += str;		 //电池状态
		sprintf(str,"\"hwsmokesensorstatus\": %s,\n", pCab->HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str());strJson += str;		 //烟雾传感器状态
		sprintf(str,"\"hwsmokesensorstatus2\": %s,\n", pCab->HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str());strJson += str;		 //烟雾传感器状态
		sprintf(str,"\"hwwatersensorstatus\": %s,\n", pCab->HUAWEIDevAlarm.hwWaterAlarmTraps.c_str());strJson += str;		 //水浸传感器状态
		sprintf(str,"\"hwwatersensorstatus2\": %s,\n", pCab->HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str());strJson += str;		 //水浸传感器状态
		sprintf(str,"\"hwdoorsensorstatus\": %s,\n", pCab->HUAWEIDevAlarm.hwDoorAlarmTraps.c_str());strJson += str;		 //（电池柜）门磁传感器状态
		sprintf(str,"\"hwdoorsensorstatus2\": %s,\n", pCab->HUAWEIDevAlarm.hwDoorAlarmTraps2.c_str());strJson += str;		 //（设备柜）门磁传感器状态
		sprintf(str,"\"hwdcairequipaddress\": %d,\n", pCab->hwAirAddrbuf[0]);strJson += str;		 //空调地址（设备柜）
		sprintf(str,"\"hwdcairequipaddress2\": %d,\n", pCab->hwAirAddrbuf[1]);strJson += str;		 //空调地址（电池柜）
		sprintf(str,"\"hwtemhumequipaddress\": %d,\n", pCab->hwTemAddrbuf[0]);strJson += str;		 //温湿度地址（设备柜）
		sprintf(str,"\"hwtemhumequipaddress2\": %d,\n", pCab->hwTemAddrbuf[1]);strJson += str;		 //空调地址（电池柜）
		if(pCab->HUAWEIDevValue.strhwAcbBatVolt!="2147483647")
			{sprintf(str,"\"hwacbbatvolt\": %.1f,\n", atoi(pCab->HUAWEIDevValue.strhwAcbBatVolt.c_str())/10.0);strJson += str;}		 //单个电池电压
		else	
			{sprintf(str,"\"hwacbbatvolt\": %d,\n", pCab->HUAWEIDevValue.strhwAcbBatVolt.c_str());strJson += str;}		 //单个电池电压
		sprintf(str,"\"hwacbbatcurr\": %s,\n", pCab->HUAWEIDevValue.strhwAcbBatCurr.c_str());strJson += str;		 //单个电池电流
		sprintf(str,"\"hwacbbatsoh\": %s,\n", pCab->HUAWEIDevValue.strhwAcbBatSoh.c_str());strJson += str;		 //单个电池串SOH
		sprintf(str,"\"hwacbbatcapacity\": %s,\n", pCab->HUAWEIDevValue.strhwAcbBatCapacity.c_str());strJson += str;		 //单个电池容量
		//2019-11-19新增4个门锁状态
		status=0;	sprintf(str,"\"hwequcabfrontdoorstatus\": \"%d\",\n", status);strJson += str;		 //设备柜前门锁状态
		status=0;	sprintf(str,"\"hwequcabbackdoorstatus\": \"%d\",\n", status);strJson += str;		 //设备柜后门锁状态
		status=0;	sprintf(str,"\"hwbatcabfrontdoorstatus\": \"%d\",\n", status);strJson += str;		 //电池柜前门锁状态
		status=0;	sprintf(str,"\"hwbatcabbackdoorstatus\": \"%d\",\n", status);strJson += str;		 //电池柜后门锁状态
#if 0
		status=LockerStatusGet(0);	sprintf(str,"\"hwequcabfrontdoorstatus\": \"%d\",\n", status);strJson += str;		 //设备柜前门锁状态
		status=LockerStatusGet(1);	sprintf(str,"\"hwequcabbackdoorstatus\": \"%d\",\n", status);strJson += str;		 //设备柜后门锁状态
		status=LockerStatusGet(2);	sprintf(str,"\"hwbatcabfrontdoorstatus\": \"%d\",\n", status);strJson += str;		 //电池柜前门锁状态
		status=LockerStatusGet(3);	sprintf(str,"\"hwbatcabbackdoorstatus\": \"%d\",\n", status);strJson += str;		 //电池柜后门锁状态
#endif
	//CABINETTYPE  1：华为（包括华为单门 双门等） 5：中兴; 6：金晟安; 7：爱特斯 StrVersionNo
	//2019-12-13新增飞达中兴机柜状态
	#elif(CABINETTYPE==5)//飞达中兴
		sprintf(str,"\"isonline\":\"%d\",\n",HUAWEIDevValue.hwLinked); strJson = strJson + str;//连接状态
		sprintf(str,"\"acbgroupbatonline\":\"%d\",\n",HUAWEIDevValue.AcbGroupBatOnline); strJson = strJson + str;//锂电池组是否在线
		sprintf(str,"\"dcaironline\":\"%d\",\n",HUAWEIDevValue.DcAirOnline); strJson = strJson + str;//直流空调是否在线（设备柜）
		sprintf(str,"\"dcaironline2\":\"%d\",\n",HUAWEIDevValue.DcAirOnline2); strJson = strJson + str;//直流空调是否在线（电池柜）
		sprintf(str,"\"temhumonline\":\"%d\",\n",HUAWEIDevValue.TemHumOnline); strJson = strJson + str;//温湿度传感器是否在线（设备柜）
		sprintf(str,"\"temhumonline2\":\"%d\",\n",HUAWEIDevValue.TemHumOnline2); strJson = strJson + str;//温湿度传感器是否在线（电池柜）

		sprintf(str,"\"hwmonequipmanufacturer\": \"中兴\",\n");	strJson += str;//设备生产商
		strJson = strJson + "\"hwaporablvoltage\": " + HUAWEIDevValue.strhwApOrAblVoltage.c_str() + ",\n";	//A/AB电压 220
		strJson = strJson + "\"hwbporbclvoltage\": " + HUAWEIDevValue.strhwBpOrBclVoltage.c_str() + ",\n";	//B/BC电压 221
		strJson = strJson + "\"hwcporcalvoltage\": " + HUAWEIDevValue.strhwCpOrCalVoltage.c_str() + ",\n";	//C/CA电压 222
		strJson = strJson + "\"hwaphasecurrent\": " + HUAWEIDevValue.strhwAphaseCurrent.c_str() + ",\n";	//A相电流 223
		strJson = strJson + "\"hwbphasecurrent\": " + HUAWEIDevValue.strhwBphaseCurrent.c_str() + ",\n";	//B相电流 224
		strJson = strJson + "\"hwcphasecurrent\": " + HUAWEIDevValue.strhwCphaseCurrent.c_str() + ",\n";	//C相电流 225
		strJson = strJson + "\"hwenvtemperature\": " + HUAWEIDevValue.strhwEnvTemperature[0].c_str() + ",\n";	//环境温度值 228
		strJson = strJson + "\"hwenvtemperature2\": " + HUAWEIDevValue.strhwEnvTemperature[1].c_str() + ",\n";	//环境温度值 228
		strJson = strJson + "\"hwenvhumidity\": " + HUAWEIDevValue.strhwEnvHumidity[0].c_str() + ",\n"; //环境湿度值 229
		strJson = strJson + "\"hwenvhumidity2\": " + HUAWEIDevValue.strhwEnvHumidity[1].c_str() + ",\n";	//环境湿度值 229
		strJson = strJson + "\"hwdcairenterchanneltemp\": " + HUAWEIDevValue.strhwDcAirEnterChannelTemp[0].c_str() + ",\n"; //空调回风口温度 236
		strJson = strJson + "\"hwdcairenterchanneltemp2\": " + HUAWEIDevValue.strhwDcAirEnterChannelTemp[1].c_str() + ",\n";	//空调回风口温度 236
		strJson = strJson + "\"hwdcairpowerontemppoint\": " + HUAWEIDevValue.strhwDcAirPowerOnTempPoint[0].c_str() + ",\n"; //空调开机温度点 237
		strJson = strJson + "\"hwdcairpowerontemppoint2\": " + HUAWEIDevValue.strhwDcAirPowerOnTempPoint[1].c_str() + ",\n";	//空调开机温度点 237
		//环境传感器(新增加)
		strJson = strJson + "\"hwsetenvtempupperlimit\": " + HUAWEIDevValue.strhwSetEnvTempUpperLimit[0].c_str() + ",\n";	//环境温度告警上限
		strJson = strJson + "\"hwsetenvtempupperlimit2\": " + HUAWEIDevValue.strhwSetEnvTempUpperLimit[1].c_str() + ",\n";	//环境温度告警上限
		strJson = strJson + "\"hwsetenvtemplowerlimit\": " + HUAWEIDevValue.strhwSetEnvTempLowerLimit[0].c_str() + ",\n";	//环境温度告警下限
		strJson = strJson + "\"hwsetenvtemplowerlimit2\": " + HUAWEIDevValue.strhwSetEnvTempLowerLimit[1].c_str() + ",\n";	//环境温度告警下限
		strJson = strJson + "\"hwsetenvhumidityupperlimit\": " + HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[0].c_str() + ",\n";	//环境湿度告警上限
		strJson = strJson + "\"hwsetenvhumidityupperlimit2\": " + HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[1].c_str() + ",\n";	//环境湿度告警上限
		strJson = strJson + "\"hwsetenvhumiditylowerlimit\": " + HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[0].c_str() + ",\n";	//环境湿度告警下限
		strJson = strJson + "\"hwsetenvhumiditylowerlimit2\": " + HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[1].c_str() + ",\n";	//环境湿度告警下限
	    strJson = strJson + "\"hwdooralarmtraps\": " + HUAWEIDevAlarm.hwDoorAlarmTraps.c_str() + ",\n";	//门禁告警(设备柜) 241
	    strJson = strJson + "\"hwdooralarmtraps2\": " + HUAWEIDevAlarm.hwDoorAlarmTraps2.c_str() + ",\n";	//门禁告警（电池柜） 241
	    strJson = strJson + "\"hwwateralarmtraps\": " + HUAWEIDevAlarm.hwWaterAlarmTraps.c_str() + ",\n";	//水浸告警 242
	    strJson = strJson + "\"hwwateralarmtraps2\": " + HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str() + ",\n";	//水浸告警 242
	    strJson = strJson + "\"hwsmokealarmtraps\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str() + ",\n";	//烟雾告警 243
	    strJson = strJson + "\"hwsmokealarmtraps2\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str() + ",\n";	//烟雾告警 243
		//供配电
		//电源告警
		sprintf(str,"\"hwacinputfailalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputFailAlarm.c_str());strJson += str; 	 //交流电源输入停电告警
		sprintf(str,"\"hwacinputl1failalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputL1FailAlarm.c_str());strJson += str; 	 //交流电源输入L1 相缺相告警
		sprintf(str,"\"hwacinputl2failalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputL2FailAlarm.c_str());strJson += str; 	 //交流电源输入L2 相缺相告警
		sprintf(str,"\"hwacinputl3failalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputL3FailAlarm.c_str());strJson += str; 	 //交流电源输入L3 相缺相告警
		sprintf(str,"\"hwsmokesensorstatus\": %s,\n", HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str());strJson += str; 	 //烟雾传感器状态
		sprintf(str,"\"hwsmokesensorstatus2\": %s,\n", HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str());strJson += str; 	 //烟雾传感器状态
		sprintf(str,"\"hwwatersensorstatus\": %s,\n", HUAWEIDevAlarm.hwWaterAlarmTraps.c_str());strJson += str; 	 //水浸传感器状态
		sprintf(str,"\"hwwatersensorstatus2\": %s,\n", HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str());strJson += str; 	 //水浸传感器状态
		sprintf(str,"\"hwdoorsensorstatus\": %s,\n", HUAWEIDevAlarm.hwDoorAlarmTraps.c_str());strJson += str;		 //（电池柜）门磁传感器状态
		sprintf(str,"\"hwdoorsensorstatus2\": %s,\n", HUAWEIDevAlarm.hwDoorAlarmTraps2.c_str());strJson += str;		 //（电池柜）门磁传感器状态

		sprintf(str,"\"rectifiermodulevol\": %s,\n", HUAWEIDevValue.RectifierModuleVol.c_str());strJson += str;		 //整流器输出电压
		sprintf(str,"\"rectifiermodulecurr\": %s,\n", HUAWEIDevValue.RectifierModuleCurr.c_str());strJson += str;		 //整流器输出电流
		sprintf(str,"\"rectifiermoduletemp\": %s,\n", HUAWEIDevValue.RectifierModuleTemp.c_str());strJson += str;		 //整流器机内温度
	    //空调
		sprintf(str,"\"in_fanstate\": %s,\n", HUAWEIDevValue.StrIn_FanState.c_str());strJson += str;		 //内风机状态 0代表关闭 1代表开启
		sprintf(str,"\"in_fanstate2\": %s,\n", HUAWEIDevValue.StrIn_FanState2.c_str());strJson += str;		 //内风机状态2 0代表关闭 1代表开启
		sprintf(str,"\"out_fanstate\": %s,\n", HUAWEIDevValue.StrOut_FanState.c_str());strJson += str;		 //外风机状态 0代表关闭 1代表开启
		sprintf(str,"\"out_fanstate2\": %s,\n", HUAWEIDevValue.StrOut_FanState2.c_str());strJson += str;		 //外风机状态2 0代表关闭 1代表开启
		//中兴机柜增加开关电源报警
		sprintf(str,"\"switchpowercom_alarm\": %s,\n", HUAWEIDevAlarm.SwitchPowerCom_alarm.c_str());strJson += str;		 //开关电源断线告警 0代表正常 1代表告警
		sprintf(str,"\"rectifiermodulecom_alarm\": %s,\n", HUAWEIDevAlarm.RectifierModuleCom_alarm.c_str());strJson += str;		 //整流模块通讯故障 0代表正常 1代表告警
		sprintf(str,"\"hwacinputfailalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputFailAlarm.c_str());strJson += str; 	 //交流电源输入停电告警
		//空调
		sprintf(str,"\"air_high_temper_alarm\": %s,\n", HUAWEIDevAlarm.Air_High_temper_alarm.c_str());strJson += str;		 //高温告警 0代表正常 1代表告警
		sprintf(str,"\"air_lower_temper_alarm\": %s,\n", HUAWEIDevAlarm.Air_Lower_temper_alarm.c_str());strJson += str;		 //低温告警 0代表正常 1代表告警
		sprintf(str,"\"air_heater_alarm\": %s,\n", HUAWEIDevAlarm.Air_Heater_alarm.c_str());strJson += str;		 //加热器故障告警 0代表正常 1代表告警
		sprintf(str,"\"air_temper_sensor_alarm\": %s,\n", HUAWEIDevAlarm.Air_Temper_Sensor_alarm.c_str());strJson += str;		 //温度传感器故障 0代表正常 1代表告警
	#elif(CABINETTYPE==6)//金晟安
		sprintf(str,"\"isonline\":\"%d\",\n",HUAWEIDevValue.hwLinked); strJson = strJson + str;//连接状态
		sprintf(str,"\"acbgroupbatonline\":\"%d\",\n",HUAWEIDevValue.AcbGroupBatOnline); strJson = strJson + str;//锂电池组是否在线
		sprintf(str,"\"dcaironline\":\"%d\",\n",HUAWEIDevValue.DcAirOnline); strJson = strJson + str;//直流空调是否在线（设备柜）
		sprintf(str,"\"dcaironline2\":\"%d\",\n",HUAWEIDevValue.DcAirOnline2); strJson = strJson + str;//直流空调是否在线（电池柜）
		sprintf(str,"\"temhumonline\":\"%d\",\n",HUAWEIDevValue.TemHumOnline); strJson = strJson + str;//温湿度传感器是否在线（设备柜）
		sprintf(str,"\"temhumonline2\":\"%d\",\n",HUAWEIDevValue.TemHumOnline2); strJson = strJson + str;//温湿度传感器是否在线（电池柜）

		sprintf(str,"\"hwmonequipmanufacturer\": \"金晟安\",\n");	strJson += str;//设备生产商
		strJson = strJson + "\"hwacbgroupbatvolt\": " + HUAWEIDevValue.strhwAcbGroupBatVolt.c_str() + ",\n";	//锂电电池电压 214
		strJson = strJson + "\"hwacbgrouptotalcapacity\": " + HUAWEIDevValue.strhwAcbGroupTotalCapacity.c_str() + ",\n";	//锂电电池总容量 216
		strJson = strJson + "\"hwacbgrouptotalremaincapacity\": " + HUAWEIDevValue.strhwAcbGroupTotalRemainCapacity.c_str() + ",\n";	//锂电电池剩余容量 217
		strJson = strJson + "\"hwdcoutputvoltage\": " + HUAWEIDevValue.strhwDcOutputVoltage.c_str() + ",\n";	//DC输出电压 226
		strJson = strJson + "\"hwenvtemperature\": " + HUAWEIDevValue.strhwEnvTemperature[0].c_str() + ",\n";	//环境温度值 228
		strJson = strJson + "\"hwenvtemperature2\": " + HUAWEIDevValue.strhwEnvTemperature[1].c_str() + ",\n";	//环境温度值 228
		strJson = strJson + "\"hwenvhumidity\": " + HUAWEIDevValue.strhwEnvHumidity[0].c_str() + ",\n"; //环境湿度值 229
		strJson = strJson + "\"hwenvhumidity2\": " + HUAWEIDevValue.strhwEnvHumidity[1].c_str() + ",\n";	//环境湿度值 229
		strJson = strJson + "\"hwdcairrunstatus\": " + HUAWEIDevValue.strhwDcAirRunStatus[0].c_str() + ",\n";	//空调运行状态 231
		strJson = strJson + "\"hwdcairrunstatus2\": " + HUAWEIDevValue.strhwDcAirRunStatus[1].c_str() + ",\n";	//空调运行状态 231
		strJson = strJson + "\"hwdcaircompressorrunstatus\": " + HUAWEIDevValue.strhwDcAirCompressorRunStatus[0].c_str() + ",\n";	//空调压缩机运行状态 232
		strJson = strJson + "\"hwdcaircompressorrunstatus2\": " + HUAWEIDevValue.strhwDcAirCompressorRunStatus[1].c_str() + ",\n";	//空调压缩机运行状态 232
		strJson = strJson + "\"hwdcairenterchanneltemp\": " + HUAWEIDevValue.strhwDcAirEnterChannelTemp[0].c_str() + ",\n"; //空调回风口温度 236
		strJson = strJson + "\"hwdcairenterchanneltemp2\": " + HUAWEIDevValue.strhwDcAirEnterChannelTemp[1].c_str() + ",\n";	//空调回风口温度 236
		strJson = strJson + "\"hwdcairpowerontemppoint\": " + HUAWEIDevValue.strhwDcAirPowerOnTempPoint[0].c_str() + ",\n"; //空调开机温度点 237
		strJson = strJson + "\"hwdcairpowerontemppoint2\": " + HUAWEIDevValue.strhwDcAirPowerOnTempPoint[1].c_str() + ",\n";	//空调开机温度点 237
		strJson = strJson + "\"hwdcairpowerofftemppoint\": " + HUAWEIDevValue.strhwDcAirPowerOffTempPoint[0].c_str() + ",\n";	//空调关机温度点 238
		strJson = strJson + "\"hwdcairpowerofftemppoint2\": " + HUAWEIDevValue.strhwDcAirPowerOffTempPoint[1].c_str() + ",\n";	//空调关机温度点 238
		strJson = strJson + "\"hwenvtempalarmtraps\": " + HUAWEIDevAlarm.hwEnvTempAlarmTraps.c_str() + ",\n";	//设备柜环境温度告警 239
		strJson = strJson + "\"hwenvtempalarmtraps2\": " + HUAWEIDevAlarm.hwEnvTempAlarmTraps2.c_str() + ",\n"; //电池柜环境温度告警 239
		strJson = strJson + "\"hwenvhumialarmresumetraps\": " + HUAWEIDevAlarm.hwEnvHumiAlarmTraps.c_str() + ",\n"; //设备柜环境湿度告警 240
		strJson = strJson + "\"hwenvhumialarmresumetraps2\": " + HUAWEIDevAlarm.hwEnvHumiAlarmTraps2.c_str() + ",\n";	//电池柜环境湿度告警 240
		strJson = strJson + "\"hwdooralarmtraps\": " + HUAWEIDevAlarm.hwDoorAlarmTraps.c_str() + ",\n"; //门禁告警(设备柜) 241
		strJson = strJson + "\"hwdooralarmtraps2\": " + HUAWEIDevAlarm.hwDoorAlarmTraps2.c_str() + ",\n";	//门禁告警（电池柜） 241
		strJson = strJson + "\"hwwateralarmtraps\": " + HUAWEIDevAlarm.hwWaterAlarmTraps.c_str() + ",\n";	//水浸告警 242
		strJson = strJson + "\"hwwateralarmtraps2\": " + HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str() + ",\n"; //水浸告警 242
		strJson = strJson + "\"hwsmokealarmtraps\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str() + ",\n";	//烟雾告警 243
		strJson = strJson + "\"hwsmokealarmtraps2\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str() + ",\n"; //烟雾告警 243
		strJson = strJson + "\"hwair_cond_infan_alarm\": " + HUAWEIDevAlarm.hwair_cond_infan_alarm.c_str() + ",\n"; //空调内风机故障 245
		strJson = strJson + "\"hwair_cond_infan_alarm2\": " + HUAWEIDevAlarm.hwair_cond_infan_alarm2.c_str() + ",\n";	//空调内风机故障 245
		strJson = strJson + "\"hwair_cond_outfan_alarm\": " + HUAWEIDevAlarm.hwair_cond_outfan_alarm.c_str() + ",\n";	//空调外风机故障 246
		strJson = strJson + "\"hwair_cond_outfan_alarm2\": " + HUAWEIDevAlarm.hwair_cond_outfan_alarm2.c_str() + ",\n"; //空调外风机故障 246
		strJson = strJson + "\"hwair_cond_evap_freezing_alarm\": " + HUAWEIDevAlarm.hwair_cond_evap_freezing_alarm.c_str() + ",\n"; //空调蒸发器冻结 249
		strJson = strJson + "\"hwair_cond_evap_freezing_alarm2\": " + HUAWEIDevAlarm.hwair_cond_evap_freezing_alarm2.c_str() + ",\n";	//空调蒸发器冻结 249
		//锂电(新增加)
		strJson = strJson + "\"hwacbgrouptemperature\": " + HUAWEIDevValue.strhwAcbGroupTemperature.c_str() + ",\n";	//电池温度
		//开关电源(新增加)
		strJson = strJson + "\"hwsetacslowervoltlimit\": " + HUAWEIDevValue.strhwSetAcsLowerVoltLimit.c_str() + ",\n";	//AC欠压点设置
		//环境传感器(新增加)
		strJson = strJson + "\"hwsetenvtempupperlimit\": " + HUAWEIDevValue.strhwSetEnvTempUpperLimit[0].c_str() + ",\n";	//环境温度告警上限
		strJson = strJson + "\"hwsetenvtempupperlimit2\": " + HUAWEIDevValue.strhwSetEnvTempUpperLimit[1].c_str() + ",\n";	//环境温度告警上限
		strJson = strJson + "\"hwsetenvtemplowerlimit\": " + HUAWEIDevValue.strhwSetEnvTempLowerLimit[0].c_str() + ",\n";	//环境温度告警下限
		strJson = strJson + "\"hwsetenvtemplowerlimit2\": " + HUAWEIDevValue.strhwSetEnvTempLowerLimit[1].c_str() + ",\n";	//环境温度告警下限
		strJson = strJson + "\"hwsetenvhumidityupperlimit\": " + HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[0].c_str() + ",\n";	//环境湿度告警上限
		strJson = strJson + "\"hwsetenvhumidityupperlimit2\": " + HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[1].c_str() + ",\n";	//环境湿度告警上限
		strJson = strJson + "\"hwsetenvhumiditylowerlimit\": " + HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[0].c_str() + ",\n";	//环境湿度告警下限
		strJson = strJson + "\"hwsetenvhumiditylowerlimit2\": " + HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[1].c_str() + ",\n";	//环境湿度告警下限
		//供配电
		//电源告警
		sprintf(str,"\"hwacinputfailalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputFailAlarm.c_str());strJson += str; 	 //交流电源输入停电告警
		sprintf(str,"\"hwacinputl1failalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputL1FailAlarm.c_str());strJson += str; 	 //交流电源输入L1 相缺相告警
		//锂电池告警
		sprintf(str,"\"hwacbgroup_temphigh_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_temphigh_alarm.c_str());strJson += str;		 //电池温度高
		sprintf(str,"\"hwacbgroup_templow_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_templow_alarm.c_str());strJson += str; 	 //电池温度低
		//2019-08-20新增
		sprintf(str,"\"hwacbgroupbatrunningstate\": %s,\n", HUAWEIDevValue.strhwAcbGroupBatRunningState.c_str());strJson += str;		 //电池状态
		sprintf(str,"\"hwsmokesensorstatus\": %s,\n", HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str());strJson += str; 	 //烟雾传感器状态
		sprintf(str,"\"hwsmokesensorstatus2\": %s,\n", HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str());strJson += str;		 //烟雾传感器状态
		sprintf(str,"\"hwwatersensorstatus\": %s,\n", HUAWEIDevAlarm.hwWaterAlarmTraps.c_str());strJson += str; 	 //水浸传感器状态
		sprintf(str,"\"hwwatersensorstatus2\": %s,\n", HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str());strJson += str;		 //水浸传感器状态
		sprintf(str,"\"hwdoorsensorstatus\": %s,\n", HUAWEIDevAlarm.hwDoorAlarmTraps.c_str());strJson += str;		 //（电池柜）门磁传感器状态
		sprintf(str,"\"hwdoorsensorstatus2\": %s,\n", HUAWEIDevAlarm.hwDoorAlarmTraps2.c_str());strJson += str; 	 //（设备柜）门磁传感器状态
		sprintf(str,"\"hwacbbatvolt\": %d,\n", HUAWEIDevValue.strhwAcbBatVolt.c_str());strJson += str;		 //单个电池电压
		sprintf(str,"\"hwacbbatcapacity\": %s,\n", HUAWEIDevValue.strhwAcbBatCapacity.c_str());strJson += str;		 //单个电池容量
#if 0
		//2019-11-19新增4个门锁状态
		status=LockerStatusGet(0);	sprintf(str,"\"hwequcabfrontdoorstatus\": \"%d\",\n", status);strJson += str;		 //设备柜前门锁状态
		status=LockerStatusGet(1);	sprintf(str,"\"hwequcabbackdoorstatus\": \"%d\",\n", status);strJson += str;		 //设备柜后门锁状态
		status=LockerStatusGet(2);	sprintf(str,"\"hwbatcabfrontdoorstatus\": \"%d\",\n", status);strJson += str;		 //电池柜前门锁状态
		status=LockerStatusGet(3);	sprintf(str,"\"hwbatcabbackdoorstatus\": \"%d\",\n", status);strJson += str;		 //电池柜后门锁状态
#endif
		sprintf(str,"\"upscityvol\": %s,\n", HUAWEIDevValue.StrUpsCityVol.c_str());strJson += str;		 //Ups市电电压
		sprintf(str,"\"upscityvol2\": %s,\n", HUAWEIDevValue.StrUpsCityVol2.c_str());strJson += str;		 //Ups2市电电压
		sprintf(str,"\"upsovol\": %s,\n", HUAWEIDevValue.StrUpsOVol.c_str());strJson += str;		 //Ups输出电压
		sprintf(str,"\"upsovol2\": %s,\n", HUAWEIDevValue.StrUpsOVol2.c_str());strJson += str;		 //Ups2输出电压
		sprintf(str,"\"upstemp\": %s,\n", HUAWEIDevValue.StrUpsTemp.c_str());strJson += str;		 //Ups温度
		sprintf(str,"\"upstemp2\": %s,\n", HUAWEIDevValue.StrUpsTemp2.c_str());strJson += str;		 //Ups2温度
		sprintf(str,"\"upsloadout\": %s,\n", HUAWEIDevValue.StrUpsLoadout.c_str());strJson += str;		 //Ups负载百分比
		sprintf(str,"\"upsloadout2\": %s,\n", HUAWEIDevValue.StrUpsLoadout2.c_str());strJson += str;		 //Ups2负载百分比
		sprintf(str,"\"upsfreqin\": %s,\n", HUAWEIDevValue.StrUpsFreqin.c_str());strJson += str;		 //Ups输入频率
		sprintf(str,"\"upsfreqin\": %s,\n", HUAWEIDevValue.StrUpsFreqin2.c_str());strJson += str;		 //Ups2输入频率
		sprintf(str,"\"upsisbypass\": %s,\n", HUAWEIDevValue.StrUpsIsBypass.c_str());strJson += str;		 //Ups是否旁路
		sprintf(str,"\"upsisbypass2\": %s,\n", HUAWEIDevValue.StrUpsIsBypass2.c_str());strJson += str;		 //Ups2是否旁路
		sprintf(str,"\"upsonoff\": %s,\n", HUAWEIDevValue.StrUpsOnOff.c_str());strJson += str;		 //Ups开关机状态
		sprintf(str,"\"upsonoff\": %s,\n", HUAWEIDevValue.StrUpsOnOff2.c_str());strJson += str;		 //Ups2开关机状态
	    //空调
		sprintf(str,"\"in_fanstate\": %s,\n", HUAWEIDevValue.StrIn_FanState.c_str());strJson += str;		 //内风机状态 0代表关闭 1代表开启
		sprintf(str,"\"in_fanstate2\": %s,\n", HUAWEIDevValue.StrIn_FanState2.c_str());strJson += str;		 //内风机状态2 0代表关闭 1代表开启
		sprintf(str,"\"out_fanstate\": %s,\n", HUAWEIDevValue.StrOut_FanState.c_str());strJson += str;		 //外风机状态 0代表关闭 1代表开启
		sprintf(str,"\"out_fanstate2\": %s,\n", HUAWEIDevValue.StrOut_FanState2.c_str());strJson += str;		 //外风机状态2 0代表关闭 1代表开启
		//报警
		sprintf(str,"\"ups_alarm\": %s,\n", HUAWEIDevAlarm.Ups_Alarm.c_str());strJson += str;		 //ups故障状态
		sprintf(str,"\"ups_alarm2\": %s,\n", HUAWEIDevAlarm.Ups_Alarm2.c_str());strJson += str;		 //ups2故障状态
		sprintf(str,"\"acbgrouplowvoltalarm\": %s,\n", HUAWEIDevAlarm.AcbgroupLowVoltAlarm.c_str());strJson += str;		 //电池电压低报警
		//空调
		sprintf(str,"\"air_high_temper_alarm\": %s,\n", HUAWEIDevAlarm.Air_High_temper_alarm.c_str());strJson += str;		 //高温告警 0代表正常 1代表告警
		sprintf(str,"\"air_lower_temper_alarm\": %s,\n", HUAWEIDevAlarm.Air_Lower_temper_alarm.c_str());strJson += str;		 //低温告警 0代表正常 1代表告警
		sprintf(str,"\"air_heater_alarm\": %s,\n", HUAWEIDevAlarm.Air_Heater_alarm.c_str());strJson += str;		 //加热器故障告警 0代表正常 1代表告警
		sprintf(str,"\"air_temper_sensor_alarm\": %s,\n", HUAWEIDevAlarm.Air_Temper_Sensor_alarm.c_str());strJson += str;		 //温度传感器故障 0代表正常 1代表告警
	#elif(CABINETTYPE==7)//爱特思
		sprintf(str,"\"isonline\":\"%d\",\n",HUAWEIDevValue.hwLinked); strJson = strJson + str;//连接状态
		sprintf(str,"\"acbgroupbatonline\":\"%d\",\n",HUAWEIDevValue.AcbGroupBatOnline); strJson = strJson + str;//锂电池组是否在线
		sprintf(str,"\"dcaironline\":\"%d\",\n",HUAWEIDevValue.DcAirOnline); strJson = strJson + str;//直流空调是否在线（设备柜）
		sprintf(str,"\"dcaironline2\":\"%d\",\n",HUAWEIDevValue.DcAirOnline2); strJson = strJson + str;//直流空调是否在线（电池柜）
		sprintf(str,"\"temhumonline\":\"%d\",\n",HUAWEIDevValue.TemHumOnline); strJson = strJson + str;//温湿度传感器是否在线（设备柜）
		sprintf(str,"\"temhumonline2\":\"%d\",\n",HUAWEIDevValue.TemHumOnline2); strJson = strJson + str;//温湿度传感器是否在线（电池柜）

		sprintf(str,"\"hwmonequipmanufacturer\": \"爱特思\",\n"); strJson += str;//设备生产商
		strJson = strJson + "\"hwacbgroupbatvolt\": " + HUAWEIDevValue.strhwAcbGroupBatVolt.c_str() + ",\n";	//锂电电池电压 214
		strJson = strJson + "\"hwacbgroupbatcurr\": " + HUAWEIDevValue.strhwAcbGroupBatCurr.c_str() + ",\n";	//锂电电池电流 215
		strJson = strJson + "\"hwacbgrouptotalcapacity\": " + HUAWEIDevValue.strhwAcbGroupTotalCapacity.c_str() + ",\n";	//锂电电池总容量 216
		strJson = strJson + "\"hwacbgrouptotalremaincapacity\": " + HUAWEIDevValue.strhwAcbGroupTotalRemainCapacity.c_str() + ",\n";	//锂电电池剩余容量 217
		strJson = strJson + "\"hwaporablvoltage\": " + HUAWEIDevValue.strhwApOrAblVoltage.c_str() + ",\n";	//A/AB电压 220
		strJson = strJson + "\"hwaphasecurrent\": " + HUAWEIDevValue.strhwAphaseCurrent.c_str() + ",\n";	//A相电流 223
		strJson = strJson + "\"hwenvtemperature\": " + HUAWEIDevValue.strhwEnvTemperature[0].c_str() + ",\n";	//环境温度值 228
		strJson = strJson + "\"hwenvhumidity\": " + HUAWEIDevValue.strhwEnvHumidity[0].c_str() + ",\n"; //环境湿度值 229
		strJson = strJson + "\"hwdooralarmtraps\": " + HUAWEIDevAlarm.hwDoorAlarmTraps.c_str() + ",\n"; //门禁告警(设备柜) 241
		//锂电(新增加)
		strJson = strJson + "\"hwacbgrouptemperature\": " + HUAWEIDevValue.strhwAcbGroupTemperature.c_str() + ",\n";	//电池温度
		//供配电
		//电源告警
		sprintf(str,"\"hwacinputfailalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputFailAlarm.c_str());strJson += str; 	 //交流电源输入停电告警
		sprintf(str,"\"hwacinputl1failalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputL1FailAlarm.c_str());strJson += str; 	 //交流电源输入L1 相缺相告警
		sprintf(str,"\"hwacbgroup_poweroff_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_poweroff_alarm.c_str());strJson += str;		 //电池下电
		sprintf(str,"\"hwsmokesensorstatus\": %s,\n", HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str());strJson += str; 	 //烟雾传感器状态
		sprintf(str,"\"hwwatersensorstatus\": %s,\n", HUAWEIDevAlarm.hwWaterAlarmTraps.c_str());strJson += str; 	 //水浸传感器状态
		sprintf(str,"\"hwacbbatvolt\": %d,\n", HUAWEIDevValue.strhwAcbBatVolt.c_str());strJson += str;		 //单个电池电压
		sprintf(str,"\"hwacbbatcurr\": %s,\n", HUAWEIDevValue.strhwAcbBatCurr.c_str());strJson += str;		 //单个电池电流

		sprintf(str,"\"upscityvol\": %s,\n", HUAWEIDevValue.StrUpsCityVol.c_str());strJson += str;		 //
		sprintf(str,"\"upsovol\": %s,\n", HUAWEIDevValue.StrUpsOVol.c_str());strJson += str;		 //
		sprintf(str,"\"upstemp\": %s,\n", HUAWEIDevValue.StrUpsTemp.c_str());strJson += str;		 //
		strJson = strJson + "\"upsloadout\": \"" + HUAWEIDevValue.StrUpsLoadout.c_str() + "\",\n";	//Ups电池负载 %比
		//空调
		sprintf(str,"\"coolerstate\": %s,\n", HUAWEIDevValue.StrCoolerState.c_str());strJson += str;		 //制冷器状态
		sprintf(str,"\"in_fanstate\": %s,\n", HUAWEIDevValue.StrIn_FanState.c_str());strJson += str;		 //内风机状态 0代表关闭 1代表开启
		sprintf(str,"\"out_fanstate\": %s,\n", HUAWEIDevValue.StrOut_FanState.c_str());strJson += str;		 //外风机状态 0代表关闭 1代表开启
		sprintf(str,"\"heaterstate\": %s,\n", HUAWEIDevValue.StrHeaterState.c_str());strJson += str;		 //加热器状态
		//报警
		sprintf(str,"\"ups_alarm\": %s,\n", HUAWEIDevAlarm.Ups_alarm.c_str());strJson += str;		 //ups故障状态
		//空调
		sprintf(str,"\"air_cooler_alarm\": %s,\n", HUAWEIDevAlarm.Air_Cooler_alarm.c_str());strJson += str;		 //制冷器故障告警
		sprintf(str,"\"air_high_temper_alarm\": %s,\n", HUAWEIDevAlarm.Air_High_temper_alarm.c_str());strJson += str;		 //高温告警 0代表正常 1代表告警
		sprintf(str,"\"air_lower_temper_alarm\": %s,\n", HUAWEIDevAlarm.Air_Lower_temper_alarm.c_str());strJson += str;		 //低温告警 0代表正常 1代表告警
		sprintf(str,"\"air_heater_alarm\": %s,\n", HUAWEIDevAlarm.Air_Heater_alarm.c_str());strJson += str;		 //加热器故障告警 0代表正常 1代表告警
		sprintf(str,"\"air_temper_sensor_alarm\": %s,\n", HUAWEIDevAlarm.Air_Temper_Sensor_alarm.c_str());strJson += str;		 //温度传感器故障 0代表正常 1代表告警
		sprintf(str,"\"air_high_vol_alarm\": %s,\n", HUAWEIDevAlarm.Air_High_Vol_alarm.c_str());strJson += str;		 //电压高压告警 0代表正常 1代表告警
		sprintf(str,"\"air_lower_vol_alarm\": %s,\n", HUAWEIDevAlarm.Air_Lower_Vol_alarm.c_str());strJson += str;		 //电压低压告警 0代表正常 1代表告警
	#elif(CABINETTYPE==8 || CABINETTYPE==10)//诺龙/亚邦
		sprintf(str,"\"isonline\":\"%d\",\n",HUAWEIDevValue.hwLinked); strJson = strJson + str;//连接状态
		sprintf(str,"\"acbgroupbatonline\":\"%d\",\n",HUAWEIDevValue.AcbGroupBatOnline); strJson = strJson + str;//锂电池组是否在线
		sprintf(str,"\"dcaironline\":\"%d\",\n",HUAWEIDevValue.DcAirOnline); strJson = strJson + str;//直流空调是否在线（设备柜）
		sprintf(str,"\"dcaironline2\":\"%d\",\n",HUAWEIDevValue.DcAirOnline2); strJson = strJson + str;//直流空调是否在线（电池柜）
		sprintf(str,"\"temhumonline\":\"%d\",\n",HUAWEIDevValue.TemHumOnline); strJson = strJson + str;//温湿度传感器是否在线（设备柜）
		sprintf(str,"\"temhumonline2\":\"%d\",\n",HUAWEIDevValue.TemHumOnline2); strJson = strJson + str;//温湿度传感器是否在线（电池柜）

		if(StrCabinetType=="8")
		{
			sprintf(str,"\"hwmonequipmanufacturer\": \"诺龙\",\n");	strJson += str;//设备生产商
		}
		else if(StrCabinetType=="10")
		{
			sprintf(str,"\"hwmonequipmanufacturer\": \"亚邦\",\n");	strJson += str;//设备生产商
		}
		strJson = strJson + "\"hwacbgroupbatvolt\": " + HUAWEIDevValue.strhwAcbGroupBatVolt.c_str() + ",\n";	//锂电电池电压 214
		strJson = strJson + "\"hwacbgrouptotalcapacity\": " + HUAWEIDevValue.strhwAcbGroupTotalCapacity.c_str() + ",\n";	//锂电电池总容量 216
		strJson = strJson + "\"hwaporablvoltage\": " + HUAWEIDevValue.strhwApOrAblVoltage.c_str() + ",\n";	//A/AB电压 220
		strJson = strJson + "\"hwbporbclvoltage\": " + HUAWEIDevValue.strhwBpOrBclVoltage.c_str() + ",\n";	//B/BC电压 221
		strJson = strJson + "\"hwaphasecurrent\": " + HUAWEIDevValue.strhwAphaseCurrent.c_str() + ",\n";	//A相电流 223
		strJson = strJson + "\"hwbphasecurrent\": " + HUAWEIDevValue.strhwBphaseCurrent.c_str() + ",\n";	//B相电流 224
		strJson = strJson + "\"hwenvtemperature\": " + HUAWEIDevValue.strhwEnvTemperature[0].c_str() + ",\n";	//环境温度值 228
		strJson = strJson + "\"hwenvtemperature2\": " + HUAWEIDevValue.strhwEnvTemperature[1].c_str() + ",\n";	//环境温度值 228
		strJson = strJson + "\"hwenvhumidity\": " + HUAWEIDevValue.strhwEnvHumidity[0].c_str() + ",\n"; //环境湿度值 229
		strJson = strJson + "\"hwenvhumidity2\": " + HUAWEIDevValue.strhwEnvHumidity[1].c_str() + ",\n";	//环境湿度值 229
		strJson = strJson + "\"hwdcairrunstatus\": " + HUAWEIDevValue.strhwDcAirRunStatus[0].c_str() + ",\n";	//空调运行状态 231
		strJson = strJson + "\"hwdcairrunstatus2\": " + HUAWEIDevValue.strhwDcAirRunStatus[1].c_str() + ",\n";	//空调运行状态 231
		strJson = strJson + "\"hwdcaircompressorrunstatus\": " + HUAWEIDevValue.strhwDcAirCompressorRunStatus[0].c_str() + ",\n";	//空调压缩机运行状态 232
		strJson = strJson + "\"hwdcaircompressorrunstatus2\": " + HUAWEIDevValue.strhwDcAirCompressorRunStatus[1].c_str() + ",\n";	//空调压缩机运行状态 232
		strJson = strJson + "\"hwdcairenterchanneltemp\": " + HUAWEIDevValue.strhwDcAirEnterChannelTemp[0].c_str() + ",\n"; //空调回风口温度 236
		strJson = strJson + "\"hwdcairenterchanneltemp2\": " + HUAWEIDevValue.strhwDcAirEnterChannelTemp[1].c_str() + ",\n";	//空调回风口温度 236
		strJson = strJson + "\"hwenvtempalarmtraps\": " + HUAWEIDevAlarm.hwEnvTempAlarmTraps.c_str() + ",\n";	//设备柜环境温度告警 239
		strJson = strJson + "\"hwenvtempalarmtraps2\": " + HUAWEIDevAlarm.hwEnvTempAlarmTraps2.c_str() + ",\n"; //电池柜环境温度告警 239
		strJson = strJson + "\"hwdooralarmtraps\": " + HUAWEIDevAlarm.hwDoorAlarmTraps.c_str() + ",\n"; //门禁告警(设备柜) 241
		strJson = strJson + "\"hwdooralarmtraps2\": " + HUAWEIDevAlarm.hwDoorAlarmTraps2.c_str() + ",\n";	//门禁告警（电池柜） 241
		strJson = strJson + "\"hwwateralarmtraps\": " + HUAWEIDevAlarm.hwWaterAlarmTraps.c_str() + ",\n";	//水浸告警 242
		strJson = strJson + "\"hwwateralarmtraps2\": " + HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str() + ",\n"; //水浸告警 242
		strJson = strJson + "\"hwsmokealarmtraps\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str() + ",\n";	//烟雾告警 243
		strJson = strJson + "\"hwsmokealarmtraps2\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str() + ",\n"; //烟雾告警 243
		//锂电(新增加)
		strJson = strJson + "\"hwacbgrouptemperature\": " + HUAWEIDevValue.strhwAcbGroupTemperature.c_str() + ",\n";	//电池温度
		//供配电
		//电源告警
		sprintf(str,"\"hwacinputfailalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputFailAlarm.c_str());strJson += str; 	 //交流电源输入停电告警
		sprintf(str,"\"hwacinputl1failalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputL1FailAlarm.c_str());strJson += str; 	 //交流电源输入L1 相缺相告警
		sprintf(str,"\"hwacinputl2failalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputL2FailAlarm.c_str());strJson += str; 	 //交流电源输入L2 相缺相告警
		//锂电池告警
		sprintf(str,"\"hwacbgroup_temphigh_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_temphigh_alarm.c_str());strJson += str;		 //电池温度高
		sprintf(str,"\"hwacbgroup_temphigh_alarm2\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_temphigh_alarm2.c_str());strJson += str;		 //电池温度高
		sprintf(str,"\"hwacbgroup_templow_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_templow_alarm.c_str());strJson += str; 	 //电池温度低
		sprintf(str,"\"hwacbgroup_templow_alarm2\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_templow_alarm2.c_str());strJson += str; 	 //电池温度低
		sprintf(str,"\"hwacbbatvolt\": %d,\n", HUAWEIDevValue.strhwAcbBatVolt.c_str());strJson += str;		 //单个电池电压
#if 0
		status=LockerStatusGet(0);	sprintf(str,"\"hwequcabfrontdoorstatus\": \"%d\",\n", status);strJson += str;		 //设备柜前门锁状态
		status=LockerStatusGet(2);	sprintf(str,"\"hwbatcabfrontdoorstatus\": \"%d\",\n", status);strJson += str;		 //电池柜前门锁状态
#endif
		sprintf(str,"\"upscityvol\": %s,\n", HUAWEIDevValue.StrUpsCityVol.c_str());strJson += str;		 //Ups市电电压(设备柜)
		sprintf(str,"\"upscityvol2\": %s,\n", HUAWEIDevValue.StrUpsCityVol2.c_str());strJson += str;		 //Ups市电电压(电池柜)
		sprintf(str,"\"upsovol\": %s,\n", HUAWEIDevValue.StrUpsOVol.c_str());strJson += str;		 //Ups输出电压(设备柜)
		sprintf(str,"\"upsovol2\": %s,\n", HUAWEIDevValue.StrUpsOVol2.c_str());strJson += str;		 //Ups输出电压(电池柜)
	
	    //空调
		sprintf(str,"\"coolervol\": %s,\n", HUAWEIDevValue.StrCoolerVol.c_str());strJson += str;		 //空调电压(设备柜)
		sprintf(str,"\"coolervol2\": %s,\n", HUAWEIDevValue.StrCoolerVol2.c_str());strJson += str;		 //空调电压(电池柜)
		sprintf(str,"\"coolercur\": %s,\n", HUAWEIDevValue.StrCoolerCur.c_str());strJson += str;		 //空调电流(设备柜)
		sprintf(str,"\"coolercur2\": %s,\n", HUAWEIDevValue.StrCoolerCur2.c_str());strJson += str;		 //空调电流(电池柜)
		sprintf(str,"\"in_fanstate\": %s,\n", HUAWEIDevValue.StrIn_FanState.c_str());strJson += str;		 //内风机状态(设备柜) 0代表关闭 1代表开启
		sprintf(str,"\"in_fanstate2\": %s,\n", HUAWEIDevValue.StrIn_FanState2.c_str());strJson += str;		 //内风机状态(电池柜) 0代表关闭 1代表开启
		sprintf(str,"\"out_fanstate\": %s,\n", HUAWEIDevValue.StrOut_FanState.c_str());strJson += str;		 //外风机状态(设备柜) 0代表关闭 1代表开启
		sprintf(str,"\"out_fanstate2\": %s,\n", HUAWEIDevValue.StrOut_FanState2.c_str());strJson += str;		 //外风机状态(电池柜) 0代表关闭 1代表开启

		//告警
		sprintf(str,"\"ups_alarm\": %s,\n", HUAWEIDevAlarm.Ups_alarm.c_str());strJson += str;		 //ups故障状态（设备柜）
		sprintf(str,"\"ups_alarm2\": %s,\n", HUAWEIDevAlarm.Ups_alarm2.c_str());strJson += str;		 //ups故障状态（电池柜）
		sprintf(str,"\"ups_city_off_alarm\": %s,\n", HUAWEIDevAlarm.Ups_city_off_alarm.c_str());strJson += str;		 //ups市电断开（设备柜）
		sprintf(str,"\"ups_city_off_alarm2\": %s,\n", HUAWEIDevAlarm.Ups_city_off_alarm2.c_str());strJson += str;		 //ups市电断开（电池柜）
		//空调
		sprintf(str,"\"air_high_temper_alarm\": %s,\n", HUAWEIDevAlarm.Air_High_temper_alarm.c_str());strJson += str;		 //高温告警（设备柜）
		sprintf(str,"\"air_high_temper_alarm2\": %s,\n", HUAWEIDevAlarm.Air_High_temper_alarm2.c_str());strJson += str;		 //高温告警（电池柜）
	#elif(CABINETTYPE==9)//容尊
		sprintf(str,"\"isonline\":\"%d\",\n",HUAWEIDevValue.hwLinked); strJson = strJson + str;//连接状态
		sprintf(str,"\"acbgroupbatonline\":\"%d\",\n",HUAWEIDevValue.AcbGroupBatOnline); strJson = strJson + str;//锂电池组是否在线
		sprintf(str,"\"dcaironline\":\"%d\",\n",HUAWEIDevValue.DcAirOnline); strJson = strJson + str;//直流空调是否在线（设备柜）
		sprintf(str,"\"dcaironline2\":\"%d\",\n",HUAWEIDevValue.DcAirOnline2); strJson = strJson + str;//直流空调是否在线（电池柜）
		sprintf(str,"\"temhumonline\":\"%d\",\n",HUAWEIDevValue.TemHumOnline); strJson = strJson + str;//温湿度传感器是否在线（设备柜）
		sprintf(str,"\"temhumonline2\":\"%d\",\n",HUAWEIDevValue.TemHumOnline2); strJson = strJson + str;//温湿度传感器是否在线（电池柜）

		sprintf(str,"\"hwmonequipmanufacturer\": \"容尊\",\n"); strJson += str;//设备生产商
		strJson = strJson + "\"hwacbgroupbatvolt\": " + HUAWEIDevValue.strhwAcbGroupBatVolt.c_str() + ",\n";	//锂电电池电压 214
		strJson = strJson + "\"hwacbgroupbatcurr\": " + HUAWEIDevValue.strhwAcbGroupBatCurr.c_str() + ",\n";	//锂电电池电流 215
		strJson = strJson + "\"hwacbgrouptotalcapacity\": " + HUAWEIDevValue.strhwAcbGroupTotalCapacity.c_str() + ",\n";	//锂电电池总容量 216
		strJson = strJson + "\"hwacbgrouptotalremaincapacity\": " + HUAWEIDevValue.strhwAcbGroupTotalRemainCapacity.c_str() + ",\n";	//锂电电池剩余容量 217
		strJson = strJson + "\"hwaporablvoltage\": " + HUAWEIDevValue.strhwApOrAblVoltage.c_str() + ",\n";	//A/AB电压 220
		strJson = strJson + "\"hwaphasecurrent\": " + HUAWEIDevValue.strhwAphaseCurrent.c_str() + ",\n";	//A相电流 223
		strJson = strJson + "\"hwdcoutputvoltage\": " + HUAWEIDevValue.strhwDcOutputVoltage.c_str() + ",\n";	//DC输出电压 226
		strJson = strJson + "\"hwdcoutputcurrent\": " + HUAWEIDevValue.strhwDcOutputCurrent.c_str() + ",\n";	//DC输出电流 227
		strJson = strJson + "\"hwenvtemperature\": " + HUAWEIDevValue.strhwEnvTemperature[0].c_str() + ",\n";	//环境温度值 228
		strJson = strJson + "\"hwenvhumidity\": " + HUAWEIDevValue.strhwEnvHumidity[0].c_str() + ",\n"; //环境湿度值 229
		strJson = strJson + "\"hwdcairrunstatus\": " + HUAWEIDevValue.strhwDcAirRunStatus[0].c_str() + ",\n";	//空调运行状态 231
		strJson = strJson + "\"hwdcairrunstatus2\": " + HUAWEIDevValue.strhwDcAirRunStatus[1].c_str() + ",\n";	//空调运行状态 231
		strJson = strJson + "\"hwenvtempalarmtraps\": " + HUAWEIDevAlarm.hwEnvTempAlarmTraps.c_str() + ",\n";	//设备柜环境温度告警 239
		strJson = strJson + "\"hwenvhumialarmresumetraps\": " + HUAWEIDevAlarm.hwEnvHumiAlarmTraps.c_str() + ",\n"; //设备柜环境湿度告警 240
		strJson = strJson + "\"hwdooralarmtraps\": " + HUAWEIDevAlarm.hwDoorAlarmTraps.c_str() + ",\n"; //门禁告警(设备柜) 241
		strJson = strJson + "\"hwdooralarmtraps2\": " + HUAWEIDevAlarm.hwDoorAlarmTraps2.c_str() + ",\n";	//门禁告警（电池柜） 241
		strJson = strJson + "\"hwwateralarmtraps\": " + HUAWEIDevAlarm.hwWaterAlarmTraps.c_str() + ",\n";	//水浸告警 242
		strJson = strJson + "\"hwsmokealarmtraps\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str() + ",\n";	//烟雾告警 243
		strJson = strJson + "\"hwair_cond_infan_alarm\": " + HUAWEIDevAlarm.hwair_cond_infan_alarm.c_str() + ",\n"; //空调内风机故障 245
		strJson = strJson + "\"hwair_cond_infan_alarm2\": " + HUAWEIDevAlarm.hwair_cond_infan_alarm2.c_str() + ",\n";	//空调内风机故障 245
		strJson = strJson + "\"hwair_cond_outfan_alarm\": " + HUAWEIDevAlarm.hwair_cond_outfan_alarm.c_str() + ",\n";	//空调外风机故障 246
		strJson = strJson + "\"hwair_cond_outfan_alarm2\": " + HUAWEIDevAlarm.hwair_cond_outfan_alarm2.c_str() + ",\n"; //空调外风机故障 246
		strJson = strJson + "\"hwair_cond_return_port_sensor_alarm\": " + HUAWEIDevAlarm.hwair_cond_return_port_sensor_alarm.c_str() + ",\n";	//空调回风口传感器故障 248
		strJson = strJson + "\"hwair_cond_return_port_sensor_alarm2\": " + HUAWEIDevAlarm.hwair_cond_return_port_sensor_alarm2.c_str() + ",\n"; //空调回风口传感器故障 248
		strJson = strJson + "\"hwacbgrouptemperature\": " + HUAWEIDevValue.strhwAcbGroupTemperature.c_str() + ",\n";	//电池温度
		sprintf(str,"\"hwacinputfailalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputFailAlarm.c_str());strJson += str; 	 //交流电源输入停电告警
		sprintf(str,"\"hwacinputl1failalarm\": %s,\n", HUAWEIDevAlarm.hwAcInputL1FailAlarm.c_str());strJson += str; 	 //交流电源输入L1 相缺相告警
		sprintf(str,"\"hwacbgroup_discharge_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_discharge_alarm.c_str());strJson += str; 	 //电池放电告警
		sprintf(str,"\"hwacbgroup_charge_overcurrent_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_charge_overcurrent_alarm.c_str());strJson += str;		 //电池充电过流
		sprintf(str,"\"hwacbgroup_temphigh_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_temphigh_alarm.c_str());strJson += str;		 //电池温度高
		sprintf(str,"\"hwacbgroup_templow_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_templow_alarm.c_str());strJson += str; 	 //电池温度低
		sprintf(str,"\"hwacbgroup_poweroff_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_poweroff_alarm.c_str());strJson += str;		 //电池下电
		sprintf(str,"\"hwacbgroup_fusebreak_alarm\": %s,\n", HUAWEIDevAlarm.hwAcbGroup_fusebreak_alarm.c_str());strJson += str; 	 //电池熔丝断
		sprintf(str,"\"hwacbgroupbatrunningstate\": %s,\n", HUAWEIDevValue.strhwAcbGroupBatRunningState.c_str());strJson += str;		 //电池状态
		sprintf(str,"\"hwsmokesensorstatus\": %s,\n", HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str());strJson += str; 	 //烟雾传感器状态
		sprintf(str,"\"hwwatersensorstatus\": %s,\n", HUAWEIDevAlarm.hwWaterAlarmTraps.c_str());strJson += str; 	 //水浸传感器状态
		sprintf(str,"\"hwdoorsensorstatus\": %s,\n", HUAWEIDevAlarm.hwDoorAlarmTraps.c_str());strJson += str;		 //（电池柜）门磁传感器状态

		strJson = strJson + "\"upscityvol\": \"" + HUAWEIDevValue.StrUpsCityVol.c_str() + "\",\n";	//Ups市电电压
		strJson = strJson + "\"upscityfreq\": \"" + HUAWEIDevValue.StrUpsCityFreq.c_str() + "\",\n";	//ups输入频率
		strJson = strJson + "\"upsovol\": \"" + HUAWEIDevValue.StrUpsOVol.c_str() + "\",\n";	//Ups输出电压
		strJson = strJson + "\"upsoamp\": \"" + HUAWEIDevValue.StrUpsOAmp.c_str() + "\",\n";	//Ups输出电流
		strJson = strJson + "\"upstemp\": \"" + HUAWEIDevValue.StrUpsTemp.c_str() + "\",\n";	//Ups温度
		strJson = strJson + "\"upsbatload\": \"" + HUAWEIDevValue.StrUpsBatLoad.c_str() + "\",\n";	//Ups电池负载 %比
		strJson = strJson + "\"upsbypass\": \"" + HUAWEIDevValue.UpsByPass.c_str() + "\",\n";	//Ups旁路模式
		strJson = strJson + "\"upsworkmode\": \"" + HUAWEIDevValue.StrUpsWorkMode.c_str() + "\",\n";	//Ups工作模式	  0:在线式，1:后备式
		strJson = strJson + "\"upsbatlowvol_alarm\": \"" + HUAWEIDevAlarm.UpsBatLowVol_alarm.c_str() + "\",\n";	//ups电池电压低
		strJson = strJson + "\"ups_alarm\": \"" + HUAWEIDevAlarm.Ups_alarm.c_str() + "\",\n";	// Ups故障
		strJson = strJson + "\"ups_on_off_alarm\": \"" + HUAWEIDevAlarm.Ups_On_off_alarm.c_str() + "\",\n";	//1:关机 0:开机
		strJson = strJson + "\"in_fanstate\": \"" + HUAWEIDevValue.StrIn_FanState.c_str() + "\",\n";	//设备柜内风机状态 0代表关闭 1代表开启
		strJson = strJson + "\"in_fanstate2\": \"" + HUAWEIDevValue.StrIn_FanState2.c_str() + "\",\n";	//内风机状态 0代表关闭 1代表开启
		strJson = strJson + "\"out_fanstate\": \"" + HUAWEIDevValue.StrOut_FanState.c_str() + "\",\n";	//电源柜外风机状态 0代表关闭 1代表开启
		strJson = strJson + "\"out_fanstate2\": \"" + HUAWEIDevValue.StrOut_FanState2.c_str() + "\",\n";	//外风机状态 0代表关闭 1代表开启
		strJson = strJson + "\"dcairtec_alarm\": \"" + HUAWEIDevAlarm.DcAirTEC_alarm.c_str() + "\",\n";	//设备柜TEC运行故障
		strJson = strJson + "\"dcairtec_alarm2\": \"" + HUAWEIDevAlarm.DcAirTEC_alarm2.c_str() + "\",\n";	//电源柜TEC运行故障
		strJson = strJson + "\"hwbackdooralarmtraps\": \"" + HUAWEIDevAlarm.hwBackDoorAlarmTraps.c_str() + "\",\n";	//后门禁告警(设备柜)
		strJson = strJson + "\"hwbackdooralarmtraps2\": \"" + HUAWEIDevAlarm.hwBackDoorAlarmTraps2.c_str() + "\",\n";	//后门禁告警(电池柜)
		strJson = strJson + "\"dc_power_alarm\": \"" + HUAWEIDevAlarm.DC_Power_alarm.c_str() + "\",\n";	//开关电源主报警
		strJson = strJson + "\"bat_alarm\": \"" + HUAWEIDevAlarm.Bat_alarm.c_str() + "\",\n";	//电池状态  0:正常，1：告警
	#elif(CABINETTYPE==11)//艾特网能
		sprintf(str,"\"isonline\":\"%d\",\n",HUAWEIDevValue.hwLinked); strJson = strJson + str;//连接状态
		if(HUAWEIDevValue.strBatteryStatus!="2147483647") HUAWEIDevValue.AcbGroupBatOnline=true;
		sprintf(str,"\"acbgroupbatonline\":\"%d\",\n",HUAWEIDevValue.AcbGroupBatOnline); strJson = strJson + str;//锂电池组是否在线
		if(HUAWEIDevValue.strCoolSta[0]!="2147483647") HUAWEIDevValue.DcAirOnline=true;
		sprintf(str,"\"dcaironline\":\"%d\",\n",HUAWEIDevValue.DcAirOnline); strJson = strJson + str;//直流空调是否在线（设备柜）
		if(HUAWEIDevValue.strCoolSta[1]!="2147483647") HUAWEIDevValue.DcAirOnline2=true;
		sprintf(str,"\"dcaironline2\":\"%d\",\n",HUAWEIDevValue.DcAirOnline2); strJson = strJson + str;//直流空调是否在线（电池柜）
		if(HUAWEIDevValue.strhwEnvTemperature[EQUIPMENT_CABINET]!="2147483647") HUAWEIDevValue.TemHumOnline=true;
		sprintf(str,"\"temhumonline\":\"%d\",\n",HUAWEIDevValue.TemHumOnline); strJson = strJson + str;//温湿度传感器是否在线（设备柜）
		if(HUAWEIDevValue.strhwEnvTemperature[BATTERY_CABINET]!="2147483647") HUAWEIDevValue.TemHumOnline2=true;
		sprintf(str,"\"temhumonline2\":\"%d\",\n",HUAWEIDevValue.TemHumOnline2); strJson = strJson + str;//温湿度传感器是否在线（电池柜）

		sprintf(str,"\"hwmonequipmanufacturer\": \"艾特网能\",\n"); strJson += str;//设备生产商
	    strJson = strJson + "\"hwacbgroupbatvolt\": " + HUAWEIDevValue.strPositiveBatPackVol.c_str() + ",\n";	//锂电电池电压 214
	    strJson = strJson + "\"hwacbgrouptotalremaincapacity\": " + HUAWEIDevValue.strBatteryCapacity.c_str() + ",\n";	//锂电电池剩余容量 217
	    strJson = strJson + "\"hwacbgroupbackuptime\": " + HUAWEIDevValue.strBatteryRemaindingTime.c_str() + ",\n";	//电池备电时长 218
	    strJson = strJson + "\"hwaporablvoltage\": " + HUAWEIDevValue.strVoltage[EQUIPMENT_CABINET].c_str() + ",\n";	//A/AB电压 220
	    strJson = strJson + "\"hwbporbclvoltage\": " + HUAWEIDevValue.strVoltage[BATTERY_CABINET].c_str() + ",\n";	//B/BC电压 221
	    strJson = strJson + "\"hwaphasecurrent\": " + HUAWEIDevValue.strCurrent[EQUIPMENT_CABINET].c_str() + ",\n";	//A相电流 223
	    strJson = strJson + "\"hwbphasecurrent\": " + HUAWEIDevValue.strCurrent[BATTERY_CABINET].c_str() + ",\n";	//B相电流 224
	    strJson = strJson + "\"hwenvtemperature\": " + HUAWEIDevValue.strhwEnvTemperature[EQUIPMENT_CABINET].c_str() + ",\n";	//环境温度值 228
	    strJson = strJson + "\"hwenvtemperature2\": " + HUAWEIDevValue.strhwEnvTemperature[BATTERY_CABINET].c_str() + ",\n";	//环境温度值 228
	    strJson = strJson + "\"hwenvhumidity\": " + HUAWEIDevValue.strhwEnvHumidity[EQUIPMENT_CABINET].c_str() + ",\n";	//环境湿度值 229
	    strJson = strJson + "\"hwenvhumidity2\": " + HUAWEIDevValue.strhwEnvHumidity[BATTERY_CABINET].c_str() + ",\n";	//环境湿度值 229
		if(HUAWEIDevValue.strInternalFanSta[0]!="0") HUAWEIDevValue.strhwDcAirRunStatus[0]="1";
		else if(HUAWEIDevValue.strInternalFanSta[0]!="1") HUAWEIDevValue.strhwDcAirRunStatus[0]="2";
		strJson = strJson + "\"hwdcairrunstatus\": " + HUAWEIDevValue.strhwDcAirRunStatus[0].c_str() + ",\n";	//空调运行状态 231
		if(HUAWEIDevValue.strInternalFanSta[1]!="0") HUAWEIDevValue.strhwDcAirRunStatus[1]="1";
		else if(HUAWEIDevValue.strInternalFanSta[1]!="1") HUAWEIDevValue.strhwDcAirRunStatus[1]="2";
	    strJson = strJson + "\"hwdcairrunstatus2\": " + HUAWEIDevValue.strhwDcAirRunStatus[1].c_str() + ",\n";	//空调运行状态 231
		if(HUAWEIDevValue.strCoolSta[0]!="0") HUAWEIDevValue.strhwDcAirCompressorRunStatus[0]="1";
		else if(HUAWEIDevValue.strCoolSta[0]!="1") HUAWEIDevValue.strhwDcAirCompressorRunStatus[0]="2";
	    strJson = strJson + "\"hwdcaircompressorrunstatus\": " + HUAWEIDevValue.strhwDcAirCompressorRunStatus[0].c_str() + ",\n";	//空调压缩机运行状态 232
		if(HUAWEIDevValue.strCoolSta[1]!="0") HUAWEIDevValue.strhwDcAirCompressorRunStatus[1]="1";
		else if(HUAWEIDevValue.strCoolSta[1]!="1") HUAWEIDevValue.strhwDcAirCompressorRunStatus[1]="2";
	    strJson = strJson + "\"hwdcaircompressorrunstatus2\": " + HUAWEIDevValue.strhwDcAirCompressorRunStatus[1].c_str() + ",\n";	//空调压缩机运行状态 232
	    strJson = strJson + "\"hwenvtempalarmtraps\": " + HUAWEIDevAlarm.strHighTemperatureWarn[0].c_str() + ",\n";	//高温/低温/高高温告警
	    strJson = strJson + "\"hwenvtempalarmtraps2\": " + HUAWEIDevAlarm.strHighTemperatureWarn[1].c_str() + ",\n";	//高温/低温/高高温告警
	    strJson = strJson + "\"hwenvhumialarmresumetraps\": " + HUAWEIDevAlarm.strHighHumidityWarn[0].c_str() + ",\n";	//高湿/低湿告警
	    strJson = strJson + "\"hwenvhumialarmresumetraps2\": " + HUAWEIDevAlarm.strHighHumidityWarn[1].c_str() + ",\n";	//高湿/低湿告警
	    strJson = strJson + "\"hwwateralarmtraps\": " + HUAWEIDevAlarm.hwWaterAlarmTraps.c_str() + ",\n";	//水浸告警 242
	    strJson = strJson + "\"hwwateralarmtraps2\": " + HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str() + ",\n";	//水浸告警 242
	    strJson = strJson + "\"hwsmokealarmtraps\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str() + ",\n";	//烟雾告警 243
	    strJson = strJson + "\"hwsmokealarmtraps2\": " + HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str() + ",\n";	//烟雾告警 243
		//防雷器告警
	    strJson = strJson + "\"hwacspdalarmtraps\": " + HUAWEIDevAlarm.strSpdAlarmTraps[EQUIPMENT_CABINET][0].c_str() + ",\n";	//交流防雷器故障
	    strJson = strJson + "\"hwdcspdalarmtraps\": " + HUAWEIDevAlarm.strSpdAlarmTraps[BATTERY_CABINET][0].c_str() + ",\n";	//直流防雷器故障
		//新增加的状态
		//锂电(新增加)
	    strJson = strJson + "\"hwacbgrouptemperature\": " + HUAWEIDevValue.strhwAcbGroupTemperature.c_str() + ",\n";	//电池温度
		//供配电
		//电源告警
		sprintf(str,"\"hwacinputfailalarm\": %s,\n", HUAWEIDevAlarm.strACInputFailure.c_str());strJson += str;		 //交流电源输入停电告警
		sprintf(str,"\"hwacbgroup_poweroff_alarm\": %s,\n", HUAWEIDevAlarm.strBatteryLowVol.c_str());strJson += str;		 //电池下电
		//2019-08-20新增
		sprintf(str,"\"hwacbgroupbatrunningstate\": %s,\n", HUAWEIDevValue.strBatteryStatus.c_str());strJson += str;		 //电池状态
		sprintf(str,"\"hwsmokesensorstatus\": %s,\n", HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str());strJson += str;		 //烟雾传感器状态
		sprintf(str,"\"hwsmokesensorstatus2\": %s,\n", HUAWEIDevAlarm.hwSmokeAlarmTraps2.c_str());strJson += str;		 //烟雾传感器状态
		sprintf(str,"\"hwwatersensorstatus\": %s,\n", HUAWEIDevAlarm.hwWaterAlarmTraps.c_str());strJson += str;		 //水浸传感器状态
		sprintf(str,"\"hwwatersensorstatus2\": %s,\n", HUAWEIDevAlarm.hwWaterAlarmTraps2.c_str());strJson += str;		 //水浸传感器状态
		//2019-11-19新增4个门锁状态
		sprintf(str,"\"hwequcabfrontdoorstatus\": %s,\n", HUAWEIDevAlarm.strDoorAlarmTraps[EQUIPMENT_CABINET][0].c_str());strJson += str;		 //设备柜前门锁状态
		sprintf(str,"\"hwequcabbackdoorstatus\": %s,\n", HUAWEIDevAlarm.strDoorAlarmTraps[EQUIPMENT_CABINET][1].c_str());strJson += str;		 //设备柜后门锁状态
		sprintf(str,"\"hwbatcabfrontdoorstatus\": %s,\n", HUAWEIDevAlarm.strDoorAlarmTraps[BATTERY_CABINET][0].c_str());strJson += str;		 //电池柜前门锁状态
		sprintf(str,"\"hwbatcabbackdoorstatus\": %s,\n", HUAWEIDevAlarm.strDoorAlarmTraps[BATTERY_CABINET][1].c_str());strJson += str;		 //电池柜后门锁状态
#endif
	}
	else
	{
		char msg[256];
		sprintf(msg,"HWCabinet now=0x%x last=0x%x \r\n",GetTickCount(),pCab->HUAWEIDevValue.hwTimeStamp);
		WriteLog(msg);
		initHUAWEIGantry(pCab);
		initHUAWEIALARM(pCab);
		sprintf(str,"\"isonline\":\"%d\",\n",pCab->HUAWEIDevValue.hwLinked); strJson = strJson + str;//连接状态
	}
    pthread_mutex_unlock(&pCab->CabinetdataMutex);
	
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\"\n";	//时间
    strJson +=  "}\n\0";
	
	mstrjson=strJson;
}


void SetjsonIPSwitchStatusStr(int messagetype,string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j,ipswitchcnt; 
	static int recordno=0;
	CswitchClient *pCIPS=NULL;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;

    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	strJson = strJson + "\"ipswitchcnt\": \"" + pConf->StrIPSwitchCount + "\",\n";	//交换机数量
	strJson = strJson + "\"ipswitchlist\": [\n";	//交换机列表
	ipswitchcnt=atoi(pConf->StrIPSwitchCount.c_str());
	for(i=0;i<ipswitchcnt;i++)
	{
		pCIPS = pCswitchClient[i];
		if(pCIPS!=NULL)
		{
			strJson +=	"{\n";
			sprintf(str,"\"name\":\"ipswitch%d\",\n",i+1); strJson = strJson + str;//名称
			if(pConf->StrIPSwitchType == "1" || pConf->StrIPSwitchType == "")
				{sprintf(str,"\"factoryname\":\"华为\",\n"); strJson = strJson + str;}//生产商
			else if(pConf->StrIPSwitchType == "2" && i==0 && pConf->StrCabinetType=="9")	//广深沿江第一个是新华三，第二个是华为
				{sprintf(str,"\"factoryname\":\"新华三\",\n"); strJson = strJson + str;}//生产商
			else if(pConf->StrIPSwitchType == "2" && i==1 && pConf->StrCabinetType=="9")	//广深沿江第一个是新华三，第二个是华为
				{sprintf(str,"\"factoryname\":\"华为\",\n"); strJson = strJson + str;}//生产商
				
			if(pCIPS->HUAWEIDevValue.hwswitchEntityLinked && GetTickCount()-pCIPS->HUAWEIDevValue.hwswitchEntityTimeStamp>10*60) //10分钟没有更新数据，恢复默认
			{	
				initHUAWEIswitchEntity(pCIPS);
				pCIPS->HUAWEIDevValue.hwswitchEntityLinked=false;
				sprintf(str,"\"isonline\":\"%d\"\n",pCIPS->HUAWEIDevValue.hwswitchEntityLinked); strJson = strJson + str;//连接状态
			}
			else
			{
				sprintf(str,"\"devicemodel\":\"%s\",\n",pCIPS->HUAWEIDevValue.strhwswitchEntityDevModel.c_str()); strJson = strJson + str;//设备型号
				sprintf(str,"\"isonline\":\"%d\",\n",pCIPS->HUAWEIDevValue.hwswitchEntityLinked); strJson = strJson + str;//连接状态
				sprintf(str,"\"cpuusage\":\"%s\",\n",pCIPS->HUAWEIDevValue.strhwswitchEntityCpuUsage.c_str()); strJson = strJson + str;//CPU使用率
				sprintf(str,"\"memusage\":\"%s\",\n",pCIPS->HUAWEIDevValue.strhwswitchEntityMemUsage.c_str()); strJson = strJson + str;//内存使用率
				sprintf(str,"\"temperature\":\"%s\",\n",pCIPS->HUAWEIDevValue.strhwswitchEntityTemperature.c_str()); strJson = strJson + str;//温度
				strJson = strJson + pCIPS->strswitchjson;								//交换机网络数据
			}
			strJson +=	"}\n";
			if(i!=ipswitchcnt-1)
				strJson = strJson + ",";	
		}
		else
		{
			strJson +=	"{\n";
			sprintf(str,"\"isonline\":\"0\"\n"); strJson = strJson + str;//连接状态
			strJson +=	"}\n";
			if(i!=ipswitchcnt-1)
				strJson = strJson + ",";	
		}
	}
    strJson +=  "]\n";
	strJson +=	"}\n";

	mstrjson = strJson;
	//*len=strJson.length();
	//memcpy(json,(char*)strJson.c_str(),*len);
	
}


void SetjsonFireWallStatusStr(int messagetype,string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j,firewarecnt; 
	static int recordno=0;
	CfirewallClient *pCfw=NULL;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;

    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	strJson = strJson + "\"firewarecnt\": \"" + pConf->StrFireWareCount + "\",\n";	//防火墙数量
	strJson = strJson + "\"firewarelist\": [\n";	//防火墙列表
	firewarecnt=atoi(pConf->StrFireWareCount.c_str());
	for(i=0;i<firewarecnt;i++)
	{
		pCfw = pCfirewallClient[i];
		if(pCfw!=NULL)
		{
			strJson +=	"{\n";
			sprintf(str,"\"name\":\"fireware%d\",\n",i+1); strJson = strJson + str;//名称
			if(pConf->StrFireWareType == "1" || pConf->StrFireWareType == "")
				{sprintf(str,"\"factoryname\":\"华为\",\n"); strJson = strJson + str;}//生产商
			else if(pConf->StrFireWareType == "2")
				{sprintf(str,"\"factoryname\":\"迪普\",\n"); strJson = strJson + str;}//生产商
			else if(pConf->StrFireWareType == "3")
				{sprintf(str,"\"factoryname\":\"深信服\",\n"); strJson = strJson + str;}//生产商
			if(pCfw->HUAWEIDevValue.hwEntityLinked && GetTickCount()-pCfw->HUAWEIDevValue.hwEntityTimeStamp>10*60) //10分钟没有更新数据，恢复默认
			{
				initHUAWEIEntity(pCfw);
				pCfw->HUAWEIDevValue.hwEntityLinked=false;
				sprintf(str,"\"isonline\":\"%d\"\n",pCfw->HUAWEIDevValue.hwEntityLinked); strJson = strJson + str;//连接状态
			}
			else
			{
				sprintf(str,"\"devicemodel\":\"%s\",\n",pCfw->HUAWEIDevValue.strhwEntityDevModel.c_str()); strJson = strJson + str;//设备型号
				sprintf(str,"\"isonline\":\"%d\",\n",pCfw->HUAWEIDevValue.hwEntityLinked); strJson = strJson + str;//连接状态
				sprintf(str,"\"cpuusage\":\"%s\",\n",pCfw->HUAWEIDevValue.strhwEntityCpuUsage.c_str()); strJson = strJson + str;//CPU使用率
				sprintf(str,"\"memusage\":\"%s\",\n",pCfw->HUAWEIDevValue.strhwEntityMemUsage.c_str()); strJson = strJson + str;//内存使用率
				sprintf(str,"\"temperature\":\"%s\",\n",pCfw->HUAWEIDevValue.strhwEntityTemperature.c_str()); strJson = strJson + str;//温度
				strJson = strJson + pCfw->strfirewalljson;								//网络数据
			}
			strJson +=	"}\n";
			if(i!=firewarecnt-1)
				strJson = strJson + ",";		
		}
		else
		{
			strJson +=	"{\n";
			sprintf(str,"\"isonline\":\"0\"\n"); strJson = strJson + str;//连接状态
			strJson +=	"}\n";
			if(i!=firewarecnt-1)
				strJson = strJson + ",";	
		}
	}
	strJson +=	"]\n";
	strJson +=	"}\n";

	mstrjson = strJson;
	//*len=strJson.length();
	//memcpy(json,(char*)strJson.c_str(),*len);
	
}


void SetjsonAtlasStatusStr(int messagetype,string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j,atlascnt; 
	static int recordno=0;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	CsshClient *pCAtlas;
		
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;

    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	strJson = strJson + "\"atlascnt\": \"" + pConf->StrAtlasCount + "\",\n";	//Atlas数量
	strJson = strJson + "\"atlaslist\": [\n";	//Atlas列表
	atlascnt=atoi(pConf->StrAtlasCount.c_str()); 
	for(i=0;i<atlascnt;i++)
	{
		pCAtlas=pCsshClient[i];
		if(pCAtlas!=NULL && pCAtlas->stuAtlasState.Linked && GetTickCount()-pCAtlas->stuAtlasState.TimeStamp>10*60) //10分钟不更新，恢复默认
			init_atlas_struct(pCAtlas);
	}	
	for(i=0;i<atlascnt;i++)
	{
		pCAtlas=pCsshClient[i];
		if(pCAtlas!=NULL)
			strJson = strJson + pCAtlas->stuAtlasState.stratlasdata;	
		if(i<atlascnt-1)
			strJson = strJson + ",";	
	}
	strJson +=	"]\n";
	strJson +=	"}\n";

	mstrjson = strJson;
	
}


void SetjsonIPStr(int messagetype,string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j,vehplatecnt; 
	static int recordno=0;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
    //IP 地址
	strJson = strJson + "\"ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"mask\":\""+ pConf->StrMask +"\",\n";	//子网掩码
	strJson = strJson + "\"gateway\":\""+ pConf->StrGateway +"\",\n";	//网关
	strJson = strJson + "\"dns\":\""+ pConf->StrDNS +"\"\n";	//DNS地址
	strJson +=	"}\n\0";
	
	mstrjson = strJson;
	//*len=strJson.length();
	//memcpy(json,(char*)strJson.c_str(),*len);
	
}

void SetjsonIP2Str(int messagetype,string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j,vehplatecnt; 
	static int recordno=0;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
    //IP 地址
	strJson = strJson + "\"ipaddr2\":\""+ pConf->StrIP2 +"\",\n";	//IP地址
	strJson = strJson + "\"mask2\":\""+ pConf->StrMask2 +"\",\n";	//子网掩码
	strJson = strJson + "\"gateway2\":\""+ pConf->StrGateway2 +"\",\n";	//网关
	strJson = strJson + "\"dns2\":\""+ pConf->StrDNS2 +"\"\n";	//DNS地址
	strJson +=	"}\n\0";
	
	mstrjson = strJson;
	//*len=strJson.length();
	//memcpy(json,(char*)strJson.c_str(),*len);
	
}


void SetjsonSpdAIStatusStr(int messagetype,string &mstrjson)
{
	char str[100],sDateTime[30];
	int spdcnt;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	SpdClient *pSpd=pCSpdClent;		//SPD防雷器
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;

    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	strJson = strJson + "\"spdcnt\": \"" + pConf->StrSPDCount + "\",\n";	//防雷器数量
	strJson = strJson + "\"spdlist\": [\n";	//防雷器列表
	spdcnt=atoi(pConf->StrSPDCount.c_str());
	for(int i=0;i<spdcnt;i++)
	{
		strJson +=	"{\n";
		sprintf(str,"\"spdid\":\"%d\",\n",i+1); strJson = strJson + str;//编号
		sprintf(str,"\"name\":\"spd%d\",\n",i+1); strJson = strJson + str;//名称
		sprintf(str,"\"ip\":\"%s\",\n",pConf->StrSPDIP[i].c_str()); strJson = strJson + str;//ip地址
		if(pSpd!=NULL && pSpd->stuSpd_Param->Linked[i])
		{
			sprintf(str,"\"isonline\":\"%d\",\n",pSpd->stuSpd_Param->Linked[i]); strJson = strJson + str;//连接状态
			if(pConf->StrSPDType=="1")
				strJson = strJson + "\"factoryname\":\"雷迅\",\n"; //防雷器厂商
			else if(pConf->StrSPDType=="2")
				strJson = strJson + "\"factoryname\":\"华咨圣泰\",\n";	//防雷器厂商
			else if(pConf->StrSPDType=="3")
				strJson = strJson + "\"factoryname\":\"宽永\",\n";	//防雷器厂商
			else if(pConf->StrSPDType=="4")
				strJson = strJson + "\"factoryname\":\"中普同安\",\n";	//防雷器厂商
			else if(pConf->StrSPDType=="5")
				strJson = strJson + "\"factoryname\":\"中普众合\",\n";	//防雷器厂商
			else if(pConf->StrSPDType=="6")
				strJson = strJson + "\"factoryname\":\"宽永0M\",\n";	//防雷器厂商
			else
				strJson = strJson + "\"factoryname\":\"\",\n";	//防雷器厂商
			sprintf(str,"\"spd_temp\": \"%.1f\",\n", pSpd->stuSpd_Param->rSPD_data[i].spd_temp);strJson += str;		// 防雷器温度
			sprintf(str,"\"envi_temp\": \"%.1f\",\n", pSpd->stuSpd_Param->rSPD_data[i].envi_temp);strJson += str;		//环境温度
			sprintf(str,"\"struck_cnt\": \"%.0f\",\n", pSpd->stuSpd_Param->rSPD_data[i].struck_cnt);strJson += str;		//雷击计数
			sprintf(str,"\"leak_alarm_threshold\": \"%.3f\",\n", pSpd->stuSpd_Param->rSPD_data[i].leak_alarm_threshold);strJson += str;		// 报警阈值
			
			sprintf(str,"\"life_time\": \"%.2f\",\n", pSpd->stuSpd_Param->rSPD_data[i].life_time);strJson += str;		// 防雷器寿命值
			sprintf(str,"\"remotestatusalarm\": \"%d\",\n", pSpd->stuSpd_Param->rSPD_data[i].DI_C1_status);strJson += str;		//防雷器脱扣状态报警
			sprintf(str,"\"linegroundstatusalarm\": \"%d\",\n", pSpd->stuSpd_Param->rSPD_data[i].DI_grd_alarm);strJson += str;	//线路&接地状态告警
			sprintf(str,"\"leakcuralarm\": \"%d\"\n", pSpd->stuSpd_Param->rSPD_data[i].DI_leak_alarm);strJson += str;		//漏电流告警
		}
		else
		{
			sprintf(str,"\"isonline\":\"0\"\n"); strJson = strJson + str;//连接状态
		}
		if(i==spdcnt-1)
			strJson = strJson + "}\n";	
		else
			strJson = strJson + "},\n"; 
	}
	strJson +=	"]\n";
	strJson +=	"}\n";

	mstrjson = strJson;
}


//28 接地电阻参数
void SetjsonSpdResStatusStr(int messagetype,string &mstrjson)
{
	char str[100],sDateTime[30];
	int spdcnt;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	SpdClient *pSpd=pCSpdClent;		//SPD防雷器
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;

    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
	sprintf(str,"\"spdresip\": \"%s\",\n", pConf->StrSPDIP[SPD_NUM].c_str());strJson += str;		// 接地检测器地址
	sprintf(str,"\"spdresport\": \"%s\",\n", pConf->StrSPDPort[SPD_NUM].c_str());strJson += str;		// 接地检测器端口
	if(pSpd!=NULL && pSpd->stuSpd_Param!=NULL && pSpd->stuSpd_Param->Linked[SPD_NUM])
	{
		sprintf(str,"\"isonline\":\"%d\",\n",pSpd->stuSpd_Param->Linked[SPD_NUM]); strJson = strJson + str;//连接状态
		sprintf(str,"\"alarm\": \"%d\",\n", pSpd->stuSpd_Param->rSPD_res.alarm);strJson += str; 	//接地报警 0x0c
		sprintf(str,"\"grd_res\": \"%.3f\",\n", pSpd->stuSpd_Param->rSPD_res.grd_res_real);strJson += str;		//接地电阻值
		sprintf(str,"\"grd_volt\": \"%d\",\n", pSpd->stuSpd_Param->rSPD_res.grd_volt);strJson += str;		// 电压值 0x0f
		sprintf(str,"\"spdresid\": \"%s\",\n", pConf->StrSPDAddr[SPD_NUM].c_str());strJson += str;		// 更改id地址
		sprintf(str,"\"spdres_alarm_value\": \"%d\",\n", pSpd->stuSpd_Param->rSPD_res.alarm_value);strJson += str;		// 报警值修改
	}
	else
	{
		sprintf(str,"\"isonline\":\"0\",\n"); strJson = strJson + str;//连接状态
	}
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\"\n";	//时间
	strJson +=	"}\n";

	mstrjson = strJson;
}



void SetjsonLTAlarmTableStr(char* table, string &mstrjson)
{
	char str[100],sDateTime[30];
	int i,j,vehplatecnt; 
	CabinetClient *pCab=pCabinetClient[0];//华为机柜状态
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson,strTable;
	strTable = table;
	
    strJson +=  "{\n";
    strJson +=  "\"param\": {\n";
	strJson +=	"\"action\":\""+ strTable + "\",\n";
	strJson +=	"\"entity\": {\n";
	
	sprintf(str,"\"id\":\"%s%04d%02d%02d%02d%02d%02d\",\n",pConf->StrFlagID.c_str(),pTM->tm_year + 1900, 
				pTM->tm_mon + 1, pTM->tm_mday,pTM->tm_hour, pTM->tm_min, pTM->tm_sec); strJson = strJson + str;//id
	strJson = strJson + "\"gantryid\": \"" + pConf->StrFlagID +"\",\n";			//门架编号
	strJson = strJson + "\"time\": \"" + sDateTime + "\",\n";	//状态数据生成时间
	strJson = strJson + "\"direction\": \"" + pConf->StrDirection +"\",\n";		//6 行车方向
	strJson = strJson + "\"dirdescription\": \"" + pConf->StrDirDescription +"\",\n";		//7 行车方向说明
    strJson = strJson + "\"vmctrl_ipaddr\": \"" + pConf->StrIP.c_str() + "\",\n";	//控制器IP地址 212

    pthread_mutex_lock(&pCab->CabinetdataMutex);
	
	//华为机柜字段
	//设备信息 
    strJson = strJson + "\"monequipsoftwareversion\": \"" + pCab->HUAWEIDevValue.strhwMonEquipSoftwareVersion.c_str() + "\",\n";	//软件版本
//    strJson = strJson + "\"monequipmanufacturer\": \"" + HUAWEIDevValue.strhwMonEquipManufacturer.c_str() + "\",\n";	//设备生产商
    strJson = strJson + "\"cabinettype\": " + pConf->StrCabinetType.c_str() + ",\n";	//机柜类型 213
    strJson = strJson + "\"acbgroupbatvolt\": " + pCab->HUAWEIDevValue.strhwAcbGroupBatVolt.c_str() + ",\n";	//锂电电池电压 214
    strJson = strJson + "\"envtempalarmtraps\": " + pCab->HUAWEIDevAlarm.hwEnvTempAlarmTraps.c_str() + ",\n";	//环境温度告警 239
    strJson = strJson + "\"envhumialarmresumetraps\": " + pCab->HUAWEIDevAlarm.hwEnvHumiAlarmTraps.c_str() + ",\n";	//环境湿度告警 240
    strJson = strJson + "\"dooralarmtraps\": " + pCab->HUAWEIDevAlarm.hwDoorAlarmTraps.c_str() + ",\n";	//门禁告警 241
    strJson = strJson + "\"wateralarmtraps\": " + pCab->HUAWEIDevAlarm.hwWaterAlarmTraps.c_str() + ",\n";	//水浸告警 242
    strJson = strJson + "\"smokealarmtraps\": " + pCab->HUAWEIDevAlarm.hwSmokeAlarmTraps.c_str() + ",\n";	//烟雾告警 243
	//供配电
	//电源告警
	sprintf(str,"\"acinputfailalarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcInputFailAlarm.c_str());strJson += str;		 //交流电源输入停电告警
	sprintf(str,"\"acinputl1failalarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcInputL1FailAlarm.c_str());strJson += str;		 //交流电源输入L1	相缺相告警
	sprintf(str,"\"acinputl2failalarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcInputL2FailAlarm.c_str());strJson += str;		 //交流电源输入L2	相缺相告警
	sprintf(str,"\"acinputl3failalarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcInputL3FailAlarm.c_str());strJson += str;		 //交流电源输入L3	相缺相告警
	sprintf(str,"\"dcvoltalarmtraps\": %d,\n", pCab->HUAWEIDevAlarm.hwDcVoltAlarmTraps.c_str());strJson += str;		 //直流电源输出告警
	sprintf(str,"\"loadlvdalarmtraps\": %d,\n", pCab->HUAWEIDevAlarm.hwLoadLvdAlarmTraps.c_str());strJson += str;		 //LLVD1下电告警
	//锂电池告警
	sprintf(str,"\"acbgroup_comm_fail_alarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_comm_fail_alarm.c_str());strJson += str;		 //所有锂电通信失败
	sprintf(str,"\"acbgroup_discharge_alarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_discharge_alarm.c_str());strJson += str;		 //电池放电告警
	sprintf(str,"\"acbgroup_charge_overcurrent_alarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_charge_overcurrent_alarm.c_str());strJson += str;		 //电池充电过流
	sprintf(str,"\"acbgroup_temphigh_alarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_temphigh_alarm.c_str());strJson += str;		 //电池温度高
	sprintf(str,"\"acbgroup_templow_alarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_templow_alarm.c_str());strJson += str;		 //电池温度低
	sprintf(str,"\"acbgroup_poweroff_alarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_poweroff_alarm.c_str());strJson += str;		 //电池下电
	sprintf(str,"\"acbgroup_fusebreak_alarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_fusebreak_alarm.c_str());strJson += str;		 //电池熔丝断
	sprintf(str,"\"acbgroup_moduleloss_alarm\": %d,\n", pCab->HUAWEIDevAlarm.hwAcbGroup_moduleloss_alarm.c_str());strJson += str;		 //模块丢失
	//防雷器告警
    strJson = strJson + "\"acspdalarmtraps\": " + pCab->HUAWEIDevAlarm.hwACSpdAlarmTraps.c_str() + ",\n";	//交流防雷器故障
    strJson = strJson + "\"dcspdalarmtraps\": " + pCab->HUAWEIDevAlarm.hwDCSpdAlarmTraps.c_str() + ",\n";	//直流防雷器故障
	//直流空调告警
    strJson = strJson + "\"air_cond_infan_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_infan_alarm.c_str() + ",\n";	//空调内风机故障 245
    strJson = strJson + "\"air_cond_outfan_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_outfan_alarm.c_str() + ",\n";	//空调外风机故障 246
    strJson = strJson + "\"air_cond_comp_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_comp_alarm.c_str() + ",\n";	//空调压缩机故障 247
    strJson = strJson + "\"air_cond_return_port_sensor_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_return_port_sensor_alarm.c_str() + ",\n";	//空调回风口传感器故障 248
    strJson = strJson + "\"air_cond_evap_freezing_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_evap_freezing_alarm.c_str() + ",\n";	//空调蒸发器冻结 249
    strJson = strJson + "\"air_cond_freq_high_press_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_freq_high_press_alarm.c_str() + ",\n";	//空调频繁高压力 250
    strJson = strJson + "\"air_cond_comm_fail_alarm\": " + pCab->HUAWEIDevAlarm.hwair_cond_comm_fail_alarm.c_str() + ",\n";	//空调通信失败告警 251
    strJson = strJson + "\"ishandle\": 0\n";	//告警处理标记

    pthread_mutex_unlock(&pCab->CabinetdataMutex);
	
	strJson +=	" }\n";
	strJson +=	" }\n";
	strJson +=	"}\n\0";
	
    mstrjson = strJson;
    //*len=strJson.length();
    //memcpy(json,(char*)strJson.c_str(),*len);
	
}



void SetjsonDealLockerStr(int messagetype,UINT32 cardid,UINT16 lockaddr,string &mstrjson)
{
	char str[100],sDateTime[30];
	int i; 
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS 
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);	

	std::string strJson;
	
    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"flagroadid\":\""+ pConf->StrFlagRoadID +"\",\n";		// ETC 门架路段编号
	strJson = strJson + "\"flagid\": \"" + pConf->StrFlagID +"\",\n";			// ETC 门架编号
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
//	sprintf(str,"\"cabineid\":%d,\n",lockaddr); strJson = strJson + str;	//机柜ID
	for (i = 0; i < LOCK_NUM; i++)
	{
		if(lockaddr==atoi(pConf->StrAdrrLock[i].c_str()))
		{sprintf(str,"\"cabineid\":%d,\n",i+1); strJson = strJson + str;}	//机柜ID
	}

	sprintf(str,"\"cardid\":\"%llu\",\n",cardid); strJson = strJson + str;	//ID卡号
	sprintf(str,"\"operate\":%d\n",ACT_UNLOCK); strJson = strJson + str;	//操作请求
	strJson +=	"}\n\0";

    mstrjson = strJson;
}

// 64位卡号发送函数
void SetjsonDealLockerStr64(int messagetype,UINT64 cardid,UINT16 lockaddr,string &mstrjson)
{
	char str[100],sDateTime[30];
	int i;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体

    time_t nSeconds;
    struct tm * pTM;

    time(&nSeconds);
    pTM = localtime(&nSeconds);

    //系统日期和时间,格式: yyyymmddHHMMSS
    sprintf(sDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);

	std::string strJson;

    strJson +=  "{\n";
	sprintf(str,"\"messagetype\":%d,\n",messagetype); strJson = strJson + str;//消息类型
	strJson = strJson + "\"vmctrl_ipaddr\":\""+ pConf->StrIP +"\",\n";	//IP地址
	strJson = strJson + "\"flagroadid\":\""+ pConf->StrFlagRoadID +"\",\n";		// ETC 门架路段编号
	strJson = strJson + "\"flagid\": \"" + pConf->StrFlagID +"\",\n";			// ETC 门架编号
	strJson = strJson + "\"opttime\": \"" + sDateTime + "\",\n";	//时间
	strJson = strJson + "\"cabinettype\":"+ pConf->StrCabinetType +",\n";	//机柜类型
//	sprintf(str,"\"cabineid\":%d,\n",lockaddr); strJson = strJson + str;	//机柜ID
	for (i = 0; i < LOCK_NUM; i++)
	{
		if(lockaddr==atoi(pConf->StrAdrrLock[i].c_str()))
		{sprintf(str,"\"cabineid\":%d,\n",i+1); strJson = strJson + str;}	//机柜ID
	}

	sprintf(str,"\"cardid\":\"%llu\",\n",cardid); strJson = strJson + str;	//ID卡号
	sprintf(str,"\"operate\":%d\n",ACT_UNLOCK); strJson = strJson + str;	//操作请求
	strJson +=	"}\n\0";

    mstrjson = strJson;
}


bool jsonstrSPDReader(char* jsonstr, int len, UINT8 *pstuRCtrl)
{
//	printf("%s \t\n",jsonstr);
	char key[50],value[128],keytmp[50];
	int valueint,arraysize;

	cJSON *json=0, *jsonkey=0, *jsonvalue=0, *jsonlist=0, *jsonitem=0;
	int i;
	//解析数据包
	json = cJSON_Parse(jsonstr);
	if(json==0) return false;

	REMOTE_CONTROL *pRCtrl=(REMOTE_CONTROL *)pstuRCtrl;
	
	memset(pRCtrl,ACT_HOLD,sizeof(REMOTE_CONTROL));
	pRCtrl->hwsetenvtemplowerlimit[0]=ACT_HOLD_FF;	//环境温度告警下限255:保持；-20-20（有效）；-20（缺省值）
	pRCtrl->hwsetenvtemplowerlimit[1]=ACT_HOLD_FF;	//环境温度告警下限255:保持；-20-20（有效）；-20（缺省值）
	pRCtrl->hwsetenvhumidityupperlimit[0]=ACT_HOLD_FF;	//环境湿度告警上限 255:保持；0-100（有效）；95（缺省值）
	pRCtrl->hwsetenvhumidityupperlimit[1]=ACT_HOLD_FF;	//环境湿度告警上限 255:保持；0-100（有效）；95（缺省值）
	pRCtrl->hwsetenvhumiditylowerlimit[0]=ACT_HOLD_FF;	//环境湿度告警下限 255:保持；0-100（有效）；5（缺省值）
	pRCtrl->hwsetenvhumiditylowerlimit[1]=ACT_HOLD_FF;	//环境湿度告警下限 255:保持；0-100（有效）；5（缺省值）
	pRCtrl->hwdcairpowerontemppoint[0]=ACT_HOLD_FF;		//空调开机温度点 255:保持； -20-80（有效）；45(缺省值)
	pRCtrl->hwdcairpowerontemppoint[1]=ACT_HOLD_FF;		//空调开机温度点 255:保持； -20-80（有效）；45(缺省值)
	pRCtrl->hwdcairpowerofftemppoint[0]=ACT_HOLD_FF;		//空调关机温度点  		  255:保持； -20-80（有效）；37(缺省值)
	pRCtrl->hwdcairpowerofftemppoint[1]=ACT_HOLD_FF;		//空调关机温度点  		  255:保持； -20-80（有效）；37(缺省值)
	
	//SPD 列表
    jsonlist = cJSON_GetObjectItem(json, "spdlist");
    if(jsonlist!=0)
    {
        arraysize=cJSON_GetArraySize(jsonlist);
        for(i=0;i<arraysize;i++)
        {
            jsonitem=cJSON_GetArrayItem(jsonlist,i);
            if(jsonitem != NULL)
            {
            	//雷击计数清零
				sprintf(key,"clearcounter");
                jsonkey=cJSON_GetObjectItem(jsonitem,key);
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					printf("%s %s\n",key,value);
					pRCtrl->DO_spdcnt_clear[i]=atoi(value); 
                }
            	//总雷击计数清0
				sprintf(key,"cleartotalcounter");
                jsonkey=cJSON_GetObjectItem(jsonitem,key);
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					printf("%s %s\n",key,value);
					pRCtrl->DO_totalspdcnt_clear[i]=atoi(value); 
                }
            	//雷击时间清0
				sprintf(key,"strucktimerecclear");
                jsonkey=cJSON_GetObjectItem(jsonitem,key);
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					printf("%s %s\n",key,value);
					pRCtrl->DO_psdtime_clear[i]=atoi(value); 
                }
            	//在线时间清0
				sprintf(key,"onlinetimeclear");
                jsonkey=cJSON_GetObjectItem(jsonitem,key);
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					printf("%s %s\n",key,value);
					pRCtrl->DO_daytime_clear[i]=atoi(value); 
                }
            	//漏电流报警阈值
				sprintf(key,"leak_alarm_threshold");
                jsonkey=cJSON_GetObjectItem(jsonitem,key);
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					printf("%s %s\n",key,value);
					pRCtrl->spdleak_alarm_threshold[i]=atof(value); 
                }
            	//外接漏电流控制
				sprintf(key,"extleakcurrctrl");
                jsonkey=cJSON_GetObjectItem(jsonitem,key);
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					printf("%s %s\n",key,value);
					pRCtrl->DO_leak_type[i]=atoi(value); 
                }
            	//防雷器设备地址
				sprintf(key,"modbus_addr");
                jsonkey=cJSON_GetObjectItem(jsonitem,key);
                if(jsonkey != NULL)
                {
					sprintf(value,"%s",jsonkey->valuestring);
					printf("%s %s\n",key,value);
					pRCtrl->spd_modbus_addr[i]=atoi(value); 
                }
            }
        }
    }
}
	


