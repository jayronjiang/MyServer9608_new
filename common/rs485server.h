#ifndef __RS485SERVER_H__
#define __RS485SERVER_H__

#include "modbus.h"
#include "registers.h"

//#define LBIT(i)	(1UL<<(i))	// 要注意32位平台LONG也是32位
//#define BITS_MSK_GET(bitsoff, bits) (((0x01UL<<(bits))-1) <<(bitsoff)) // 获取从bitsoff 开始共bits位全1掩码
#define NULL_VAR	0xFF	// 定义0xFF为不相关的值, 不适用NULL是因为NULL默认是0，0是我们需要的

/*最大支持数目,实际以配置文件为准*/
#define LOCK_MAX_NUM			4
#define VA_METER_BD_MAX_NUM		6
#define POWER_BD_MAX_NUM		3


#define CARD_NUM		5	// 暂时为5张卡
#define VA_INTERVAL		4	// 单位为 DEV_INTERVAL_TIME
#define VA_LOOP_NUM		6	// 电子锁轮询循环计数
#define DEV_DISCON_INTERVAL		10	// 多久判断一次断线?3.6s
#define DEV_DISC_TIMEOUT		(30)		// 30s,485短一点,未回应就不会再有回应了

#define POWER_DISCON_INTERVAL	95	// 多久判断一次断线?36s
#define POWER_DISC_TIMEOUT		(10*60UL)		// 10分钟



#define DOOR_LOCK_ADDR_1		91			// address of the LOCKER
#define DOOR_LOCK_ADDR_2		92			// address of the LOCKER

/*电源控制器地址*/
#define POWER_CTRL_ADDR_1			71		/*电源控制器地址*/
#define IO_CTRL_ADDR_1				61		/*IO控制器地址*/


/*REMOTE CONTROL definition*/
#define	SINGLE_WRITE_HW		0x06	// function of the LOCKER
#define DOOR_LOCK_REG		0x4002 		// remote control

#define	REMOTE_UNLOCK		0x33		// OPEN CMD of the locker
#define	REMOTE_LOCK			0xCC		// CLOSE CMD of the locker


/*Status reading definition*/
#define DOOR_STATUS_REG			0x1000 		// status reading
#define LOCKER_ID_LENTH			10		// it only use 10 bytes from the ID card.
#define LOCKER_REG_NUM			9		// 4+5(字)
#define FRAME_HEAD_NUM 			3		/*读数据时返回帧有效数据前数据个数*/

#define VER_REG					921		//10000，表示版本V1.00.00
#define VER_DATA_NUM			2		// 921~922

/*经测试,锁的回复数据为23个字节,共24ms,加上回复延时共50ms,*/
/*而电压电流传感器的回复数据有VA_DATA_NUM=42个,加上回复延时共80ms左右，完全是够的*/
/*350ms,缩短时间,如果是4把锁轮询要快,最差的情况为4把锁全部配置,加上一次其它轮询共*/
/*350ms*5 = 1.75s,而1.4s是常态*/
#define INTERVAL_TIME			350000
#define DEV_INTERVAL_TIME		360000		/*360ms,错开一点*/

/*VOLT-AMP sampling definition*/
#define REAL_DATA_NUM			42  	/*需实时更新数据长度，0x69-0x40*/
#define VA_STATION_ADDRESS_1	81
#define VA_STATION_ADDRESS_2	82
#define VA_REG					0x40
#define VA_DATA_NUM				42

#define ENABLE					1
#define DISABLE					0

#define LOCKER_CLOSED			0
#define LOCKER_OPEN				1

/*The name of the RS485 coms*/
typedef enum
{
	RS485_1 = 0,
	RS485_2,
	RS485_NUM
}RS485_COM_LIST;



/*The name of the RS485 devices*/
typedef enum
{
	LOCKER_1 = 0,
	LOCKER_2,
	LOCKER_3,
	LOCKER_4,		// 最多4把锁
	VA_METER_1,
	VA_METER_2,
	VA_METER_3,
	VA_METER_4,
	VA_METER_5,
	VA_METER_6,
	POWER_BD_1,		// 电源控制板1
	POWER_BD_2,		// 电源控制板2
	POWER_BD_3,		// 电源控制板1
	IO_BD_1,		// IO控制板1
	IO_BD_2,		// IO控制板2
	IO_BD_3,		// IO控制板3

	RS485_DEV_MAX_NUM
}RS485_DEV_LIST;

#if 0

//消息等待枚举
typedef enum
{
	WAIT_NONE = 0,
	WAIT_LOCKER_1_MSG,
	WAIT_LOCKER_2_MSG,
	WAIT_LOCKER_3_MSG,
	WAIT_LOCKER_4_MSG,	// 最多支持4把电子锁
	WAIT_VA_DATA_1_MSG,
	WAIT_VA_DATA_2_MSG,
	WAIT_VA_DATA_3_MSG,
	WAIT_VA_DATA_4_MSG,
	WAIT_VA_DATA_5_MSG,
	WAIT_VA_DATA_6_MSG,

	WAIT_LOCKER_1_UNLOCK_RES,
	WAIT_LOCKER_1_LOCK_RES,
	WAIT_LOCKER_2_UNLOCK_RES,
	WAIT_LOCKER_2_LOCK_RES,
	WAIT_LOCKER_3_UNLOCK_RES,
	WAIT_LOCKER_3_LOCK_RES,
	WAIT_LOCKER_4_UNLOCK_RES,
	WAIT_LOCKER_4_LOCK_RES,

	WAIT_POWER_1_CTRL_CLOSE_RES,
	WAIT_POWER_1_CTRL_OPEN_RES,
	WAIT_POWER_2_CTRL_CLOSE_RES,
	WAIT_POWER_2_CTRL_OPEN_RES,
	WAIT_POWER_3_CTRL_CLOSE_RES,
	WAIT_POWER_3_CTRL_OPEN_RES,
	WAIT_POWER_4_CTRL_CLOSE_RES,
	WAIT_POWER_4_CTRL_OPEN_RES,
	WAIT_POWER_5_CTRL_CLOSE_RES,
	WAIT_POWER_5_CTRL_OPEN_RES,
	WAIT_POWER_6_CTRL_CLOSE_RES,
	WAIT_POWER_6_CTRL_OPEN_RES,

	WAIT_POWER_7_CTRL_CLOSE_RES,
	WAIT_POWER_7_CTRL_OPEN_RES,
	WAIT_POWER_8_CTRL_CLOSE_RES,
	WAIT_POWER_8_CTRL_OPEN_RES,
	WAIT_POWER_9_CTRL_CLOSE_RES,
	WAIT_POWER_9_CTRL_OPEN_RES,
	WAIT_POWER_10_CTRL_CLOSE_RES,
	WAIT_POWER_10_CTRL_OPEN_RES,
	WAIT_POWER_11_CTRL_CLOSE_RES,
	WAIT_POWER_11_CTRL_OPEN_RES,
	WAIT_POWER_12_CTRL_CLOSE_RES,
	WAIT_POWER_12_CTRL_OPEN_RES,

	WAIT_POWER_1_VER_RES,
	WAIT_POWER_2_VER_RES,
	WAIT_POWER_3_VER_RES,

	WAIT_MSG_NUM
}WAIT_MSG_LIST;


//485轮询类型枚举,可扩展
typedef enum
{
	LOCKER_1_STATUS = 0,
	LOCKER_2_STATUS,
	LOCKER_3_STATUS,
	LOCKER_4_STATUS,

	VOLT_AMP_GET_FLAG_1,
	VOLT_AMP_GET_FLAG_2,
	VOLT_AMP_GET_FLAG_3,
	VOLT_AMP_GET_FLAG_4,
	VOLT_AMP_GET_FLAG_5,
	VOLT_AMP_GET_FLAG_6,

	POLLING_NUM			// 10
}POLLING_LIST;

/*电子锁控制定义*/
typedef enum
{
	LOCKER_1_CTRL_UNLOCK = 0,
	LOCKER_1_CTRL_LOCK,

	LOCKER_2_CTRL_UNLOCK,
	LOCKER_2_CTRL_LOCK,

	LOCKER_3_CTRL_UNLOCK,
	LOCKER_3_CTRL_LOCK,

	LOCKER_4_CTRL_UNLOCK,
	LOCKER_4_CTRL_LOCK,

	LOCKER_CTRL_NUM			//8
}LOCKER_CTRL_LIST;

/*电源 控制定义*/
typedef enum
{
	POWER_1_CTRL_CLOSE = 0,		//0,分闸
	POWER_1_CTRL_OPEN,			// 合闸
	POWER_2_CTRL_CLOSE,
	POWER_2_CTRL_OPEN,
	POWER_3_CTRL_CLOSE,
	POWER_3_CTRL_OPEN,
	POWER_4_CTRL_CLOSE,
	POWER_4_CTRL_OPEN,
	POWER_5_CTRL_CLOSE,
	POWER_5_CTRL_OPEN,
	POWER_6_CTRL_CLOSE,
	POWER_6_CTRL_OPEN,

	POWER_7_CTRL_CLOSE,		//12
	POWER_7_CTRL_OPEN,
	POWER_8_CTRL_CLOSE,
	POWER_8_CTRL_OPEN,
	POWER_9_CTRL_CLOSE,
	POWER_9_CTRL_OPEN,
	POWER_10_CTRL_CLOSE,
	POWER_10_CTRL_OPEN,
	POWER_11_CTRL_CLOSE,
	POWER_11_CTRL_OPEN,
	POWER_12_CTRL_CLOSE,
	POWER_12_CTRL_OPEN,

	POWER_CTRL_NUM			//24
}POWER_CTRL_LIST;
#endif

/*电子锁控制定义*/
typedef enum
{
	POWER_BD_RD_1 = 0,
#if (POWER_BD_NUM >=2)
	POWER_BD_RD_2,
#endif
	#if (POWER_BD_NUM >=3)
	POWER_BD_RD_3,
#endif
	POWER_BD_RD_NUM			//3
}VER_READ_LIST;


/*结构联合声明短整型*/
/*typedef union int_union
{
	UINT16 i;
	UINT8 b[2];
}INTEGER_UNION;*/

/*结构联合声明长整型*/
/*typedef union long_union
{
	UINT32 i;
	UINT8 b[4];
}LONG_UNION;*/


/*the struct definition of the LOCKER*/
typedef struct locker_struct
{
	UINT16 address;		// modbus station address
	UINT16 status;		// 0:close 1:open
	UINT16 open_reason;	// 0:close 1:cmd 2:reserve 3:key
	UINT16 report_cnt;	// >128 it will be cleared.
	UINT16 id_length;
	UINT8 id[96];
}LOCKER_HW_PARAMS;


/*485设备配置列表定义*/
typedef struct RS485Dev_Map_Reg
{
	UINT16 status;				/*轮询时使用的名称*/
	UINT16 enable;				/*是否使能*/
	UINT16 position;			/*位置,暂时未使用*/
	UINT16 addr;				/*地址*/
} RS485_Reg_Table;


//extern UINT32  ctrl_flag;	// 写标志
extern UINT32  locker_ctrl_flag;
extern UINT32	power_ctrl_flag[POWER_BD_MAX_NUM];
extern UINT8 actual_locker_num;
extern UINT8 actual_485dev_num;
extern RS485_Reg_Table Var_Table[RS485_DEV_MAX_NUM];
extern string StrAdrrLock[LOCK_MAX_NUM];	//门锁的地址
extern string StrAdrrVAMeter[VA_METER_BD_MAX_NUM];	//电压电流传感器的地址
extern string StrAdrrPower[POWER_BD_MAX_NUM];	//电源板的地址
extern string StrDoSeq[SWITCH_COUNT];
extern UINT16 DoSeq[SWITCH_COUNT];
extern int *polling_arr;		// 注意存储的是Var_Table中被使能的status,作为轮询的标志
extern int *polling_subarr;


void rs485init(void);
int DealComm485(unsigned char *buf,unsigned short int len, RS485_COM_LIST seq);
void lockerPollingInit();
void comm_VAData_analyse(unsigned char *buf,unsigned short int len,RS485_COM_LIST seq);
int DealLockerMsg(unsigned char seq,unsigned char *buf,unsigned short int len);
void Rs485_table_set(UINT16 name, UINT16 enable, UINT16 position, UINT16 address);
UINT16 Rs485_table_enable_get(UINT16 name);
int Power_ctrl_process(UINT32 *pctrl_flag, UINT32 *pread_flag, RS485_DEV_LIST dev_name);
int Locker_ctrl_process(UINT32 *pctrl_flag, RS485_DEV_LIST dev_name);
int Dev_polling_process(UINT32 *pcomm_flag);
int Power_readback_process(UINT32 *pread_flag, UINT32 *pcomm_flag, RS485_DEV_LIST dev_name);
void *Locker_DataPollingthread(void *param);
void lockerDataMalloc(void);
UINT16 DoorStatusFromLocker(void);
void lockerDataInit(bool first_entry);


#endif


