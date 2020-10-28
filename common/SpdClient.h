#ifndef __SPDCLIENT_H__
#define __SPDCLIENT_H__

#include <string>
#include "registers.h"

using namespace std; 


#define BIT(i)	(1<<(i))	// 32位平台32位?


#define TYPE_LEIXUN		1
#define TYPE_HUAZI		2
#define TYPE_KY			3	// 宽永
#define TYPE_ZPTA		4	// 中普同安
#define TYPE_ZPZH		5	// 中普众合
#define TYPE_KY0M		6	// 宽永0M型号,广西路段
#define TYPE_TY			7	// 图粤


#define TYPE_MAX_NUM	8	// 支持的数量


#define SPD_DEFALT_ADDR			0xF0	// 默认设备地址， 不配置就是1
#define NULL_VALUE				0		// 定义没有这个参数时的默认值
/**********************************************/
// 雷迅检测器的定义
#define AI_SPD_ID_ADDR					10		// 改设备id的地址，浮点型
#define AI_LEAK_THRESHOLD_ADDR			18		// 浮点型

#define DO_ADDR_CNT_CLR				0	// 雷击计数清0地址
#define DO_ADDR_TOTAL_CLR			1	// 雷击总数清0地址
#define DO_ADDR_LEAK_SET			2	// 漏电流设置
#define DO_ADDR_STRUCK_TIME_CLR		4	// 雷击时间清0
#define DO_ADDR_WORK_TIME_CLR		5	// 在线时间清0


#define DO_ON_CMD	0xFF00	// 清零
#define DO_ON_CMD_KY	0x01
#define DO_OFF_CMD		0x0

#define RES_TEST_ADDR		0x11	// 测试接地电阻值
#define RES_ID_ADDR			0x12		// 改接地电阻设备id的地址
#define RES_ALARM_ADDR		0x13		// 更改报警值的地址
#define RES_TEST_EN			0x1			// 接地电阻测试开始

#define TIME_SET_ADDR		22	// 时间设置22~27
#define TIME_SET_LEN		6	// 寄存器长度6


//读命令码
#define SPD_DO_CMD		0x01
#define SPD_DI_CMD		0x02
#define SPD_READ_CMD		0x03
#define SPD_RES_READ_CMD	0x03

//写命令码
#define SPD_DO_CTRL_CMD		0x05
#define SPD_RES_SET_CMD		0x06
#define SPD_WRITE_CMD		0x10

#define SPD_ADDR			0x0B	// 设备地址
#define SPD_RES_ADDR		0x09	// 接地电阻设备地址

#define SPD_AI_ADDR			0
#define SPD_AI_NUM			58		// 读58,后面多了一个不用的

// 全部读出来
#define SPD_DI_ADDR			0
#define SPD_DI_NUM			8	// DI5未规定

#define SPD_DO_ADDR			0
#define SPD_DO_NUM			6	// DO3未规定

#define SPD_RES_STATUS_ADDR			0x0C
#define SPD_RES_STATUS_NUM			8	// 8个数据,后面3个是可读可写的
// 或者中普同安的从0x0D开始，读6个数据值
#define SPD_RES_VALUE_ADDR			0x0D
#define SPD_RES_VALUE_NUM			6	// 6个数据,后面3个是可读可写的


/**********************************************/
// 华咨检测器的定义
#define HZ_ADDR		0x01
#define HZ_SPD_READ	0x0C
#define HZ_RES_READ	0x0D

#define HZ_SPD_LEN 0x2E
#define HZ_RES_LEN 	0x1A

#define HZ_HEAD_NUM		12		// 收到的数据前面有12个前导
#define HZ_SPD_DATA_NUM	15		// 15个数据
#define HZ_RES_DATA_NUM	5		// 5个数据

#define HZ_BUF_LEN		256		// 因为协议并不长，定义为256已经足够
/**********************************************/
// 宽永检测器的定义
#define KY_READ_CMD			0x03
#define KY_WRITE_CMD		0x06

#define KY_RUN_ADDR			0x63
#define KY_RUN_NUM			12		// 运行数据从63H开始,12个

#define KY_DI_ADDR			0x200	// 遥信数据从200H开始，2个
#define KY_DI_NUM			2

#define KY_HIS_ADDR			0xFF	// 遥信数据从FFH开始，8个
#define KY_HIS_NUM			8

#define KY_RES_VALUE_ADDR			0x01	// 1：接地电阻值,2:设备地址 3：报警值 4:接地电阻测试 5:使能测试模式
#define KY_RES_ID_ADDR				0x02
#define KY_RES_ALARM_ADDR			0x03
#define KY_RES_TEST_ADDR			0x04

#define KY_RES_NUM					1		// 宽永的比较坑,1次只能读1个数量

#define KY_SHIELD_INTERVAL			20		// 宽永的电阻测试需要2s以上才有回复，10不能再次测试，为了保险，设为20s
#define KY_CLEAR_ADDR				0x0407


/**********************************************/
// 图粤检测器的定义
#define TY_RES_READ_CMD				0x03
#define TY_RES_WRITE_CMD			0x06
#define TY_RES_ADD					0x01	// 地阻数据
#define TY_RES_NUM					3

#define TY_RES_VALUE_ADDR			0x01	// 1：接地电阻值,2:设备地址 3：报警值 
#define TY_RES_ID_ADDR				0x02
#define TY_RES_ALARM_ADDR			0x03


#define TY_RUN_READ_CMD		0x04
#define TY_RUN_ADDR			0x01
#define TY_RUN_NUM			27		// 运行数据从01H开始,27个

#define TY_DI_READ_CMD		0x02
#define TY_DI_ADDR			0x00	// 遥信数据从00H开始，1个
#define TY_DI_NUM			8		// 8位

#define TY_HIS_READ_CMD		0x02
#define TY_HIS_ADDR			0x00	// 历史数据，还是DI值
#define TY_HIS_NUM			1


/**********************************************/
// 中普同安检测器的定义，自定义的协议
#define ZPTA_READ_CMD		0x52		// 读命令0x52
#define ZPTA_RES_CMD		0x41		// 回复0x41
#define ZPTA_SPD_LEN		15			// 发送，接收都是15个字节
#define ZPTA_DI_NUM			8
#define ZPTA_HEAD_NUM 		4			// 前导符4个


/**********************************************/
// 中普众合检测器的定义，标志modbus协议
#define ZPZH_READ_CMD		0x04		// 读命令0x04
#define ZPZH_SPD_ADDR		0			// 寄存器地址从0开始
#define ZPZH_SPD_LEN		4			// SPD是4个字节
#define ZPZH_RES_LEN		2			// 接地电阻是2个字节
#define ZPZH_HEAD_NUM 		3			// 前导符3个


/**********************************************/
// 一些基本设置
#define SPD_INTERVAL_TIME		400000		// 400ms,让控制命令更快下发
#define SPD_POLLING_INTERVAL 	3			// 1.05s轮询一次参数
#define SPD_ASSERT_INTERVAL 	20			// 20*400ms = 8s, 8s判断一次断线
#define SPD_TEST_RES_INTERVAL	7202		// 1.05*7200 = 7200s,2个小时,错开一点点
#define SPD_TIME_SYN_INTERVAL	72000		// 时间同步间隔,24小时 = 72000*1.2 s
#define SPD_DISC_TIMEOUT		(30)		// 30s


// 定义一个函数指针，方便后面定义数组
// 这里比较复杂，首先线程函数：void *pthread(void *param)
// 中的pthread是一个函数名，它返回一个void*指针，接收一个void*的参数
// 因此第一步  void* (*func)(void *); 中void*表示返回值
// (*func)表示一个指向函数指针， void*表示接收参数
// 第二步: typedef void* (*NETWORK_PTHREAD)(void *); 加上typedef
// 并把func 改成 NETWORK_PTHREAD，使用 NETWORK_PTHREAD定义指针数组
typedef void* (*NETWORK_PTHREAD)(void *);


// 4字节表示一个float型
typedef union {
 float f;
 unsigned char c[4];
} FDATA;


// PSD子结构体--AI输入
typedef struct spd_ai_struct
{
	FDATA leak_current;	// 漏电流
	FDATA ref_volt;		// 基准电压
	FDATA real_volt;	// 实时电压
	FDATA spd_temp;		// 防雷器温度
	FDATA envi_temp;	// 环境温度
	FDATA id;			// 设备地址
	FDATA struck_cnt;	// 雷击计数
	FDATA struck_total;	// 合计雷击计数
	FDATA soft_version;		// 软件版本号
	FDATA leak_alarm_threshold;		// 报警阈值
	FDATA day_time;				// 在线天数

	UINT16 systime_year;		// 在线时间_年份
	UINT16 systime_month;		// 在线时间_月份
	UINT16 systime_day;		// 在线时间_日期
	UINT16 systime_hour;		// 在线时间_时
	UINT16 systime_min;		// 在线时间_分
	UINT16 systime_sec;		// 在线时间_秒

	// 最近一次防雷发生时间
	UINT16 last_1_struck_year;
	UINT16 last_1_struck_month;
	UINT16 last_1_struck_day;
	UINT16 last_1_struck_hour;
	UINT16 last_1_struck_min;

	UINT16 life_time;			// 寿命值0%-100%

	// 最近第2次防雷发生时间
	UINT16 last_2_struck_year;
	UINT16 last_2_struck_month;
	UINT16 last_2_struck_day;
	UINT16 last_2_struck_hour;
	UINT16 last_2_struck_min;

	UINT16 reversed0;

	// 最近第3次防雷发生时间
	UINT16 last_3_struck_year;
	UINT16 last_3_struck_month;
	UINT16 last_3_struck_day;
	UINT16 last_3_struck_hour;
	UINT16 last_3_struck_min;

	UINT16 reversed1;

	// 最近第4次防雷发生时间
	UINT16 last_4_struck_year;
	UINT16 last_4_struck_month;
	UINT16 last_4_struck_day;
	UINT16 last_4_struck_hour;
	UINT16 last_4_struck_min;

	UINT16 reversed2;

	// 最近第5次防雷发生时间
	UINT16 last_5_struck_year;
	UINT16 last_5_struck_month;
	UINT16 last_5_struck_day;
	UINT16 last_5_struck_hour;
	UINT16 last_5_struck_min;		// 57长度
}SPD_AI_PARAMS;


typedef struct spd_di_struct
{
	UINT8 SPD_DI;	// DI数据
}SPD_DI_PARAMS;

typedef struct spd_do_struct
{
	UINT8 SPD_DO;
}SPD_DO_PARAMS;

// 华咨的协议参数
typedef struct spd_hz_struct
{
	UINT16 breaker_alarm;	// 脱扣
	UINT16 reserved0;
	UINT16 grd_alarm;		// 接地报警
	UINT16 struck_cnt;		// 雷击计数
	UINT16 reserved1;
	UINT16 volt_A;			// A相电压
	UINT16 volt_B;
	UINT16 volt_C;
	UINT16 leak_A;			// A相漏电流
	UINT16 leak_B;
	UINT16 leak_C;
	UINT16 spd_temp;		// 防雷器温度
	UINT16 envi_temp;		// 环境温度
	UINT16 reserved2;
	UINT16 life_time;
}SPD_HZ_PARAMS;


// 宽永的协议参数
typedef struct spd_ky_struct
{
	UINT16 current_A;		// A相电流
	UINT16 current_B;
	UINT16 current_C;
	UINT16 volt_A;			// A相电压
	UINT16 volt_B;
	UINT16 volt_C;
	UINT16 struck_cnt;					// 雷击计数
	UINT16 struck_strenth;				// 雷击强度
	UINT16 struck_strenth_strongest;	// 最大雷击强度
	UINT16 temp;				// 防雷器温度
	UINT16 moist;				// 湿度
	UINT16 life_time;			// 劣化程度,和生命值是相反的

	// 报警值
	UINT16 life_time_alarm;		// 生命周期报警
	UINT16 di_alarm;			// bit0:外接脱扣, bit1:未用,bit2:接地

	UINT16 struk_peak;			// 峰值
	// 最近一次防雷发生时间
	UINT16 struck_year;
	UINT16 struck_month;
	UINT16 struck_day;
	UINT16 struck_hour;
	UINT16 struck_min;
	UINT16 struck_sec;
	UINT16 his_num;
}SPD_KY_PARAMS;


// 宽永的协议参数
typedef struct spd_ty_struct
{
	UINT16 DI_alarm;		// DI报警
	UINT16 online_day;
	UINT16 online_hour;
	UINT16 online_min;		// 在线时长
	UINT16 reserved1;			//4
	UINT16 reserved2;
	UINT16 reserved3;
	UINT16 envi_temp;
	UINT16 spd_tem1;
	UINT16 spd_tem2;
	UINT16 reserved4;			//10
	UINT16 reserved5;
	UINT16 life_time;		//12生命值
	UINT16 reserved6;			//13
	UINT16 struck_cnt;					// 雷击计数
	UINT16 struck_cnt_total;				// 总计数
	UINT16 reserved7;			//16
	UINT16 reserved8;
	UINT16 reserved9;

	// 报警值
	UINT16 temp_alarm_total;	// 温度报警次数
	UINT16 C1_alarm_total;			// 遥信报警次数
	UINT16 line_alarm_total;		// 线路报警次数
	UINT16 reserved10;					//22
	UINT16 leak_alarm_total;		// 漏电流报警次数
	UINT16 struck_current_total;		// 雷击电流次数
	UINT16 reserved11;			// 25
	UINT16 leak_amp;		// 漏电流zhi
	UINT16 volt_A;			// 27电压
}SPD_TY_PARAMS;


// 中普同安的协议参数
typedef struct spd_zpta_struct
{
	UINT8 SPD_DI[ZPTA_DI_NUM];	// 只有DI数据,且1个DI占1个字节
}SPD_ZPTA_PARAMS;


// 中普众合的协议参数
typedef struct spd_zpzh_struct
{
	UINT8 temp_h;	// 温度高字节
	UINT8 temp_l;	// 温度低字节
	UINT8 struck_cnt;	// 雷击次数
	UINT8 DI;			// 开关量报警
}SPD_ZPZH_PARAMS;



// 统一的协议参数
typedef struct spd_real_struct
{
	UINT16 id;				// 设备地址
	float ref_volt;			// 基准电压
	float real_volt;		// 实时电网电压, 单相
	float volt_A;			// A相电压
	float volt_B;

	float volt_C;
	float leak_current;		// 漏电流
	float leak_A;			// A相漏电流
	float leak_B;
	float leak_C;

	float struck_cnt;	// 雷击计数
	float struck_total;	// 合计雷击计数
	float spd_temp;			// 防雷器温度
	float envi_temp;		// 环境温度
	float life_time;		// 寿命值0%-100%
	float soft_version;				// 软件版本号
	float leak_alarm_threshold;		// 报警阈值

	float day_time;				// 在线天数

	//float leak_id;			// 漏电流设备的id, 广东飞达的雷迅单独有漏电流检测器

	// DI报警的值
	UINT8 DI_bit_0;			// 对DI位的解析
	UINT8 DI_bit_1;			// 对DI位的解析
	UINT8 DI_bit_2;			// 对DI位的解析
	UINT8 DI_C1_status;		// bit3,C1遥信
	UINT8 DI_grd_alarm;		// bit4,接地告警
	UINT8 DI_bit_5;	// 对DI位的解析
	UINT8 DI_leak_alarm;		// bit6,漏电流告警
	UINT8 DI_volt_alarm;		// bit7,市电电压告警


	// DO报警的值
	UINT8 DO_spdcnt_clear;	// 雷击计数清0
	UINT8 DO_totalspdcnt_clear;	// 总雷击计数清0
	UINT8 DO_leak_type;		// 0:内置漏电流，1：外接漏电流
	UINT8 DO_bit3;
	UINT8 DO_psdtime_clear;	// 雷击时间清0
	UINT8 DO_daytime_clear;	// 在线时间清0
	UINT8 DO_bit6;
	UINT8 DO_bit7;

	// 系统时间
	UINT16 systime_year;		// 在线时间_年份
	UINT16 systime_month;		// 在线时间_月份
	UINT16 systime_day;		// 在线时间_日期
	UINT16 systime_hour;		// 在线时间_时
	UINT16 systime_min;		// 在线时间_分
	UINT16 systime_sec;		// 在线时间_秒
	// 最近一次防雷发生时间
	UINT16 last_1_struck_year;
	UINT16 last_1_struck_month;
	UINT16 last_1_struck_day;
	UINT16 last_1_struck_hour;
	UINT16 last_1_struck_min;

	// 最近第2次防雷发生时间
	UINT16 last_2_struck_year;
	UINT16 last_2_struck_month;
	UINT16 last_2_struck_day;
	UINT16 last_2_struck_hour;
	UINT16 last_2_struck_min;

	// 最近第3次防雷发生时间
	UINT16 last_3_struck_year;
	UINT16 last_3_struck_month;
	UINT16 last_3_struck_day;
	UINT16 last_3_struck_hour;
	UINT16 last_3_struck_min;

	// 最近第4次防雷发生时间
	UINT16 last_4_struck_year;
	UINT16 last_4_struck_month;
	UINT16 last_4_struck_day;
	UINT16 last_4_struck_hour;
	UINT16 last_4_struck_min;

	// 最近第5次防雷发生时间
	UINT16 last_5_struck_year;
	UINT16 last_5_struck_month;
	UINT16 last_5_struck_day;
	UINT16 last_5_struck_hour;
	UINT16 last_5_struck_min;
}SPD_REAL_PARAMS;



typedef struct spd_res_st_struct
{
	UINT16 alarm;			// 接地报警 0x0c
	UINT16 grd_res_value;	// 协议取得的接地电阻值 0x0d
	UINT16 grd_res_dot_num;	// 接地电阻小数位 0x0e
	UINT16 grd_volt;		// 电压值 0x0f

	UINT16 reserved0;		// 0x10
	UINT16 test;			// 测试	// 0x11
	UINT16 id;				// 更改id地址	// 0x12
	UINT16 alarm_value;		// 报警值修改	// 0x13

	float grd_res_real;		// 真实的接地电阻值	//不在协议内计算的值
}SPD_RES_ST_PARAMS;

#if 0
typedef struct spd_res_ctrl_struct
{
	UINT16 test;			// 测试
	UINT16 id;				// 更改id地址
	UINT16 alarm_value;		// 报警值修改
}SPD_RES_CTRL_PARAMS;
#endif

typedef struct spd_modbus_array_struct
{
   //UINT8	addr;	// 设备地址
   UINT8    func_code;
   UINT16 	reg_addr;
   UINT16 	reg_num;
   //void   (*Function)(PDUResponse_Struct *Response);
}SPD_FunctionArray_Struct;


/*防雷读写类型定义*/
typedef enum
{
	// 雷迅的轮询标志
	SPD_AI_DATA = 0,
	SPD_DI_DATA,
	SPD_DO_DATA,

	// 华咨的轮询标志
	SPD_HZ_DATA,
	//SPD_HZ_DATA_2,

	// 宽永的轮询标志
	SPD_RUN_DATA,
	SPD_REMOTE_DATA,	// 遥信
	SPD_REC_DATA,		// 历史记录

	SPD_RES_DATA,	// 接地电阻部分

	SPD_DATA_NUM
}SPD_DATA_LIST;

/*电源 控制定义*/
typedef enum
{
	SPD_AI_SET = 0,		// 模拟量设置
	SPD_DO_SET,			// DO控制
	SPD_TIME_SET,		// 时间同步

	SPD_RES_SET,		// 接地电阻设置

	SPD_CTRL_NUM		//	4
}SPD_CTRL_LIST;


// 防雷器结构体
typedef struct spd_struct
{
	// 雷迅的防雷
	SPD_AI_PARAMS dSPD_AIdata[SPD_NUM];
	SPD_DI_PARAMS dSPD_DI[SPD_NUM];
	SPD_DO_PARAMS dSPD_DO[SPD_NUM];

	// 华咨有2个防雷
	SPD_HZ_PARAMS dSPD_HZ[SPD_NUM];	// 华咨的防雷器

	// 宽永有2个防雷
	SPD_KY_PARAMS dSPD_KY[SPD_NUM];

	// 图粤有1个防雷
	SPD_TY_PARAMS dSPD_TY[SPD_NUM];

	// 中普同安有2个防雷
	SPD_ZPTA_PARAMS dSPD_ZPTA[SPD_NUM];

	// 中普众合有2个防雷
	SPD_ZPZH_PARAMS dSPD_ZPZH[SPD_NUM];

	// 共性，接地电阻值数据, 不需要再设置一个协议值了
	SPD_RES_ST_PARAMS rSPD_res;

	// 上面的防雷都要转换成与后台交互的协议数据
	SPD_REAL_PARAMS rSPD_data[SPD_NUM];

	UINT32 TimeStamp[SPD_NUM+RES_NUM]; 			//状态获取时间戳
	bool Linked[SPD_NUM+RES_NUM];				//连接状态 0:断开 1：连接
}SPD_PARAMS;


// 防雷器结构体
typedef struct spd_ctrl_value_struct
{
	UINT16 ref_addr;		// 要控制的寄存器地址
	FDATA f_ai_set;		// 要控制的值,根据地址不同而不同
	UINT16 do_set;		//
	UINT16 res_set;		//
}SPD_CTRL_VALUE;



typedef int (*SpdWriteconfigBack)(unsigned int mRetID);
typedef int (*SpdSetconfigBack)(string StrKEY,string StrSetconfig,unsigned int mRetID);


class SpdClient
{
public:
       SpdClient(void);
       ~SpdClient(void);
	  
	  
       int sockfd_spd[SPD_NUM+RES_NUM];	// 还有1个接地电阻
       int udpfd_spd[SPD_NUM+RES_NUM];	// 华咨的是udp协议

       // 不能使用sockfd_spd作为连接标志，因为从获取socket到connect动作失败中间有很长一段时间状态不是真实的
       int connected_flag[SPD_NUM+RES_NUM];

       UINT16  spd_net_flag[SPD_NUM] ;	// SPD轮询标志
       UINT16  spd_ctl_flag[SPD_NUM] ;	// SPD控制标志
       UINT8  WAIT_spd_res_flag[SPD_NUM];	// 等待信息回应
       UINT8 SPD_Type ;
       UINT8 SPD_num  ;
       //UINT8 SPD_Address[SPD_NUM+RES_NUM];
	   UINT8 SPD_Address[SPD_NUM+RES_NUM] = 
       {
	     SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
	     SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
	     SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
	     SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
	     SPD_DEFALT_ADDR
        };
	
       SPD_FunctionArray_Struct g_SPD_LX_Fun_Array[SPD_DATA_NUM] = 
       {
	     {SPD_READ_CMD,SPD_AI_ADDR,SPD_AI_NUM},
	     {SPD_DI_CMD,SPD_DI_ADDR,SPD_DI_NUM},
	     {SPD_DO_CMD,SPD_DO_ADDR,SPD_DO_NUM},
	     // 这项对应的是华咨的,但是华咨不是MODBUS格式，所以没用
	     {0,0,0},
	     // 宽永的读数据
	     {0,0,0},
	     {0,0,0},
	     {0,0,0},
	     // 接地电阻值
	     {SPD_RES_READ_CMD,SPD_RES_STATUS_ADDR,SPD_RES_STATUS_NUM},
       };
   
	   SPD_FunctionArray_Struct g_SPD_KY_Fun_Array[SPD_DATA_NUM] = 
       {
	     {0,0,0},
	     {0,0,0},
	     {0,0,0},
	     // 这项对应的是华咨的,但是华咨不是MODBUS格式，所以没用
	     {0,0,0},
	     // 宽永的读数据
	     {KY_READ_CMD,KY_RUN_ADDR,KY_RUN_NUM},
	     {KY_READ_CMD,KY_DI_ADDR,KY_DI_NUM},
	     {KY_READ_CMD,KY_HIS_ADDR,KY_HIS_NUM},
	     // 接地电阻值
	     {KY_READ_CMD,KY_RES_VALUE_ADDR,KY_RES_NUM},
       };

	   SPD_FunctionArray_Struct g_SPD_TY_Fun_Array[SPD_DATA_NUM] = 
       {
	     {0,0,0},
	     {0,0,0},
	     {0,0,0},
	     // 这项对应的是华咨的,但是华咨不是MODBUS格式，所以没用
	     {0,0,0},
	     // 图粤,使用宽永的位置,类型上进行区分
	     {TY_RUN_READ_CMD,TY_RUN_ADDR,TY_RUN_NUM},
	     {TY_DI_READ_CMD,TY_DI_ADDR,TY_DI_NUM},
	     {TY_DI_READ_CMD,TY_DI_ADDR,TY_DI_NUM},
	     // 接地电阻值
	     {TY_RES_READ_CMD,TY_RES_ADD,TY_RES_NUM},
       };
   
	   
	   NETWORK_PTHREAD network_thread_SPD_HZ[SPD_NUM] = 
       {
	     NetWork_DataGet_thread_SPD_HZ1,NetWork_DataGet_thread_SPD_HZ2,\
	     NetWork_DataGet_thread_SPD_HZ3,NetWork_DataGet_thread_SPD_HZ4,\
	     NetWork_DataGet_thread_SPD_HZ5,NetWork_DataGet_thread_SPD_HZ6,\
	     NetWork_DataGet_thread_SPD_HZ7,NetWork_DataGet_thread_SPD_HZ8,\
	     NetWork_DataGet_thread_SPD_HZ9,NetWork_DataGet_thread_SPD_HZ10,\
	     NetWork_DataGet_thread_SPD_HZ11,NetWork_DataGet_thread_SPD_HZ12,\
	     NetWork_DataGet_thread_SPD_HZ13,NetWork_DataGet_thread_SPD_HZ14,\
	     NetWork_DataGet_thread_SPD_HZ15,NetWork_DataGet_thread_SPD_HZ16
       };

       //UINT8 SPD_Res_Address = SPD_RES_ADDR;
       SPD_CTRL_VALUE SPD_ctrl_value[SPD_NUM];

       pthread_mutex_t SPDdataHandleMutex;


       string StrSPDType;	//PSD厂家类型,1:雷迅,2:华咨,3...
       string StrSPDCount;	//PSD数量
       string StrSPDIP[SPD_NUM+RES_NUM];	//SPD控制器IP地址
       string StrSPDPort[SPD_NUM+RES_NUM];	//SPD控制器端口
       string StrSPDAddr[SPD_NUM+RES_NUM];			//SPD控制器硬件地址
       char gsSPDIP[SPD_NUM+RES_NUM][20];	//PSD-IP地址
       char gsSPDPort[SPD_NUM+RES_NUM][10];	//PSD端口
       SPD_PARAMS *stuSpd_Param;		//防雷器结构体

        // 华咨的网络有关变量定义
       struct sockaddr_in HZSPDAddr[SPD_NUM+RES_NUM];

       pthread_mutex_t HZSPDMutex[SPD_NUM+RES_NUM];

       UINT8 HZ_reset_flag[SPD_NUM+RES_NUM];
       UINT8 HZ_reset_pre[SPD_NUM+RES_NUM] ;	// 对HZ_reset_flag预先处理

       UINT8 KY_res_read_seq ;	// 宽永的接地每次只能读一个地址，用变量来改变读的地址
       UINT16 KY_test_disable_cnt ;


       unsigned int m_RetID;
       SpdSetconfigBack m_SpdSetconfigBack;
	   SpdWriteconfigBack m_SpdWriteconfigBack;
       int SetSpdSetconfig(SpdSetconfigBack mSpdSetconfigBack,unsigned int mretID);
	   int SetSpdWriteconfig(SpdWriteconfigBack mSpdWriteconfigBack,unsigned int mretID);
       void StartNetSpd(void);
	   int Ex_SPD_Set_Process(int seq,SPD_CTRL_LIST SPD_ctrl_event, UINT16 set_addr, FDATA ai_data,UINT16 data);

       int obtain_net();
       //void char_to_int(UINT8* buffer,UINT16* value);
       //extern void WriteLog(char* str);
       //UINT32 timestamp_get(void);
       //UINT32 timestamp_delta(UINT32 const timestamp);


       void RealDataCopy(int seq,SPD_DATA_LIST msg_t);
	   
private:	  
	   void HZ_char_to_int(UINT8* buffer,UINT16* value);
	   void char_to_float(UINT8* buffer,FDATA* value);
	   void SPD_timeStamp_update(int seq);
	   void SPD_vars_init(int seq);
	   void SPD_disconnct_process(void);
	   int SPD_Read_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 REFS_COUNT);
       int SPD_HZ_Read_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 REFS_COUNT);
       int SPD_ZPTA_Read_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 REFS_COUNT);
	   int SPD_DO_Ctrl_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 cmd);
	   int SPD_AI_Set_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 REFS_COUNT, FDATA *data);
	   int SPD_Res_Set_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 cmd);
	   int SPD_Time_Set_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR,UINT16 REFS_COUNT);
	   int spd_send_process(UINT16 seq,UINT16 *pnet_flag);
	   int spd_ctrl_process(UINT16 seq,UINT16 *pctrl_flag);
	   int obtain_net_psd(UINT16 seq);
	   void DealSPDAiMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealSPDDiMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealSPDDoMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealSPDResStatusMsg(unsigned char *buf,unsigned short int len);
	   void DealHZSPDMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealHZResMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealKYResMsg(unsigned char *buf,unsigned short int len);
	   void DealKYSPDMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealTYResMsg(unsigned char *buf,unsigned short int len);
	   void DealTYSPDMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealZPTASPDMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealZPTAResMsg(unsigned char *buf,unsigned short int len);
	   int DealZPZHSPDMsg(int seq,unsigned char *buf,unsigned short int len);
	   void DealZPZHResMsg(unsigned char *buf,unsigned short int len);
	   int DealNetSPD(int skt,unsigned char *buf,unsigned short int len);
	   void Spd_Reconnect_Loop(int *connect_flag, int seq);
	   void SPD_ZP_Data_Get_Func(int seq);
	   bool udp_param_reset(int Pos);
	   void SPD_HZ_Data_Get_Func(int seq);
	   
	   void DataGet_Thread_Create(UINT16 SPD_t);
	   void SPD_data_init(void);


       static void *NetWork_DataGet_thread_SPD_HZ1(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ1_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ2(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ2_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ3(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ3_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ4(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ4_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ5(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ5_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ6(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ6_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ7(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ7_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ8(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ8_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ9(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ9_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ10(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ10_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ11(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ11_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ12(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ12_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ13(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ13_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ14(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ14_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ15(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ15_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZ16(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZ16_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_HZRes(void*arg);
	   void *NetWork_DataGet_thread_SPD_HZRes_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_ZP1(void*arg);
	   void *NetWork_DataGet_thread_SPD_ZP1_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_ZP2(void*arg);
	   void *NetWork_DataGet_thread_SPD_ZP2_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_ZP3(void*arg);
	   void *NetWork_DataGet_thread_SPD_ZP3_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_ZP4(void*arg);
	   void *NetWork_DataGet_thread_SPD_ZP4_run(void);
	   
	   static void *NetWork_DataGet_thread_SPD_ZPres(void*arg);
	   void *NetWork_DataGet_thread_SPD_ZPres_run(void);
	   
		   
       static void *NetWork_DataGet_thread_SPD_L(void*arg);
	   void *NetWork_DataGet_thread_SPD_L_run(void);
	   
       static void *NetWork_server_thread_SPD(void*arg);
	   void *NetWork_server_thread_SPD_run(void);


};



#endif


