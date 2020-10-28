/**************************Copyright (c)****************************************************
 * 								
 * 						
 * 	
 * ------------------------文件信息---------------------------------------------------
 * 文件名:
 * 版	本:
 * 描	述: modbus协议的寄存器处理宏定义，用户接口
 * 
 * --------------------------------------------------------------------------------------- 
 * 作	者: 
 * 日	期: 
 * 版	本:
 * 描	述:
 ****************************************************************************************/

#ifndef	__REGISTERS_H
#define	__REGISTERS_H

#include <string>

using namespace std;

typedef signed char       	INT8;
typedef unsigned char       UINT8;
typedef signed short        INT16;
typedef unsigned short      UINT16;
typedef signed int      	INT32;
typedef unsigned int      	UINT32;
typedef unsigned long long  UINT64;

/*机柜类型定义*/
#define HUAWEI_DUALCAB_DUALDOOR		1		
#define HUAWEI_DUALCAB_SINGDOOR		2
#define HUAWEI_SINGCAB_DUALDOOR		3		
#define HUAWEI_SINGCAB_SINGDOOR		4
#define ZHONGXING_CAB				5		
#define JINSHENGAN_CAB				6		
#define AITESI_CAB					7		
#define NUOLONG_CAB					8		
#define RONGZHUN_CAB				9		
#define YABANG_CAB					10		
#define AITEWANG_CAB				11		
#define HUARUAN_CAB					12		

/*http 请求类型*/
#define HTTPGET						1		
#define HTTPPOST					2

/*寄存器属性定义*/
#define READONLY						0		/*只读寄存器*/
#define WRITEONLY					1		/*只写寄存器*/
#define WRITEREAD					2		/*R/W寄存器*/

/*操作寄存器的错误码*/
#define REG_OK						0x00		/*正确*/
#define FUNCCODE_ERR					0x01		/*不支持的功能码*/
#define REGADDR_ERR					0x02		/*寄存器地址非法*/
#define DATA_ERR						0x03		/*数据错误*/
#define OPERATION_ERR				0x04		/*无效操作*/

#define FRAME_HEAD_NUM 				3	/*MODBUS读数据时返回帧有效数据前数据个数*/


/*电源控制器地址*/
#define POWER_CTRL_ADDR			71		/*电源控制器地址*/

/*SPARK01装置寄存器的范围*/
/*实时数据寄存器*/
#define DATA_START_ADDR			0		/*数据寄存器起始地址*/
#define DATA_REG_MAX			68		/*本版本所支持的最大寄存器数*/

/*RSU天线数据寄存器*/
#define RSU_START_ADDR			0		/*RSU天线数据寄存器起始地址*/
#define RSU_REG_MAX			12		/*RSU天线数据寄存器数*/

/*UPS数据寄存器*/
#define UPS_START_ADDR			40		/*UPS数据寄存器起始地址*/
#define UPS_REG_MAX			39		/*UPS数据寄存器数*/

/*防雷器数据寄存器*/
#define SPD_START_ADDR			90		/*数据寄存器起始地址*/
#define SPD_REG_MAX			3		/*最大寄存器数*/

/*环境数据寄存器*/
#define ENVI_START_ADDR			300		/*环境数据寄存器起始地址*/
#define ENVI_REG_MAX			29		/*环境数据寄存器数量，暂定为11*/

/*装置信息寄存器*/
#define DEVICEINFO_START_ADDR			900
#define DEVICEINFO_REG_MAX				23

/*装置参数寄存器*/
#define PARAMS_START_ADDR		1200		/*设备参数寄存器开始地址*/
#define PARAMS_REG_MAX			5			/*本版本所支持的最大寄存器数*/ 

/*空调参数寄存器*/
#define AIRCOND_START_ADDR		1220		/*设备参数寄存器开始地址*/
#define AIRCOND_REG_MAX			5			/*本版本所支持的最大寄存器数*/ 

/*遥控寄存器*/
#define DO_START_ADDR					1500
#define DO_REG_MAX						49
#define REMOTE_RESET_REG					1548		/*遥控复位*/

//预留2路动环控制器
#define HWSERVER_NUM 2
//预留9路300万车牌识别
#define VEHPLATE_NUM 12
//预留3路900万车牌识别
#define VEHPLATE900_NUM 3
//预留2路RSU控制器
#define RSUCTL_NUM 2
//预留12路天线
#define ANTENNA_NUM 12
//预留2台交换机
#define IPSWITCH_NUM 2
//预留2台防火墙
#define FIREWARE_NUM 2
//预留4路CAM
#define CAM_NUM 4
//预留2路ATLAS
#define ATLAS_NUM 2
//预留16路防雷检测器，华咨有最多10路防雷检测
#define SPD_NUM 16
// 预留1路网口接地电阻
#define RES_NUM 1	
#define SPD_DEFALT_ADDR			0xF0	// 默认设备地址， 不配置就是1
#define NULL_VALUE				0		// 定义没有这个参数时的默认值
/*电源控制器地址*/
#define POWER_CTRL_ADDR_1			71		/*电源控制器地址*/
#define IO_CTRL_ADDR_1				61		/*IO控制器地址*/
/*电表地址*/
#define VA_STATION_ADDRESS_1	81
#define VA_STATION_ADDRESS_2	82
//预留2组温湿度
#define TEMHUMI_NUM 1
#define TEMHUMI_ADDRESS_1	0x01
#define TEMHUMI_ADDRESS_2	0x11
//预留1台空调
#define AIRCON_NUM 1
#define AIRCON_ADDRESS_1	0x06
#define AIRCON_ADDRESS_2	0x16

// 最大支持6个伏安表, 每个伏安表为6组电流电压值
#define VA_METER_BD_NUM		6
#define VA_PHASE_NUM 6
//最大支持4路电子门锁
#define LOCK_NUM			4
//最大支持3层电源板
#define POWER_BD_NUM			3
//最大支持36路开关数量
#define SWITCH_COUNT	36			
//最大支持16路无接线设备数量
#define UNWIRE_SWITCH_COUNT	16			
//最大支持4路232串口
#define RS232_NUM			4
//最大支持4路485串口
#define RS485_CNT			4


/*功能码*/
#define	READ_REGS				0x03           //读寄存器
#define	PRESET_REGS			0x10                   //写寄存器
#define	FORCE_COIL				0x05           //设置继电器输出状态

//遥控寄存器定义 老控制板
#define RSU1_REG			1500		//RSU天线1 0xFF00: 遥合;0xFF01: 遥分
#define DOOR_DO_REG			1501 		//电子门锁 0xFF00: 关锁;0xFF01: 开锁
#define AUTORECLOSURE_REG	1502		//自动重合闸0xFF00: 遥合;0xFF01: 遥分
//#define VPLATE1_REG			1503		//车牌识别1 0xFF00: 遥合;0xFF01: 遥分

//遥控寄存器定义 新12DO控制板
#define VPLATE1_REG			1500		//车牌识别1 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE2_REG			1501		//车牌识别2 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE3_REG			1502		//车牌识别3 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE4_REG			1503		//车牌识别4 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE5_REG			1504		//车牌识别5 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE6_REG			1505		//车牌识别6 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE7_REG			1506		//车牌识别7 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE8_REG			1507		//车牌识别8 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE9_REG			1508		//车牌识别9 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE10_REG			1509		//车牌识别10 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE11_REG			1510		//车牌识别11 0xFF00: 遥合; 0xFF01: 遥分
#define VPLATE12_REG			1511		//车牌识别12 0xFF00: 遥合; 0xFF01: 遥分

#define AIRCONDSET_REG			1220		//空调开关机寄存器1220
#define AIRCOLDSTARTPOINT_REG 	1221		//空调制冷点//1221 			
#define AIRCOLDLOOP_REG			1222		//空调制冷回差//1222		
#define AIRHOTSTARTPOINT_REG 	1223		//空调制热点//1223 
#define AIRHOTLOOP_REG			1224		//空调制热回差//1224	

#define SYSRESET		1		//系统重启

//遥控开关定义
#define	ACT_HOLD		0           //保持状态
#define	ACT_CLOSE		1           //分闸
#define	ACT_OPEN		2           //合闸
#define	ACT_SOFTWARERST 3           //软重启
#define	ACT_HARDWARERST 4           //断电重启
#define	ACT_HOLD_FF		255           //保持状态

/*使能定义*/
#define	SWITCH_ON		0xFF00           //合闸
#define	SWITCH_OFF		0xFF01           //分闸

//门锁开关定义
#define	ACT_LOCK		1           //关锁
#define	ACT_UNLOCK		2           //开锁


#pragma pack(push, 1)
typedef enum BAUDRATE
{
    BAUDRATE_1200      = 0,			//1200
    BAUDRATE_2400		=1,
	BAUDRATE_4800		=2,
	BAUDRATE_9600		=3,
	BAUDRATE_19200		=4,
	BAUDRATE_115200		=5
}BAUDRATE;

/*寄存器对象定义*/
typedef struct __Register__
{
	UINT16 *pAddr;				/*寄存器地址*/
	UINT8 bAtrrib;				/*寄存器属性定义RO,WO,RW*/
	UINT16 nLenth;				/*寄存器长度*/
}Reg_Object;

/*modubs寄存器列表列表定义*/
typedef struct __Modbus_Map_Reg
{
	UINT16 nRegNo;				/*寄存器号*/
	Reg_Object reg;				/*寄存器号所对应的寄存器*/
} Map_Reg_Table;


/*//每路RSU天线数据结构体
typedef struct va_phase_struct
{
	UINT16 vln;		//RSU天线电压
	UINT16 amp;		//RSU天线电流
	UINT16 enable;	// 投切标志
}VA_PHASE_PARAMS;


//RSU天线数据结构体
typedef struct va_meter_struct
{
	UINT16 address;
	VA_PHASE_PARAMS phase[VA_PHASE_NUM];		// 每个伏安表6路电流电压值
	UINT32 TimeStamp; 		//获取时间戳
	bool Linked;
}VA_METER_PARAMS;*/


// 电源板的数据状态
typedef struct power_stamp_struct
{
	UINT32 TimeStamp; 		//获取时间戳
	bool Linked;
}POWER_STAMP_PARAMS;


//装置参数寄存器
typedef struct device_params_struct	/*共384个字节*/
{
	UINT16 Address;		// 终端设备地址设置 1200
	UINT16 BaudRate_1;	// 串口1波特率 1201
	UINT16 BaudRate_2;	// 串口2波特率 1202
	UINT16 BaudRate_3;	// 串口3波特率 1203
	UINT16 BaudRate_4;	// 串口4波特率 1204
}DEVICE_PARAMS;

//控制器参数结构体
#define TYPE_LEIXUN		1	// 与SPDCLIENT.H要保持一致
typedef struct vmctl_config_struct
{
	string STRCONFIG = "";
	string STRWIFIWAP = "";
	
	string user;			//登录用户
	string password;		//密码
	string authority;		//用户权限
	string token;			//票据
	string StrstationID;	//虚拟站编号
	string StrstationName;	//虚拟站名
	string StrNET;			//网络方式
	string StrDHCP; 		//是否DHCP
	string StrIP;			//IP地址
	string StrMask; 		//子网掩码
	string StrGateway;		//网关
	string StrDNS;			//DNS地址
	
	string StrIP2;			//IP2地址
	string StrMask2;			//子网掩码
	string StrGateway2; 	//网关
	string StrDNS2; 		//DNS地址
	
    string StrCabinetType;            //机柜类型  1:华为；2:利通
    //门架信息
    string StrFlagNetRoadID;    //ETC 门架路网编号
    string StrFlagRoadID ;      //ETC 门架路段编号
    string StrFlagID;            //ETC 门架编号
    string StrPosId;             //ETC 门架序号
    string StrDirection;         //行车方向
    string StrDirDescription;    //行车方向说明

    //参数设置
    string StrHWServerCount;	//华为动环服务器数量
    string StrHWCabinetCount;	//华为机柜数量
    string StrHWServer;         //华为服务器IP地址
    string StrHWGetPasswd;      //SNMP GET 密码
    string StrHWSetPasswd;      //SNMP SET 密码
    string StrHWServer2;         //金晟安服务器IP地址
    string StrHWGetPasswd2;      //金晟安 SNMP GET 密码
    string StrHWSetPasswd2;      //金晟安 SNMP SET 密码
    string StrServerURL1;      //服务器1推送地址
    string StrServerURL2;      //服务器2推送地址
    string StrServerURL3;      //服务器3推送地址
    string StrServerURL4;      //门锁4推送地址
    string StrStationURL;      //控制器接收地址
    
    string StrRSUCount;            //RSU数量
	int RSUCount;	//RSU数量
    string StrRSUType;            //RSU类型 1：门架通用RSU；2：示范工程华为RSU
    string StrRSUIP[RSUCTL_NUM];            //RSU控制器IP地址
    string StrRSUPort[RSUCTL_NUM];          //RSU控制器端口
    string StrVehPlateCount;            //识别仪数量
    string StrVehPlateIP[VEHPLATE_NUM];      //识别仪IP地址(预留12路)
    string StrVehPlatePort[VEHPLATE_NUM];    //识别仪端口(预留12路)
    string StrVehPlateKey[VEHPLATE_NUM];    //识别仪用户名密码(预留12路)
    string StrVehPlate900Count;            //900识别仪数量
    string StrVehPlate900IP[VEHPLATE900_NUM];      //900识别仪IP地址(预留12路)
    string StrVehPlate900Port[VEHPLATE900_NUM];    //900识别仪端口(预留12路)
    string StrVehPlate900Key[VEHPLATE900_NUM];    //900识别仪用户名密码(预留12路)
    string StrCAMCount;            //RSU数量
    string StrCAMIP[CAM_NUM];            //监控摄像头IP地址
    string StrCAMPort[CAM_NUM];          //监控摄像头端口(预留4路CAM)
    string StrCAMKey[CAM_NUM];            //监控摄像头用户名密码

    string StrFireWareType;				//防火墙类型
	int IntFireWareType = 1; //防火墙类型 1：华为,2：迪普，3：深信服
    string StrFireWareCount;            //防火墙数量
    string StrFireWareIP[FIREWARE_NUM];       //防火墙地址(预留2台)
    string StrFireWareGetPasswd[FIREWARE_NUM];   //防火墙get密码
    string StrFireWareSetPasswd[FIREWARE_NUM];   //防火墙set密码
    string StrIPSwitchType;				//交换机类型
	int IntIPSwitchType = 1; //交换机类型	1：华为,2：华三
    string StrIPSwitchCount;            //交换机数量
    string StrIPSwitchIP[IPSWITCH_NUM];         //交换机地址(预留2台)
    string StrIPSwitchGetPasswd[IPSWITCH_NUM];     //交换机get密码
    string StrIPSwitchSetPasswd[IPSWITCH_NUM];     //交换机set密码 
    string StrAtlasType;				//Atlas类型 1：Atlas,2：研华
    string StrAtlasCount;            //Atlas数量
    string StrAtlasIP[ATLAS_NUM];         //Atlas地址(预留2台)
    string StrAtlasPasswd[ATLAS_NUM];     //Atlas密码
	string StrSPDType;	//PSD厂家类型,1:雷迅,2:华咨,3...
	string StrSPDCount;	//PSD数量
	UINT8 SPD_Type = TYPE_LEIXUN;
	UINT8 SPD_num = SPD_NUM;
	string StrSPDIP[SPD_NUM+RES_NUM];	//SPD控制器IP地址
	string StrSPDPort[SPD_NUM+RES_NUM];	//SPD控制器端口
	string StrSPDAddr[SPD_NUM+RES_NUM];		//SPD控制器硬件地址
	string StrLeakAddr; 		//漏电检测装置地址
	string StrSpdRes_Alarm_Value;		//接地电阻监测器报警值
	UINT8 SPD_Address[SPD_NUM+RES_NUM] = 
	{
		SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
		SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
		SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
		SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
		SPD_DEFALT_ADDR
	};
	UINT8 HZ_reset_flag[SPD_NUM+RES_NUM] = {false,};
	UINT8 HZ_reset_pre[SPD_NUM+RES_NUM] = {false,}; // 对HZ_reset_flag预先处理
    
    string StrLockType;       //门锁类型
    string StrLockNum;       //门锁数量
    string StrAdrrLock[LOCK_NUM];         //门锁地址	最多4把锁
    string StrAdrrVAMeter[VA_METER_BD_NUM];     //电能表地址 最多6个表 每层2个
    string StrAdrrPower[POWER_BD_NUM];       //电源板地址 最多3层
    string StrDoCount;						//DO数量
    string StrDeviceNameSeq[SWITCH_COUNT];	//设备名称的配置 最多36个设备
    string StrDoSeq[SWITCH_COUNT];	//设备名称与 DO映射 最多36路开关，与设备名称对应
	UINT16 DoSeq[SWITCH_COUNT]={0,};	// 另外定义一个专门用来存储映射的数组,stuRemote_Ctrl会被清0
    string StrPoweroff_Or_Not[SWITCH_COUNT];	//DO市电停电时是否断电
	string StrUnWireDevName[SWITCH_COUNT];	//没接线设备名的配置
	string StrUnWireDo[SWITCH_COUNT];	//没接线设备DO配置

    //硬件信息
	string StrWIFIUSER; 	//WIIFI用户名
	string StrWIFIKEY;		//WIIFI密码
	
	string StrdeviceType="XY-TMC-001";	//设备型号
	string StrVersionNo;	//当前版本号
	string StrSoftDate; //当前版本日期
    string StrID;				//硬件ID
    string StrCpuAlarmValue;		//CPU使用率报警阈值
    string StrCpuTempAlarmValue;		//CPU温度报警阈值
    string StrMemAlarmValue; 		//内存使用率报警阈值
    string StrsecSoftVersion[3]; 		//副版本号，支持最多3个电源板
}VMCONTROL_CONFIG;

//电源控制设备的配置
typedef struct Control_CONFIG_struct	//
{
	UINT16 vehplate;	// 设备名称
	UINT16 do_seq;		// 对应的DO
}CONTROL_CONFIG;

//外设采集回调时间戳
typedef struct CallBackTimeStamp_struct	//
{
	unsigned long AirConTimeStamp; 		//空调状态获取时间戳
	unsigned long TempHumTimeStamp; 		//温湿度状态获取时间戳
}CallBackTimeStamp;

//遥控寄存器
typedef struct Remote_Control_struct	//
{
	UINT16 rsu1;				//1500 RSU天线1 0xFF00: 遥合;0xFF01: 遥分
	UINT16 door_do;				//1501 电子门锁 0xFF00: 关锁;0xFF01: 开锁
	UINT16 autoreclosure;		//1502 自动重合闸0xFF00: 遥合;0xFF01: 遥分
	
	UINT16 vehplate[VEHPLATE_NUM];			//车牌识别1 0xFF00: 遥合;0xFF01: 遥分
	UINT16 rsucontrlor[RSUCTL_NUM];			//rsu 0xFF00: 遥合;0xFF01: 遥分
	UINT16 antenna[ANTENNA_NUM];			//天线 0xFF00: 遥合;0xFF01: 遥分
	UINT16 fireware[FIREWARE_NUM];			//防火墙 0xFF00: 遥合;0xFF01: 遥分
	UINT16 ipswitch[IPSWITCH_NUM];			//交换机 0xFF00: 遥合;0xFF01: 遥分
	
	UINT16 doseq[SWITCH_COUNT];			//备用do 0xFF00: 遥合;0xFF01: 遥分

	UINT16 SysReset;			//系统重启 1548
	UINT16 FrontDoorCtrl;			//前门电子门锁 0：保持 1：关锁：2：开锁
	UINT16 BackDoorCtrl;			//后门电子门锁0：保持 1：关锁：2：开锁
	UINT16 SideDoorCtrl;			//侧门电子门锁0：保持 1：关锁：2：开锁
	UINT16 RightSideDoorCtrl;			//侧门电子门锁0：保持 1：关锁：2：开锁
	UINT16 AutoReclosure_Close;		//自动重合闸-合闸
	UINT16 AutoReclosure_Open;		//自动重合闸-分闸
	char systemtime[50];		//设置控制器时间
	
	UINT16 aircondset;		//空调关机//1220					1
	UINT16 aircoldstartpoint;		//空调制冷点//1221				50
	UINT16 aircoldloop; 	//空调制冷回差//1222					10
	UINT16 airhotstartpoint;		//空调制热点//1223				15
	UINT16 airhotloop;		//空调制热回差//1224					10

	INT16 hwctrlmonequipreset;		//控制单板复位 0：保持；1：热复位；
	INT16 hwsetacsuppervoltlimit;	//AC过压点设置	0:保持；50-600（有效）；280（缺省值）
	INT16 hwsetacslowervoltlimit;	//AC欠压点设置	0:保持；50-600（有效）；180（缺省值）
	INT16 hwsetdcsuppervoltlimit;	//设置DC过压点	0:保持；53-600（有效）；58（缺省值）
	INT16 hwsetdcslowervoltlimit;	//设置DC欠压点	0:保持；35 - 57（有效）；45（缺省值）
	INT16 hwsetenvtempupperlimit[2];	//环境温度告警上限 0:保持；25-80（有效）；55（缺省值）
	INT16 hwsetenvtemplowerlimit[2];	//环境温度告警下限255:保持；-20-20（有效）；-20（缺省值）
	INT16 hwsetenvhumidityupperlimit[2];	//环境湿度告警上限 255:保持；0-100（有效）；95（缺省值）
	INT16 hwsetenvhumiditylowerlimit[2];	//环境湿度告警下限 255:保持；0-100（有效）；5（缺省值）
	INT16 hwcoolingdevicesmode;		//温控模式				0：保持；1：纯风扇模式；2：纯空调模式；3：智能模式；
	INT16 hwdcairpowerontemppoint[2];		//空调开机温度点 255:保持； -20-80（有效）；45(缺省值)
	INT16 hwdcairpowerofftemppoint[2];		//空调关机温度点  		  255:保持； -20-80（有效）；37(缺省值)
	INT16 hwdcairctrlmode[2];			//空调控制模式 0：保持；1：自动；2：手动
	INT16 hwctrlsmokereset[2];			//控制烟感复位 0：保持；1：不需复位；2：复位

	float spd_modbus_addr[SPD_NUM];			//防雷器设备地址
	float spdleak_alarm_threshold[SPD_NUM];			//漏电流报警阈值
	// DO报警的值
	UINT8 DO_spdcnt_clear[SPD_NUM];	// 雷击计数清0
	UINT8 DO_totalspdcnt_clear[SPD_NUM];	// 总雷击计数清0
	UINT8 DO_leak_type[SPD_NUM];		// 0:内置漏电流，1：外接漏电流
	UINT8 DO_psdtime_clear[SPD_NUM];	// 雷击时间清0
	UINT8 DO_daytime_clear[SPD_NUM];	// 在线时间清0
	//接地电阻
	UINT16 spdres_id;				// 更改id地址	// 0x12
	UINT16 spdres_alarm_value;		// 报警值修改	// 0x13
}REMOTE_CONTROL;

#pragma pack(pop)
/*寄存器操作*/
UINT8 Read_Register(UINT16 nStartRegNo, UINT16 nRegNum, UINT8 *pdatabuf, UINT8 *perr);
UINT8 Write_Register(UINT16 nStartRegNo, INT16 nRegNum, const UINT8 *pdatabuf, UINT8 datalen, UINT8 *perr);

extern UINT16  System_Reset;

#endif






