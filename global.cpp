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
#include "debug.h"
#include "comport.h"
#include "tea.h"
#include <iconv.h>
#include "config.h"
#include <cstring>
#include <sstream>
#include "global.h"
#include "lock.h"

extern CabinetClient *pCabinetClient[HWSERVER_NUM];//华为机柜状态
extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体
extern CANNode *pCOsCan;		//Can对象
extern CfirewallClient *pCfirewallClient[FIREWARE_NUM];
extern CswitchClient *pCswitchClient[IPSWITCH_NUM];
extern Artc *pCArtRSU[RSUCTL_NUM];;					//RSU对象
extern Huawei *pCHWRSU[RSUCTL_NUM];;					//RSU对象
extern IPCam *pCVehplate[VEHPLATE_NUM];
extern IPCam *pCVehplate900[VEHPLATE900_NUM];
extern CsshClient *pCsshClient[ATLAS_NUM];			//ATLAS对象
extern Lock::Info_S LockInfo[LOCK_NUM];			//电子锁结构体

void SetIPinfo()
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    string strwrconfig = "IP=" + pConf->StrIP + "\n" + "Mask=" + pConf->StrMask + "\n" + "Gateway=" + pConf->StrGateway + "\n" + "DNS=" + pConf->StrDNS + "\n" ;
    WriteNetconfig((char *)(strwrconfig.c_str()),strwrconfig.length());


}

void SetIPinfo2()
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    string strwrconfig = "IP=" + pConf->StrIP2 + "\n" + "Mask=" + pConf->StrMask2 + "\n" + "Gateway=" + pConf->StrGateway2 + "\n" + "DNS=" + pConf->StrDNS2 + "\n" ;
    WriteNetconfig2((char *)(strwrconfig.c_str()),strwrconfig.length());
}

void GetIPinfo(IPInfo *ipInfo)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
	sprintf(ipInfo->ip,pConf->StrIP.c_str());
	sprintf(ipInfo->submask ,pConf->StrMask.c_str());
	sprintf(ipInfo->gateway_addr,pConf->StrGateway.c_str());
	sprintf(ipInfo->dns ,pConf->StrDNS.c_str());
}

void GetIPinfo2(IPInfo *ipInfo)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	
    sprintf(ipInfo->ip,pConf->StrIP2.c_str());
    sprintf(ipInfo->submask ,pConf->StrMask2.c_str());
    sprintf(ipInfo->gateway_addr,pConf->StrGateway2.c_str());
    sprintf(ipInfo->dns ,pConf->StrDNS2.c_str());
}

/************************ 涉及到全局变量的函数*****************************/
// 取CAN控制板的电压值
float voltTrueGetFromDev(uint8_t seq)
{
	CANNode *pCan=pCOsCan;
	if (seq < PHASE_MAX_NUM)
	{
		return pCan->canNode[seq].phase.vln;
	}
}


// 取CAN控制板的电压值
float ampTrueGetFromDev(uint8_t seq)
{
	CANNode *pCan=pCOsCan;
	if (seq < PHASE_MAX_NUM)
	{
		return pCan->canNode[seq].phase.amp;
	}
}




// 该机柜还是在线? 返回：1：在线；2：离线
// seq: 0:设备柜
// 		1:电池柜
uint16_t linkStGetFrombox(uint8_t seq)
{
	CabinetClient *pCab=pCabinetClient[seq];//华为机柜状态
	if(pCab==NULL) return 0;
	
	uint16_t re_val = 0;
	bool hwBox = pCab->HUAWEIDevValue.hwLinked;

	if (hwBox)
	{
		re_val = 1;
	}
	else
	{
		re_val = 2;
	}
	return re_val;
}


// 该设备离线还是在线?
// 返回：1：在线，2：离线 	0:未配置
uint16_t linkStGetFromDevice(uint8_t seq)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	int _pos=0,num=0;
	uint16_t linkSt;
	IPCam::State_S State;
	Artc::RsuInfo_S ArtcState;
	
	string strDeviceName = pConf->StrDeviceNameSeq[seq];
	if(strDeviceName.find("rsu")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(3,_pos-3).c_str());

		if(pConf->StrRSUType=="1")
		{
			ArtcState=pCArtRSU[num-1]->getRsuInfo();
			linkSt=ArtcState.controler.linked;
		}
//		else if(pConf->StrRSUType=="2")
//			linkSt=pCHWRSU[num-1]->RsuInfo_S.controler.linked;
		if(linkSt==0)		//离线代码转换
		{
			linkSt=2;
		}
		DEBUG_PRINTF("rsu %s %d\r\n",strDeviceName.c_str(),linkSt);
	}
	else if(strDeviceName.find("vehplate900")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(11,_pos-11).c_str());
		State = pCVehplate900[num-1]->getState();
		if(State.linked==0)		//离线代码转换
		{
			linkSt=2;
		}
		DEBUG_PRINTF("vehplate900 %s %d\r\n",strDeviceName.c_str(),linkSt);
	}
	else if(strDeviceName.find("vehplate")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		State = pCVehplate[num-1]->getState();
		if(State.linked==0)		//离线代码转换
		{
			linkSt=2;
		}
		DEBUG_PRINTF("vehplate %s %d\r\n",strDeviceName.c_str(),linkSt);
	}
	else if(strDeviceName.find("cam")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(3,_pos-3).c_str());
		linkSt=2;
		DEBUG_PRINTF("cam %s %d\r\n",strDeviceName.c_str(),linkSt);
	}
	else if(strDeviceName.find("ipswitch")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		linkSt=pCswitchClient[num-1]->HUAWEIDevValue.hwswitchEntityLinked;
		if(linkSt==0)		//离线代码转换
		{
			linkSt=2;
		}
		DEBUG_PRINTF("ipswitch %s %d\r\n",strDeviceName.c_str(),linkSt);
	}
	else if(strDeviceName.find("fireware")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		linkSt=pCfirewallClient[num-1]->HUAWEIDevValue.hwEntityLinked;
		if(linkSt==0)		//离线代码转换
		{
			linkSt=2;
		}
		DEBUG_PRINTF("fireware %s %d\r\n",strDeviceName.c_str(),linkSt);
	}
	else if(strDeviceName.find("atlas")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(5,_pos-5).c_str());
		linkSt=pCsshClient[num-1]->stuAtlasState.Linked;
		if(linkSt==0)		//离线代码转换
		{
			linkSt=2;
		}
		DEBUG_PRINTF("atlas %s %d\r\n",strDeviceName.c_str(),linkSt);
	}
	/* 未配置,不显示
	else
	{
		linkSt=2;
		DEBUG_PRINTF("null %s %d\r\n",strDeviceName.c_str(),linkSt);
	}
	*/
	return linkSt;
}


// 该设备离线还是在线?
// 1：有电，2：没电
uint16_t powerStGetFromDevice(uint8_t seq)
{
	CANNode *pCan=pCOsCan;
	if (seq < PHASE_MAX_NUM)
	{
		if(pCan->canNode[seq].phase.vln< POWER_DOWN_VALUE)
		{
			return 2;
		}
		else
		{
			return 1;
		}
	}
}



// 该通道是否配置? IP地址、端口、密码等只要存在空值或0.0.0.0，返回true
bool isChannelNull(uint8_t seq)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	int _pos=0,num=0;
	string strDeviceName = pConf->StrDeviceNameSeq[seq];
	bool isnull=false;
	
	DEBUG_PRINTF("isChannelNull seq=%d %s \n",seq,strDeviceName.c_str());

	if(strDeviceName.find("rsu")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(3,_pos-3).c_str());
		if(pConf->StrRSUIP[num-1]=="" || pConf->StrRSUIP[num-1]=="0.0.0.0" || pConf->StrRSUPort[num-1]=="")
			isnull=true;
		DEBUG_PRINTF("channel %d %s %d\r\n",seq,strDeviceName.c_str(),isnull);
	}
	else if(strDeviceName.find("vehplate900")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(11,_pos-11).c_str());
		if(pConf->StrVehPlate900IP[num-1]=="" || pConf->StrVehPlate900IP[num-1]=="0.0.0.0" || pConf->StrVehPlate900Port[num-1]=="" || pConf->StrVehPlate900Key[num-1]=="")
			isnull=true;
		DEBUG_PRINTF("channel %d %s %d\r\n",seq,strDeviceName.c_str(),isnull);
	}
	else if(strDeviceName.find("vehplate")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		if(pConf->StrVehPlateIP[num-1]=="" || pConf->StrVehPlateIP[num-1]=="0.0.0.0" || pConf->StrVehPlatePort[num-1]=="" || pConf->StrVehPlateKey[num-1]=="")
			isnull=true;
		DEBUG_PRINTF("channel %d %s %d\r\n",seq,strDeviceName.c_str(),isnull);
	}
	else if(strDeviceName.find("cam")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(3,_pos-3).c_str());
		if(pConf->StrCAMIP[num-1]=="" || pConf->StrCAMIP[num-1]=="0.0.0.0" || pConf->StrCAMPort[num-1]=="" || pConf->StrCAMKey[num-1]=="")
			isnull=true;
		DEBUG_PRINTF("channel %d %s %d\r\n",seq,strDeviceName.c_str(),isnull);
	}
	else if(strDeviceName.find("ipswitch")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		if(pConf->StrIPSwitchIP[num-1]=="" || pConf->StrIPSwitchIP[num-1]=="0.0.0.0" || pConf->StrIPSwitchGetPasswd[num-1]=="" || pConf->StrIPSwitchSetPasswd[num-1]=="")
			isnull=true;
		DEBUG_PRINTF("channel %d %s %d\r\n",seq,strDeviceName.c_str(),isnull);
	}
	else if(strDeviceName.find("fireware")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		if(pConf->StrFireWareIP[num-1]=="" || pConf->StrFireWareIP[num-1]=="0.0.0.0" || pConf->StrFireWareGetPasswd[num-1]=="" || pConf->StrFireWareSetPasswd[num-1]=="")
			isnull=true;
		DEBUG_PRINTF("channel %d %s %d\r\n",seq,strDeviceName.c_str(),isnull);
	}
	else if(strDeviceName.find("atlas")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(5,_pos-5).c_str());
		if(pConf->StrAtlasIP[num-1]=="" || pConf->StrAtlasIP[num-1]=="0.0.0.0" || pConf->StrAtlasPasswd[num-1]=="")
			isnull=true;
		DEBUG_PRINTF("channel %d %s %d\r\n",seq,strDeviceName.c_str(),isnull);
	}
	else
	{
		isnull=true;
		DEBUG_PRINTF("channel %d %s %d\r\n",seq,strDeviceName.c_str(),isnull);
	}
	return isnull;
}

// 单柜还是双柜?返回柜子个数
uint8_t getBoxNum(void)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	return atoi(pConf->StrHWCabinetCount.c_str());
}

// 从配置文件中读取CAN控制器版本号,只读地址为1的控制器
string getCanVersion(uint8_t seq)
{
	string re_str = "NULL";
	CANNode *pCan=pCOsCan;

	// 任何一路有动力板，都取这路的版本号
	for (int i=0;i < PHASE_MAX_NUM;i++)
	{
		if (pCan->canNode[i].phase.version != "")
		{
			re_str = pCan->canNode[i].phase.version;
			break;
		}
	}
	return re_str;
}


// 获取机柜n的IP地址
string IPGetFromBox(uint8_t seq)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	string serverip = pConf->StrHWServer;
	if (seq == 1)
	{
		serverip =  pConf->StrHWServer2;
	}

	if (serverip == "")
	{
		serverip = "未配置";
	}

	return serverip;
}


// 获取该机柜是离线还是在线?
//返回
string linkStringGetFromBox(uint8_t seq)
{
	CabinetClient *pCab=pCabinetClient[seq];//华为机柜状态
	
	string str_re = "";
	bool linked = pCab->HUAWEIDevValue.hwLinked;
	
/*	if (seq == 1)
	{
		linked = pCab->HUAWEIDevValue.hwLinked2;
	}*/
	
	if (linked)
	{
		str_re = "在线";
	}
	else
	{
		str_re = "离线";
	}
	return str_re;
}

// 取8个字节的ID号
unsigned long long IDgetFromConfig(void)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	unsigned long long IDDataSaved;	// 64位长度存ID

	/*不能用atoi和atol，它们最大只能转换长整型,超出的会返回0x7FFFFFFF*/
	IDDataSaved = (unsigned long long)strtoll(pConf->StrID.c_str(),NULL,10);
	return IDDataSaved;
}


// seq:DO序号, DeviceName：返回的设备名称，ip:返回的ip地址,port:返回的端口号
void IPgetFromDevice(uint8_t seq,string& DeviceName, string& ip, string& port)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	int _pos=0,num=0;
	string strDeviceName = pConf->StrDeviceNameSeq[seq];
	stringstream   ss; 
	string str_temp = "";
	
	DEBUG_PRINTF("IPgetFromDevice seq=%d %s \n",seq,strDeviceName.c_str());

	if(strDeviceName.find("rsu")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(3,_pos-3).c_str());
		
		if(num==1)
		{
			DeviceName="RSU控制器(主)";
		}
		else if(num==2)
		{
			DeviceName="RSU控制器(备)";
		}
		ip = pConf->StrRSUIP[num-1];
		port = pConf->StrRSUPort[num-1];
		DEBUG_PRINTF("rsu %s %s  %s\r\n",DeviceName.c_str(),ip.c_str(),port.c_str());
	}
	else if(strDeviceName.find("vehplate900")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(11,_pos-11).c_str());
		ss.str("");	// 清掉内容
		ss << "全景摄像机"<<num;
		DeviceName = ss.str();
		ip = pConf->StrVehPlate900IP[num-1];		//全景摄像机IP地址
		port = pConf->StrVehPlate900Port[num-1];	//全景摄像机port
		DEBUG_PRINTF("vehplate900 %s %s  %s\r\n",DeviceName.c_str(),ip.c_str(),port.c_str());
	}
	else if(strDeviceName.find("vehplate")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		ss.str("");	// 清掉内容
		ss << "车牌识别仪"<<num;
		DeviceName = ss.str();
		ip = pConf->StrVehPlateIP[num-1];		//车牌识别仪IP地址
		port = pConf->StrVehPlatePort[num-1];	//车牌识别仪port
		DEBUG_PRINTF("vehplate %s %s  %s\r\n",DeviceName.c_str(),ip.c_str(),port.c_str());
	}
	else if(strDeviceName.find("cam")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(3,_pos-3).c_str());
		ss.str("");	// 清掉内容
		ss << "监控摄像头"<<num;
		DeviceName = ss.str();
		
		ip = pConf->StrCAMIP[num-1];			//监控摄像头IP地址
		port = pConf->StrCAMPort[num-1];		//监控摄像头PORT
		DEBUG_PRINTF("cam %s %s  %s\r\n",DeviceName.c_str(),ip.c_str(),port.c_str());
	}
	else if(strDeviceName.find("ipswitch")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		if(num==1)
		{
			DeviceName = "业务交换机";
		}
		else if(num==2)
		{
			DeviceName = "监控交换机";
		}
		ip = pConf->StrIPSwitchIP[num-1];		//交换机IP地址
		port = "";						//port为空
		DEBUG_PRINTF("ipswitch %s %s  %s\r\n",DeviceName.c_str(),ip.c_str(),port.c_str());
	}
	else if(strDeviceName.find("fireware")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(8,_pos-8).c_str());
		if(num==1)
		{
			DeviceName = "业务防火墙";
		}
		else if(num==2)
		{
			DeviceName = "监控防火墙";
		}
		ip = pConf->StrFireWareIP[num-1];	//防火墙IP地址
		port = "";						//port为空
		DEBUG_PRINTF("fireware %s %s  %s\r\n",DeviceName.c_str(),ip.c_str(),port.c_str());
	}
	else if(strDeviceName.find("atlas")==0)
	{
		_pos=strDeviceName.find("_");
		
		num=atoi(strDeviceName.substr(5,_pos-5).c_str());
		if(num==1)
		{
			DeviceName = "atlas(主)";
		}
		else if(num==2)
		{
			DeviceName = "atlas(备)";
		}
		ip = pConf->StrAtlasIP[num-1];	//atlasIP地址
		port = "";					//port为空
		DEBUG_PRINTF("atlas %s %s  %s\r\n",DeviceName.c_str(),ip.c_str(),port.c_str());
	}
	else
	{
		DeviceName = "无";
		ip = "";
		port = "";
		DEBUG_PRINTF("null %s %s  %s\r\n",DeviceName.c_str(),ip.c_str(),port.c_str());
	}

	// 后面加1个",",与其它隔开
	if (DeviceName != "无")
	{
		str_temp = ",";
		DeviceName += str_temp;
		if (ip != "")
		{
			ip += str_temp;
		}

		if (port != "")
		{
			port += str_temp;
		}
	}
	else
	{
		str_temp = ",";
		DeviceName += str_temp;
	}
}


void VAgetFromDevice(uint8_t seq, string& volt, string& amp)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	int _pos=0,num=0;
	string strDeviceName = pConf->StrDeviceNameSeq[seq];
	stringstream   ss; 
	float int_v = 0;
	float int_a = 0;
	//string str_volt;
	//string str_amp;

	int_v = voltTrueGetFromDev(seq);
	ss.str("");	// 清掉内容
	ss << "电压:"<< int_v<<"V,";
	volt = ss.str();

	int_a = ampTrueGetFromDev(seq);
	ss.str("");	// 清掉内容
	ss << "电流:" << int_a<<"A";
	amp = ss.str();
	
	DEBUG_PRINTF("VAgetFromDevice seq=%d %s  %s\n",seq,volt.c_str(),amp.c_str());
}


// 从电子锁判断开门状态,关:LOCKER_CLOSED, 开:LOCKER_OPEN
// 这个函数是电子锁那边的，需要重写
uint16_t DoorStatusFromLocker(void)
{
	uint16_t reval = LOCKER_CLOSED;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体

	for(int i=0;i<LOCK_NUM;i++)
	{
		if (pConf->StrAdrrLock[i].length() != 0)
		{
			// 任何一把锁为打开,就是打开状态，屏幕要亮
			if (LockInfo[i].isOpen == LOCKER_OPEN)
			{
				reval = LOCKER_OPEN;
				break;
			}
		}
	}	
	return reval;
}




