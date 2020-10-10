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
#include "initmodule.h"

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
//extern SpdClient *pCSpdClent;		//SPD防雷器


//初始化机柜结构体
void initHUAWEIGantry(CabinetClient *pCab)
{
   pCab->HUAWEIDevValue.hwLinked=false;
   pCab->HUAWEIDevValue.AcbGroupBatOnline=false;		   //锂电池组是否在线
   pCab->HUAWEIDevValue.DcAirOnline=false;			   //直流空调是否在线（设备柜）
   pCab->HUAWEIDevValue.DcAirOnline2=false;			   //直流空调是否在线（电池柜）
   pCab->HUAWEIDevValue.TemHumOnline=false;			   //温湿度传感器是否在线（设备柜）
   pCab->HUAWEIDevValue.TemHumOnline2=false;			   //温湿度传感器是否在线（电池柜）
   //锂电
   pCab->HUAWEIDevValue.strhwAcbGroupBatVolt="2147483647";                //电池电压 "51.1"
   pCab->HUAWEIDevValue.strhwAcbGroupBatCurr="2147483647";            //电池电流
   pCab->HUAWEIDevValue.strhwAcbGroupTotalCapacity="2147483647";                //电池总容量
   pCab->HUAWEIDevValue.strhwAcbGroupTotalRemainCapacity="2147483647";               //电池剩余容量
   pCab->HUAWEIDevValue.strhwAcbGroupBackupTime="2147483647";              //电池备电时长
   pCab->HUAWEIDevValue.strhwAcbGroupBatSoh="2147483647";             //电池 SOH
   //开关电源
   pCab->HUAWEIDevValue.strhwApOrAblVoltage="2147483647";                //A/AB 电压
   pCab->HUAWEIDevValue.strhwBpOrBclVoltage="2147483647";                //B/BC 电压
   pCab->HUAWEIDevValue.strhwCpOrCalVoltage="2147483647";                //C/CA 电压
   pCab->HUAWEIDevValue.strhwAphaseCurrent="2147483647";               //A 相电流
   pCab->HUAWEIDevValue.strhwBphaseCurrent="2147483647";              //B 相电流
   pCab->HUAWEIDevValue.strhwCphaseCurrent="2147483647";             //C 相电流
   pCab->HUAWEIDevValue.strhwDcOutputVoltage="2147483647";             //DC 输出电压
   pCab->HUAWEIDevValue.strhwDcOutputCurrent="2147483647";               //DC 输出电流
   //环境传感器
   pCab->HUAWEIDevValue.strhwEnvTemperature[0]="2147483647";              //环境温度值
   pCab->HUAWEIDevValue.strhwEnvTemperature[1]="2147483647";              //环境温度值
   pCab->HUAWEIDevValue.strhwEnvHumidity[0]="2147483647";            //环境湿度值
   pCab->HUAWEIDevValue.strhwEnvHumidity[1]="2147483647";            //环境湿度值
   //直流空调
//	pCab->HUAWEIDevValue.strhwDcAirCtrlMode[0]="2147483647";			//空调控制模式
//	pCab->HUAWEIDevValue.strhwDcAirCtrlMode[1]="2147483647";		   //空调控制模式
	pCab->HUAWEIDevValue.strhwDcAirRunStatus[0]="2147483647";			//空调运行状态
	pCab->HUAWEIDevValue.strhwDcAirRunStatus[1]="2147483647";			//空调运行状态
	pCab->HUAWEIDevValue.strhwDcAirCompressorRunStatus[0]="2147483647";		//空调压缩机运行状态
	pCab->HUAWEIDevValue.strhwDcAirCompressorRunStatus[1]="2147483647";		//空调压缩机运行状态
	pCab->HUAWEIDevValue.strhwDcAirInnrFanSpeed[0]="2147483647";			//空调内机转速
	pCab->HUAWEIDevValue.strhwDcAirInnrFanSpeed[1]="2147483647";			//空调内机转速
	pCab->HUAWEIDevValue.strhwDcAirOuterFanSpeed[0]="2147483647";			//空调外风机转速
	pCab->HUAWEIDevValue.strhwDcAirOuterFanSpeed[1]="2147483647";			//空调外风机转速
	pCab->HUAWEIDevValue.strhwDcAirCompressorRunTime[0]="2147483647";		//空调压缩机运行时间
	pCab->HUAWEIDevValue.strhwDcAirCompressorRunTime[1]="2147483647";		//空调压缩机运行时间
	pCab->HUAWEIDevValue.strhwDcAirEnterChannelTemp[0]="2147483647";		//空调回风口温度
	pCab->HUAWEIDevValue.strhwDcAirEnterChannelTemp[1]="2147483647";		//空调回风口温度
	pCab->HUAWEIDevValue.strhwDcAirPowerOnTempPoint[0]="2147483647";		//空调开机温度点
	pCab->HUAWEIDevValue.strhwDcAirPowerOnTempPoint[1]="2147483647";		//空调开机温度点
	pCab->HUAWEIDevValue.strhwDcAirPowerOffTempPoint[0]="2147483647";		//空调关机温度点
	pCab->HUAWEIDevValue.strhwDcAirPowerOffTempPoint[1]="2147483647";		//空调关机温度点
	
	//设备信息
	pCab->HUAWEIDevValue.strhwMonEquipSoftwareVersion="";	//软件版本
	pCab->HUAWEIDevValue.strhwMonEquipManufacturer="";		//设备生产商
	//锂电(新增加)
	pCab->HUAWEIDevValue.strhwAcbGroupTemperature="2147483647";			//电池温度
	pCab->HUAWEIDevValue.strhwAcbGroupOverCurThr="2147483647";			//充电过流告警点
	pCab->HUAWEIDevValue.strhwAcbGroupHighTempThr="2147483647";		//高温告警点
	pCab->HUAWEIDevValue.strhwAcbGroupLowTempTh="2147483647";			//低温告警点
	pCab->HUAWEIDevValue.strhwAcbGroupDodToAcidBattery="2147483647";	//锂电放电DOD
	//开关电源(新增加)
   	pCab->HUAWEIDevValue.strhwSetAcsUpperVoltLimit="2147483647";		//AC过压点设置
   	pCab->HUAWEIDevValue.strhwSetAcsLowerVoltLimit="2147483647";		//AC欠压点设置
   	pCab->HUAWEIDevValue.strhwSetDcsUpperVoltLimit="2147483647";		//设置DC过压点
   	pCab->HUAWEIDevValue.strhwSetDcsLowerVoltLimit="2147483647";		//设置DC欠压点
   	pCab->HUAWEIDevValue.strhwSetLvdVoltage="2147483647";				//设置LVD电压
	//环境传感器(新增加)
   	pCab->HUAWEIDevValue.strhwSetEnvTempUpperLimit[0]="2147483647";		//环境温度告警上限
   	pCab->HUAWEIDevValue.strhwSetEnvTempUpperLimit[1]="2147483647";		//环境温度告警上限
   	pCab->HUAWEIDevValue.strhwSetEnvTempLowerLimit[0]="2147483647";		//环境温度告警下限
   	pCab->HUAWEIDevValue.strhwSetEnvTempLowerLimit[1]="2147483647";		//环境温度告警下限
//   	pCab->HUAWEIDevValue.strhwSetEnvTempUltraHighTempThreshold="2147483647";		//环境高高温告警点
   	pCab->HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[0]="2147483647";		//环境湿度告警上限
   	pCab->HUAWEIDevValue.strhwSetEnvHumidityUpperLimit[1]="2147483647";		//环境湿度告警上限
   	pCab->HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[0]="2147483647";		//环境湿度告警下限
   	pCab->HUAWEIDevValue.strhwSetEnvHumidityLowerLimit[1]="2147483647";		//环境湿度告警下限
	//直流空调(新增加)
	pCab->HUAWEIDevValue.strhwDcAirRunTime[0]="2147483647";				//空调运行时间
	pCab->HUAWEIDevValue.strhwDcAirRunTime[1]="2147483647";				//空调运行时间
	pCab->HUAWEIDevValue.strhwCoolingDevicesMode="2147483647";			//温控模式
	
	//2019-08-20新增
	pCab->HUAWEIDevValue.strhwAcbGroupBatRunningState="2147483647";		//电池状态
	pCab->HUAWEIDevValue.strhwDcAirEquipAddress="2147483647";				//空调地址
	pCab->HUAWEIDevValue.strhwTemHumEquipAddress="2147483647";			//温湿度地址
	//单个锂电池2019-08-20新增
	pCab->HUAWEIDevValue.strhwAcbBatVolt="2147483647";					//单个电池电压
	pCab->HUAWEIDevValue.strhwAcbBatCurr="2147483647";					//单个电池电流
	pCab->HUAWEIDevValue.strhwAcbBatSoh="2147483647";						//单个电池串SOH
	pCab->HUAWEIDevValue.strhwAcbBatCapacity="2147483647";				//单个电池容量

#if(CABINETTYPE == 5 || CABINETTYPE == 6) //5:飞达中兴;6:金晟安
    //中兴机柜增加
    pCab->HUAWEIDevValue.RectifierModuleVol="2147483647";  //整流器输出电压
    pCab->HUAWEIDevValue.RectifierModuleCurr="2147483647"; //整流器输出电流
    pCab->HUAWEIDevValue.RectifierModuleTemp="2147483647";//整流器机内温度
	//Ups
	pCab->HUAWEIDevValue.StrUpsCityVol="2147483647";	 //Ups市电电压
	pCab->HUAWEIDevValue.StrUpsCityVol2="2147483647";	 //Ups2市电电压
	pCab->HUAWEIDevValue.StrUpsOVol="2147483647";		//Ups输出电压
	pCab->HUAWEIDevValue.StrUpsOVol2="2147483647";		//Ups2输出电压
	pCab->HUAWEIDevValue.StrUpsTemp="2147483647";		//Ups温度
	pCab->HUAWEIDevValue.StrUpsTemp2="2147483647";		//Ups2温度
    pCab->HUAWEIDevValue.StrUpsLoadout="2147483647";		//UPS负载百分比
	pCab->HUAWEIDevValue.StrUpsLoadout2="2147483647";		//UPS2负载百分比
	pCab->HUAWEIDevValue.StrUpsFreqin="2147483647";		//UPS输入频率
	pCab->HUAWEIDevValue.StrUpsFreqin2="2147483647";		//UPS2输入频率
	pCab->HUAWEIDevValue.StrUpsIsBypass="2147483647";		//UPS是否旁路
	pCab->HUAWEIDevValue.StrUpsIsBypass2="2147483647";	//UPS2是否旁路
	pCab->HUAWEIDevValue.StrUpsOnOff="2147483647";		//UPS开关机状态
	pCab->HUAWEIDevValue.StrUpsOnOff2="2147483647";		//UPS2开关机状态
	//空调
	pCab->HUAWEIDevValue.StrIn_FanState="2147483647";	//内风机状态 0代表关闭 1代表开启
	pCab->HUAWEIDevValue.StrIn_FanState2="2147483647";	//内风机状态 0代表关闭 1代表开启
	pCab->HUAWEIDevValue.StrOut_FanState="2147483647"; //外风机状态 0代表关闭 1代表开启
	pCab->HUAWEIDevValue.StrOut_FanState2="2147483647"; //外风机状态 0代表关闭 1代表开启

#elif(CABINETTYPE == 7) //爱特思
	//爱特思
	//Ups
	pCab->HUAWEIDevValue.StrUpsCityVol="2147483647";	 //Ups市电电压
	pCab->HUAWEIDevValue.StrUpsOVol="2147483647";		//Ups输出电压
	pCab->HUAWEIDevValue.StrUpsTemp="2147483647";		//Ups温度
    pCab->HUAWEIDevValue.StrUpsLoadout="2147483647";		//UPS负载百分比
	//空调
	pCab->HUAWEIDevValue.StrCoolerState="2147483647";	//制冷器状态 0代表关闭 1代表开启
	pCab->HUAWEIDevValue.StrIn_FanState="2147483647";	//内风机状态 0代表关闭 1代表开启
	pCab->HUAWEIDevValue.StrOut_FanState="2147483647"; //外风机状态 0代表关闭 1代表开启
	pCab->HUAWEIDevValue.StrHeaterState="2147483647";	//加热器状态  0代表关闭 1代表开启
#elif(CABINETTYPE == 8 || CABINETTYPE == 10) //诺龙/亚邦
    pCab->HUAWEIDevValue.StrUpsCityVol="2147483647";    //Ups市电电压
    pCab->HUAWEIDevValue.StrUpsCityVol2="2147483647";    //Ups市电电压
    pCab->HUAWEIDevValue.StrUpsOVol="2147483647";      //Ups输出电压
    pCab->HUAWEIDevValue.StrUpsOVol2="2147483647";      //Ups输出电压
    //空调
    pCab->HUAWEIDevValue.StrCoolerVol="2147483647";    //空调电压
    pCab->HUAWEIDevValue.StrCoolerVol2="2147483647";    //空调电压
    pCab->HUAWEIDevValue.StrCoolerCur="2147483647";    //空调电流
    pCab->HUAWEIDevValue.StrCoolerCur2="2147483647";    //空调电流
    pCab->HUAWEIDevValue.StrIn_FanState="2147483647";  //内风机状态 0代表关闭 1代表开启
    pCab->HUAWEIDevValue.StrIn_FanState2="2147483647";  //内风机状态 0代表关闭 1代表开启
    pCab->HUAWEIDevValue.StrOut_FanState="2147483647"; //外风机状态 0代表关闭 1代表开启
    pCab->HUAWEIDevValue.StrOut_FanState2="2147483647"; //外风机状态 0代表关闭 1代表开启
	
#elif(CABINETTYPE == 9) //广深沿江
	//Ups
	pCab->HUAWEIDevValue.StrUpsCityVol="2147483647";	 //Ups市电电压
	pCab->HUAWEIDevValue.StrUpsCityFreq="2147483647";		//Ups输出电压
	pCab->HUAWEIDevValue.StrUpsOVol="2147483647";		//Ups温度
	pCab->HUAWEIDevValue.StrUpsOAmp="2147483647";
	pCab->HUAWEIDevValue.StrUpsTemp="2147483647";
	pCab->HUAWEIDevValue.StrUpsBatVol="2147483647";
	pCab->HUAWEIDevValue.StrUpsBatLoad="2147483647";
	pCab->HUAWEIDevValue.StrUpsWorkMode="2147483647";
	pCab->HUAWEIDevValue.StrUpsIsTesting="2147483647";
	pCab->HUAWEIDevValue.UpsByPass="2147483647";
	
	//锂电0：设备柜电池，1：电源柜
	pCab->HUAWEIDevValue.strBatVol[0]="2147483647";
	pCab->HUAWEIDevValue.strBatVol[1]="2147483647";
	pCab->HUAWEIDevValue.strBatAmp[0]="2147483647";
	pCab->HUAWEIDevValue.strBatAmp[1]="2147483647";
	pCab->HUAWEIDevValue.strBatTotalCapacity[0]="2147483647";
	pCab->HUAWEIDevValue.strBatTotalCapacity[1]="2147483647";
	pCab->HUAWEIDevValue.strBatLeftCapacity[0]="2147483647";
	pCab->HUAWEIDevValue.strBatLeftCapacity[1]="2147483647";
	pCab->HUAWEIDevValue.strBatSoh[0]="2147483647";
	pCab->HUAWEIDevValue.strBatSoh[1]="2147483647";
	pCab->HUAWEIDevValue.strBatSoc[0]="2147483647";
	pCab->HUAWEIDevValue.strBatSoc[1]="2147483647";
	pCab->HUAWEIDevValue.strBatCycleCnt[0]="2147483647";
	pCab->HUAWEIDevValue.strBatCycleCnt[1]="2147483647";
	pCab->HUAWEIDevValue.strBatCellVolAve[0]="2147483647";
	pCab->HUAWEIDevValue.strBatCellVolAve[1]="2147483647";
	pCab->HUAWEIDevValue.strBatCellTemAve[0]="2147483647";
	pCab->HUAWEIDevValue.strBatCellTemAve[1]="2147483647";
	pCab->HUAWEIDevValue.strChargeStatus[0]="2147483647";
	pCab->HUAWEIDevValue.strChargeStatus[1]="2147483647";
	pCab->HUAWEIDevValue.strDisChargeStatus[0]="2147483647";
	pCab->HUAWEIDevValue.strDisChargeStatus[1]="2147483647";

	// 空调
	pCab->HUAWEIDevValue.strhwDcAirTECStatus="2147483647";
	pCab->HUAWEIDevValue.strhwDcAirTECStatus2="2147483647";
	pCab->HUAWEIDevValue.StrIn_FanState="2147483647";
	pCab->HUAWEIDevValue.StrIn_FanState2="2147483647";
	pCab->HUAWEIDevValue.StrOut_FanState="2147483647";
	pCab->HUAWEIDevValue.StrOut_FanState2="2147483647";
#elif(CABINETTYPE == 11)
	/* UPS */
	int i;
	pCab->HUAWEIDevValue.strUpsAcBypassVolPhA="2147483647";				//交流旁路电压ph_A 数据
	pCab->HUAWEIDevValue.strUpsAcBypassCurrentPhA="2147483647";			//交流旁路电流ph_A
	pCab->HUAWEIDevValue.strUpsAcBypassFrequencyPhA="2147483647";		//交流旁路频率ph_A
	pCab->HUAWEIDevValue.strUpsAcBypassPFA="2147483647";				//交流旁路 PF_A
	pCab->HUAWEIDevValue.strUpsAcInputVolPhA="2147483647";				//交流输入电压ph_A
	pCab->HUAWEIDevValue.strUpsAcInputCurrentPhA="2147483647";			//交流输入电流ph_A
	pCab->HUAWEIDevValue.strUpsAcInputFreqPhA="2147483647";				//交流输入频率ph_A
	pCab->HUAWEIDevValue.strUpsAcInputPFA="2147483647";					//交流输入PF_A
	pCab->HUAWEIDevValue.strUpsAcOutputVolPhA="2147483647";				//交流输出电压ph_A
	pCab->HUAWEIDevValue.strUpsAcOutputCurrentPhA="2147483647";			//交流输出电流ph_A
	pCab->HUAWEIDevValue.strUpsAcOutputFreqPhA="2147483647";			//交流输出频率ph_A
	pCab->HUAWEIDevValue.strUpsAcOutputPFA="2147483647";					//交流输出PF_A
	pCab->HUAWEIDevValue.strUpsOutputApparentPowerP_A="2147483647";			//输出视在功率ph_A
	pCab->HUAWEIDevValue.strUpsOutputActivePowerPhA="2147483647";			//输出有功功率ph_A
	pCab->HUAWEIDevValue.strUpsOutputReactivePowerPhA="2147483647";			//输出无功功率ph_A
	pCab->HUAWEIDevValue.strUpsLoadPercentagePhA="2147483647";			//负载百分数ph_A
	pCab->HUAWEIDevValue.strPositiveBatPackVol="2147483647";					//正电池组电压
	pCab->HUAWEIDevValue.strBatteryRemaindingTime="2147483647";					//电池剩余时间
	pCab->HUAWEIDevValue.strBatteryCapacity="2147483647";					//电池容量
	pCab->HUAWEIDevValue.strModuleNInverterVolA="2147483647";					//模块N逆变显示电压A
	pCab->HUAWEIDevValue.strModuleNBypassVolA="2147483647";					//模块N旁路显示电压A
	pCab->HUAWEIDevValue.strUpsSerialNum="2147483647";					//UPS系列号
	pCab->HUAWEIDevValue.strBatteryCycleCounts="2147483647";					//Battery cycle counts
	pCab->HUAWEIDevValue.strMaxCellTemperature="2147483647";					//Max cell temperature
	pCab->HUAWEIDevValue.strMinCellTemperature="2147483647";					//Min cell temperature
	pCab->HUAWEIDevValue.strEnvironmentTemperature="2147483647";					//Environment temperatureii
	pCab->HUAWEIDevValue.strPowerSupplyMode="2147483647";					//供电方式
	pCab->HUAWEIDevValue.strBatteryStatus="2147483647";					//电池状态
	pCab->HUAWEIDevValue.strBatteryConnectionStatus="2147483647";					//电池连接状态
	pCab->HUAWEIDevValue.strMaintenanceBypassOpenState="2147483647"; //	维修旁路空开状态
	for(i=0;i<CELL_VOTAGE_NUM;i++)
		pCab->HUAWEIDevValue.strCellVoltage[i]="2147483647";					//	单体电池

	/*电表  4130919800001*/
	for(i=0;i<2;i++)
	{
		pCab->HUAWEIDevValue.strCurCombinedTotalActPower[i]="2147483647"; 		//	当前组合有功总电能
		pCab->HUAWEIDevValue.strCurTotalPositiveActPower[i]="2147483647"; 	//	当前正向有功总电能
		pCab->HUAWEIDevValue.strCurTotalReverseActPower[i]="2147483647"; 		//	当前反向有功总电能
		pCab->HUAWEIDevValue.strVoltage[i]="2147483647"; 								//	电压
		pCab->HUAWEIDevValue.strCurrent[i]="2147483647"; 								//	电流
		pCab->HUAWEIDevValue.strActivePower[i]="2147483647"; 							//	有功功率
		pCab->HUAWEIDevValue.strPowerFactor[i]="2147483647"; 							//	功率因数
		pCab->HUAWEIDevValue.strFrequency[i]="2147483647"; 							//	频率
		
		/*空调  4020729800001 [0:设备柜 1:电池柜]*/
		pCab->HUAWEIDevValue.strCoolSta[i]="2147483647"; //制冷状态
		pCab->HUAWEIDevValue.strHeaterSta[i]="2147483647"; //加热器状态
		pCab->HUAWEIDevValue.strInternalFanSta[i]="2147483647"; //内风机状态
		pCab->HUAWEIDevValue.strExternalFanSta[i]="2147483647"; //外风机状态
		pCab->HUAWEIDevValue.strEmergencyFanSta[i]="2147483647"; //应急风机状态
		pCab->HUAWEIDevValue.strCabinetTemperature[i]="2147483647"; //柜内温度
		pCab->HUAWEIDevValue.strEvaporatorTemperature[i]="2147483647"; //蒸发器温度
		pCab->HUAWEIDevValue.strCondenserTemperature[i]="2147483647"; //冷凝器温度
		pCab->HUAWEIDevValue.stRoutsideCabinetTemperature[i]="2147483647"; //柜外温度
		pCab->HUAWEIDevValue.strAcVoltage[i]="2147483647"; //交流电压
		pCab->HUAWEIDevValue.strDcVoltage[i]="2147483647"; //直流电压
		pCab->HUAWEIDevValue.strTemperature[i]="2147483647"; //湿度值
		pCab->HUAWEIDevValue.strHeatStartTem[i]="2147483647"; //加热开启温度
		pCab->HUAWEIDevValue.strHeatStptTem[i]="2147483647"; //加热停止温度
		pCab->HUAWEIDevValue.strCabinetHighTemWarn[i]="2147483647"; //柜内温度高温告警点
		pCab->HUAWEIDevValue.strCabinetLowTemWarn[i]="2147483647"; //柜内温度低温告警点
		pCab->HUAWEIDevValue.strCondensertHighTemProtectVal[i]="2147483647"; //冷凝器温度高温保护点
		pCab->HUAWEIDevValue.strEvaporatorFreezingProtectVal[i]="2147483647"; //蒸发器温度冻结保护点
		pCab->HUAWEIDevValue.strDehumidificationStartVal[i]="2147483647"; //除湿开启湿度
		pCab->HUAWEIDevValue.strDehumidificationStopVal[i]="2147483647"; //除湿停止湿度
		pCab->HUAWEIDevValue.strHumidityWarniVal[i]="2147483647"; //湿度告警限值
		pCab->HUAWEIDevValue.strHumidityCorrectionValue[i]="2147483647"; //湿度校正值
	}

#endif
}

//初始化机柜告警结构体
void initHUAWEIALARM(CabinetClient *pCab)
{
	pCab->HUAWEIDevAlarm.hwEnvTempAlarmTraps="0";		//设备柜高温/低温告警
	pCab->HUAWEIDevAlarm.hwEnvTempAlarmTraps2="0";		//电池柜高温/低温告警
	pCab->HUAWEIDevAlarm.hwEnvHumiAlarmTraps="0";		//设备柜高湿/低湿告警
	pCab->HUAWEIDevAlarm.hwEnvHumiAlarmTraps2="0";		//电池柜高湿/低湿告警
	pCab->HUAWEIDevAlarm.hwSpareDigitalAlarmTraps="0";	//输入干接点告警
	pCab->HUAWEIDevAlarm.hwDoorAlarmTraps="0";		//门禁告警(设备柜)
	pCab->HUAWEIDevAlarm.hwDoorAlarmTraps2="0";		//门禁告警(电池柜)
	pCab->HUAWEIDevAlarm.hwWaterAlarmTraps="0";		//水浸告警(设备柜)
	pCab->HUAWEIDevAlarm.hwWaterAlarmTraps2="0";		//水浸告警(电池柜)
	pCab->HUAWEIDevAlarm.hwSmokeAlarmTraps="0";		//烟感告警(设备柜)
	pCab->HUAWEIDevAlarm.hwSmokeAlarmTraps2="0";		//烟感告警(电池柜)
	pCab->HUAWEIDevAlarm.hwair_cond_infan_alarm="0";		//空调内风机故障
	pCab->HUAWEIDevAlarm.hwair_cond_infan_alarm2="0";		//空调内风机故障
	pCab->HUAWEIDevAlarm.hwair_cond_outfan_alarm="0";		//空调外风机故障
	pCab->HUAWEIDevAlarm.hwair_cond_outfan_alarm2="0";		//空调外风机故障
	pCab->HUAWEIDevAlarm.hwair_cond_comp_alarm="0";		//空调压缩机故障
	pCab->HUAWEIDevAlarm.hwair_cond_comp_alarm2="0";		//空调压缩机故障
	pCab->HUAWEIDevAlarm.hwair_cond_return_port_sensor_alarm="0";		//空调回风口传感器故障
	pCab->HUAWEIDevAlarm.hwair_cond_return_port_sensor_alarm2="0";		//空调回风口传感器故障
	pCab->HUAWEIDevAlarm.hwair_cond_evap_freezing_alarm="0";		//空调蒸发器冻结
	pCab->HUAWEIDevAlarm.hwair_cond_evap_freezing_alarm2="0";		//空调蒸发器冻结
	pCab->HUAWEIDevAlarm.hwair_cond_freq_high_press_alarm="0";		//空调频繁高压力
	pCab->HUAWEIDevAlarm.hwair_cond_freq_high_press_alarm2="0";		//空调频繁高压力
	pCab->HUAWEIDevAlarm.hwair_cond_comm_fail_alarm="0";		//空调通信失败告警
	pCab->HUAWEIDevAlarm.hwair_cond_comm_fail_alarm2="0";		//空调通信失败告警
	//新增加告警
	pCab->HUAWEIDevAlarm.hwACSpdAlarmTraps="0";					//交流防雷器故障
	pCab->HUAWEIDevAlarm.hwDCSpdAlarmTraps="0";					//直流防雷器故障
	//电源告警
	pCab->HUAWEIDevAlarm.hwAcInputFailAlarm="0";				//交流电源输入停电告警
	pCab->HUAWEIDevAlarm.hwAcInputL1FailAlarm="0";				//交流电源输入L1	相缺相告警
	pCab->HUAWEIDevAlarm.hwAcInputL2FailAlarm="0";				//交流电源输入L2	相缺相告警
	pCab->HUAWEIDevAlarm.hwAcInputL3FailAlarm="0";				//交流电源输入L3	相缺相告警
	pCab->HUAWEIDevAlarm.hwDcVoltAlarmTraps="0";				//直流电源输出告警
	pCab->HUAWEIDevAlarm.hwLoadLvdAlarmTraps="0";				//LLVD1下电告警
	//锂电池告警
	pCab->HUAWEIDevAlarm.hwAcbGroup_comm_fail_alarm="0";		//所有锂电通信失败
	pCab->HUAWEIDevAlarm.hwAcbGroup_discharge_alarm="0";		//电池放电告警
	pCab->HUAWEIDevAlarm.hwAcbGroup_charge_overcurrent_alarm="0";	//电池充电过流
	pCab->HUAWEIDevAlarm.hwAcbGroup_temphigh_alarm="0";		//电池温度高
	pCab->HUAWEIDevAlarm.hwAcbGroup_temphigh_alarm2="0";		//电池温度高
	pCab->HUAWEIDevAlarm.hwAcbGroup_templow_alarm="0";		//电池温度低
	pCab->HUAWEIDevAlarm.hwAcbGroup_templow_alarm2="0";		//电池温度低
	pCab->HUAWEIDevAlarm.hwAcbGroup_poweroff_alarm="0";		//电池下电
	pCab->HUAWEIDevAlarm.hwAcbGroup_fusebreak_alarm="0";		//电池熔丝断
	pCab->HUAWEIDevAlarm.hwAcbGroup_moduleloss_alarm="0";		//模块丢失
	
#if(CABINETTYPE == 5) //飞达中兴
    //中兴机柜增加开关电源报警
    pCab->HUAWEIDevAlarm.SwitchPowerCom_alarm="0";  //开关电源断线告警
    pCab->HUAWEIDevAlarm.RectifierModuleCom_alarm="0"; //整流模块通讯故障
    //空调
    pCab->HUAWEIDevAlarm.Air_High_temper_alarm="0";  //高温告警
    pCab->HUAWEIDevAlarm.Air_Lower_temper_alarm="0"; //低温告警
    pCab->HUAWEIDevAlarm.Air_Heater_alarm="0";       //加热器故障告警
    pCab->HUAWEIDevAlarm.Air_Temper_Sensor_alarm="0";       //温度传感器故障
#elif(CABINETTYPE == 6) //金晟安
    //空调
	pCab->HUAWEIDevAlarm.AcbgroupLowVoltAlarm="0";	//电池电压低报警
    pCab->HUAWEIDevAlarm.Ups_Alarm="0";  //ups故障状态
    pCab->HUAWEIDevAlarm.Ups_Alarm2="0";  //ups故障状态
    pCab->HUAWEIDevAlarm.Air_High_temper_alarm="0";  //高温告警
    pCab->HUAWEIDevAlarm.Air_Lower_temper_alarm="0"; //低温告警
    pCab->HUAWEIDevAlarm.Air_Heater_alarm="0";       //加热器故障告警
    pCab->HUAWEIDevAlarm.Air_Temper_Sensor_alarm="0";       //温度传感器故障
#elif(CABINETTYPE == 7) //爱特思
    pCab->HUAWEIDevAlarm.Ups_alarm="0";  //ups故障状态
    //空调
    pCab->HUAWEIDevAlarm.Air_Cooler_alarm="0";       //制冷器故障告警
    pCab->HUAWEIDevAlarm.Air_High_temper_alarm="0";  //高温告警
    pCab->HUAWEIDevAlarm.Air_Lower_temper_alarm="0"; //低温告警
    pCab->HUAWEIDevAlarm.Air_Heater_alarm="0";       //加热器故障告警
    pCab->HUAWEIDevAlarm.Air_Temper_Sensor_alarm="0";       //温度传感器故障
    pCab->HUAWEIDevAlarm.Air_High_Vol_alarm="0"; //电压高压告警
    pCab->HUAWEIDevAlarm.Air_Lower_Vol_alarm="0";//电压低压告警
#elif(CABINETTYPE == 8 || CABINETTYPE == 10) //诺龙/亚邦
    pCab->HUAWEIDevAlarm.Ups_alarm="0";  //ups故障状态（设备柜）
    pCab->HUAWEIDevAlarm.Ups_alarm2="0";  //ups故障状态（电池柜）
    pCab->HUAWEIDevAlarm.Ups_city_off_alarm="0";  //ups市电断开（设备柜）
    pCab->HUAWEIDevAlarm.Ups_city_off_alarm2="0";  //ups市电断开（电池柜）
    //空调
    pCab->HUAWEIDevAlarm.Air_High_temper_alarm="0";  //高温告警（设备柜）
    pCab->HUAWEIDevAlarm.Air_High_temper_alarm2="0";  //高温告警（电池柜）
    
#elif(CABINETTYPE == 9) //广深沿江 荣尊堡
	//Ups
	pCab->HUAWEIDevAlarm.UpsCityVolAbn_alarm="0";	// Ups市电电压异常
	pCab->HUAWEIDevAlarm.UpsBatLowVol_alarm="0"; 	// ups电池电压低
	pCab->HUAWEIDevAlarm.Ups_alarm="0";				// Ups故障
	pCab->HUAWEIDevAlarm.Ups_On_off_alarm="0";		//1:关机 0:开机
	
	// 开关电源
	pCab->HUAWEIDevAlarm.DC_Power_alarm="0";	  // 开关电源主报警
	pCab->HUAWEIDevAlarm.Bat_alarm="0";		// 电池状态  0:正常，1：告警
	
	// 锂电池,0:设备柜，1：电源柜
	pCab->HUAWEIDevAlarm.Over_Vol_alarm[0]="0";	 // 过压保护
	pCab->HUAWEIDevAlarm.Over_Vol_alarm[1]="0";	 // 过压保护
	pCab->HUAWEIDevAlarm.Under_Vol_alarm[0]="0";	 // 欠压保护
	pCab->HUAWEIDevAlarm.Under_Vol_alarm[1]="0";	 // 欠压保护
	pCab->HUAWEIDevAlarm.Chg_Over_Amp_alarm[0]="0";	 // 充电过流保护
	pCab->HUAWEIDevAlarm.Chg_Over_Amp_alarm[1]="0";	 // 充电过流保护
	pCab->HUAWEIDevAlarm.Dsc_Over_Amp_alarm[0]="0";	 // 放电过流保护
	pCab->HUAWEIDevAlarm.Dsc_Over_Amp_alarm[1]="0";	 // 放电过流保护
	pCab->HUAWEIDevAlarm.Shortage_alarm[0]="0";		 // 短路保护
	pCab->HUAWEIDevAlarm.Shortage_alarm[1]="0";		 // 短路保护
	pCab->HUAWEIDevAlarm.Over_Tem_alarm[0]="0";		 // 过温保护
	pCab->HUAWEIDevAlarm.Over_Tem_alarm[1]="0";		 // 过温保护
	pCab->HUAWEIDevAlarm.Under_Tem_alarm[0]="0";		 // 欠温保护
	pCab->HUAWEIDevAlarm.Under_Tem_alarm[1]="0";		 // 欠温保护
	pCab->HUAWEIDevAlarm.Capicity_Left_alarm[0]="0";		 // 剩余容量告警
	pCab->HUAWEIDevAlarm.Capicity_Left_alarm[1]="0";		 // 剩余容量告警
	
	
	// 温湿度状态
	pCab->HUAWEIDevAlarm.Temp_status="0";	// 温度状态 0：正常，1：异常
	pCab->HUAWEIDevAlarm.Moist_status="0";	// 湿度状态 0：正常，1：异常
	
	// 门禁,有4个，原来的为前门禁，新加的为后门禁
	pCab->HUAWEIDevAlarm.hwBackDoorAlarmTraps="0";		//后门禁告警(设备柜)
	pCab->HUAWEIDevAlarm.hwBackDoorAlarmTraps2="0";		//后门禁告警(电池柜)
	
	// TEC故障
	pCab->HUAWEIDevAlarm.DcAirTEC_alarm="0";// 设备柜TEC运行故障
	pCab->HUAWEIDevAlarm.DcAirTEC_alarm2="0";// 电源柜TEC运行故障
#elif(CABINETTYPE == 11) //艾特网能
	//Ups
	int i,j;
	for(i=0;i<2;i++)
	{
		for(j=0;j<2;j++)
		{
			pCab->HUAWEIDevAlarm.strSpdAlarmTraps[i][j]="0";				//防雷器故障[0:设备柜 1:电池柜] [0:防雷1 1:防雷2]
			pCab->HUAWEIDevAlarm.strDoorAlarmTraps[i][j]="\"0\"";			//门禁告警[0:设备柜 1:电池柜] [0:前门电子锁 1:后门电子锁]
		}
	}
	/* USP警告 */
	pCab->HUAWEIDevAlarm.strEPO="0"; //	EPO
	pCab->HUAWEIDevAlarm.strInadequateStartupCapacityInverters="0"; //	逆变器启动容量不足
	pCab->HUAWEIDevAlarm.strGeneratorAccess="0"; //	发电机接入
	pCab->HUAWEIDevAlarm.strACInputFailure="0"; //	交流输入故障
	pCab->HUAWEIDevAlarm.strBypassPhaseSequenceFail="0"; //	旁路相序故障
	pCab->HUAWEIDevAlarm.strBypassVolFail="0"; //	旁路电压故障
	pCab->HUAWEIDevAlarm.strBypassFail="0"; //	旁路故障
	pCab->HUAWEIDevAlarm.strBypassOverload="0"; //	旁路过载
	pCab->HUAWEIDevAlarm.strBypassOverloadOT="0"; //	旁路过载超时
	pCab->HUAWEIDevAlarm.strBypassHypertracking="0"; //	旁路超跟踪
	pCab->HUAWEIDevAlarm.strOutputShortCircuit="0"; //	输出短路
	pCab->HUAWEIDevAlarm.strBatteryEOD="0"; //	电池EOD
	pCab->HUAWEIDevAlarm.strBatSelfCheckSta="0"; //	电池自检状态
	pCab->HUAWEIDevAlarm.strStartupProhibited="0"; //	禁止开机
	pCab->HUAWEIDevAlarm.strManualBypass="0"; //	手动旁路
	pCab->HUAWEIDevAlarm.strBatteryLowVol="0"; //	电池低压
	pCab->HUAWEIDevAlarm.strBatteryReversal="0"; //	电池接反
	pCab->HUAWEIDevAlarm.strInputNWireDisconnec="0"; //	输入N线断开
	pCab->HUAWEIDevAlarm.strBypassFanFail="0"; //	旁路风扇故障
	pCab->HUAWEIDevAlarm.strEODSystemProhibit="0"; //	EOD系统禁止
	pCab->HUAWEIDevAlarm.strCTWeldingReverse="0"; //	CT焊反
	
	for(i=0;i<2;i++)
	{
		/* 空调 [0:设备柜 1:电池柜] */
		pCab->HUAWEIDevAlarm.strCabinetIntSensorFault[i]="0"; //柜内传感器故障
		pCab->HUAWEIDevAlarm.strEvaporatorTemperatureSensorFault[i]="0"; //蒸发器温度传感器故障
		pCab->HUAWEIDevAlarm.strCondenserTemSensorFault[i]="0"; //冷凝器温度传感器故障
		pCab->HUAWEIDevAlarm.strCabinetExtSensorFault[i]="0"; //柜外温度传感器故障
		pCab->HUAWEIDevAlarm.strHumiditySensorFault[i]="0"; //湿度传感器故障
		pCab->HUAWEIDevAlarm.strHighHumidityWarn[i]="0"; //湿度过高告警
		pCab->HUAWEIDevAlarm.strRefrigerationWarn[i]="0"; //制冷告警
		pCab->HUAWEIDevAlarm.strHighTemperatureWarn[i]="0"; //高温告警
		pCab->HUAWEIDevAlarm.strLowTemperatureWarn[i]="0"; //低温告警
		pCab->HUAWEIDevAlarm.strHeaterWarn[i]="0"; //加热器告警
		pCab->HUAWEIDevAlarm.strOverVoltageWarn[i]="0"; //过电压告警
		pCab->HUAWEIDevAlarm.strUnderVoltageWarn[i]="0"; //欠电压告警
	}
#endif
}
//初始化防火墙结构体
void initHUAWEIEntity(CfirewallClient *pfw)
{
	pfw->HUAWEIDevValue.hwEntityTimeStamp=timestamp_get();
	pfw->HUAWEIDevValue.hwEntityLinked=false;
	//防火墙
	pfw->HUAWEIDevValue.strhwEntityCpuUsage="2147483647";				//CPU 
	pfw->HUAWEIDevValue.strhwEntityMemUsage ="2147483647";			   //内存使用率
	pfw->HUAWEIDevValue.strhwEntityTemperature="2147483647"; 		   //温度
	pfw->HUAWEIDevValue.strhwEntityFactory="";				 //生产商 
	pfw->HUAWEIDevValue.strhwEntityDevModel="";				  //设备型号 
}

//初始化交换机结构体
void initHUAWEIswitchEntity(CswitchClient *psw)
{
	psw->HUAWEIDevValue.hwswitchEntityTimeStamp=timestamp_get();
	psw->HUAWEIDevValue.hwswitchEntityLinked=false;
	//交换机
	psw->HUAWEIDevValue.strhwswitchEntityCpuUsage="2147483647";			//CPU 
	psw->HUAWEIDevValue.strhwswitchEntityMemUsage="2147483647";			//内存使用率
	psw->HUAWEIDevValue.strhwswitchEntityTemperature="2147483647";		//温度
	psw->HUAWEIDevValue.strhwswitchEntityFactory=""; 			   //生产商 
	psw->HUAWEIDevValue.strhwswitchEntityDevModel="";				//设备型号 
}

void init_atlas_struct(CsshClient *pAtlas)
{
	//初始化
	pAtlas->stuAtlasState.TimeStamp=timestamp_get();
	pAtlas->stuAtlasState.Linked=false;
	
	pAtlas->stuAtlasState.stratlasdata = "{\"isonline\":\"0\"}";//连接状态
}


//初始化SPD
void init_SPD(SpdClient *pSpd, VMCONTROL_CONFIG *pConf)
{
	if(pSpd==NULL) return;

	pSpd->SetSpdSetconfig(OnSpdSetconfigBack, 100);
	pSpd->SetSpdWriteconfig(OnSpdSetWriteconfig, 100);

    //防雷器配置
    pSpd->StrSPDType = pConf->StrSPDType;	//SPD类型
	pSpd->SPD_Type = pConf->SPD_Type;

    pSpd->StrSPDCount = pConf->StrSPDCount;	//SPD数量
	pSpd->SPD_num = pConf->SPD_num;

	// 防雷检测
	for(int i=0;i<SPD_NUM;i++)
	{
		pSpd->StrSPDAddr[i] = pConf->StrSPDAddr[i];
		pSpd->SPD_Address[i] = pConf->SPD_Address[i];

		pSpd->StrSPDIP[i] = pConf->StrSPDIP[i];	//SPD IP 地址
		pSpd->StrSPDPort[i] = pConf->StrSPDPort[i];	//SPD端口
		sprintf(pSpd->gsSPDIP[i],"%s",pConf->StrSPDIP[i].c_str());
		sprintf(pSpd->gsSPDPort[i],"%s",pConf->StrSPDPort[i].c_str());
	}
	// 接地电阻只有1个
	pSpd->StrSPDAddr[SPD_NUM] = pConf->StrSPDAddr[SPD_NUM];
	pSpd->SPD_Address[SPD_NUM] = pConf->SPD_Address[SPD_NUM];
	sprintf(pSpd->gsSPDIP[SPD_NUM],"%s",pConf->StrSPDIP[SPD_NUM].c_str());
	sprintf(pSpd->gsSPDPort[SPD_NUM],"%s",pConf->StrSPDPort[SPD_NUM].c_str());

	// 接地电阻如果是网络型的
	pSpd->StrSPDIP[SPD_NUM] = pConf->StrSPDIP[SPD_NUM];			//接地电阻 IP 地址
	pSpd->StrSPDPort[SPD_NUM] = pConf->StrSPDPort[SPD_NUM];			//接地电阻端口

	//初始化防雷器结构体
	memset(pSpd->stuSpd_Param,0,sizeof(SPD_PARAMS));

	pSpd->StartNetSpd();
}


void Init_DO(VMCONTROL_CONFIG *pConf)
{
	/////////////////  DO配置表开始/////////////////////////////////////////////
	int i,temp = 0;	//统计到底有几个DO
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

}

int TrapCallBack(string Stroid,int AlarmID,int mgetIndex,unsigned int mRetID)
{
    int mnew = 0;
	printf("TrapAlarmBack oid=%s, ID=%d, Index=%d\r\n",Stroid.c_str(),AlarmID,mgetIndex);
}

/* 华为RSU回调函数 */
void HuaweiCallback(void *data, void *userdata) {
    Huawei::RsuInfo_S *info = (Huawei::RsuInfo_S *)data;

    printf("cpuRate = %s\n", info->cpuRate.c_str());
    printf("memRate = %s\n", info->memRate.c_str());
    printf("status = %s\n", info->status.c_str());
    printf("temperture = %s\n", info->temperture.c_str());
    printf("softVer = %s\n", info->softVer.c_str());
    printf("hardVer = %s\n", info->hardVer.c_str());
    printf("seriNum = %s\n", info->seriNum.c_str());
    printf("longitude = %s\n", info->longitude.c_str());
    printf("latitude = %s\n", info->latitude.c_str());
}

/* 门锁回调函数 */
void LockCallback(uint8_t addr, Lock::Info_S info, void *userdata) {
    printf("Lock(%d) isOpen:%02X Card{%02X %02X %02X %02X} Event:%02X isAuthorCard:%02X\n", addr, info.isOpen, info.cardId[0], info.cardId[1], info.cardId[2], info.cardId[3], info.event,
           info.isAuthorCard);
}

void UartCallback(uint8_t port, uint8_t *buf, uint32_t len, void *userdata) {
    int i;
    printf("Uart %d receive len = %d ,data: { ", port, len);
    for (i = 0; i < len; i++)
        printf("%02X ", buf[i]);
    printf(" }\n");
}

/* 空调回调函数 */
void AirConditionCallback(AirCondition::AirInfo_S info, void *userdata){
    // printf("Air condition callback\n");
    int i;
    
    printf("Air condition callback!! ver:%s\n",info.version.c_str());

    printf("Run state:\n");
    for (i = 0; i < sizeof(AirCondition::Runsta_S) / sizeof(string); i++) {
        printf("  %s\n",((string *)&info.runSta)[i].c_str());
    }

    printf("\nSensor state:\n");
    for (i = 0; i < sizeof(AirCondition::SensorSta_S) / sizeof(string); i++) {
        printf("  %s\n",((string *)&info.sensorSta)[i].c_str());
    }

    printf("\nWarning:\n");
    for (i = 0; i < sizeof(AirCondition::Warning_S) / sizeof(string); i++) {
        printf("  %s\n",((string *)&info.warnings)[i].c_str());
    }

    printf("\nSystem value:\n");
    for (i = 0; i < sizeof(AirCondition::SysVal_S) / sizeof(string); i++) {
        printf("  %s\n",((string *)&info.sysVal)[i].c_str());
    }
}

/* 温湿度回调函数 */
void TemHumiCallback(uint8_t addr,TemHumi::Info_S info,void *userdata){
    printf("TemHumiCallback tempture = %f,humidity = %f,dew = %f\n",info.tempture,info.humidity,info.dew);
}

/* IO设备回调函数 */
void IODevCallback(IODev::DevType_EN type,bool sta,void *userdata){
    printf("type = %d , sta = %d\n",type,sta);
}




