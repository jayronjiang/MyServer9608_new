#ifndef __SNMP_H__
#define __SNMP_H__

#include <string> 
#include "server.h" 
//#include "config.h"


using namespace std; 

#define HWGANTRY_COUNT 54

int snmpInit(void);

int snmpInit(void);
int SendSnmpOid(string mSnmpOid);
int snmptrapInit(void);
int mywalkappinit(void);
int SendWalkSnmpOid(string mSnmpOid);
int ClearvecWalkSnmp(void);


#pragma pack(push, 1)
typedef enum
{
	//锂电
	hwAcbGroupBatVolt=1,                //电池电压
	hwAcbGroupBatCurr=2,            //电池电流
	hwAcbGroupTotalCapacity=3,                //电池总容量
	hwAcbGroupTotalRemainCapacity=4,               //电池剩余容量
	hwAcbGroupBackupTime=5,              //电池备电时长
	hwAcbGroupBatSoh=6,             //电池 SOH
	//开关电源
	hwApOrAblVoltage=7,                //A/AB 电压
	hwBpOrBclVoltage=8,                //B/BC 电压
	hwCpOrCalVoltage=9,                //C/CA 电压
	hwAphaseCurrent=10,               //A 相电流
	hwBphaseCurrent=11,              //B 相电流
	hwCphaseCurrent=12,             //C 相电流
	hwDcOutputVoltage=13,             //DC 输出电压
	hwDcOutputCurrent=14,               //DC 输出电流
	//环境传感器
	hwEnvTemperature=15,              //环境温度值
	hwEnvHumidity=16,             //环境湿度值
	//直流空调
	hwDcAirCtrlMode=17,			//空调控制模式
	hwDcAirRunStatus=18,			//空调运行状态
	hwDcAirCompressorRunStatus=19,		//空调压缩机运行状态
	hwDcAirInnrFanSpeed=20,			//空调内机转速
	hwDcAirOuterFanSpeed=21,			//空调外风机转速
	hwDcAirCompressorRunTime=22,		//空调压缩机运行时间
	hwDcAirEnterChannelTemp=23,		//空调回风口温度
	hwDcAirPowerOnTempPoint=24,		//空调开机温度点
	hwDcAirPowerOffTempPoint=25,		//空调关机温度点

	//设备信息
	hwMonEquipSoftwareVersion=26,	//软件版本
	hwMonEquipManufacturer=27,		//设备生产商
	//锂电(新增加)
	hwAcbGroupTemperature=28,			//电池温度
	hwAcbGroupOverCurThr=29,			//充电过流告警点
	hwAcbGroupHighTempThr=30,		//高温告警点
	hwAcbGroupLowTempTh=31,			//低温告警点
	hwAcbGroupDodToAcidBattery=32,	//锂电放电DOD
	//开关电源(新增加)
   	hwSetAcsUpperVoltLimit=33,		//AC过压点设置
   	hwSetAcsLowerVoltLimit=34,		//AC欠压点设置
   	hwSetDcsUpperVoltLimit=35,		//设置DC过压点
   	hwSetDcsLowerVoltLimit=36,		//设置DC欠压点
   	hwSetLvdVoltage=37,				//设置LVD电压
	//环境传感器(新增加)
   	hwSetEnvTempUpperLimit=38,		//环境温度告警上限
   	hwSetEnvTempLowerLimit=39,		//环境温度告警下限
   	hwSetEnvTempUltraHighTempThreshold=40,		//环境高高温告警点
   	hwSetEnvHumidityUpperLimit=41,		//环境湿度告警上限
   	hwSetEnvHumidityLowerLimit=42,		//环境湿度告警下限
	//直流空调(新增加)
	hwDcAirRunTime=43,				//空调运行时间
	hwCoolingDevicesMode=44,			//温控模式

	//2019-08-20新增
	hwAcbGroupBatRunningState=45,		//电池状态
	hwSmokeSensorStatus=46,				//烟雾传感器状态
	hwWaterSensorStatus=47,				//水浸传感器状态
	hwDoorSensorStatus=48,				//门磁传感器状态
    //hwDcAirEquipAddress=49,				//空调地址
    //hwTemHumEquipAddress=50,			//温湿度地址
	//单个锂电池
    hwAcbBatVolt=49,					//单个电池电压
    hwAcbBatCurr=50,					//单个电池电流
    hwAcbBatSoh=51,						//单个电池串SOH
    hwAcbBatCapacity=52,				//单个电池容量
    //2019-09-05新增
    hwCtrlSmokeReset=53,				 //控制烟感复位 1,2，255
    hwCtrlMonEquipReset=54,             //控制单板复位 1,2,255

    hwDcAirEquipAddress=8000,			//空调地址
    hwTemHumEquipAddress=8001,			//温湿度地址


    //防火墙
    hwEntityCpuUCheck = 10000,             //查询
    hwEntityCpuUsage = 10001,              //CPU
    hwEntityMemUsage = 10002,                 //内存使用率
    hwEntityTemperature = 10003,               //温度
    hwEntityDescr = 10004,                  //接口查询
    hwEntityOperStatus = 10005,             //接口状态查询
    hwEntityInOctets = 10006,               //总字节数
    hwEntityInErrors = 10007,               //出错数
    hwEntityOutOctets = 10008,               //总字节数
    hwEntityOutErrors = 10009,               //出错数
    hwEntityDevModel = 10010,                //型号


    //交换机
    hwswitchEntityCpuCheck = 11000,             //查询
    hwswitchEntityCpuUsage = 11001,              //CPU
    hwswitchEntityMemUsage = 11002,              //内存使用率
    hwswitchEntityTemperature = 11003,            //温度
    hwswitchEntityDescr = 11004,                  //接口查询
    hwswitchEntityOperStatus = 11005,             //接口状态查询
    hwswitchEntityInOctets = 11006,               //总字节数
    hwswitchEntityInErrors = 11007,               //出错数
    hwswitchEntityOutOctets = 11008,               //总字节数
    hwswitchEntityOutErrors = 11009,               //出错数
    hwswitchEntityDevModel = 11010,               //型号


    //迪普防火墙
    dpEntityCpuUCheck = 12000,             //查询
    dpEntityCpuUsage = 12001,              //CPU
    dpEntityMemUsage = 12002,                 //内存使用率
    dpEntityTemperature = 12003,               //温度
    dpEntityDescr = 12004,                  //接口查询
    dpEntityOperStatus = 12005,             //接口状态查询
    dpEntityInOctets = 12006,               //总字节数
    dpEntityInErrors = 12007,               //出错数
    dpEntityOutOctets = 12008,               //总字节数
    dpEntityOutErrors = 12009,               //出错数
    dpEntityDevModel = 12010,                //型号
	
    //华三交换机
    hsswitchEntityCpuCheck = 13000,             //查询
    hsswitchEntityCpuUsage = 13001,              //CPU
    hsswitchEntityMemUsage = 13002,              //内存使用率
    hsswitchEntityTemperature = 13003,            //温度
    hsswitchEntityDescr = 13004,                  //接口查询
    hsswitchEntityOperStatus = 13005,             //接口状态查询
    hsswitchEntityInOctets = 13006,               //总字节数
    hsswitchEntityInErrors = 13007,               //出错数
    hsswitchEntityOutOctets = 13008,               //总字节数
    hsswitchEntityOutErrors = 13009,               //出错数
    hsswitchEntityDevModel = 13010,             //型号


    //深信服防火墙
    sfEntityCpuUCheck = 14000,             //查询
    sfEntityCpuUsage = 14001,              //CPU
    sfEntityMemUsage = 14002,                 //内存使用率
    sfEntityTemperature = 14003,               //温度
    sfEntityDescr = 14004,                  //接口查询
    sfEntityOperStatus = 14005,             //接口状态查询
    sfEntityInOctets = 14006,               //总字节数
    sfEntityInErrors = 14007,               //出错数
    sfEntityOutOctets = 14008,               //总字节数
    sfEntityOutErrors = 14009,               //出错数
    sfEntityDevModel = 14010                //型号


}EM_HUAWEIGantry;

typedef struct
{
	unsigned long hwTimeStamp; 		//机柜状态获取时间戳
	bool hwLinked;					//设备柜机柜状态 0:断开 1：连接
//	bool hwLinked2;					//电源柜机柜状态 0:断开 1：连接
	bool AcbGroupBatOnline;			//锂电池组是否在线
	bool DcAirOnline;				//直流空调是否在线（设备柜）
	bool DcAirOnline2;				//直流空调是否在线（电池柜）
	bool TemHumOnline;				//温湿度传感器是否在线（设备柜）
	bool TemHumOnline2;				//温湿度传感器是否在线（电池柜）
	
	//锂电
	string strhwAcbGroupBatVolt;                //电池电压 "51.1"
	string strhwAcbGroupBatCurr;            //电池电流
	string strhwAcbGroupTotalCapacity;                //电池总容量
	string strhwAcbGroupTotalRemainCapacity;               //电池剩余容量
	string strhwAcbGroupBackupTime;              //电池备电时长
	string strhwAcbGroupBatSoh;             //电池 SOH
	//开关电源
	string strhwApOrAblVoltage;                //A/AB 电压
	string strhwBpOrBclVoltage;                //B/BC 电压
	string strhwCpOrCalVoltage;                //C/CA 电压
	string strhwAphaseCurrent;               //A 相电流
	string strhwBphaseCurrent;              //B 相电流
	string strhwCphaseCurrent;             //C 相电流
	string strhwDcOutputVoltage;             //DC 输出电压
	string strhwDcOutputCurrent;               //DC 输出电流
	//环境传感器
    string strhwEnvTemperature[2];              //环境温度值
    string strhwEnvHumidity[2];            //环境湿度值
	//直流空调
//    string strhwDcAirCtrlMode[2];			//空调控制模式
    string strhwDcAirRunStatus[2];			//空调运行状态
    string strhwDcAirCompressorRunStatus[2];		//空调压缩机运行状态
    string strhwDcAirInnrFanSpeed[2];			//空调内机转速
    string strhwDcAirOuterFanSpeed[2];			//空调外风机转速
    string strhwDcAirCompressorRunTime[2];		//空调压缩机运行时间
    string strhwDcAirEnterChannelTemp[2];		//空调回风口温度
    string strhwDcAirPowerOnTempPoint[2];		//空调开机温度点
    string strhwDcAirPowerOffTempPoint[2];		//空调关机温度点
	
	//设备信息 
	string strhwMonEquipSoftwareVersion;	//软件版本
	string strhwMonEquipManufacturer;		//设备生产商
	//锂电(新增加)
	string strhwAcbGroupTemperature;			//电池温度
	string strhwAcbGroupOverCurThr;			//充电过流告警点
	string strhwAcbGroupHighTempThr;		//高温告警点
	string strhwAcbGroupLowTempTh;			//低温告警点
	string strhwAcbGroupDodToAcidBattery;	//锂电放电DOD
	//开关电源(新增加)
   	string strhwSetAcsUpperVoltLimit;		//AC过压点设置
   	string strhwSetAcsLowerVoltLimit;		//AC欠压点设置
   	string strhwSetDcsUpperVoltLimit;		//设置DC过压点
   	string strhwSetDcsLowerVoltLimit;		//设置DC欠压点
   	string strhwSetLvdVoltage;				//设置LVD电压
	//环境传感器(新增加)
    string strhwSetEnvTempUpperLimit[2];		//环境温度告警上限
    string strhwSetEnvTempLowerLimit[2];		//环境温度告警下限
   	string strhwSetEnvTempUltraHighTempThreshold;		//环境高高温告警点
    string strhwSetEnvHumidityUpperLimit[2];		//环境湿度告警上限
    string strhwSetEnvHumidityLowerLimit[2];		//环境湿度告警下限
	//直流空调(新增加)
    string strhwDcAirRunTime[2];				//空调运行时间
	string strhwCoolingDevicesMode;			//温控模式
	//2019-08-20新增
	string strhwAcbGroupBatRunningState;		//电池状态
	string strhwDcAirEquipAddress;				//空调地址
	string strhwTemHumEquipAddress;			//温湿度地址
	//单个锂电池2019-08-20新增
	string strhwAcbBatVolt;					//单个电池电压
	string strhwAcbBatCurr;					//单个电池电流
	string strhwAcbBatSoh;						//单个电池串SOH
	string strhwAcbBatCapacity;				//单个电池容量
    //2019-09-05新增
    string strhwCtrlSmokeReset;             //控制烟感复位 1,2，255
    string strhwCtrlMonEquipReset;          //控制单板复位 1,2,255

    //防火墙
	unsigned long hwEntityTimeStamp; 		//防火墙获取时间戳
	bool hwEntityLinked;					//连接状态
    string strhwEntityCpuUsage;                //CPU 
    string strhwEntityMemUsage ;              //内存使用率
    string strhwEntityTemperature;            //温度
	unsigned long hwEntityTimeStamp1; 		//防火墙获取时间戳
	bool hwEntityLinked1;					//连接状态
    string strhwEntityCpuUsage1;                //CPU
    string strhwEntityMemUsage1 ;              //内存使用率
    string strhwEntityTemperature1;            //温度
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


#if(CABINETTYPE == 5 || CABINETTYPE == 6) //5:飞达中兴;6:金晟安
    //中兴机柜增加
    string RectifierModuleVol;  //整流器输出电压
    string RectifierModuleCurr; //整流器输出电流
    string RectifierModuleTemp;//整流器机内温度
    //Ups
    string StrUpsCityVol;    //Ups市电电压
    string StrUpsCityVol2;    //Ups市电电压
    string StrUpsOVol;      //Ups输出电压
    string StrUpsOVol2;      //Ups输出电压
    string StrUpsTemp;      //Ups温度
    string StrUpsTemp2;      //Ups温度
    string StrUpsLoadout;		//UPS负载百分比
	string StrUpsLoadout2;		//UPS2负载百分比
	string StrUpsFreqin;		//UPS输入频率
	string StrUpsFreqin2;		//UPS2输入频率
	string StrUpsIsBypass;		//UPS是否旁路
	string StrUpsIsBypass2;	//UPS2是否旁路
	string StrUpsOnOff;		//UPS开关机状态
	string StrUpsOnOff2;		//UPS2开关机状态
    //空调
    string StrIn_FanState;  //内风机状态 0代表关闭 1代表开启
    string StrIn_FanState2;  //内风机状态 0代表关闭 1代表开启
    string StrOut_FanState; //外风机状态 0代表关闭 1代表开启
    string StrOut_FanState2; //外风机状态 0代表关闭 1代表开启

#elif(CABINETTYPE == 7) //爱特思
    //爱特思
    //Ups
    string StrUpsCityVol;    //Ups市电电压
    string StrUpsOVol;      //Ups输出电压
    string StrUpsTemp;      //Ups温度
    string StrUpsLoadout;		//UPS负载百分比
    //空调
    string StrCoolerState;  //制冷器状态 0代表关闭 1代表开启
    string StrIn_FanState;  //内风机状态 0代表关闭 1代表开启
    string StrOut_FanState; //外风机状态 0代表关闭 1代表开启
    string StrHeaterState;  //加热器状态  0代表关闭 1代表开启
#elif(CABINETTYPE == 8 || CABINETTYPE == 10) //诺龙/亚邦
    //Ups
    string StrUpsCityVol;    //Ups市电电压
    string StrUpsCityVol2;    //Ups市电电压
    string StrUpsOVol;      //Ups输出电压
    string StrUpsOVol2;      //Ups输出电压
    //空调
    string StrCoolerVol;    //空调电压
    string StrCoolerVol2;    //空调电压
    string StrCoolerCur;    //空调电流
    string StrCoolerCur2;    //空调电流
    string StrIn_FanState;  //内风机状态 0代表关闭 1代表开启
    string StrIn_FanState2;  //内风机状态 0代表关闭 1代表开启
    string StrOut_FanState; //外风机状态 0代表关闭 1代表开启
    string StrOut_FanState2; //外风机状态 0代表关闭 1代表开启
#elif(CABINETTYPE == 9) //广深沿江
    //Ups
    string StrUpsCityVol;    // Ups市电电压
    string StrUpsCityFreq;	 // ups输入频率
    string StrUpsOVol;      //Ups输出电压
    string StrUpsOAmp;      //Ups输出电流
    string StrUpsTemp;      //Ups温度
    string StrUpsBatVol;      //Ups电池电压
    string StrUpsBatLoad;      //Ups电池负载 %比
	string StrUpsWorkMode;      //Ups工作模式     0:在线式，1:后备式
	string StrUpsIsTesting;      //Ups是否测试     0:否，1:是
	string UpsByPass;      //	Ups旁路模式

	//锂电0：设备柜电池，1：电源柜
	string strBatVol[2];               	 	//电池电压 "51.1"
	string strBatAmp[2];            		//电池电流
	string strBatTotalCapacity[2];          //电池总容量
	string strBatLeftCapacity[2];           //电池剩余容量
	string strBatSoh[2];             			//电池 SOH，%老化程度
	string strBatSoc[2];             			//电池 SOC，%剩余电荷
	string strBatCycleCnt[2];             		//电池 循环次数
	string strBatCellVolAve[2];					// 平均电芯电压,注意单位是V，已经转了
	string strBatCellTemAve[2];					// 平均电芯温度
	string strChargeStatus[2];					// 充电中0：是，1：否
	string strDisChargeStatus[2];				// 放电中0：是，1：否

	// 空调
	string strhwDcAirTECStatus; // 设备柜TEC运行状态 0代表关闭 1代表开启
	string strhwDcAirTECStatus2; // 电源柜TEC运行状态
	string StrIn_FanState;	//设备柜内风机状态 0代表关闭 1代表开启
	string StrIn_FanState2;  //内风机状态 0代表关闭 1代表开启
	string StrOut_FanState; //电源柜外风机状态 0代表关闭 1代表开启
	string StrOut_FanState2; //外风机状态 0代表关闭 1代表开启

#elif(CABINETTYPE == 11) //艾特网

#define EQUIPMENT_CABINET						(0)
#define BATTERY_CABINET							(1)

//电表
#define WATT_HOUR_METER							"4130919800001"
//空调
#define AIR_CONDITION							"4020729800001"
//UPS
#define UPS_1									"4010719800001"
//温湿度
#define TEMPERATURE_HUMIDITY					"2060719800001"
//烟感
#define SMOKE_SENSATION							"2030119800001"
//水浸
#define FLOOD_SENSATION							"2050119800001"
//防雷1
#define SPD_ALARM_1								"2060119800001"
//防雷2
#define SPD_ALARM_2								"2060119800002"
//前门电子锁
#define FRONT_DOOR_LOCK							"6050819800001"
//后门电子锁
#define BACK_DOOR_LOCK							"6050819800002"


//UPS独立电池数
#define	CELL_VOTAGE_NUM							(30)

	/* UPS */
	string strUpsAcBypassVolPhA;				//交流旁路电压ph_A 数据
	string strUpsAcBypassCurrentPhA;			//交流旁路电流ph_A
	string strUpsAcBypassFrequencyPhA;		//交流旁路频率ph_A
	string strUpsAcBypassPFA;				//交流旁路 PF_A
	string strUpsAcInputVolPhA;				//交流输入电压ph_A
	string strUpsAcInputCurrentPhA;			//交流输入电流ph_A
	string strUpsAcInputFreqPhA;				//交流输入频率ph_A
	string strUpsAcInputPFA;					//交流输入PF_A
	string strUpsAcOutputVolPhA;				//交流输出电压ph_A
	string strUpsAcOutputCurrentPhA;			//交流输出电流ph_A
	string strUpsAcOutputFreqPhA;			//交流输出频率ph_A
	string strUpsAcOutputPFA;					//交流输出PF_A
	string strUpsOutputApparentPowerP_A;			//输出视在功率ph_A
	string strUpsOutputActivePowerPhA;			//输出有功功率ph_A
	string strUpsOutputReactivePowerPhA;			//输出无功功率ph_A
	string strUpsLoadPercentagePhA;			//负载百分数ph_A
	string strPositiveBatPackVol;					//正电池组电压
	string strBatteryRemaindingTime;					//电池剩余时间
	string strBatteryCapacity;					//电池容量
	string strModuleNInverterVolA;					//模块N逆变显示电压A
	string strModuleNBypassVolA;					//模块N旁路显示电压A
	string strUpsSerialNum;					//UPS系列号
	string strBatteryCycleCounts;					//Battery cycle counts
	string strMaxCellTemperature;					//Max cell temperature
	string strMinCellTemperature;					//Min cell temperature
	string strEnvironmentTemperature;					//Environment temperatureii
	string strPowerSupplyMode;					//供电方式
	string strBatteryStatus;					//电池状态
	string strBatteryConnectionStatus;					//电池连接状态
	string strMaintenanceBypassOpenState; //	维修旁路空开状态
	string strCellVoltage[CELL_VOTAGE_NUM];					//	单体电池

	/*电表  4130919800001*/
	string strCurCombinedTotalActPower[2]; 		//	当前组合有功总电能
	string strCurTotalPositiveActPower[2]; 	//	当前正向有功总电能
	string strCurTotalReverseActPower[2]; 		//	当前反向有功总电能
	string strVoltage[2]; 								//	电压
	string strCurrent[2]; 								//	电流
	string strActivePower[2]; 							//	有功功率
	string strPowerFactor[2]; 							//	功率因数
	string strFrequency[2]; 							//	频率

	/*空调  4020729800001 [0:设备柜 1:电池柜]*/
	string strCoolSta[2]; //制冷状态
	string strHeaterSta[2]; //加热器状态
	string strInternalFanSta[2]; //内风机状态
	string strExternalFanSta[2]; //外风机状态
	string strEmergencyFanSta[2]; //应急风机状态
	string strCabinetTemperature[2]; //柜内温度
	string strEvaporatorTemperature[2]; //蒸发器温度
	string strCondenserTemperature[2]; //冷凝器温度
	string stRoutsideCabinetTemperature[2]; //柜外温度
	string strAcVoltage[2]; //交流电压
	string strDcVoltage[2]; //直流电压
	string strTemperature[2]; //湿度值
	string strHeatStartTem[2]; //加热开启温度
	string strHeatStptTem[2]; //加热停止温度
	string strCabinetHighTemWarn[2]; //柜内温度高温告警点
	string strCabinetLowTemWarn[2]; //柜内温度低温告警点
	string strCondensertHighTemProtectVal[2]; //冷凝器温度高温保护点
	string strEvaporatorFreezingProtectVal[2]; //蒸发器温度冻结保护点
	string strDehumidificationStartVal[2]; //除湿开启湿度
	string strDehumidificationStopVal[2]; //除湿停止湿度
	string strHumidityWarniVal[2]; //湿度告警限值
	string strHumidityCorrectionValue[2]; //湿度校正值



#endif

}THUAWEIGantry;

typedef struct
{
	unsigned long timestamp; 		//获取时间戳
	string hwEnvTempAlarmTraps;		//高温/低温/高高温告警
    string hwEnvTempAlarmTraps2;		//高温/低温/高高温告警
	string hwEnvHumiAlarmTraps;		//高湿/低湿告警
    string hwEnvHumiAlarmTraps2;		//高湿/低湿告警
	string hwSpareDigitalAlarmTraps;	//输入干接点告警
	string hwDoorAlarmTraps;		//门禁告警(设备柜)
	string hwDoorAlarmTraps2;		//门禁告警(电池柜)
	string hwWaterAlarmTraps;		//水浸告警(设备柜)
	string hwWaterAlarmTraps2;		//水浸告警(电池柜)
	string hwSmokeAlarmTraps;		//烟感告警(设备柜)
	string hwSmokeAlarmTraps2;		//烟感告警(电池柜)

	string hwair_cond_infan_alarm;	//空调内风机故障
    string hwair_cond_infan_alarm2;	//空调内风机故障
	string hwair_cond_outfan_alarm;	//空调外风机故障
    string hwair_cond_outfan_alarm2;//空调外风机故障
	string hwair_cond_comp_alarm;	//空调压缩机故障
    string hwair_cond_comp_alarm2;	//空调压缩机故障
	string hwair_cond_return_port_sensor_alarm;	//空调回风口传感器故障
    string hwair_cond_return_port_sensor_alarm2;//空调回风口传感器故障
	string hwair_cond_evap_freezing_alarm;		//空调蒸发器冻结
    string hwair_cond_evap_freezing_alarm2;		//空调蒸发器冻结
	string hwair_cond_freq_high_press_alarm;	//空调频繁高压力
    string hwair_cond_freq_high_press_alarm2;	//空调频繁高压力
	string hwair_cond_comm_fail_alarm;			//空调通信失败告警
    string hwair_cond_comm_fail_alarm2;			//空调通信失败告警

	//新增加告警
	string hwACSpdAlarmTraps;					//交流防雷器故障
	string hwDCSpdAlarmTraps;					//直流防雷器故障
	//电源告警
	string hwAcInputFailAlarm;				//交流电源输入停电告警
	string hwAcInputL1FailAlarm;				//交流电源输入L1	相缺相告警
	string hwAcInputL2FailAlarm;				//交流电源输入L2	相缺相告警
	string hwAcInputL3FailAlarm;				//交流电源输入L3	相缺相告警
	string hwDcVoltAlarmTraps;				//直流电源输出告警
	string hwLoadLvdAlarmTraps;				//LLVD1下电告警
	//锂电池告警
	string hwAcbGroup_comm_fail_alarm;		//所有锂电通信失败
	string hwAcbGroup_discharge_alarm;		//电池放电告警
	string hwAcbGroup_charge_overcurrent_alarm;	//电池充电过流
	string hwAcbGroup_temphigh_alarm;		//电池温度高
    string hwAcbGroup_temphigh_alarm2;		//电池温度高
	string hwAcbGroup_templow_alarm;		//电池温度低
    string hwAcbGroup_templow_alarm2;		//电池温度低
	string hwAcbGroup_poweroff_alarm;		//电池下电
	string hwAcbGroup_fusebreak_alarm;		//电池熔丝断
	string hwAcbGroup_moduleloss_alarm;		//模块丢失

    //单个锂电池告警
    string hwAcbCom_Failure[4];		        //锂电通信失败
    string hwAcbHeater_Fault_alarm[4];		//加热器故障
    string hwAcbBoard_Hardware_Fault_alarm[4];	//单板故障
    string hwAcbLow_Temp_Protection_alarm[4];		//低温保护

    #if(CABINETTYPE == 5 || CABINETTYPE == 6) //飞达中兴
    //中兴机柜增加开关电源报警
    string SwitchPowerCom_alarm;  //开关电源断线告警
    string RectifierModuleCom_alarm; //整流模块通讯故障
	string AcbgroupLowVoltAlarm;	//电池电压低报警
	string Ups_Alarm;		//ups故障状态报警
	string Ups_Alarm2;		//Ups2故障状态报警
    //空调
    string Air_High_temper_alarm;  //高温告警
    string Air_Lower_temper_alarm; //低温告警
    string Air_Heater_alarm;       //加热器故障告警
    string Air_Temper_Sensor_alarm;       //温度传感器故障
    #elif(CABINETTYPE == 7) //爱特思
    string Ups_alarm;  //ups故障状态
    //空调
    string Air_Cooler_alarm;       //制冷器故障告警
    string Air_High_temper_alarm;  //高温告警
    string Air_Lower_temper_alarm; //低温告警
    string Air_Heater_alarm;       //加热器故障告警
    string Air_Temper_Sensor_alarm;       //温度传感器故障
    string Air_High_Vol_alarm; //电压高压告警
    string Air_Lower_Vol_alarm;//电压低压告警
    #elif(CABINETTYPE == 8 || CABINETTYPE == 10) //诺龙/亚邦
    string Ups_alarm;  //ups故障状态（设备柜）
    string Ups_alarm2;  //ups故障状态（电池柜）
    string Ups_city_off_alarm;  //ups市电断开（设备柜）
    string Ups_city_off_alarm2;  //ups市电断开（电池柜）
    //空调
    string Air_High_temper_alarm;  //高温告警（设备柜）
    string Air_High_temper_alarm2;  //高温告警（电池柜）


	#elif(CABINETTYPE == 9) //广深沿江
    //Ups
    string UpsCityVolAbn_alarm;    	// Ups市电电压异常
    string UpsBatLowVol_alarm;	 	// ups电池电压低
    string Ups_alarm;      			// Ups故障
    string Ups_On_off_alarm;      	//1:关机 0:开机

	// 开关电源
	string DC_Power_alarm;    // 开关电源主报警
	string Bat_alarm;		// 电池状态  0:正常，1：告警

	// 锂电池,0:设备柜，1：电源柜
	string Over_Vol_alarm[2];    // 过压保护
	string Under_Vol_alarm[2];	 // 欠压保护
	string Chg_Over_Amp_alarm[2];    // 充电过流保护
	string Dsc_Over_Amp_alarm[2];    // 放电过流保护
	string Shortage_alarm[2];    	 // 短路保护
	string Over_Tem_alarm[2];    	 // 过温保护
	string Under_Tem_alarm[2];    	 // 欠温保护
	string Capicity_Left_alarm[2];    	 // 剩余容量告警


	// 温湿度状态
	string Temp_status;		// 温度状态 0：正常，1：异常
	string Moist_status;	// 湿度状态 0：正常，1：异常

	// 门禁,有4个，原来的为前门禁，新加的为后门禁
	string hwBackDoorAlarmTraps;		//后门禁告警(设备柜)
	string hwBackDoorAlarmTraps2;		//后门禁告警(电池柜)

	// TEC故障
	string DcAirTEC_alarm; // 设备柜TEC运行故障
	string DcAirTEC_alarm2; // 电源柜TEC运行故障
#elif(CABINETTYPE == 11) //艾特网
	string strSpdAlarmTraps[2][2];				//防雷器故障[0:设备柜 1:电池柜] [0:防雷1 1:防雷2]
	string strDoorAlarmTraps[2][2];			//门禁告警[0:设备柜 1:电池柜] [0:前门电子锁 1:后门电子锁]

	/* USP警告 */
	string strEPO; //	EPO
	string strInadequateStartupCapacityInverters; //	逆变器启动容量不足
	string strGeneratorAccess; //	发电机接入
	string strACInputFailure; //	交流输入故障
	string strBypassPhaseSequenceFail; //	旁路相序故障
	string strBypassVolFail; //	旁路电压故障
	string strBypassFail; //	旁路故障
	string strBypassOverload; //	旁路过载
	string strBypassOverloadOT; //	旁路过载超时
	string strBypassHypertracking; //	旁路超跟踪
	string strOutputShortCircuit; //	输出短路
	string strBatteryEOD; //	电池EOD
	string strBatSelfCheckSta; //	电池自检状态
	string strStartupProhibited; //	禁止开机
	string strManualBypass; //	手动旁路
	string strBatteryLowVol; //	电池低压
	string strBatteryReversal; //	电池接反
	string strInputNWireDisconnec; //	输入N线断开
	string strBypassFanFail; //	旁路风扇故障
	string strEODSystemProhibit; //	EOD系统禁止
	string strCTWeldingReverse; //	CT焊反


	/* 空调 [0:设备柜 1:电池柜] */
	string strCabinetIntSensorFault[2]; //柜内传感器故障
	string strEvaporatorTemperatureSensorFault[2]; //蒸发器温度传感器故障
	string strCondenserTemSensorFault[2]; //冷凝器温度传感器故障
	string strCabinetExtSensorFault[2]; //柜外温度传感器故障
	string strHumiditySensorFault[2]; //湿度传感器故障
	string strHighHumidityWarn[2]; //湿度过高告警
	string strRefrigerationWarn[2]; //制冷告警
	string strHighTemperatureWarn[2]; //高温告警
	string strLowTemperatureWarn[2]; //低温告警
	string strHeaterWarn[2]; //加热器告警
	string strOverVoltageWarn[2]; //过电压告警
	string strUnderVoltageWarn[2]; //欠电压告警


#endif

}THUAWEIALARM;



typedef struct
{
  int Descr;
  string type;
  string state;
  string inoctets;
  string inerrors;
  string outoctets;
  string outerrors;


}TFIRESWITCH;




#pragma pack(pop)

int SendHUAWEIsnmp(EM_HUAWEIGantry mEM_HUAWEIGantry);
int SnmpSetOid(EM_HUAWEIGantry mEM_HUAWEIGantry,string mIntValue,int mIndex);
void UpdataHUAWEIGantryStr(char* mstr,int len,EM_HUAWEIGantry mIntHUAWEIGantry);
int DealAlarm(string Stroid,int AlarmID,int mgetIndex);
int GetAlarmID(char* sp);

#endif


