#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h> 	// fork, close
#include <arpa/inet.h>  // inet_ntoa
#include <pthread.h>
#include <netinet/tcp.h>
#include <linux/rtc.h>
#include <ctype.h>
#include <stdbool.h>
#include "tea.h"
#include "net_spd.h"
#include "modbus.h"
#include "rs485server.h"
#include <sstream>


/*************************数据定义*****************************/
static int sockfd_spd[SPD_NUM+RES_NUM];	// 还有1个接地电阻
static int udpfd_spd[SPD_NUM+RES_NUM];	// 华咨的是udp协议
// 不能使用sockfd_spd作为连接标志，因为从获取socket到connect动作失败中间有很长一段时间状态不是真实的
static int connected_flag[SPD_NUM+RES_NUM];


UINT16  spd_net_flag[SPD_NUM] = {0,};	// SPD轮询标志
UINT16  spd_ctl_flag[SPD_NUM] = {0,};	// SPD控制标志

UINT8  WAIT_spd_res_flag[SPD_NUM]={0,};	// 等待信息回应

//UINT16 net_Conneted = 0;	// 连接标志1:已经连接
//UINT16 ZP_Conneted[SPD_NUM+RES_NUM] = {0,};	// 连接标志1:已经连接


UINT8 SPD_Type = TYPE_LEIXUN;
UINT8 SPD_num = SPD_NUM;
UINT8 SPD_Address[SPD_NUM+RES_NUM] = 
{
	SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
	SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
	SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
	SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,SPD_DEFALT_ADDR,\
	SPD_DEFALT_ADDR
};


//UINT8 SPD_Res_Address = SPD_RES_ADDR;
SPD_CTRL_VALUE SPD_ctrl_value[SPD_NUM];

pthread_mutex_t SPDdataHandleMutex;


extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体
extern SPD_PARAMS *stuSpd_Param;		//防雷器结构体

// 华咨的网络有关变量定义
struct sockaddr_in HZSPDAddr[SPD_NUM+RES_NUM];

pthread_mutex_t HZSPDMutex[SPD_NUM+RES_NUM];

UINT8 HZ_reset_flag[SPD_NUM+RES_NUM] = {false,};
UINT8 HZ_reset_pre[SPD_NUM+RES_NUM] = {false,};	// 对HZ_reset_flag预先处理

UINT8 KY_res_read_seq = KY_RES_VALUE_ADDR;	// 宽永的接地每次只能读一个地址，用变量来改变读的地址
UINT16 KY_test_disable_cnt = 0;



int obtain_net();
extern void char_to_int(UINT8* buffer,UINT16* value);
//extern void WriteLog(char* str);
extern UINT64 timestamp_get(void);
extern UINT64 timestamp_delta(UINT32 const timestamp);
extern int Setconfig(string StrKEY,string StrSetconfig);
extern int Writeconfig(void);



/*雷迅功能码及功能码对应的处理函数*/
const  SPD_FunctionArray_Struct g_SPD_LX_Fun_Array[SPD_DATA_NUM] =
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

/*宽永功能码及功能码对应的处理函数*/
const  SPD_FunctionArray_Struct g_SPD_KY_Fun_Array[SPD_DATA_NUM] =
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



/******************************************************************************
*  函数名: void HZ_char_to_int(INT8U* buffer,LONG32U* value)
*
*  描述: 字符转化为整型，低字节在前
*
*
*
*  输入:
*
*  输出:
*
*  返回值:
*
*  其它:
*******************************************************************************/
void HZ_char_to_int(UINT8* buffer,UINT16* value)
{
	INTEGER_UNION int_value;

	int_value.b[0] = *(buffer);
	int_value.b[1] = *(buffer + 1);
	*value = int_value.i;
}


/******************************************************************************
*  函数名: void char_to_float(INT8U* buffer,LONG32U* value)
*
*  描述: 字符转化为长整型
*
*
*
*  输入:
*
*  输出:
*
*  返回值:
*
*  其它:
*******************************************************************************/
void char_to_float(UINT8* buffer,FDATA* value)
{
	FDATA f_value;
	UINT8 i;

	for(i=0;i<4;i++)
	{
		//f_value.c[3-i] = *(buffer + i);
		f_value.c[i] = *(buffer + i);
	}
	value->f = f_value.f;
}

// 收到合法数据，刷新时间戳,并置位
void SPD_timeStamp_update(int seq)
{
	stuSpd_Param->TimeStamp[seq]=timestamp_get();
	stuSpd_Param->Linked[seq]=true;
}


// 监测器断线后上传数据全部初始化
void SPD_vars_init(int seq)
{
	int i=0;

	stuSpd_Param->Linked[seq]=false;
	if (seq < SPD_NUM)
	{
		if (seq < SPD_num)
		{
			memset(&stuSpd_Param->rSPD_data[seq],0,sizeof(SPD_REAL_PARAMS));
		}
	}
	else
	{
		memset(&stuSpd_Param->rSPD_res,0,sizeof(SPD_RES_ST_PARAMS));
	}
}

// 断线后的逻辑处理
void SPD_disconnct_process(void)
{
	int i;
	for (i=0;i<SPD_NUM+RES_NUM; i++)
	{
		// 没有配置的直接跳过
		if ((i>=SPD_num) && (i != SPD_NUM))
		{
			continue;
		}
		// 有30s未刷新，断线
		if ((timestamp_delta(stuSpd_Param->TimeStamp[i]) > SPD_DISC_TIMEOUT) && stuSpd_Param->Linked[i])
		{
			SPD_vars_init(i);
		}
	}
}



/*接口函数，其它线程调用这个函数,进行参数的设置*/
/*ai_data: ai参数设置，data:其它参数设置*/
int Ex_SPD_Set_Process(int seq,SPD_CTRL_LIST SPD_ctrl_event, UINT16 set_addr, FDATA ai_data,UINT16 data)
{
	// 如果是SPD_NUM表明所有的防雷监测器都要设置
	if (seq < SPD_NUM)
	{
		spd_ctl_flag[seq] |= BIT(SPD_ctrl_event);	//先置标志位

		SPD_ctrl_value[seq].ref_addr = set_addr;
		if (SPD_ctrl_event == SPD_AI_SET)
		{
			SPD_ctrl_value[seq].f_ai_set = ai_data;
		}
		else if (SPD_ctrl_event == SPD_DO_SET)
		{
			SPD_ctrl_value[seq].do_set = data;
		}
		else if (SPD_ctrl_event == SPD_RES_SET)
		{
			SPD_ctrl_value[seq].res_set = data;
		}
	}
	else
	{
		for (int i = 0; i< SPD_num; i++)
		{
			spd_ctl_flag[i] |= BIT(SPD_ctrl_event);	//先置标志位

			SPD_ctrl_value[i].ref_addr = set_addr;
			if (SPD_ctrl_event == SPD_AI_SET)
			{
				SPD_ctrl_value[i].f_ai_set = ai_data;
			}
			else if (SPD_ctrl_event == SPD_DO_SET)
			{
				SPD_ctrl_value[i].do_set = data;
			}
			else if (SPD_ctrl_event == SPD_RES_SET)
			{
				SPD_ctrl_value[i].res_set = data;
			}
		}
	}

	return 0;
}



/* psd发送数据--读取模拟,DI，DO数据 */
int SPD_Read_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 REFS_COUNT)
{
    UINT8 i,j,bytSend[8]={0x00,0x00,0x00, 0x00, 0x00, 0x00,0x00, 0x00};
    int len=8;

	pthread_mutex_lock(&SPDdataHandleMutex);

    bytSend[MBUS_ADDR]        = Addr;
    bytSend[MBUS_FUNC]        = Func;	// 0x03
    bytSend[MBUS_REFS_ADDR_H] = (REFS_ADDR&0xFF00) >> 8;     // Register address
    bytSend[MBUS_REFS_ADDR_L] =  REFS_ADDR&0x00FF;
    bytSend[MBUS_REFS_COUNT_H] = (REFS_COUNT&0xFF00) >> 8;  // Register counter
    bytSend[MBUS_REFS_COUNT_L] =  REFS_COUNT&0x00FF;

    // CRC calculation
    unsigned short int CRC = CRC16(bytSend,len-2) ;
    bytSend[len-2] = (CRC&0xFF00) >> 8;     //CRC high
    bytSend[len-1] =  CRC&0x00FF;           //CRC low

	//printf("psd Rdata->socket_addr=%d:",sockfd_spd[seq]);
	//for(j=0;j<len;j++)printf("0x%02x ",bytSend[j]);printf("\r\n");

	// 真正的连接标志要connected_flag
	if ((connected_flag[seq] != 0) && (sockfd_spd[seq] != -1))
	{
		printf("write ok!");
		write(sockfd_spd[seq],bytSend,len);
	}
	pthread_mutex_unlock(&SPDdataHandleMutex);

	usleep(5000);	//delay 5ms
	return 0 ;
}


/* psd发送数据--华咨读取防雷数据,REFS_ADDR未使用*/
int SPD_HZ_Read_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 REFS_COUNT)
{
    UINT8 i,j,bytSend[24]={0x00,};
    int len=24;

	pthread_mutex_lock(&SPDdataHandleMutex);

	// 前导码为100,低位在前
    bytSend[0] = 0x64;
    bytSend[1] = 0;
    bytSend[2] = 0;
    bytSend[3] = 0;

	// 版本为1，低位在前
    bytSend[4] = 0x01;
    bytSend[5] = 0;
    bytSend[6] = 0;
    bytSend[7] = 0;

	// 长度为24 = 4*6，低位在前
    bytSend[8] = REFS_COUNT;
    bytSend[9] = 0;
    bytSend[10] = 0;
    bytSend[11] = 0;

	// 地址为1，低位在前
    bytSend[12] = Addr;
    bytSend[13] = 0;
    bytSend[14] = 0;
    bytSend[15] = 0;

	// 命令码为0x0C，低位在前
    bytSend[16] = Func;
    bytSend[17] = 0;
    bytSend[18] = 0;
    bytSend[19] = 0;

	// 校验码全为0
    bytSend[20] = 0;
    bytSend[21] = 0;
    bytSend[22] = 0;
    bytSend[23] = 0;

	printf("psd HZRdata->socket_addr=%d:",sockfd_spd[seq]);
	for(j=0;j<len;j++)printf("0x%02x ",bytSend[j]);printf("\r\n");

	//write(socket_addr,bytSend,len);
	sendto(udpfd_spd[seq], bytSend, REFS_COUNT, 0, (struct sockaddr *)&HZSPDAddr[seq], sizeof(struct sockaddr_in));
	pthread_mutex_unlock(&SPDdataHandleMutex);

	usleep(5000);	//delay 5ms
	return 0 ;
}


/* 中普同安读取防雷数据*/
int SPD_ZPTA_Read_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 REFS_COUNT)
{
    UINT8 i,j,bytSend[24]={0x00,};
	UINT8 sum_cal = 0;
    int len=0;

	pthread_mutex_lock(&SPDdataHandleMutex);

	// 前导码为100,低位在前
    bytSend[len++] = 0x48;		// 帧头
    bytSend[len++] = 0x3A;
    bytSend[len++] = Addr;
    bytSend[len++] = Func;

	// 8个字节都为0
    bytSend[len++] = 0;
    bytSend[len++] = 0;
    bytSend[len++] = 0;
    bytSend[len++] = 0;

	// 长度为24 = 4*6，低位在前
    bytSend[len++] = 0;
    bytSend[len++] = 0;
    bytSend[len++] = 0;
    bytSend[len++] = 0;

	for (i=0; i<len; i++)
	{
		sum_cal += bytSend[i];
	}

	// 地址为1，低位在前
    bytSend[len++] = sum_cal;
    bytSend[len++] = 0x45;		// 帧尾
    bytSend[len++] = 0x44;

	//printf("psd TARdata->socket_addr=%d:",sockfd_spd[seq]);
	for(j=0;j<len;j++)printf("0x%02x ",bytSend[j]);printf("\r\n");

	if ((connected_flag[seq] != 0) && (sockfd_spd[seq] != -1))
	{
		write(sockfd_spd[seq],bytSend,len);
	}
	pthread_mutex_unlock(&SPDdataHandleMutex);

	usleep(5000);	//delay 5ms
	return 0 ;
}


/* psd发送数据--读取模拟,DI，DO数据 */
int SPD_DO_Ctrl_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 cmd)
{
	UINT8 i,j,bytSend[8]={0x00,0x00,0x00, 0x00, 0x00, 0x00,0x00, 0x00};
	int len=0;

	pthread_mutex_lock(&SPDdataHandleMutex);
	bytSend[len++] = Addr;
	bytSend[len++] = Func;	// 0x05
	bytSend[len++] = (REFS_ADDR&0xFF00) >> 8;	 // Register address
	bytSend[len++] =  REFS_ADDR&0x00FF;
	bytSend[len++] = (cmd&0xFF00) >> 8;	// Register counter
	bytSend[len++] =  cmd&0x00FF;

	// CRC calculation
	unsigned short int CRC = CRC16(bytSend,len);
	bytSend[len++] = (CRC&0xFF00) >> 8; 	//CRC high
	bytSend[len++] =  CRC&0x00FF;			//CRC low

	//printf("psd Ddata->socket_addr=%d:",socket_addr);
	//for(j=0;j<len;j++)printf("0x%02x ",bytSend[j]);printf("\r\n");
	if ((connected_flag[seq] != 0) && (sockfd_spd[seq] != -1))
	{
		write(sockfd_spd[seq],bytSend,len);
	}

	pthread_mutex_unlock(&SPDdataHandleMutex);
	usleep(5000);	//delay 5ms
	return 0 ;
}

#define AI_SET_MIN	2		// AI设置每次至少2个数量
/* psd发送数据--读取模拟,DI，DO数据 ,REFS_ADDR要特别注意一一对应*/
int SPD_AI_Set_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 REFS_COUNT, FDATA *data)
{
    UINT8 i,j,bytSend[20];
    int len=0;
	int temp,temp2=0;
	FDATA *pdata = data;

	pthread_mutex_lock(&SPDdataHandleMutex);

	memset(bytSend,0,20);
    bytSend[len++]        = Addr;
    bytSend[len++]        = Func;	// 0x10
    bytSend[len++] = (REFS_ADDR&0xFF00) >> 8;     // Register address
    bytSend[len++] =  REFS_ADDR&0x00FF;
    bytSend[len++] = (REFS_COUNT&0xFF00) >> 8;  // Register counter
    temp = REFS_COUNT&0x00FF;
    bytSend[len++] =  temp;
	temp2 = temp*2;
	bytSend[len++] =  temp2;

	if (temp2 > 12)	// 字节总共超出了12,防止溢出
	{
		return 0;
	}

	for (i = 0; i <temp2/4;i++)
	{
		bytSend[len++] =  pdata->c[0];
		bytSend[len++] =  pdata->c[1];
		bytSend[len++] =  pdata->c[2];
		bytSend[len++] =  pdata->c[3];
		pdata++;
	}

    // CRC calculation
    unsigned short int CRC = CRC16(bytSend,len);	// 这里是len，不是len-2
    bytSend[len++] = (CRC&0xFF00) >> 8;     //CRC high
    bytSend[len++] =  CRC&0x00FF;           //CRC low

	//printf("psd Cdata->socket_addr=%d:",socket_addr);
	//for(j=0;j<len;j++)printf("0x%02x ",bytSend[j]);printf("\r\n");
	if ((connected_flag[seq] != 0) && (sockfd_spd[seq] != -1))
	{
		write(sockfd_spd[seq],bytSend,len);
	}
	pthread_mutex_unlock(&SPDdataHandleMutex);

	usleep(5000);	//delay 5ms
	return 0 ;
}

/* psd发送数据--设置接地电阻部分数据 */
int SPD_Res_Set_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR, UINT16 cmd)
{
	UINT8 i,j,bytSend[8]={0x00,0x00,0x00, 0x00, 0x00, 0x00,0x00, 0x00};
	int len=0;

	pthread_mutex_lock(&SPDdataHandleMutex);
	bytSend[len++]		  = Addr;
	bytSend[len++]		  = Func;	// 0x06
	bytSend[len++] = (REFS_ADDR&0xFF00) >> 8;	 // Register address
	bytSend[len++] =  REFS_ADDR&0x00FF;
	bytSend[len++] = (cmd&0xFF00) >> 8;	// Register counter
	bytSend[len++] =  cmd&0x00FF;

	// CRC calculation
	unsigned short int CRC = CRC16(bytSend,len);
	bytSend[len++] = (CRC&0xFF00) >> 8; 	//CRC high
	bytSend[len++] =  CRC&0x00FF;			//CRC low

	//printf("psd ResSetdata:");
	//for(j=0;j<len;j++)printf("0x%02x ",bytSend[j]);printf("\r\n");
	if ((connected_flag[seq] != 0) && (sockfd_spd[seq] != -1))
	{
		printf("ctrl data ok!/n/r");
		write(sockfd_spd[seq],bytSend,len);
	}

	pthread_mutex_unlock(&SPDdataHandleMutex);
	usleep(5000);	//delay 5ms
	return 0 ;
}


/* psd发送数据--时间同步 */
int SPD_Time_Set_Reg(int seq,UINT8 Addr, UINT8 Func, UINT16 REFS_ADDR,UINT16 REFS_COUNT)
{
	UINT8 i,j,bytSend[30]={0x00,};
	int len=0;
	int temp = 0;
	UINT16 real_year = 2019;

	time_t nSeconds;
	struct tm * pTM;

	pthread_mutex_lock(&SPDdataHandleMutex);
	// 取得当前时间
	time(&nSeconds);
	pTM = localtime(&nSeconds);

	bytSend[len++]        = Addr;
    bytSend[len++]        = Func;	// 0x10
    bytSend[len++] = (REFS_ADDR&0xFF00) >> 8;     // Register address
    bytSend[len++] =  REFS_ADDR&0x00FF;
    bytSend[len++] = (REFS_COUNT&0xFF00) >> 8;  // Register counter
    temp = REFS_COUNT&0x00FF;
    bytSend[len++] =  temp;
	bytSend[len++] =  temp*2;

	// tm_year是从1900开始算的
	real_year = 1900+pTM->tm_year;

	bytSend[len++] = (real_year&0xFF00) >> 8;
	bytSend[len++] = real_year&0x00FF;			// 年

	bytSend[len++] = 0;
	bytSend[len++] = (UINT8)(pTM->tm_mon+1);	// 月

	bytSend[len++] = 0;
	bytSend[len++] = (UINT8)pTM->tm_mday;		// 日

	bytSend[len++] = 0;
	bytSend[len++] = (UINT8)pTM->tm_hour;		// 时

	bytSend[len++] = 0;
	bytSend[len++] = (UINT8)pTM->tm_min;		// 分

	bytSend[len++] = 0;							// 秒
	bytSend[len++] = (UINT8)pTM->tm_sec;

	// CRC calculation
	unsigned short int CRC = CRC16(bytSend,len);
	bytSend[len++] = (CRC&0xFF00) >> 8; 	//CRC high
	bytSend[len++] =  CRC&0x00FF;			//CRC low

	//printf("psd TimeSetdata:");
	//for(j=0;j<len;j++)printf("0x%02x ",bytSend[j]);printf("\r\n");
	if ((connected_flag[seq] != 0) && (sockfd_spd[seq] != -1))
	{
		write(sockfd_spd[seq],bytSend,len);
	}

	pthread_mutex_unlock(&SPDdataHandleMutex);
	usleep(5000);	//delay 5ms
	return 0 ;
}


/*模块化spd数据读取*/
int spd_send_process(UINT16 seq,UINT16 *pnet_flag)
{
	UINT8 addr_temp;
	UINT8 func_temp = HZ_SPD_READ;
	UINT8 len_temp = 0;
	int seq_t = 0;	// 表明这是哪一台装置
	int event_j;
	UINT16 reg_addr = 0;

	if (SPD_Type == TYPE_LEIXUN)
	{
		for(event_j=SPD_AI_DATA; event_j<SPD_DATA_NUM; event_j++)
		{
			if (event_j < SPD_HZ_DATA)
			{
				addr_temp = SPD_Address[seq];
			}
			else if (event_j == SPD_RES_DATA)
			{
				addr_temp = SPD_Address[SPD_NUM];	// 接地电阻仪地址
			}
			else
			{
				// 从其它类型切换过来, 清除掉,然后继续
				*pnet_flag &= ~(BIT(event_j));
				continue;
			}

			if (*pnet_flag & (BIT(event_j)))
			{
				*pnet_flag &= ~(BIT(event_j));
				//读取防雷器和接地电阻的数据
				SPD_Read_Reg(0,addr_temp, g_SPD_LX_Fun_Array[event_j].func_code,\
						g_SPD_LX_Fun_Array[event_j].reg_addr,g_SPD_LX_Fun_Array[event_j].reg_num);
				return 0;
			}
		}
	}
	else if ((SPD_Type == TYPE_KY)|| (SPD_Type == TYPE_KY0M))
	{
		for(event_j=SPD_AI_DATA; event_j<SPD_DATA_NUM; event_j++)
		{
			if ((event_j < SPD_RES_DATA) && (event_j > SPD_HZ_DATA))
			{
				addr_temp = SPD_Address[seq];
				reg_addr = g_SPD_KY_Fun_Array[event_j].reg_addr;
			}
			else if (event_j == SPD_RES_DATA)
			{
				addr_temp = SPD_Address[SPD_NUM];	// 接地电阻仪地址
				KY_res_read_seq++;
				if (KY_res_read_seq >KY_RES_ALARM_ADDR)
				{
					KY_res_read_seq = KY_RES_VALUE_ADDR;
				}
				reg_addr = KY_res_read_seq;
			}
			else
			{
				// 从其它类型切换过来, 清除掉,然后继续
				*pnet_flag &= ~(BIT(event_j));
				continue;
			}

			if (*pnet_flag & (BIT(event_j)))
			{
				*pnet_flag &= ~(BIT(event_j));
				//读取防雷器和接地电阻的数据，雷迅和宽永都只有1个IP地址

				SPD_Read_Reg(0,addr_temp, g_SPD_KY_Fun_Array[event_j].func_code,\
							reg_addr,g_SPD_KY_Fun_Array[event_j].reg_num);
				return 0;
			}
		}
	}
	else if (SPD_Type == TYPE_ZPTA)
	{
		for(event_j=SPD_AI_DATA; event_j<SPD_DATA_NUM; event_j++)
		{
			if (event_j == SPD_HZ_DATA)
			{
				seq_t = seq;
				addr_temp = SPD_Address[seq];
				func_temp = ZPTA_READ_CMD;
			}
			else if (event_j == SPD_RES_DATA)
			{
				seq_t = SPD_NUM;	// 接地电阻单独有1个网口地址
				addr_temp = SPD_Address[SPD_NUM];	// 接地电阻仪地址
				func_temp = ZPTA_READ_CMD;
			}
			else
			{
				// 从其它类型切换过来, 清除掉,然后继续
				*pnet_flag &= ~(BIT(event_j));
				continue;
			}

			if (*pnet_flag & (BIT(event_j)))
			{
				*pnet_flag &= ~(BIT(event_j));
				//读取接地电阻的数据
				if (event_j == SPD_RES_DATA)
				{
					SPD_Read_Reg(seq_t,addr_temp, SPD_RES_READ_CMD,SPD_RES_VALUE_ADDR,SPD_RES_VALUE_NUM);
				}
				else if (event_j == SPD_HZ_DATA)
				{
					SPD_ZPTA_Read_Reg(seq_t,addr_temp, ZPTA_READ_CMD,0,0);
				}
				return 0;
			}
		}
	}

	else if (SPD_Type == TYPE_ZPZH)
	{
		for(event_j=SPD_AI_DATA; event_j<SPD_DATA_NUM; event_j++)
		{
			if (event_j == SPD_HZ_DATA)
			{
				seq_t = seq;
				addr_temp = SPD_Address[seq];
				func_temp = ZPZH_READ_CMD;
				len_temp = ZPZH_SPD_LEN;
			}
			else if (event_j == SPD_RES_DATA)
			{
				seq_t = SPD_NUM;	// 接地电阻单独有1个网口地址
				addr_temp = SPD_Address[SPD_NUM];	// 接地电阻仪地址
				func_temp = ZPZH_READ_CMD;
				len_temp = ZPZH_RES_LEN;
			}
			else
			{
				// 从其它类型切换过来, 清除掉,然后继续
				*pnet_flag &= ~(BIT(event_j));
				continue;
			}

			if (*pnet_flag & (BIT(event_j)))
			{
				*pnet_flag &= ~(BIT(event_j));
				//读取防雷检测和接地电阻的值，标准MODBUS
				SPD_Read_Reg(seq_t,addr_temp, func_temp,ZPZH_SPD_ADDR,len_temp);

				return 0;
			}
		}
	}
	// 华咨的是UDP的
	else
	{
		for(event_j=SPD_AI_DATA; event_j<SPD_DATA_NUM; event_j++)
		{
			if (event_j == SPD_HZ_DATA)
			{
				seq_t = seq;
				addr_temp = SPD_Address[seq];
				func_temp = HZ_SPD_READ;
			}
			else if (event_j == SPD_RES_DATA)
			{
				seq_t = SPD_NUM;	// 接地电阻单独有1个网口地址
				addr_temp = SPD_Address[SPD_NUM];	// 接地电阻仪地址
				func_temp = HZ_RES_READ;
			}
			else
			{
				// 从其它类型切换过来, 清除掉,然后继续
				*pnet_flag &= ~(BIT(event_j));
				continue;
			}

			if (*pnet_flag & (BIT(event_j)))
			{
				*pnet_flag &= ~(BIT(event_j));
				//读取防雷器和接地电阻的数据
				SPD_HZ_Read_Reg(seq_t,addr_temp, func_temp,NULL_VAR,24);
				return 0;
			}
		}
	}
	return 1;
}


/*模块化spd数据设置*/
int spd_ctrl_process(UINT16 seq,UINT16 *pctrl_flag)
{
	UINT8 addr_temp;
	int act_seq = 0;	// 表明这是哪一台装置
	int event_i;

	if (SPD_Type == TYPE_LEIXUN)
	{
		act_seq = 0;	// 形参传通道号，不要传socket号

		for(event_i=SPD_AI_SET; event_i<SPD_CTRL_NUM; event_i++)
		{
			if (event_i < SPD_RES_SET)	// 防雷检测地址
			{
				addr_temp = SPD_Address[seq];
			}
			else
			{
				addr_temp = SPD_Address[SPD_NUM];	// 接地电阻仪地址
			}

			if (*pctrl_flag & (BIT(event_i)))
			{
				*pctrl_flag &= ~(BIT(event_i));
				switch(event_i)
				{
				case SPD_AI_SET:
					// 不要改id,防止误操作
					if (SPD_ctrl_value[seq].ref_addr != AI_SPD_ID_ADDR)
					{
						//SPD_Address[0] = (UINT8)SPD_ctrl_value.f_ai_set.f;
						//设置防雷器的AI数据
						SPD_AI_Set_Reg(act_seq,addr_temp,SPD_WRITE_CMD,SPD_ctrl_value[seq].ref_addr,AI_SET_MIN,&SPD_ctrl_value[seq].f_ai_set);
					}
					break;

				case SPD_DO_SET:
					// DO每次只写1个寄存器
					SPD_DO_Ctrl_Reg(act_seq,addr_temp,SPD_DO_CTRL_CMD,SPD_ctrl_value[seq].ref_addr,SPD_ctrl_value[seq].do_set);
					break;

				case SPD_RES_SET:
					// 不要改id,防止误操作
					if (SPD_ctrl_value[seq].ref_addr != RES_ID_ADDR)
					{
						//SPD_Address[SPD_NUM] = (UINT8)SPD_ctrl_value.res_set;
						// RES接地电阻部分设置
						SPD_Res_Set_Reg(act_seq,addr_temp,SPD_RES_SET_CMD,SPD_ctrl_value[seq].ref_addr,SPD_ctrl_value[seq].res_set);
					}
					break;

				case SPD_TIME_SET:
					// 对时写
					SPD_Time_Set_Reg(act_seq,addr_temp,SPD_WRITE_CMD,SPD_ctrl_value[seq].ref_addr,TIME_SET_LEN);
					break;
				default:
					break;
				}
				return 0;	// 如果有事件发生就要直接返回，防止连续写
			}
		}
	}
	else if ((SPD_Type == TYPE_KY) ||(SPD_Type == TYPE_KY0M))
	{
		//socketq = sockfd_spd[0];
		act_seq = 0;	// 形参传通道号，不要传socket号

		for(event_i=SPD_AI_SET; event_i<SPD_CTRL_NUM; event_i++)
		{
			if (event_i < SPD_RES_SET)	// 防雷检测地址
			{
				addr_temp = SPD_Address[seq];
			}
			else
			{
				addr_temp = SPD_Address[SPD_NUM];	// 接地电阻仪地址
			}

			if (*pctrl_flag & (BIT(event_i)))
			{
				*pctrl_flag &= ~(BIT(event_i));
				switch(event_i)
				{
				case SPD_DO_SET:
					SPD_DO_Ctrl_Reg(act_seq,addr_temp,KY_WRITE_CMD,SPD_ctrl_value[seq].ref_addr,SPD_ctrl_value[seq].do_set);
					break;

				case SPD_RES_SET:
					// 不要改id,防止误操作
					if (SPD_ctrl_value[seq].ref_addr == KY_RES_TEST_ADDR)
					{
						//SPD_Address[SPD_NUM] = (UINT8)SPD_ctrl_value.res_set;
						// RES接地电阻部分设置, 测试电阻是读命令
						// read的参数不是socket，是seq
						SPD_Read_Reg(act_seq,addr_temp, KY_READ_CMD,SPD_ctrl_value[seq].ref_addr,KY_RES_NUM);
						KY_test_disable_cnt = KY_SHIELD_INTERVAL;
						KY_res_read_seq = KY_RES_TEST_ADDR;	// 不要把测试返回值付给了KY_RES_ALARM_ADDR
					}
					else if (SPD_ctrl_value[seq].ref_addr == KY_RES_ALARM_ADDR)
					{
						#if 0
						if (SPD_ctrl_value[seq].res_set == 0)	// 上位机是写入0禁止
						{
							SPD_ctrl_value[seq].res_set = 500;	// 宽永是写入500禁止报警功能
						}
						#endif
						SPD_Res_Set_Reg(act_seq,addr_temp,KY_WRITE_CMD,SPD_ctrl_value[seq].ref_addr,SPD_ctrl_value[seq].res_set);
					}
					break;
				default:
					break;
				}
				return 0;	// 如果有事件发生就要直接返回，防止连续写
			}
		}
	}
	else if (SPD_Type == TYPE_ZPTA)
	{
		for(event_i=SPD_AI_SET; event_i<SPD_CTRL_NUM; event_i++)
		{
			if (event_i < SPD_RES_SET)	// 防雷检测地址
			{
				addr_temp = SPD_Address[seq];
				act_seq = seq;
				//socketq = sockfd_spd[seq];
			}
			else
			{
				addr_temp = SPD_Address[SPD_NUM];	// 接地电阻仪地址
				act_seq = SPD_NUM;
				//socketq = sockfd_spd[SPD_NUM];
				//printf("socketq1 = %d/r/n",socketq);
			}

			if (*pctrl_flag & (BIT(event_i)))
			{
				*pctrl_flag &= ~(BIT(event_i));
				switch(event_i)
				{
				case SPD_RES_SET:
					// 不要改id,防止误操作
					if (SPD_ctrl_value[seq].ref_addr != RES_ID_ADDR)
					{
						//SPD_Address[SPD_NUM] = (UINT8)SPD_ctrl_value.res_set;
						// RES接地电阻部分设置
						printf("socketq2 = %d/r/n",act_seq);
						SPD_Res_Set_Reg(act_seq,addr_temp,SPD_RES_SET_CMD,SPD_ctrl_value[seq].ref_addr,SPD_ctrl_value[seq].res_set);
					}
					break;
				default:
					break;
				}
				return 0;	// 如果有事件发生就要直接返回，防止连续写
			}
		}
	}
	return 1;
}

// 连接到的是哪个网络设备?
int obtain_net_psd(UINT16 seq)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	char str[10*1024];
	const char *IPaddress;
	const char * IPport;
	int port;
	struct sockaddr_in net_psd_addr;
	if( (sockfd_spd[seq] = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
    	printf ("ERROR: Failed to obtain SPD Socket Despcritor.\n");
//		WriteLog("Socket_SPD error!\n");
    	return (0);
	}
	else
	{
    	printf ("OK: Obtain SPD Socket Despcritor sucessfully.\n");
//		WriteLog("Socket_SPD sucessfully.\n");
	}
	IPaddress = pConf->StrSPDIP[seq].c_str();//获取配置文件中的IP地址
	IPport=pConf->StrSPDPort[seq].c_str();
	port=atoi(IPport);
	/* Fill the local socket address struct */
	net_psd_addr.sin_family = AF_INET;           		// Protocol Family
	net_psd_addr.sin_port = htons (port);         		// Port number
	net_psd_addr.sin_addr.s_addr  = inet_addr (IPaddress);  	// AutoFill local address
	memset (net_psd_addr.sin_zero,0,8);          		// Flush the rest of struct
	printf("SPD-IPaddress=%s,SPD-IPport=%s\n",IPaddress,IPport);
	sprintf(str,"SPD-IPaddress=%s,SPD-IPport=%s\n",IPaddress,IPport);
//	WriteLog(str);

	// 经测试，这个函数在连接不同网段的ip时，返回时间长达30s
	// 修改IP后要等下一次connect才能成功连接
	if (connect(sockfd_spd[seq], (struct sockaddr*)&net_psd_addr, sizeof(net_psd_addr)) == -1)
   	 {
		printf("connect to SPD%d server refused!\n",seq);
//		WriteLog("connect to SPD server refused!\n");
		close(sockfd_spd[seq]);
		sockfd_spd[seq] = -1;	// 置为连接失败,否则无法判断是否失败了
		return (0);
	}
	else
	{
		printf("connect to SPD%d server sucess!\n",seq);
		printf("sockfd_spd_seq=%d/r/n",sockfd_spd[seq]);
//		WriteLog("connect to SPD server sucess!\n");
		return(1);
	}
}

#define INT_REG_POS	11	// 前面11个是实数

void DealSPDAiMsg(int seq,unsigned char *buf,unsigned short int len)
{
	if (seq >= SPD_NUM)
	{
		return;
	}

	UINT8 i;
	FDATA *fpointer = &stuSpd_Param->dSPD_AIdata[seq].leak_current;
	UINT16 *pointer = &stuSpd_Param->dSPD_AIdata[seq].systime_year;

	if(len == (SPD_AI_NUM*2+5))
	{
		// 前面11个字节是实数,4个字节
		for(i = 0;i < INT_REG_POS;i++)
		{
			char_to_float(buf + FRAME_HEAD_NUM + i*4, (fpointer+i));
		}
		#if 0
		// 打印出来看调试结果
		printf("leak_current = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.leak_current.f);
		printf("ref_volt = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.ref_volt.f);
		printf("real_volt = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.real_volt.f);
		printf("spd_temp = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.spd_temp.f);
		printf("envi_temp = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.envi_temp.f);
		printf("id = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.id.f);
		printf("struck_cnt = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.struck_cnt.f);
		printf("struck_total = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.struck_total.f);
		printf("soft_version = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.soft_version.f);
		printf("leak_alarm_threshold = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.leak_alarm_threshold.f);
		printf("day_time = %7.3f \r\n",stuSpd_Param->dSPD_AIdata.day_time.f);
		#endif

		pointer = &stuSpd_Param->dSPD_AIdata[seq].systime_year;
		// 后面的字节是整数,2个字节,另外最后一个数据没有变量对应,舍弃掉
		for(i = INT_REG_POS;i < (SPD_AI_NUM-INT_REG_POS-1);i++)		// 从11项开始,前面的实数占了2个位置，所以减掉
		{
			char_to_int(buf+(FRAME_HEAD_NUM + INT_REG_POS*4)+(i-INT_REG_POS)*2, (pointer+i-INT_REG_POS));
		}

		// 更新时间戳
		SPD_timeStamp_update(seq);
		#if 0
		printf("spd_AI begain\r\n");
		printf("systime_year = %5hd \r\n",stuSpd_Param->dSPD_AIdata.systime_year);
		printf("systime_month = %5hd \r\n",stuSpd_Param->dSPD_AIdata.systime_month);
		printf("systime_day = %5hd \r\n",stuSpd_Param->dSPD_AIdata.systime_day);
		printf("systime_hour = %5hd \r\n",stuSpd_Param->dSPD_AIdata.systime_hour);
		printf("systime_min = %5hd \r\n",stuSpd_Param->dSPD_AIdata.systime_min);
		printf("systime_sec = %5hd \r\n",stuSpd_Param->dSPD_AIdata.systime_sec);

		printf("last_1_struck_year = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_1_struck_year);
		printf("last_1_struck_month = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_1_struck_month);
		printf("last_1_struck_day = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_1_struck_day);
		printf("last_1_struck_hour = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_1_struck_hour);
		printf("last_1_struck_min = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_1_struck_min);
		printf("life_time = %5hd \r\n",stuSpd_Param->dSPD_AIdata.life_time);

		printf("last_2_struck_year = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_2_struck_year);
		printf("last_2_struck_month = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_2_struck_month);
		printf("last_2_struck_day = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_2_struck_day);
		printf("last_2_struck_hour = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_2_struck_hour);
		printf("last_2_struck_min = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_2_struck_min);
		printf("reversed0 = %5hd \r\n",stuSpd_Param->dSPD_AIdata.reversed0);

		printf("last_3_struck_year = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_3_struck_year);
		printf("last_3_struck_month = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_3_struck_month);
		printf("last_3_struck_day = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_3_struck_day);
		printf("last_3_struck_hour = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_3_struck_hour);
		printf("last_3_struck_min = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_3_struck_min);
		printf("reversed1 = %5hd \r\n",stuSpd_Param->dSPD_AIdata.reversed1);

		printf("last_4_struck_year = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_4_struck_year);
		printf("last_4_struck_month = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_4_struck_month);
		printf("last_4_struck_day = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_4_struck_day);
		printf("last_4_struck_hour = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_4_struck_hour);
		printf("last_4_struck_min = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_4_struck_min);
		printf("reversed2 = %5hd \r\n",stuSpd_Param->dSPD_AIdata.reversed2);

		printf("last_5_struck_year = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_5_struck_year);
		printf("last_5_struck_month = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_5_struck_month);
		printf("last_5_struck_day = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_5_struck_day);
		printf("last_5_struck_hour = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_5_struck_hour);
		printf("last_5_struck_min = %5hd \r\n",stuSpd_Param->dSPD_AIdata.last_5_struck_min);
		#endif
	}
}

void DealSPDDiMsg(int seq,unsigned char *buf,unsigned short int len)
{
	if (seq >= SPD_NUM)
	{
		return;
	}

	UINT8 i;
	UINT8 *pointer = &stuSpd_Param->dSPD_DI[seq].SPD_DI;

	// 只返回1个字节
	if(len == (1+5))
	{
		//printf("LXspd_DI begain->%d\r\n",seq);
		*pointer = *(buf+FRAME_HEAD_NUM);

		//printf("SPD_DI = 0x%02x \r\n",stuSpd_Param->dSPD_DI[seq].SPD_DI);
	}
}

void DealSPDDoMsg(int seq,unsigned char *buf,unsigned short int len)
{
	UINT8 i;
	UINT8 *pointer = &stuSpd_Param->dSPD_DO[seq].SPD_DO;

	// 只返回6个字节
	if(len == (1+5))
	{
		//printf("LXspd_DO begain->%d\r\n",seq);
		*pointer = *(buf+FRAME_HEAD_NUM);
		//printf("SPD_DO = 0x%02x \r\n",stuSpd_Param->dSPD_DO[seq].SPD_DO);
	}
}


// 雷迅的地阻数据
void DealSPDResStatusMsg(unsigned char *buf,unsigned short int len)
{
	UINT8 i;
	UINT16 *pointer = &stuSpd_Param->rSPD_res.alarm;
	float res_temp = 0;
	UINT16 dot_num = 0;

	// 返回字节数
	if(len == (SPD_RES_STATUS_NUM*2+5))
	{
		printf("spd_res begain\r\n");
		for (i=0;i<SPD_RES_STATUS_NUM;i++)
		{
			char_to_int(buf + FRAME_HEAD_NUM + i*2, (pointer+i));
		}

		dot_num = stuSpd_Param->rSPD_res.grd_res_dot_num;
		res_temp = (float)stuSpd_Param->rSPD_res.grd_res_value;
		if (dot_num > 0)
		{
			for (i=0;i<dot_num;i++)
			{
				// 实数除法
				res_temp = res_temp/10;
			}
			stuSpd_Param->rSPD_res.grd_res_real = res_temp;
		}
		else
		{
			stuSpd_Param->rSPD_res.grd_res_real = res_temp;
		}

		// 如果收到地阻数据，更新timestamp
		SPD_timeStamp_update(SPD_NUM);

		/*
		printf("res_alarm = 0x%02x \r\n",stuSpd_Param->rSPD_res.alarm);
		printf("grd_res_value = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_res_value);
		printf("grd_res_dot_num = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_res_dot_num);
		printf("grd_volt = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_volt);

		printf("test = 0x%02x \r\n",stuSpd_Param->rSPD_res.test);
		printf("id = 0x%02x \r\n",stuSpd_Param->rSPD_res.id);
		printf("alarm_value = 0x%02x \r\n",stuSpd_Param->rSPD_res.alarm_value);
		*/

		printf("grd_res_real = %7.3f \r\n",stuSpd_Param->rSPD_res.grd_res_real);
	}
}

// 把和下位机的协议数据转换成和后台的协议数据
void RealDataCopy(int seq,SPD_DATA_LIST msg_t)
{
	if (seq >= SPD_NUM)
	{
		return;
	}

	UINT16 *pdes = NULL;
	UINT16 *psrc = NULL;
	UINT8 i;

	switch (msg_t)
	{
	case (SPD_AI_DATA):
		// 不能memset清0, 因为会把DI值清掉
		stuSpd_Param->rSPD_data[seq].id = (UINT16)stuSpd_Param->dSPD_AIdata[seq].id.f;
		stuSpd_Param->rSPD_data[seq].ref_volt = stuSpd_Param->dSPD_AIdata[seq].ref_volt.f;
		stuSpd_Param->rSPD_data[seq].real_volt = stuSpd_Param->dSPD_AIdata[seq].real_volt.f;
		stuSpd_Param->rSPD_data[seq].volt_A = NULL_VALUE;	// 雷迅没有
		stuSpd_Param->rSPD_data[seq].volt_B = NULL_VALUE;
		stuSpd_Param->rSPD_data[seq].volt_C = NULL_VALUE;
		stuSpd_Param->rSPD_data[seq].leak_current = stuSpd_Param->dSPD_AIdata[seq].leak_current.f;
		stuSpd_Param->rSPD_data[seq].leak_A = NULL_VALUE;
		stuSpd_Param->rSPD_data[seq].leak_B = NULL_VALUE;
		stuSpd_Param->rSPD_data[seq].leak_C = NULL_VALUE;
		stuSpd_Param->rSPD_data[seq].struck_cnt = stuSpd_Param->dSPD_AIdata[seq].struck_cnt.f;
		stuSpd_Param->rSPD_data[seq].struck_total = stuSpd_Param->dSPD_AIdata[seq].struck_total.f;
		stuSpd_Param->rSPD_data[seq].spd_temp = stuSpd_Param->dSPD_AIdata[seq].spd_temp.f;
		stuSpd_Param->rSPD_data[seq].envi_temp = stuSpd_Param->dSPD_AIdata[seq].envi_temp.f;
		stuSpd_Param->rSPD_data[seq].life_time = (float)stuSpd_Param->dSPD_AIdata[seq].life_time;
		stuSpd_Param->rSPD_data[seq].soft_version = stuSpd_Param->dSPD_AIdata[seq].soft_version.f;
		stuSpd_Param->rSPD_data[seq].leak_alarm_threshold = stuSpd_Param->dSPD_AIdata[seq].leak_alarm_threshold.f;
		stuSpd_Param->rSPD_data[seq].day_time = stuSpd_Param->dSPD_AIdata[seq].day_time.f;

		pdes = &stuSpd_Param->rSPD_data[seq].systime_year;
		psrc = &stuSpd_Param->dSPD_AIdata[seq].systime_year;

		// 系统时间
		for(i = 0;i < 6;i++)
		{
			*pdes = *psrc;
			pdes++;
			psrc++;
		}
		// 最近1次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_1_struck_year;
		psrc = &stuSpd_Param->dSPD_AIdata[seq].last_1_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = *psrc;
			pdes++;
			psrc++;
		}
		// 倒数第2次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_2_struck_year;
		psrc = &stuSpd_Param->dSPD_AIdata[seq].last_2_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = *psrc;
			pdes++;
			psrc++;
		}
		// 倒数第3次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_3_struck_year;
		psrc = &stuSpd_Param->dSPD_AIdata[seq].last_3_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = *psrc;
			pdes++;
			psrc++;
		}
		// 倒数第4次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_4_struck_year;
		psrc = &stuSpd_Param->dSPD_AIdata[seq].last_4_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = *psrc;
			pdes++;
			psrc++;
		}
		// 倒数第5次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_5_struck_year;
		psrc = &stuSpd_Param->dSPD_AIdata[seq].last_5_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = *psrc;
			pdes++;
			psrc++;
		}
		#if 0
		printf("LXspd_real begain,%5hd\r\n",seq);
		printf("leak_current = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_current);
		printf("A_leak_current = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_A);
		printf("B_leak_current = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_B);
		printf("C_leak_current = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_C);
		printf("ref_volt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].ref_volt);
		printf("real_volt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].real_volt);
		printf("volt_A = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_A);
		printf("volt_B = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_B);
		printf("volt_C = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_C);
		printf("struck_cnt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].struck_cnt);
		printf("struck_total = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].struck_total);
		printf("spd_temp = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].spd_temp);
		printf("envi_temp = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].envi_temp);
		printf("id =  %5hd \r\n",stuSpd_Param->rSPD_data[seq].id);
		printf("soft_version = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].soft_version);
		printf("leak_alarm_threshold = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_alarm_threshold);
		printf("day_time = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].day_time);
		printf("life_time = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].life_time);

		printf("systime_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_year);
		printf("systime_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_month);
		printf("systime_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_day);
		printf("systime_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_hour);
		printf("systime_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_min);
		printf("systime_sec = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_sec);

		printf("last_1_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_year);
		printf("last_1_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_month);
		printf("last_1_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_day);
		printf("last_1_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_hour);
		printf("last_1_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_min);

		printf("last_2_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_year);
		printf("last_2_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_month);
		printf("last_2_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_day);
		printf("last_2_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_hour);
		printf("last_2_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_min);

		printf("last_3_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_year);
		printf("last_3_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_month);
		printf("last_3_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_day);
		printf("last_3_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_hour);
		printf("last_3_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_min);

		printf("last_4_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_year);
		printf("last_4_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_month);
		printf("last_4_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_day);
		printf("last_4_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_hour);
		printf("last_4_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_min);

		printf("last_5_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_year);
		printf("last_5_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_month);
		printf("last_5_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_day);
		printf("last_5_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_hour);
		printf("last_5_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_min);
		#endif
		break;

	case (SPD_DI_DATA):
		// 这2个0：告警，1:正常,所以要反一下
		stuSpd_Param->rSPD_data[seq].DI_C1_status = (stuSpd_Param->dSPD_DI[seq].SPD_DI & BIT(3))?0:1;
		stuSpd_Param->rSPD_data[seq].DI_leak_alarm  = (stuSpd_Param->dSPD_DI[seq].SPD_DI & BIT(6))?0:1;
		// 这里0：正常，1:告警
		stuSpd_Param->rSPD_data[seq].DI_grd_alarm = (stuSpd_Param->dSPD_DI[seq].SPD_DI & BIT(4))?1:0;
		stuSpd_Param->rSPD_data[seq].DI_volt_alarm  = (stuSpd_Param->dSPD_DI[seq].SPD_DI & BIT(7))?1:0;

		printf("C1_status%d = 0x%02x \r\n",seq,stuSpd_Param->rSPD_data[seq].DI_C1_status);
		printf("grd_alarm%d = 0x%02x \r\n",seq,stuSpd_Param->rSPD_data[seq].DI_grd_alarm);
		printf("leak_alarm%d = 0x%02x \r\n",seq,stuSpd_Param->rSPD_data[seq].DI_leak_alarm);
		printf("volt_alarm%d = 0x%02x \r\n",seq,stuSpd_Param->rSPD_data[seq].DI_volt_alarm);
		break;

	case (SPD_DO_DATA):
		stuSpd_Param->rSPD_data[seq].DO_spdcnt_clear = (stuSpd_Param->dSPD_DO[seq].SPD_DO & BIT(0))?1:0;
		stuSpd_Param->rSPD_data[seq].DO_totalspdcnt_clear = (stuSpd_Param->dSPD_DO[seq].SPD_DO & BIT(1))?1:0;
		stuSpd_Param->rSPD_data[seq].DO_leak_type = (stuSpd_Param->dSPD_DO[seq].SPD_DO & BIT(2))?1:0;
		stuSpd_Param->rSPD_data[seq].DO_psdtime_clear = (stuSpd_Param->dSPD_DO[seq].SPD_DO & BIT(4))?1:0;
		stuSpd_Param->rSPD_data[seq].DO_daytime_clear = (stuSpd_Param->dSPD_DO[seq].SPD_DO & BIT(5))?1:0;

		//printf("DO_leak_type%d = 0x%02x \r\n",seq,stuSpd_Param->rSPD_data[0].DO_leak_type);
		break;

	case (SPD_HZ_DATA):
		if (SPD_Type == TYPE_HUAZI)
		{
			stuSpd_Param->rSPD_data[seq].id = 1;	// 地址是1不会变
			stuSpd_Param->rSPD_data[seq].ref_volt = NULL_VALUE;	// 没有的清0
			stuSpd_Param->rSPD_data[seq].real_volt = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_A = (float)stuSpd_Param->dSPD_HZ[seq].volt_A/10;
			stuSpd_Param->rSPD_data[seq].volt_B = (float)stuSpd_Param->dSPD_HZ[seq].volt_B/10;
			stuSpd_Param->rSPD_data[seq].volt_C = (float)stuSpd_Param->dSPD_HZ[seq].volt_C/10;

			stuSpd_Param->rSPD_data[seq].leak_current = NULL_VALUE;
			// 漏电流转为mA单位
			stuSpd_Param->rSPD_data[seq].leak_A = (float)stuSpd_Param->dSPD_HZ[seq].leak_A/10000;
			stuSpd_Param->rSPD_data[seq].leak_B = (float)stuSpd_Param->dSPD_HZ[seq].leak_B/10000;
			stuSpd_Param->rSPD_data[seq].leak_C = (float)stuSpd_Param->dSPD_HZ[seq].leak_C/10000;

			stuSpd_Param->rSPD_data[seq].DI_C1_status = (float)stuSpd_Param->dSPD_HZ[seq].breaker_alarm;
			stuSpd_Param->rSPD_data[seq].DI_grd_alarm = (float)stuSpd_Param->dSPD_HZ[seq].grd_alarm;
			stuSpd_Param->rSPD_data[seq].struck_cnt = (float)stuSpd_Param->dSPD_HZ[seq].struck_cnt;
			stuSpd_Param->rSPD_data[seq].struck_total = (float)stuSpd_Param->dSPD_HZ[seq].struck_cnt;
			stuSpd_Param->rSPD_data[seq].spd_temp = (float)stuSpd_Param->dSPD_HZ[seq].spd_temp/10;
			stuSpd_Param->rSPD_data[seq].envi_temp = (float)stuSpd_Param->dSPD_HZ[seq].envi_temp/10;
			stuSpd_Param->rSPD_data[seq].life_time = (float)stuSpd_Param->dSPD_HZ[seq].life_time/10;
			stuSpd_Param->rSPD_data[seq].soft_version = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_alarm_threshold = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].day_time = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_0 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_1 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_2 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_5 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_leak_alarm = 0;
			stuSpd_Param->rSPD_data[seq].DI_volt_alarm = 0;
		}
		else if (SPD_Type == TYPE_ZPTA)
		{
			// 中普同安只有脱扣报警
			stuSpd_Param->rSPD_data[seq].id = 1;	// 地址是1不会变
			stuSpd_Param->rSPD_data[seq].ref_volt = NULL_VALUE;	// 没有的清0
			stuSpd_Param->rSPD_data[seq].real_volt = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_A = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_B = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_C = NULL_VALUE;

			stuSpd_Param->rSPD_data[seq].leak_current = NULL_VALUE;
			// 漏电流转为mA单位
			stuSpd_Param->rSPD_data[seq].leak_A = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_B = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_C = NULL_VALUE;

			// 接成常闭，正常是1，异常是0，所以要反一下
			stuSpd_Param->rSPD_data[seq].DI_C1_status = stuSpd_Param->dSPD_ZPTA[seq].SPD_DI[0]?0:1;
			stuSpd_Param->rSPD_data[seq].DI_grd_alarm = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].struck_cnt = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].struck_total = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].spd_temp = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].envi_temp = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].life_time = stuSpd_Param->rSPD_data[seq].DI_C1_status?0:100;	// 有报警就是0，无报警就是100%
			stuSpd_Param->rSPD_data[seq].soft_version = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_alarm_threshold = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].day_time = NULL_VALUE;
			// 第3路和第0路反过来
			stuSpd_Param->rSPD_data[seq].DI_bit_0 = stuSpd_Param->dSPD_ZPTA[seq].SPD_DI[3];
			stuSpd_Param->rSPD_data[seq].DI_bit_1 = stuSpd_Param->dSPD_ZPTA[seq].SPD_DI[1];
			stuSpd_Param->rSPD_data[seq].DI_bit_2 = stuSpd_Param->dSPD_ZPTA[seq].SPD_DI[2];
			stuSpd_Param->rSPD_data[seq].DI_bit_5 = stuSpd_Param->dSPD_ZPTA[seq].SPD_DI[5];
			stuSpd_Param->rSPD_data[seq].DI_leak_alarm = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_volt_alarm = NULL_VALUE;
		}

		else if (SPD_Type == TYPE_ZPZH)
		{
			// 中普众合只有温度，雷击次数和断线报警
			stuSpd_Param->rSPD_data[seq].id = 1;	// 地址是1不会变
			stuSpd_Param->rSPD_data[seq].ref_volt = NULL_VALUE;	// 没有的清0
			stuSpd_Param->rSPD_data[seq].real_volt = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_A = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_B = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_C = NULL_VALUE;

			stuSpd_Param->rSPD_data[seq].leak_current = NULL_VALUE;
			// 漏电流转为mA单位
			stuSpd_Param->rSPD_data[seq].leak_A = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_B = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_C = NULL_VALUE;

			// 接成常闭，正常是1，异常是0，所以要反一下
			stuSpd_Param->rSPD_data[seq].DI_C1_status = (stuSpd_Param->dSPD_ZPZH[seq].DI & BIT(0))?0:1;
			stuSpd_Param->rSPD_data[seq].DI_grd_alarm = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].struck_cnt = stuSpd_Param->dSPD_ZPZH[seq].struck_cnt;
			stuSpd_Param->rSPD_data[seq].struck_total = stuSpd_Param->dSPD_ZPZH[seq].struck_cnt;
			stuSpd_Param->rSPD_data[seq].spd_temp = (INT16)(stuSpd_Param->dSPD_ZPZH[seq].temp_h<<8 |stuSpd_Param->dSPD_ZPZH[seq].temp_l);
			stuSpd_Param->rSPD_data[seq].spd_temp /= 10;		// 实际值要除以10
			stuSpd_Param->rSPD_data[seq].envi_temp = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].life_time = stuSpd_Param->rSPD_data[seq].DI_C1_status?0:100;	// 有报警就是0，无报警就是100%
			stuSpd_Param->rSPD_data[seq].soft_version = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_alarm_threshold = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].day_time = NULL_VALUE;
			// 第3路和第0路反过来
			stuSpd_Param->rSPD_data[seq].DI_bit_0 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_1 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_2 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_5 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_leak_alarm = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_volt_alarm = NULL_VALUE;
		}

		pdes = &stuSpd_Param->rSPD_data[seq].systime_year;
		// 系统时间
		for(i = 0;i < 6;i++)
		{
			*pdes = 0;
			pdes++;
		}
		// 最近1次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_1_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}
		// 倒数第2次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_2_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}
		// 倒数第3次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_3_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}
		// 倒数第4次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_4_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}
		// 倒数第5次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_5_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}

		#if 0
		printf("HZspd_real begain,%5hd\r\n",seq);
		printf("breaker_alarm = %5hd \r\n",stuSpd_Param->rSPD_data[seq].DI_C1_status);
		printf("grd_alarm = %5hd \r\n",stuSpd_Param->rSPD_data[seq].DI_grd_alarm);
		printf("leak_current = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_current);
		printf("A_leak_current = %7.4f \r\n",stuSpd_Param->rSPD_data[seq].leak_A);
		printf("B_leak_current = %7.4f \r\n",stuSpd_Param->rSPD_data[seq].leak_B);
		printf("C_leak_current = %7.4f \r\n",stuSpd_Param->rSPD_data[seq].leak_C);
		printf("ref_volt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].ref_volt);
		printf("real_volt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].real_volt);
		printf("volt_A = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_A);
		printf("volt_B = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_B);
		printf("volt_C = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_C);
		printf("struck_cnt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].struck_cnt);
		printf("struck_total = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].struck_total);
		printf("spd_temp = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].spd_temp);
		printf("envi_temp = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].envi_temp);
		printf("id =  %5hd \r\n",stuSpd_Param->rSPD_data[seq].id);
		printf("soft_version = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].soft_version);
		printf("leak_alarm_threshold = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_alarm_threshold);
		printf("day_time = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].day_time);
		printf("life_time = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].life_time);

		printf("systime_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_year);
		printf("systime_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_month);
		printf("systime_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_day);
		printf("systime_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_hour);
		printf("systime_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_min);
		printf("systime_sec = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_sec);

		printf("last_1_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_year);
		printf("last_1_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_month);
		printf("last_1_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_day);
		printf("last_1_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_hour);
		printf("last_1_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_min);

		printf("last_2_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_year);
		printf("last_2_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_month);
		printf("last_2_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_day);
		printf("last_2_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_hour);
		printf("last_2_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_min);

		printf("last_3_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_year);
		printf("last_3_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_month);
		printf("last_3_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_day);
		printf("last_3_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_hour);
		printf("last_3_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_min);

		printf("last_4_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_year);
		printf("last_4_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_month);
		printf("last_4_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_day);
		printf("last_4_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_hour);
		printf("last_4_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_min);

		printf("last_5_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_year);
		printf("last_5_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_month);
		printf("last_5_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_day);
		printf("last_5_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_hour);
		printf("last_5_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_min);
		#endif
		break;

	case (SPD_RES_DATA):
		break;
	case (SPD_RUN_DATA):
		if (SPD_Type == TYPE_KY)
		{
			stuSpd_Param->rSPD_data[seq].id = SPD_Address[seq];	// 协议里面没有ID，直接读设置值
			stuSpd_Param->rSPD_data[seq].ref_volt = NULL_VALUE;	// 没有的清0
			stuSpd_Param->rSPD_data[seq].real_volt = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_A = (float)stuSpd_Param->dSPD_KY[seq].volt_A/10;
			stuSpd_Param->rSPD_data[seq].volt_B = (float)stuSpd_Param->dSPD_KY[seq].volt_B/10;
			stuSpd_Param->rSPD_data[seq].volt_C = (float)stuSpd_Param->dSPD_KY[seq].volt_C/10;

			// 说漏电流对应B相的数据,只取1路
			stuSpd_Param->rSPD_data[seq].leak_current = (float)stuSpd_Param->dSPD_KY[seq].current_B*10;	// 本来是要 /100但是要从A转化成mA
			// 只有电流值，没有漏电流
			stuSpd_Param->rSPD_data[seq].leak_A = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_B = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_C = NULL_VALUE;

			stuSpd_Param->rSPD_data[seq].struck_cnt = (float)stuSpd_Param->dSPD_KY[seq].struck_cnt;
			stuSpd_Param->rSPD_data[seq].struck_total = (float)stuSpd_Param->dSPD_KY[seq].struck_cnt;
			stuSpd_Param->rSPD_data[seq].spd_temp = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].envi_temp = ((float)stuSpd_Param->dSPD_KY[seq].temp-1000)/10;
			// 宽永给出的劣化程度，和寿命值相反，然后接常开，又反一次，所以就不用反了
			stuSpd_Param->rSPD_data[seq].life_time = (float)stuSpd_Param->dSPD_KY[seq].life_time/10;
			if (stuSpd_Param->rSPD_data[seq].life_time < 0)
			{
				stuSpd_Param->rSPD_data[seq].life_time = 0;
			}
			else if (stuSpd_Param->rSPD_data[seq].life_time > 100)
			{
				stuSpd_Param->rSPD_data[seq].life_time = 100;
			}
			stuSpd_Param->rSPD_data[seq].soft_version = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_alarm_threshold = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].day_time = NULL_VALUE;

			printf("KYspd_RUN begain,%5hd\r\n",seq);
		}
		else if (SPD_Type == TYPE_KY0M)
		{
			stuSpd_Param->rSPD_data[seq].id = SPD_Address[seq];	// 协议里面没有ID，直接读设置值
			stuSpd_Param->rSPD_data[seq].ref_volt = NULL_VALUE;	// 没有的清0
			stuSpd_Param->rSPD_data[seq].real_volt = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_A = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_B = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].volt_C = NULL_VALUE;

			// 说漏电流对应B相的数据,只取1路
			stuSpd_Param->rSPD_data[seq].leak_current = NULL_VALUE;	// 本来是要 /100但是要从A转化成mA
			// 只有电流值，没有漏电流
			stuSpd_Param->rSPD_data[seq].leak_A = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_B = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_C = NULL_VALUE;

			stuSpd_Param->rSPD_data[seq].struck_cnt = (float)stuSpd_Param->dSPD_KY[seq].struck_cnt;
			stuSpd_Param->rSPD_data[seq].struck_total = (float)stuSpd_Param->dSPD_KY[seq].struck_cnt;
			stuSpd_Param->rSPD_data[seq].spd_temp = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].envi_temp = NULL_VALUE;

			// 有报警就是0，无报警就是100%
			stuSpd_Param->rSPD_data[seq].life_time = stuSpd_Param->rSPD_data[seq].DI_C1_status?0:100;
			//stuSpd_Param->rSPD_data[seq].life_time = (float)stuSpd_Param->dSPD_KY[seq].life_time/10;
			if (stuSpd_Param->rSPD_data[seq].life_time < 0)
			{
				stuSpd_Param->rSPD_data[seq].life_time = 0;
			}
			else if (stuSpd_Param->rSPD_data[seq].life_time > 100)
			{
				stuSpd_Param->rSPD_data[seq].life_time = 100;
			}
			stuSpd_Param->rSPD_data[seq].soft_version = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].leak_alarm_threshold = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].day_time = NULL_VALUE;

			printf("KY0mspd_RUN begain,%5hd\r\n",seq);
		}
		#if 0
		printf("leak_current = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_current);
		printf("A_leak_current = %7.4f \r\n",stuSpd_Param->rSPD_data[seq].leak_A);
		printf("B_leak_current = %7.4f \r\n",stuSpd_Param->rSPD_data[seq].leak_B);
		printf("C_leak_current = %7.4f \r\n",stuSpd_Param->rSPD_data[seq].leak_C);
		printf("ref_volt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].ref_volt);
		printf("real_volt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].real_volt);
		printf("volt_A = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_A);
		printf("volt_B = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_B);
		printf("volt_C = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].volt_C);
		printf("struck_cnt = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].struck_cnt);
		printf("struck_total = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].struck_total);
		printf("spd_temp = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].spd_temp);
		printf("envi_temp = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].envi_temp);
		printf("id =  %5hd \r\n",stuSpd_Param->rSPD_data[seq].id);
		printf("soft_version = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].soft_version);
		printf("leak_alarm_threshold = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].leak_alarm_threshold);
		printf("day_time = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].day_time);
		printf("life_time = %7.3f \r\n",stuSpd_Param->rSPD_data[seq].life_time);
		#endif
		break;

	case (SPD_REMOTE_DATA):
		if (SPD_Type == TYPE_KY)
		{
			stuSpd_Param->rSPD_data[seq].DI_bit_0 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_1 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_2 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_5 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_leak_alarm = 0;
			stuSpd_Param->rSPD_data[seq].DI_volt_alarm = 0;

			// 0：闭合	  1:是断开, 因为外部是NO接在bit0位，所以闭合0是报警
			stuSpd_Param->rSPD_data[seq].DI_C1_status = (stuSpd_Param->dSPD_KY[seq].di_alarm & BIT(0))?0:1;
			stuSpd_Param->rSPD_data[seq].DI_grd_alarm = (stuSpd_Param->dSPD_KY[seq].di_alarm & BIT(2))?1:0;

			printf("KYspd_DI begain,%5hd\r\n",seq);
		}
		else if (SPD_Type == TYPE_KY0M)
		{
			stuSpd_Param->rSPD_data[seq].DI_bit_0 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_1 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_2 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_bit_5 = NULL_VALUE;
			stuSpd_Param->rSPD_data[seq].DI_leak_alarm = 0;
			stuSpd_Param->rSPD_data[seq].DI_volt_alarm = 0;

			// 0:闭合	  1:是断开, 因为外部是NC接在bit1位，所以断开1是报警
			// 协议改变了，接地告警变成了bit1, 1：断开，0: 闭合
			stuSpd_Param->rSPD_data[seq].DI_C1_status = (stuSpd_Param->dSPD_KY[seq].di_alarm & BIT(1))?1:0;
			// 协议改变了，接地告警变成了bit0, 1：断开，0: 接地良好
			stuSpd_Param->rSPD_data[seq].DI_grd_alarm = (stuSpd_Param->dSPD_KY[seq].di_alarm & BIT(0))?1:0;;

			printf("KY0Mspd_DI begain,%5hd\r\n",seq);
		}
		printf("breaker_alarm = %5hd \r\n",stuSpd_Param->rSPD_data[seq].DI_C1_status);
		printf("grd_alarm = %5hd \r\n",stuSpd_Param->rSPD_data[seq].DI_grd_alarm);
		break;

	case (SPD_REC_DATA):
		pdes = &stuSpd_Param->rSPD_data[seq].systime_year;
		// 系统时间
		for(i = 0;i < 6;i++)
		{
			*pdes = 0;
			pdes++;
		}
		if ((SPD_Type == TYPE_KY) || (SPD_Type == TYPE_KY0M))
		{
			pdes = &stuSpd_Param->rSPD_data[seq].last_1_struck_year;
			psrc = &stuSpd_Param->dSPD_KY[seq].struck_year;
			// 只取最近一次的时间
			for(i = 0;i < 6;i++)
			{
				*pdes = *psrc;
				pdes++;
				psrc++;
			}
			printf("KYspd_HIS begain,%5hd\r\n",seq);
		}
		#if 0
		else if (SPD_Type == TYPE_KY0M)
		{
			pdes = &stuSpd_Param->rSPD_data[seq].last_1_struck_year;
			for(i = 0;i < 6;i++)
			{
				*pdes = 0;
				pdes++;
			}
			printf("KY0Mspd_HIS begain,%5hd\r\n",seq);
		}
		#endif
		// 倒数第2次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_2_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}
		// 倒数第3次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_3_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}
		// 倒数第4次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_4_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}
		// 倒数第5次雷击
		pdes = &stuSpd_Param->rSPD_data[seq].last_5_struck_year;
		for(i = 0;i < 5;i++)
		{
			*pdes = 0;
			pdes++;
		}
		#if 0
		printf("systime_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_year);
		printf("systime_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_month);
		printf("systime_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_day);
		printf("systime_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_hour);
		printf("systime_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_min);
		printf("systime_sec = %5hd \r\n",stuSpd_Param->rSPD_data[seq].systime_sec);

		printf("last_1_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_year);
		printf("last_1_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_month);
		printf("last_1_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_day);
		printf("last_1_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_hour);
		printf("last_1_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_1_struck_min);

		printf("last_2_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_year);
		printf("last_2_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_month);
		printf("last_2_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_day);
		printf("last_2_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_hour);
		printf("last_2_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_2_struck_min);

		printf("last_3_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_year);
		printf("last_3_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_month);
		printf("last_3_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_day);
		printf("last_3_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_hour);
		printf("last_3_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_3_struck_min);

		printf("last_4_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_year);
		printf("last_4_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_month);
		printf("last_4_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_day);
		printf("last_4_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_hour);
		printf("last_4_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_4_struck_min);

		printf("last_5_struck_year = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_year);
		printf("last_5_struck_month = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_month);
		printf("last_5_struck_day = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_day);
		printf("last_5_struck_hour = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_hour);
		printf("last_5_struck_min = %5hd \r\n",stuSpd_Param->rSPD_data[seq].last_5_struck_min);
		#endif
		break;

	default:
		break;
	}
}

// 华咨防雷数据的解析
void DealHZSPDMsg(int seq,unsigned char *buf,unsigned short int len)
{
	UINT8 i;
	UINT16 *pointer = &stuSpd_Param->dSPD_HZ[seq].breaker_alarm;

	// 返回46个字节
	if(len == HZ_SPD_LEN)
	{
		//printf("hz_spd_data begain\r\n");

		for (i=0;i<HZ_SPD_DATA_NUM;i++)
		{
			HZ_char_to_int(buf + HZ_HEAD_NUM + i*2, (pointer+i));
			//printf("HZitem%5hd = %5hd \r\n",i,*(pointer+i));
		}
		//printf("life_time = %5hd \r\n",stuSpd_Param->dSPD_HZ[seq].life_time);
		// 更新SPD时间戳
		SPD_timeStamp_update(seq);
	}
}

// 华咨地阻数据的解析
void DealHZResMsg(int seq,unsigned char *buf,unsigned short int len)
{
	UINT8 i;
	UINT16 *pointer = &stuSpd_Param->rSPD_res.grd_res_value;

	// 返回26个字节
	if(len == HZ_RES_LEN)
	{
		// 前面2个字节保留
		HZ_char_to_int(buf + HZ_HEAD_NUM +2*2, pointer);
		stuSpd_Param->rSPD_res.grd_res_real = (float)stuSpd_Param->rSPD_res.grd_res_value/100;
		//printf("HZ_grd_res_real = %7.3f \r\n",stuSpd_Param->rSPD_res.grd_res_real);
		// 更新SPD时间戳
		SPD_timeStamp_update(SPD_NUM);
	}
}

/////////////////////////////////////////////////////////////////////////////
//////////////////////   宽永的函数           ////////////////////////////////////
void DealKYResMsg(unsigned char *buf,unsigned short int len)
{
	UINT8 i;
	UINT16 *pointer = &stuSpd_Param->rSPD_res.grd_res_value;

	if (len == (KY_RES_NUM*2+5))
	{
		if (KY_res_read_seq == KY_RES_VALUE_ADDR)
		{
			pointer = &stuSpd_Param->rSPD_res.grd_res_value;
			//*pointer = *(buf+FRAME_HEAD_NUM);		// 这里只赋值了1个字节
			char_to_int(buf + FRAME_HEAD_NUM, pointer);
			// 不处理
			#if 0
			if (stuSpd_Param->rSPD_res.grd_res_value >= 50000)
			{
				stuSpd_Param->rSPD_res.grd_res_value = 0xFFFF;		// 赋给1个无效值
			}
			#endif
			// 2位小数
			stuSpd_Param->rSPD_res.grd_res_real = ((float)stuSpd_Param->rSPD_res.grd_res_value/100);
		}
		else if (KY_res_read_seq == KY_RES_ID_ADDR)
		{
			pointer = &stuSpd_Param->rSPD_res.id;
			//*pointer = *(buf+FRAME_HEAD_NUM);
			char_to_int(buf + FRAME_HEAD_NUM, pointer);
		}
		else if (KY_res_read_seq == KY_RES_ALARM_ADDR)
		{
			pointer = &stuSpd_Param->rSPD_res.alarm_value;
			//*pointer = *(buf+FRAME_HEAD_NUM);
			char_to_int(buf + FRAME_HEAD_NUM, pointer);
		}

		// 宽永,更新地阻的时间戳
		SPD_timeStamp_update(SPD_NUM);
		/*
		printf("KY_grd_res_value = %5.2f \r\n",stuSpd_Param->rSPD_res.grd_res_real);
		printf("KY_id = %5hd \r\n",stuSpd_Param->rSPD_res.id);
		printf("alarm_value = %u \r\n",stuSpd_Param->rSPD_res.alarm_value);
		*/
	}
}



void DealKYSPDMsg(int seq,unsigned char *buf,unsigned short int len)
{
	if (seq >= SPD_NUM)
	{
		return;
	}

	UINT8 i;
	UINT16 *pointer = &stuSpd_Param->dSPD_KY[seq].current_A;
	static UINT16 last_struk_peak[SPD_NUM] = {0,0};

	// 只返回1个字节
	if (len == (KY_RUN_NUM*2+5))
	{
		pointer = &stuSpd_Param->dSPD_KY[seq].current_A;
		//printf("KYspd_RUN begain->%d\r\n",seq);
		for (i=0;i<KY_RUN_NUM;i++)
		{
			char_to_int(buf + FRAME_HEAD_NUM + i*2, (pointer+i));
			//printf("KYRUN%5hd = %5hd \r\n",i,*(pointer+i));
		}
		RealDataCopy(seq,SPD_RUN_DATA);
		// 更新SPD时间戳
		SPD_timeStamp_update(seq);
	}
	else if (len == (KY_DI_NUM*2+5))
	{
		pointer = &stuSpd_Param->dSPD_KY[seq].life_time_alarm;
		printf("KYspd_DI begain->%d\r\n",seq);
		for (i=0;i<KY_DI_NUM;i++)
		{
			char_to_int(buf + FRAME_HEAD_NUM + i*2, (pointer+i));
			//printf("KYDI%5hd = %5hd \r\n",i,*(pointer+i));
		}
		RealDataCopy(seq,SPD_REMOTE_DATA);
	}
	else if (len == (KY_HIS_NUM*2+5))
	{
		pointer = &stuSpd_Param->dSPD_KY[seq].struk_peak;
		printf("KYspd_HIS begain->%d\r\n",seq);
		for (i=0;i<KY_HIS_NUM;i++)
		{
			char_to_int(buf + FRAME_HEAD_NUM + i*2, (pointer+i));
			//printf("KYHIS%5hd = %5hd \r\n",i,*(pointer+i));
		}
		// 如果雷击次数已经被清0了，时间也要清0
		if (stuSpd_Param->rSPD_data[seq].struck_cnt == 0)
		{
			stuSpd_Param->rSPD_data[seq].last_1_struck_year = 0;
			stuSpd_Param->rSPD_data[seq].last_1_struck_month = 0;
			stuSpd_Param->rSPD_data[seq].last_1_struck_day = 0;
			stuSpd_Param->rSPD_data[seq].last_1_struck_hour = 0;
			stuSpd_Param->rSPD_data[seq].last_1_struck_min = 0;
		}
		// 只取最近的记录，断电后会丢失
		// 不用his_num，因为测试发现his_num是剩余条数，当只发生1条时，返回也是0
		if ((last_struk_peak[seq] == 0) && (stuSpd_Param->dSPD_KY[seq].struk_peak != 0))
		{
			printf("copy HIS begins \r\n");
			RealDataCopy(seq,SPD_REC_DATA);
		}
		last_struk_peak[seq] = stuSpd_Param->dSPD_KY[seq].struk_peak;
	}
}


/////////////////////////////////////////////////////////////////////////////
//////////////////////   中普同安的函数           ////////////////////////////////////

// 读取地址为 1 的控制板开关量输入状态：
// 48 3a 01 52 00 00 00 00 00 00 00 00 d5 45 44
// 地址为 1 的控制板收到上述指令后应答：
// 48 3a 01 41 01 01 00 00 00 00 00 00 c6 45 44
void DealZPTASPDMsg(int seq,unsigned char *buf,unsigned short int len)
{
	UINT8 i;
	UINT8 *pointer = &stuSpd_Param->dSPD_ZPTA[seq].SPD_DI[0];

	// 返回15个字节
	if(len == ZPTA_SPD_LEN)
	{
		printf("zpta_spd_data begain\r\n");
		for (i=0;i<ZPTA_DI_NUM;i++)
		{
			*(pointer+i) = *(buf+ZPTA_HEAD_NUM+i);
			printf("SPD_DI%d = %5hd \r\n",i,stuSpd_Param->dSPD_ZPTA[seq].SPD_DI[i]);
		}

		// 更新SPD时间戳
		SPD_timeStamp_update(seq);
	}
}

// 中普同安接地电阻的处理
void DealZPTAResMsg(unsigned char *buf,unsigned short int len)
{
	UINT8 i;
	UINT16 *pointer = &stuSpd_Param->rSPD_res.grd_res_value;
	float res_temp = 0;
	UINT16 dot_num = 0;

	// 返回字节数
	if(len == (SPD_RES_VALUE_NUM*2+5))
	{
		printf("zpta_spd_res begain\r\n");

		stuSpd_Param->rSPD_res.alarm = 0;
		stuSpd_Param->rSPD_res.alarm_value = 0;
		for (i=0;i<SPD_RES_VALUE_NUM;i++)
		{
			char_to_int(buf + FRAME_HEAD_NUM + i*2, (pointer+i));
		}
		dot_num = stuSpd_Param->rSPD_res.grd_res_dot_num;
		res_temp = (float)stuSpd_Param->rSPD_res.grd_res_value;
		if (dot_num > 0)
		{
			for (i=0;i<dot_num;i++)
			{
				// 实数除法
				res_temp = res_temp/10;
			}
			stuSpd_Param->rSPD_res.grd_res_real = res_temp;
		}
		else
		{
			stuSpd_Param->rSPD_res.grd_res_real = res_temp;
		}

		// 更新地阻时间戳
		SPD_timeStamp_update(SPD_NUM);

		/*
		printf("res_alarm = 0x%02x \r\n",stuSpd_Param->rSPD_res.alarm);
		printf("grd_res_value = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_res_value);
		printf("grd_res_dot_num = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_res_dot_num);
		printf("grd_volt = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_volt);

		printf("test = 0x%02x \r\n",stuSpd_Param->rSPD_res.test);
		printf("id = 0x%02x \r\n",stuSpd_Param->rSPD_res.id);
		printf("alarm_value = 0x%02x \r\n",stuSpd_Param->rSPD_res.alarm_value);
		*/

		printf("grd_res_real = %7.3f \r\n",stuSpd_Param->rSPD_res.grd_res_real);
	}
}


int DealZPZHSPDMsg(int seq,unsigned char *buf,unsigned short int len)
{
	UINT8 i,k;
	UINT8 *pointer = &stuSpd_Param->dSPD_ZPZH[0].temp_h;
	int re_num = 0;
	int remainder = 0;	//余数

	remainder = (len-5)%4;	// 数据长度是否真的正确?
	// 每个监测器4个字节，找出带有几个监测器
	if(remainder == 0)
	{
		re_num = (len-5)/4;
		printf("zpta_spd_data begain\r\n");
		for (k=0;k<re_num;k++)
		{
			pointer = &stuSpd_Param->dSPD_ZPZH[k].temp_h;
			for (i=0;i<ZPZH_SPD_LEN;i++)		// 用字节来赋值
			{
				*(pointer+i) = *(buf+ZPZH_HEAD_NUM+4*k+i);
			}
			//printf("SPD_ZHH%d = %5hd \r\n",i,stuSpd_Param->dSPD_ZPZH[k].temp_h);
			//printf("SPD_ZHL%d = %5hd \r\n",i,stuSpd_Param->dSPD_ZPZH[k].temp_l);
			//printf("SPD_ZHC%d = %5hd \r\n",i,stuSpd_Param->dSPD_ZPZH[k].struck_cnt);
			//printf("SPD_ZHD%d = %5hd \r\n",i,stuSpd_Param->dSPD_ZPZH[k].DI);

			// 更新SPD时间戳
			SPD_timeStamp_update(k);
		}
	}

	return re_num;	// 返回真正的数量
}

// 中普众合的接地电阻协议
void DealZPZHResMsg(unsigned char *buf,unsigned short int len)
{
	UINT8 i;
	UINT16 *pointer = &stuSpd_Param->rSPD_res.grd_res_value;
	float res_temp = 0;
	UINT16 dot_num = 0;

	// 返回字节数
	if(len == (ZPZH_RES_LEN*2+5))
	{
		printf("zpzh_spd_res begain\r\n");

		stuSpd_Param->rSPD_res.alarm = 0;
		stuSpd_Param->rSPD_res.alarm_value = 0;
		stuSpd_Param->rSPD_res.grd_res_dot_num = 0;
		stuSpd_Param->rSPD_res.id = 1;

		// 前面2个字节是接地电阻值
		char_to_int(buf + FRAME_HEAD_NUM, pointer);
		res_temp = (float)stuSpd_Param->rSPD_res.grd_res_value/10;
		stuSpd_Param->rSPD_res.grd_res_real = res_temp;

		// 后面2个字节是接地电阻电压值
		pointer = &stuSpd_Param->rSPD_res.grd_volt;
		char_to_int(buf + FRAME_HEAD_NUM +2, pointer);

		// 更新SPD时间戳
		SPD_timeStamp_update(SPD_NUM);

		/*
		printf("res_alarm = 0x%02x \r\n",stuSpd_Param->rSPD_res.alarm);
		printf("grd_res_value = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_res_value);
		printf("grd_res_dot_num = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_res_dot_num);
		printf("grd_volt = 0x%02x \r\n",stuSpd_Param->rSPD_res.grd_volt);

		printf("test = 0x%02x \r\n",stuSpd_Param->rSPD_res.test);
		printf("id = 0x%02x \r\n",stuSpd_Param->rSPD_res.id);
		printf("alarm_value = 0x%02x \r\n",stuSpd_Param->rSPD_res.alarm_value);
		*/
		printf("grd_res_real = %7.3f \r\n",stuSpd_Param->rSPD_res.grd_res_real);
	}
}


/////////////////////////////////////////////////////////////////////////////
//////////////////////   接收数据统一处理          //////////////////////////////////

int DealNetSPD(int skt,unsigned char *buf,unsigned short int len)
{
	UINT8 seq = 0;
	int i=0;
	int temp_num = 0;

	// 当为雷迅则外部的skt没有意义，因为是同一个IP地址来的
	if (SPD_Type == TYPE_LEIXUN)
	{
		for (i=0; i<SPD_NUM+RES_NUM; i++)
		{
			if (buf[0] == SPD_Address[i])
			{
				seq = i;	// 找出返回的数据是对应哪个地址?
				break;
			}
		}
		if (i == (SPD_NUM+RES_NUM))
		{
			return 0;	// 没找到对应的地址，无效数据，直接返回
		}

		// 如果是接地电阻的数据
		if (buf[0] == SPD_Address[SPD_NUM])
		{
			// 根据命令码来进行信息的区分
			switch(buf[1])
			{
				case SPD_RES_READ_CMD:
					DealSPDResStatusMsg(buf, len);
				break;
				default:
				break;
			}
		}
		else
		{
			// 根据命令码来进行信息的区分
			switch(buf[1])
			{
				case SPD_READ_CMD:
					DealSPDAiMsg(seq,buf, len);
					RealDataCopy(seq,SPD_AI_DATA);
				break;
				case SPD_DI_CMD:
					DealSPDDiMsg(seq,buf, len);
					RealDataCopy(seq,SPD_DI_DATA);
				break;
				// DO不处理
				case SPD_DO_CMD:
					DealSPDDoMsg(seq,buf, len);
					RealDataCopy(seq,SPD_DO_DATA);
				break;
				default:
				break;
			}
		}
	}
	// 当为宽永则外部的skt没有意义，因为是同一个IP地址来的
	if ((SPD_Type == TYPE_KY)||(SPD_Type == TYPE_KY0M))
	{
		for (i=0; i<SPD_NUM+RES_NUM; i++)
		{
			if (buf[0] == SPD_Address[i])
			{
				seq = i;	// 找出返回的数据是对应哪个地址?
				break;
			}
		}
		if (i == (SPD_NUM+RES_NUM))
		{
			return 0;	// 没找到对应的地址，无效数据，直接返回
		}

		// 如果是接地电阻的数据
		if (buf[0] == SPD_Address[SPD_NUM])
		{
			// 根据命令码来进行信息的区分
			switch(buf[1])
			{
				case KY_READ_CMD:
					DealKYResMsg(buf, len);
				break;
				default:
				break;
			}
		}
		else
		{
			// 根据命令码来进行信息的区分
			switch(buf[1])
			{
				case KY_READ_CMD:
					// 因为命令码是一样的，copy放在函数内部
					DealKYSPDMsg(seq,buf, len);
				break;
				default:
				break;
			}
		}
	}
	// 华咨的防雷
	else if (SPD_Type == TYPE_HUAZI)
	{
		// 0x64000000开头的数据
		if ((buf[0] == 0x64)&&(buf[1] == 0)&&(buf[2] == 0)&&(buf[3] == 0))
		{
			printf("spd_HZ_pos%d=\r\n",skt);
			// 根据长度区分是防雷数据还是地阻数据
			switch(buf[8])
			{
			// 防雷数据长度为46
			case (HZ_SPD_LEN):
				DealHZSPDMsg(skt,buf, len);
				RealDataCopy(skt,SPD_HZ_DATA);
			break;
			// 地阻数据长度为26,只有1个接地，seq无所谓了
			case (HZ_RES_LEN):
				DealHZResMsg(seq,buf, len);
			break;
			default:
			break;
			}
		}
	}

	else if (SPD_Type == TYPE_ZPTA)
	{
		// 0x483A开头的数据,这是防雷器
		if ((buf[0] == 0x48)&&(buf[1] == 0x3A))
		{
			DealZPTASPDMsg(skt,buf, len);
			RealDataCopy(skt,SPD_HZ_DATA);
		}
		// 接地电阻是
		else if ((buf[1] == SPD_RES_READ_CMD) && (buf[0] == SPD_Address[SPD_NUM]))
		{
			DealZPTAResMsg(buf, len);
		}
	}

	else if (SPD_Type == TYPE_ZPZH)
	{
		// 防雷检测的数据
		// 和接地电阻分开不同的网络
		if (skt < SPD_NUM)
		{
			temp_num = DealZPZHSPDMsg(skt,buf, len);
			// 中普众合的1个串口服务器连了所有的监测器
			// 目前是2个 SPD_num =1
			for (i=0;i<temp_num;i++)
			{
				RealDataCopy(i,SPD_HZ_DATA);
			}
		}
		// 接地电阻的数据
		else
		{
			DealZPZHResMsg(buf, len);
		}
	}
	return 0;
}


// SPD的TCP连接逻辑，死循环
void Spd_Reconnect_Loop(int *connect_flag, int seq)
{
	*connect_flag = 0;
	while(*connect_flag==0)
	{
		// SPD断线了
		printf("spd_-break%d\n\r",seq);
		*connect_flag=obtain_net_psd(seq);
		sleep(2); //delay 2s
	}
}


/*雷迅的接收线程*/
void *NetWork_DataGet_thread_SPD_L(void *param)
{
	param = NULL;
	int buffPos=0;
	int len,temp = 0;
	unsigned char buf[256];
	static UINT8 first_entry = 0;
	static FDATA dummy;
	static UINT16 dummy_u;
	while(1)
	{
		if ((SPD_Type == TYPE_LEIXUN) ||(SPD_Type == TYPE_KY) ||(SPD_Type == TYPE_KY0M))
		{
			if ((connected_flag[0] != 0) && (sockfd_spd[0] != -1))
			{
		      	len = read(sockfd_spd[0], buf, sizeof(buf)-1);
				//printf("llen= %d\n\r",len);
				if (len > 0)
				{
				  	buffPos = buffPos+len;
				  	if(buffPos<5) continue;

				  	//CRC
				  	unsigned short int CRC = CRC16(buf,buffPos-2) ;
				  	if((((CRC&0xFF00) >> 8)!= buf[buffPos-2]) || ((CRC&0x00FF) != buf[buffPos-1]))
					{
					  printf("psdCRC error\r\n");
			 		  if(buffPos>=256) buffPos=0;
					  continue ;
				  	}

			      	//printf("spd len=%d\r\n",buffPos) ;
				  	/*debug the information*/
					//int j ;for(j=0;j<buffPos;j++)printf("0x%02x ",buf[j]);printf("\r\n");
				  	DealNetSPD(0,buf, buffPos);
				  	buffPos=0;
					// 第一次连接对时一次
					if ((first_entry == 0)&&(SPD_Type == TYPE_LEIXUN))
					{
						first_entry = 1;
						Ex_SPD_Set_Process(SPD_NUM,SPD_TIME_SET,TIME_SET_ADDR,dummy,dummy_u);
					}
				}
				// 断线了
				else
				{
					first_entry = 0;		// 断线重连再次对时
					Spd_Reconnect_Loop(&connected_flag[0],0);
				}
			}
			else
			{
				first_entry = 0;
				Spd_Reconnect_Loop(&connected_flag[0],0);
			}
		}
      	usleep(5000); //delay 5ms
	}
}

#if 0
// 中普同安的接收逻辑
void SPD_ZPTA_Data_Get_proc(int seq)
{
	int buffPos=0;
	int len,temp = 0;
	unsigned char buf[256];
	unsigned char CRC;

	// 如果是有效的连接
	if (sockfd_spd[seq] != -1)
	{
		//printf("sockfd_spd%d = %d\r\n",seq,sockfd_spd[seq]);
		len = read(sockfd_spd[seq], buf, sizeof(buf)-1);
		printf("len= %d\n\r",len);
		if (len > 0)
		{
			buffPos = buffPos+len;
			if(buffPos<5) continue;

			//CRC,对前12个字节求和
			if (seq < SPD_NUM)
			{
				unsigned char CRC_8 = CRC_sum(buf,buffPos-3) ;
				if(CRC_8 != buf[buffPos-3])
				{
					printf("psdCRCTA error\r\n");
				 	if(buffPos>=256) buffPos=0;
					continue ;
				}
			}
			else
			{
				// 接地电阻是MODBUS协议
				unsigned short CRC_16 = CRC16(buf,buffPos-2) ;
				if((((CRC_16&0xFF00) >> 8)!= buf[buffPos-2]) || ((CRC_16&0x00FF) != buf[buffPos-1]))
				{
					printf("psdCRCTAres error\r\n");
				 	if(buffPos>=256) buffPos=0;
					continue ;
				}
			}
			printf("spd len=%d\r\n",buffPos) ;
			/*debug the information*/
			int j ;for(j=0;j<buffPos;j++)printf("0x%02x ",buf[j]);printf("\r\n");
			DealNetSPD(seq,buf, buffPos);
			buffPos=0;
		}
		// 断线了
		else
		{
			Spd_Reconnect_Loop(&ZP_Conneted[seq],seq);
		}
	}
	else
	{
		// 如果配置了，但是没连上，还要接着连,或者是接地电阻
		if ((seq <= (SPD_num-1)) || (seq == SPD_NUM))
		{
			Spd_Reconnect_Loop(&ZP_Conneted[seq],seq);
		}
	}
}


// 中普众合的接收逻辑
void SPD_ZPZH_Data_Get_proc(int seq)
{
	int buffPos=0;
	int len,temp = 0;
	unsigned char buf[256];
	unsigned char CRC;

	// 如果是有效的连接
	if (sockfd_spd[seq] != -1)
	{
		//printf("sockfd_spd%d = %d\r\n",seq,sockfd_spd[seq]);
		len = read(sockfd_spd[seq], buf, sizeof(buf)-1);
		printf("len= %d\n\r",len);
		if (len > 0)
		{
			buffPos = buffPos+len;
			if(buffPos<5) continue;

			// 接地电阻是MODBUS协议
			unsigned short CRC_16 = CRC16(buf,buffPos-2) ;
			if((((CRC_16&0xFF00) >> 8)!= buf[buffPos-2]) || ((CRC_16&0x00FF) != buf[buffPos-1]))
			{
				printf("psdCRCZHres error\r\n");
				 if(buffPos>=256) buffPos=0;
				continue ;
			}
			printf("spd len=%d\r\n",buffPos) ;
			/*debug the information*/
			int j ;for(j=0;j<buffPos;j++)printf("0x%02x ",buf[j]);printf("\r\n");
			DealNetSPD(seq,buf, buffPos);
			buffPos=0;
		}
		// 断线了
		else
		{
			Spd_Reconnect_Loop(&ZP_Conneted[seq],seq);
		}
	}
	else
	{
		// 如果配置了，但是没连上，还要接着连,或者是接地电阻
		if ((seq <= (SPD_num-1)) || (seq == SPD_NUM))
		{
			Spd_Reconnect_Loop(&ZP_Conneted[seq],seq);
		}
	}
}
#endif


/*中普的接收线程*/
void SPD_ZP_Data_Get_Func(int seq)
{
	int buffPos=0;
	int len,temp = 0;
	unsigned char buf[256];
	unsigned char CRC;

	while(1)
	{
		if (SPD_Type == TYPE_ZPTA)
		{
			// 如果是有效的连接
			if ((connected_flag[seq] != 0) && (sockfd_spd[seq] != -1))
			{
				//printf("sockfd_spd%d = %d\r\n",seq,sockfd_spd[seq]);
				len = read(sockfd_spd[seq], buf, sizeof(buf)-1);
				//printf("len= %d\n\r",len);
				if (len > 0)
				{
					buffPos = buffPos+len;
					if(buffPos<5) continue;

					//CRC,对前12个字节求和
					if (seq < SPD_NUM)
					{
						unsigned char CRC_8 = CRC_sum(buf,buffPos-3) ;
						if(CRC_8 != buf[buffPos-3])
						{
							printf("psdCRCTA error\r\n");
						 	if(buffPos>=256) buffPos=0;
							continue ;
						}
					}
					else
					{
						// 接地电阻是MODBUS协议
						unsigned short CRC_16 = CRC16(buf,buffPos-2) ;
						if((((CRC_16&0xFF00) >> 8)!= buf[buffPos-2]) || ((CRC_16&0x00FF) != buf[buffPos-1]))
						{
							printf("psdCRCTAres error\r\n");
						 	if(buffPos>=256) buffPos=0;
							continue ;
						}
					}
					//printf("spd len=%d\r\n",buffPos) ;
					/*debug the information*/
					//int j ;for(j=0;j<buffPos;j++)printf("0x%02x ",buf[j]);printf("\r\n");
					DealNetSPD(seq,buf, buffPos);
					buffPos=0;
				}
				// 断线了
				else
				{
					printf("reconncted again/n/r");
					Spd_Reconnect_Loop(&connected_flag[seq],seq);
				}
			}
			else
			{
				// 如果配置了，但是没连上，还要接着连,或者是接地电阻
				if ((seq <= (SPD_num-1)) || (seq == SPD_NUM))
				{
					printf("reconncted again/n/r");
					Spd_Reconnect_Loop(&connected_flag[seq],seq);
				}
			}
		}
		else if (SPD_Type == TYPE_ZPZH)
		{
			if ((connected_flag[seq] != 0) && (sockfd_spd[seq] != -1))
			{
				//printf("sockfd_spd%d = %d\r\n",seq,sockfd_spd[seq]);
				len = read(sockfd_spd[seq], buf, sizeof(buf)-1);
				//printf("len= %d\n\r",len);
				if (len > 0)
				{
					buffPos = buffPos+len;
					if(buffPos<5) continue;

					// 接地电阻是MODBUS协议
					unsigned short CRC_16 = CRC16(buf,buffPos-2) ;
					if((((CRC_16&0xFF00) >> 8)!= buf[buffPos-2]) || ((CRC_16&0x00FF) != buf[buffPos-1]))
					{
						printf("psdCRCZHres error\r\n");
						 if(buffPos>=256) buffPos=0;
						continue ;
					}
					printf("spd len=%d\r\n",buffPos) ;
					/*debug the information*/
					//int j ;for(j=0;j<buffPos;j++)printf("0x%02x ",buf[j]);printf("\r\n");
					DealNetSPD(seq,buf, buffPos);
					buffPos=0;
				}
				// 断线了
				else
				{
					Spd_Reconnect_Loop(&connected_flag[seq],seq);
				}
			}
			else
			{
				// 如果配置了，但是没连上，还要接着连,或者是接地电阻
				// 中普众合就只有2个IP地址
				if ((seq == 0) || (seq == SPD_NUM))
				{
					printf("reconncted again/n/r");
					Spd_Reconnect_Loop(&connected_flag[seq],seq);
				}
			}
		}

      	usleep(5000); //delay 5ms
	}
}



bool udp_param_reset(int Pos)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	bool reval = false;
	const char *IPaddress;
	const char * IPport;
	int port;

	if ((Pos < SPD_num) || (Pos==SPD_NUM))
	{
		IPaddress = pConf->StrSPDIP[Pos].c_str();	//获取配置文件中的IP地址
		IPport=pConf->StrSPDPort[Pos].c_str();
		port=atoi(IPport);
	    bzero(&HZSPDAddr[Pos],sizeof(HZSPDAddr[Pos]));

	    HZSPDAddr[Pos].sin_family = AF_INET;
	    HZSPDAddr[Pos].sin_addr.s_addr = inet_addr(IPaddress);
	    HZSPDAddr[Pos].sin_port = htons(port);	// Port number
		printf("Pos=%d,hzSPD-IPaddress=%s,hzSPD-IPport=%s\n",Pos,IPaddress,IPport);
		reval = true;
	}
	return reval;
}


void SPD_HZ_Data_Get_Func(int seq)
{
	int Pos=seq;
	int len,temp;
	int addr_len =sizeof(struct sockaddr_in);
	int udp_exist = false;

    if((udpfd_spd[Pos] = socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("socket_1");
        exit(1);
    }

    int nRecvBufLen = HZ_BUF_LEN;
    setsockopt(udpfd_spd[Pos], SOL_SOCKET, SO_RCVBUF, ( const char* )&nRecvBufLen, sizeof( int ));
	int nSendBufLen = HZ_BUF_LEN;
    setsockopt(udpfd_spd[Pos], SOL_SOCKET, SO_SNDBUF, ( const char* )&nSendBufLen, sizeof( int ) );
    struct timeval timeout={8,0};//3s
    setsockopt(udpfd_spd[Pos],SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));

	// 获取配置文件中的IP地址
	// udp_exist 表明这个UDP是有配置的
	udp_exist = udp_param_reset(Pos);

	struct sockaddr_in *Recvaddr = &HZSPDAddr[Pos];
    int DataLen = 0;
    char *recvBuf = new char[HZ_BUF_LEN];
    while(1)
    {
    	if ((SPD_Type == TYPE_HUAZI)&&((Pos < SPD_num) || (Pos == SPD_NUM)))
    	{
	    	if (HZ_reset_flag[Pos] == true)
	    	{
	    		HZ_reset_flag[Pos] = false;
				udp_exist= udp_param_reset(Pos);
	    	}
	        memset(recvBuf,0,HZ_BUF_LEN);
			if (udp_exist)
			{
	        	DataLen = recvfrom(udpfd_spd[Pos],recvBuf,HZ_BUF_LEN,0,(struct sockaddr *)Recvaddr,(socklen_t*)&addr_len);
		        if(DataLen > 0)
		        {
					// 打印出来
					pthread_mutex_lock(&HZSPDMutex[Pos]);
					printf("spd_HZ_len=%d\r\n",DataLen);
					int j ;for(j=0;j<DataLen;j++) printf("0x%02x ",recvBuf[j]);printf("\r\n");
					DealNetSPD(Pos,(unsigned char *)recvBuf,DataLen);
					pthread_mutex_unlock(&HZSPDMutex[Pos]);
		        }
			}
    	}
		usleep(5000); //delay 5ms
    }
    close(udpfd_spd[Pos]);
}

////////////////////////////////////////////////////////////////
/*华咨的防雷器1接收线程*/
void *NetWork_DataGet_thread_SPD_HZ1(void *param)
{
	param = NULL;

	SPD_HZ_Data_Get_Func(0);	// 0,1,2 即SPD1，SPD2, 接地电阻
}


void *NetWork_DataGet_thread_SPD_HZ2(void *param)
{
	param = NULL;

	SPD_HZ_Data_Get_Func(1);
}

void *NetWork_DataGet_thread_SPD_HZ3(void *param)
{
	param = NULL;

	SPD_HZ_Data_Get_Func(2);
}

void *NetWork_DataGet_thread_SPD_HZ4(void *param)
{
	param = NULL;

	SPD_HZ_Data_Get_Func(3);
}

void *NetWork_DataGet_thread_SPD_HZ5(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(4);
}

void *NetWork_DataGet_thread_SPD_HZ6(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(5);
}

void *NetWork_DataGet_thread_SPD_HZ7(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(6);
}


void *NetWork_DataGet_thread_SPD_HZ8(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(7);
}


void *NetWork_DataGet_thread_SPD_HZ9(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(8);
}


void *NetWork_DataGet_thread_SPD_HZ10(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(9);
}


void *NetWork_DataGet_thread_SPD_HZ11(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(10);
}


void *NetWork_DataGet_thread_SPD_HZ12(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(11);
}


void *NetWork_DataGet_thread_SPD_HZ13(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(12);
}


void *NetWork_DataGet_thread_SPD_HZ14(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(13);
}


void *NetWork_DataGet_thread_SPD_HZ15(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(14);
}


void *NetWork_DataGet_thread_SPD_HZ16(void *param)
{
	param = NULL;
	SPD_HZ_Data_Get_Func(15);
}


void *NetWork_DataGet_thread_SPD_HZRes(void *param)
{
	param = NULL;

	SPD_HZ_Data_Get_Func(SPD_NUM);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void *NetWork_DataGet_thread_SPD_ZP1(void *param)
{
	param = NULL;

	SPD_ZP_Data_Get_Func(0);
}

void *NetWork_DataGet_thread_SPD_ZP2(void *param)
{
	param = NULL;

	SPD_ZP_Data_Get_Func(1);
}

void *NetWork_DataGet_thread_SPD_ZP3(void *param)
{
	param = NULL;

	SPD_ZP_Data_Get_Func(2);
}


void *NetWork_DataGet_thread_SPD_ZP4(void *param)
{
	param = NULL;

	SPD_ZP_Data_Get_Func(3);
}

void *NetWork_DataGet_thread_SPD_ZPres(void *param)
{
	param = NULL;

	SPD_ZP_Data_Get_Func(SPD_NUM);
}
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// 定义一个函数指针数组，因为数量太多了
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


// 统一创建接收线程
// 宽永和雷迅共用1个线程，2者处理方式是一样的
void DataGet_Thread_Create(UINT16 SPD_t)
{
	// 雷迅和宽永的接收线程
	// 1个串口服务器下面通过485挂几个防雷检测器和接地电阻仪
	if ((SPD_t == TYPE_LEIXUN) || (SPD_t == TYPE_KY) ||(SPD_Type == TYPE_KY0M))
	{
		pthread_t tNetwork_dataget_SPD_L;
		if (pthread_create(&tNetwork_dataget_SPD_L, NULL, NetWork_DataGet_thread_SPD_L,NULL))
		{
			printf("NetWork SPD data create failed!\n");
//			WriteLog("NetWork SPD data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_L);
	}

	// 华咨的接收线程
	// UPD通信，几个设备几个UDP
	else if (SPD_t == TYPE_HUAZI)
	{
		for (int i=0;i<(SPD_NUM+RES_NUM);i++)
		{
			pthread_mutex_init(&HZSPDMutex[i],NULL);
		}

		pthread_t tNetwork_dataget_SPD_HZ[SPD_NUM];
		for (int i=0;i<SPD_NUM;i++)
		{
			if (pthread_create(&(tNetwork_dataget_SPD_HZ[i]), NULL, network_thread_SPD_HZ[i],NULL))
			{
				printf("HZ NetWork SPD_%d data create failed!\n",i);
			}
		}

		#if 0
		pthread_t tNetwork_dataget_SPD_HZ1;
		if (pthread_create(&tNetwork_dataget_SPD_HZ1, NULL, NetWork_DataGet_thread_SPD_HZ1,NULL))
		{
			printf("NetWork SPD_1 data create failed!\n");
//			WriteLog("NetWork SPD_1 data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_HZ1);

		// 华咨的SPD2
		pthread_t tNetwork_dataget_SPD_HZ2;
		if (pthread_create(&tNetwork_dataget_SPD_HZ2, NULL, NetWork_DataGet_thread_SPD_HZ2,NULL))
		{
			printf("NetWork SPD_2 data create failed!\n");
//			WriteLog("NetWork SPD_2 data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_HZ2);

		// 华咨的SPD3
		pthread_t tNetwork_dataget_SPD_HZ3;
		if (pthread_create(&tNetwork_dataget_SPD_HZ3, NULL, NetWork_DataGet_thread_SPD_HZ3,NULL))
		{
			printf("NetWork SPD_3 data create failed!\n");
//			WriteLog("NetWork SPD_3 data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_HZ3);

		// 华咨的SPD3
		pthread_t tNetwork_dataget_SPD_HZ4;
		if (pthread_create(&tNetwork_dataget_SPD_HZ4, NULL, NetWork_DataGet_thread_SPD_HZ4,NULL))
		{
			printf("NetWork SPD_4 data create failed!\n");
//			WriteLog("NetWork SPD_4 data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_HZ4);
		#endif

		pthread_t tNetwork_dataget_SPD_HZres;
		if (pthread_create(&tNetwork_dataget_SPD_HZres, NULL, NetWork_DataGet_thread_SPD_HZRes,NULL))
		{
			printf("NetWork SPD_RES data create failed!\n");
//			WriteLog("NetWork SPD_RES data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_HZres);
	}

	// 中普同安和中普众合,几个设备就几个TCP
	else if ((SPD_t == TYPE_ZPTA) || (SPD_t == TYPE_ZPZH))
	{
		pthread_t tNetwork_dataget_SPD_ZP1;
		if (pthread_create(&tNetwork_dataget_SPD_ZP1, NULL, NetWork_DataGet_thread_SPD_ZP1,NULL))
		{
			printf("NetWork SPD data create failed!\n");
//			WriteLog("NetWork SPD data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_ZP1);

		pthread_t tNetwork_dataget_SPD_ZP2;
		if (pthread_create(&tNetwork_dataget_SPD_ZP2, NULL, NetWork_DataGet_thread_SPD_ZP2,NULL))
		{
			printf("NetWork SPD data create failed!\n");
//			WriteLog("NetWork SPD data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_ZP2);

		pthread_t tNetwork_dataget_SPD_ZP3;
		if (pthread_create(&tNetwork_dataget_SPD_ZP3, NULL, NetWork_DataGet_thread_SPD_ZP3,NULL))
		{
			printf("NetWork SPD data create failed!\n");
//			WriteLog("NetWork SPD data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_ZP3);

		pthread_t tNetwork_dataget_SPD_ZP4;
		if (pthread_create(&tNetwork_dataget_SPD_ZP4, NULL, NetWork_DataGet_thread_SPD_ZP4,NULL))
		{
			printf("NetWork SPD data create failed!\n");
//			WriteLog("NetWork SPD data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_ZP4);

		pthread_t tNetwork_dataget_SPD_ZPres;
		if (pthread_create(&tNetwork_dataget_SPD_ZPres, NULL, NetWork_DataGet_thread_SPD_ZPres,NULL))
		{
			printf("NetWork SPD data create failed!\n");
//			WriteLog("NetWork SPD data create failed!\n");
		}
		pthread_detach(tNetwork_dataget_SPD_ZPres);
	}
}


void* NetWork_server_thread_SPD(void*arg)
{
	char str[10*1024];
	int i,j,temp;
	int port,nlen;
	struct sockaddr_in net_spd_addr;
	unsigned short crc_spd;
	//int bFlag=0;
	static UINT16 op_counter = 0;
	static UINT16 poll_cnt = 0;
	static UINT16 seq_cnt = 0;		// 标志现在轮询到了几个寄存器了
	// 线程开始测试一次电阻，不然要等10分钟, 延时20s,先对时
	static UINT32 ctrl_counter = (SPD_TEST_RES_INTERVAL-10);
	FDATA dummy;
	UINT16 dummy_u;

	static bool entry[TYPE_MAX_NUM-1] = {false,};
	static UINT16 con_cnt[SPD_NUM+RES_NUM] ={0,};	// 连接次数
	static bool any_connected = false;		// 如果有几个TCP连接，任意1个连接好了的标志

	// 断线判断变量
	static UINT16 connect_assert_cnt = 0;	// 多久判断一次断线
	static bool connect_assert_flag = false;

	// 一些参数先进行初始化
	for (i = 0; i < SPD_NUM; i++)
	{
		memset (&SPD_ctrl_value[i],0,sizeof(SPD_CTRL_VALUE));
	}
	for (i = 0; i < SPD_NUM+RES_NUM; i++)
	{
		con_cnt[i] = 0;
	}
	for (i = 0; i < TYPE_MAX_NUM-1; i++)
	{
		entry[i] = false;
	}
	dummy.f=NULL_VAR;
	temp=0;

	/////////////////////////////////////////////////////////////////////////////
	////////////////////////    开始连接任务          /////////////////////////////////
	// 雷迅和宽永处理是一样的，都是1个串口服务器+485设备
	if ((SPD_Type == TYPE_LEIXUN) || (SPD_Type == TYPE_KY) || (SPD_Type == TYPE_KY0M))
	{
		connected_flag[0]=0;		// 开始连接前置0
		while(connected_flag[0]==0)
		{
			// 雷迅的防雷只有1个IP地址
			connected_flag[0]=obtain_net_psd(0);
			sprintf(str,"net_Conneted=%d\n",connected_flag[0]);
//			WriteLog(str);
			if(connected_flag[0]==0)
			{
				printf("connect _psd error\n");
//				WriteLog("IN NETWORK_Server_thread connect _psd error!\n");
				sleep(2);
			}
		}
	}
	// 只有开始有初始化，因此切换到中普同安必须要重启
	// 中普同安和中普众合都是TCP网络型,共用1个,有1点区别
	// 中普同安是2个SPD->2个IP+1个接地->1个IP,SPD是自定义协议，接地是和雷迅一样的,modbus协议
	// 中普众合是2个SPD->1个IP,而且协议已经集成到了串口服务器,+1个接地->1个IP，协议也是SPD一致，也都是MODBUS
	else if ((SPD_Type == TYPE_ZPTA) || (SPD_Type == TYPE_ZPZH))
	{
		printf("ZP begin\n");
		while (any_connected == false)	// 1个都没连上，要一直等待
		{
			for (i = 0; i < SPD_NUM+RES_NUM; i++)
			{
				con_cnt[i] = 0;	//再次重连，次数清0
				// 如果数量没有配置, 直接跳过
				if ((i>=SPD_num) && (i != SPD_NUM))
				{
					continue;
				}
				if (SPD_Type == TYPE_ZPZH)
				{
					// 中普众合SPD共用1个IP地址
					if (i==1)
					{
						continue;
					}
				}
				//printf("itemNum = %d\n\r",i);

				connected_flag[i]=0;		// 开始连接前置0
				while ((connected_flag[i]==0)&&(con_cnt[i]++ <1))	// 连1次,因为连接的时间太长了
				{
					// 中普同安的防雷有3个IP地址
					connected_flag[i]=obtain_net_psd(i);
					sprintf(str,"ZP_Conneted=%d\n",connected_flag[i]);
//					WriteLog(str);
					if(connected_flag[i]==0)
					{
						printf("connect _psd error\n");
//						WriteLog("IN NETWORK_Server_thread connect _psd error!\n");
						sleep(2);
					}
					else
					{
						any_connected = true;	// 任意1个连接上了，先取到数据再说
					}
				}
			}
		}
	}
	// 华咨的防雷检测
	else
	{
		// 连一次，给sockfd_spd一个值，不然切换的时候会出现异常
		obtain_net_psd(0);
	}
	printf("any_connected = %d\n\r",any_connected);
	// 连接成功后再创建接收的线程
	DataGet_Thread_Create(SPD_Type);
	sleep(2);	//连接后等待2s稳定

	while(1)
	{
		// 华咨和中普众合都没有设置参数
		if ((SPD_Type == TYPE_HUAZI) || (SPD_Type == TYPE_ZPZH))
		{
			// 如果是华咨,没有设置参数，忽略
			for (i = 0; i < SPD_NUM; i++)
			{
				spd_ctl_flag[i] = 0;
			}
		}
		//printf("spd_ctl_flag_0=%d,spd_ctl_flag_1=%d\r\n",spd_ctl_flag[0],spd_ctl_flag[1]);
		if ((spd_ctl_flag[0]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[1]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[2]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[3]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[4]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[5]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[6]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[7]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[8]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[9]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[10]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[11]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[12]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[13]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[14]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[15]&BITS_MSK_GET(0,SPD_CTRL_NUM)))
		{
			// 有参数设置需求，马上设置参数，取消轮询
			for (i = 0; i < SPD_NUM; i++)
			{
				spd_net_flag[i] = 0;
			}
		}
		else
		{
			poll_cnt++;
			//printf("poll_cnt=%d\r\n",poll_cnt);
			// 400*3 = 1.2s，即1.2s轮询一个项目
			if (poll_cnt >= SPD_POLLING_INTERVAL)
			{
				poll_cnt = 0;
				if (SPD_Type == TYPE_LEIXUN)
				{
					// 把接地电阻的测试值复位，确保切换后能快速测试接地电阻值
					if (entry[TYPE_LEIXUN-1] == false)
					{
						for (i=0;i<(TYPE_MAX_NUM-1);i++)
						{
							entry[i] = false;
						}
						entry[TYPE_LEIXUN-1] = true;
						ctrl_counter = (SPD_TEST_RES_INTERVAL-10);
					}

					if (op_counter >= SPD_DATA_NUM)
					{
						op_counter = 0;
						seq_cnt++;
						// 按config设置的数量来
						if ((seq_cnt >= SPD_num) || (seq_cnt >= SPD_NUM))
						{
							seq_cnt = 0;
						}
					}
					// 要放在下面，否则不会执行上面的if语句
					else if (op_counter >= SPD_HZ_DATA)
					{
						op_counter = SPD_RES_DATA;	// 跳过第二个防雷器
					}
					if ((ctrl_counter % SPD_TEST_RES_INTERVAL) ==0)
					{
						// 接地电阻10分钟测试一次
						// Ex_SPD_Set_Process(SPD_NUM,SPD_RES_SET,RES_TEST_ADDR,dummy,RES_TEST_EN);
						// 注意，只有1个接地电阻，只需要测试一次就可以了
						Ex_SPD_Set_Process(0,SPD_RES_SET,RES_TEST_ADDR,dummy,RES_TEST_EN);
					}
					if (ctrl_counter >= SPD_TIME_SYN_INTERVAL)
					{
						ctrl_counter = 0;
						Ex_SPD_Set_Process(0,SPD_TIME_SET,TIME_SET_ADDR,dummy,dummy_u);
					}
					ctrl_counter++;		// 每隔一段时间进行一次接地电阻测试
					//printf("ctrl_counter = 0x%08x \r\n",ctrl_counter);
				}
				else if (SPD_Type == TYPE_HUAZI)
				{
					if (entry[TYPE_HUAZI-1] == false)
					{
						for (i=0;i<(TYPE_MAX_NUM-1);i++)
						{
							entry[i] = false;
						}
						entry[TYPE_HUAZI-1] = true;
					}

					if (op_counter < SPD_HZ_DATA)
					{
						op_counter = SPD_HZ_DATA;
					}

					if ((op_counter > SPD_RES_DATA) || (op_counter >= SPD_DATA_NUM))
					{
						op_counter = SPD_HZ_DATA;
						seq_cnt++;
						// 按config设置的数量来
						if ((seq_cnt >= SPD_num) || (seq_cnt >= SPD_NUM))
						{
							seq_cnt = 0;
						}
					}
					// 要放在下面,否则上面的if不会执行
					else if (op_counter > SPD_HZ_DATA)
					{
						op_counter = SPD_RES_DATA;
					}
				}

				// 宽永的处理
				// 广西宽永只有几个数据，而且没有接地电阻，但是协议地址是一致的，
				// 所以保持和宽永的处理一致，但是没有的不处理就是了
				else if ((SPD_Type == TYPE_KY) ||(SPD_Type == TYPE_KY0M))
				{
					// 把接地电阻的测试值复位，确保切换后能快速测试接地电阻值
					if (entry[SPD_Type-1] == false)
					{
						for (i=0;i<(TYPE_MAX_NUM-1);i++)
						{
							entry[i] = false;
						}
						entry[SPD_Type-1] = true;
						ctrl_counter = (SPD_TEST_RES_INTERVAL-10);
					}

					// 轮询逻辑
					if (op_counter <= SPD_RUN_DATA)
					{
						op_counter = SPD_RUN_DATA;	// 防止异常值
					}
					if (op_counter >= SPD_DATA_NUM)
					{
						op_counter = SPD_RUN_DATA;
						seq_cnt++;
						// 按config设置的数量来，防止SPD_num被设置成大于SPD_NUM
						if ((seq_cnt >= SPD_num) || (seq_cnt >= SPD_NUM))
						{
							seq_cnt = 0;
						}
					}

					// 设置逻辑, 广西宽永的接地电阻如果没有，则无反应，不处理
					if ((ctrl_counter % SPD_TEST_RES_INTERVAL) ==0)
					{
						ctrl_counter = 0;
						// 接地电阻2个小时测试一次
						Ex_SPD_Set_Process(0,SPD_RES_SET,KY_RES_TEST_ADDR,dummy,RES_TEST_EN);
					}
					ctrl_counter++;		// 每隔一段时间进行一次接地电阻测试
				}

				// 共用华咨的轮询标志，2个监测器的机制很类似
				else if (SPD_Type == TYPE_ZPTA)
				{
					// 把接地电阻的测试值复位，确保切换后能快速测试接地电阻值
					if (entry[TYPE_ZPTA-1] == false)
					{
						for (i=0;i<(TYPE_MAX_NUM-1);i++)
						{
							entry[i] = false;
						}
						entry[TYPE_ZPTA-1] = true;
						ctrl_counter = (SPD_TEST_RES_INTERVAL-10);
					}
					if (op_counter < SPD_HZ_DATA)
					{
						op_counter = SPD_HZ_DATA;
					}

					if ((op_counter > SPD_RES_DATA) || (op_counter >= SPD_DATA_NUM))
					{
						op_counter = SPD_HZ_DATA;
						seq_cnt++;
						// 按config设置的数量来
						if ((seq_cnt >= SPD_num) || (seq_cnt >= SPD_NUM))	// 防止溢出
						{
							seq_cnt = 0;
						}
					}
					// 要放在下面,否则上面的if不会执行
					else if (op_counter > SPD_HZ_DATA)
					{
						op_counter = SPD_RES_DATA;
					}

					// 设置逻辑
					if ((ctrl_counter % SPD_TEST_RES_INTERVAL) ==0)
					{
						ctrl_counter = 0;
						// 接地电阻30分钟测试一次
						Ex_SPD_Set_Process(0,SPD_RES_SET,RES_TEST_ADDR,dummy,RES_TEST_EN);
					}
					ctrl_counter++;		// 每隔一段时间进行一次接地电阻测试
				}

				else if (SPD_Type == TYPE_ZPZH)
				{
					// 把接地电阻的测试值复位，确保切换后能快速测试接地电阻值
					if (entry[TYPE_ZPZH-1] == false)
					{
						for (i=0;i<(TYPE_MAX_NUM-1);i++)
						{
							entry[i] = false;
						}
						entry[TYPE_ZPZH-1] = true;
						ctrl_counter = (SPD_TEST_RES_INTERVAL-10);
					}
					if (op_counter < SPD_HZ_DATA)
					{
						op_counter = SPD_HZ_DATA;
					}

					if ((op_counter > SPD_RES_DATA) || (op_counter >= SPD_DATA_NUM))
					{
						op_counter = SPD_HZ_DATA;
						seq_cnt++;
						// 按config设置的数量来
						if ((seq_cnt >= SPD_num) || (seq_cnt >= SPD_NUM))	// 防止溢出
						{
							seq_cnt = 0;
						}
					}
					// 要放在下面,否则上面的if不会执行
					else if (op_counter > SPD_HZ_DATA)
					{
						op_counter = SPD_RES_DATA;
					}
				}

				//printf("SPD_Type=%d,SPD_num=%d",SPD_Type,SPD_num);
				if (seq_cnt < SPD_NUM)
				{
					spd_net_flag[seq_cnt] |= BIT(op_counter);
					//printf("seq_cnt=%d,spd_net_flag=%02x\r\n",seq_cnt,spd_net_flag[seq_cnt]);
					if (SPD_Type == TYPE_KY)
					{
						if (KY_test_disable_cnt > 0)
						{
							KY_test_disable_cnt--;
							// 如果电阻测试冷冻时间未到，不进行读写
							// 宽永的接地电阻测试需要一个冷冻时间，这个时间段内不能接收任何数据
							spd_net_flag[seq_cnt] = 0;
						}
					}
				}
				op_counter++;		// 轮询间隔标志
			}
			connect_assert_cnt++;
			if (connect_assert_cnt >= SPD_ASSERT_INTERVAL)
			{
				connect_assert_cnt = 0;
				connect_assert_flag = true;
			}
		}
		//printf("SPD_Type=%d,SPD_num=%d\r\n",SPD_Type,SPD_num);
		// 因为后面对spd_ctl_flag又进行了操作，准备设置前再次屏蔽其他轮询
		if ((spd_ctl_flag[0]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[1]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[2]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[3]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[4]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[5]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[6]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[7]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[8]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[9]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[10]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[11]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[12]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[13]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			|| (spd_ctl_flag[14]&BITS_MSK_GET(0,SPD_CTRL_NUM)) ||(spd_ctl_flag[15]&BITS_MSK_GET(0,SPD_CTRL_NUM)))
		{
			// 有参数设置需求，马上设置参数，取消轮询
			for (i = 0; i < SPD_NUM; i++)
			{
				spd_net_flag[i] = 0;
			}
		}

		// 统一处理的设置函数，这种结构注定上位机只能一次设置一个，且要有个间隔时间,400ms
		for (i=0; i<SPD_NUM; i++)
		{
			if (spd_ctl_flag[i]&BITS_MSK_GET(0,SPD_CTRL_NUM))
			{
				//printf("spd_ctl_flag%d=0x%04x\r\n",i,spd_ctl_flag[i]);
				spd_ctrl_process(i,&spd_ctl_flag[i]);
				break;	// 有事件发生直接跳出循环，下次再轮询
			}

			if (spd_net_flag[i]&BITS_MSK_GET(0,SPD_DATA_NUM))
			{
				printf("spd_net_flag%d=0x%04x\r\n",i,spd_net_flag[i]);
				spd_send_process(i,&spd_net_flag[i]);
				break;	// 有事件发生直接跳出循环，下次再轮询
			}
		}

		// 是否断线判断，断线包含了网络断线和485断线
		if (connect_assert_flag)
		{
			connect_assert_flag = false;
			SPD_disconnct_process();
		}

		usleep(SPD_INTERVAL_TIME);	// 参数设置间隔
	}
}


void SPD_data_init(void)
{
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	int i=0;

	for (i=0;i<SPD_NUM+RES_NUM;i++)
	{
		sockfd_spd[i] = -1;		// 如果TCP，用这个SOCKET变量
		udpfd_spd[i] = -1;		// 如果UDP，用这个SOCKET变量
		connected_flag[i] = 0;	// 是否连接成功的标志,0:不成功，1:成功
		// 设备地址启动就会读取，这里不要赋值，否则覆盖了
		//SPD_Address[i] = SPD_DEFALT_ADDR;		// 如果是串口服务器拖485设备的，需要设备地址
		stuSpd_Param->TimeStamp[i]=timestamp_get();
		stuSpd_Param->Linked[i]=false;
	}
	// 数组大小不一样，分开初始化
	for (i=0;i<SPD_NUM;i++)
	{
		spd_net_flag[i] = 0;	// 轮询数据的变量标志
		spd_ctl_flag[i] = 0;	// 设置数据的变量标志
		WAIT_spd_res_flag[i] = 0;	// 等待回复的标志
	}

	// 华咨可能会有多达10路的参数要配置，可以只配置1路，然后直接拷贝
	if (SPD_Type == TYPE_HUAZI)
	{
		stringstream ss;
		char key[30];
		// 如果用户只配置了防雷监测器0,则其它的为空的都拷贝这个设置
		for(i=1;i<pConf->SPD_num;i++)
		{
			if ((pConf->SPD_Address[0] != 0)&&(pConf->StrSPDAddr[i] == ""))
			{
				// 设备地址自动+1
				pConf->SPD_Address[i] = pConf->SPD_Address[0]+i;
				ss.str("");		// 清掉内容
				// 必须要先转为int才能输入到ss
				ss<<(int)pConf->SPD_Address[i];
	        	pConf->StrSPDAddr[i] = ss.str();
				sprintf(key,"SPD%dAddr=",i+1);
				Setconfig(key,pConf->StrSPDAddr[i]);
			}
			if ((pConf->StrSPDIP[0] != "")&&((pConf->StrSPDIP[i] == "")||(pConf->StrSPDIP[i] == "0.0.0.0")))
			{
				pConf->StrSPDIP[i] = pConf->StrSPDIP[0];			//IP copy
				sprintf(key,"SPD%dIP=",i+1);
				Setconfig(key,pConf->StrSPDIP[i].c_str());
			}
			if ((pConf->StrSPDPort[0] != "")&&(pConf->StrSPDPort[i] == ""))
			{
				pConf->StrSPDPort[i] = pConf->StrSPDPort[0];		//spd端口 copy
				sprintf(key,"SPD%dPort=",i+1);
				Setconfig(key,pConf->StrSPDPort[i].c_str());
			}		
		}

		// 接地电阻也可以copy
		i = SPD_NUM;
		if ((pConf->SPD_Address[0] != 0)&&(pConf->StrSPDAddr[i] == ""))
		{
			// 设备地址自动+1
			pConf->SPD_Address[i] = pConf->SPD_Address[0]+pConf->SPD_num;
			ss.str("");		// 清掉内容
			ss<<(int)pConf->SPD_Address[i];
	        pConf->StrSPDAddr[i] = ss.str();
			sprintf(key,"SPDResAddr=");
			printf("StrSPDAddr%d=%s\r\n",i,pConf->StrSPDAddr[i].c_str());
			Setconfig(key,pConf->StrSPDAddr[i].c_str());
		}
		if ((pConf->StrSPDIP[0] != "")&&((pConf->StrSPDIP[i] == "")||(pConf->StrSPDIP[i] == "0.0.0.0")))
		{
			pConf->StrSPDIP[i] = pConf->StrSPDIP[0];			//IP copy
			sprintf(key,"SPDResIP=");
			Setconfig(key,pConf->StrSPDIP[i]);
		}
		if ((pConf->StrSPDPort[0] != "")&&(pConf->StrSPDPort[i] == ""))
		{
			pConf->StrSPDPort[i] = pConf->StrSPDPort[0];		//spd端口 copy
			sprintf(key,"SPDResPort=");
			Setconfig(key,pConf->StrSPDPort[i]);
		}
		Writeconfig();
	}
}

void init_net_spd()
{
	SPD_data_init();
	// 数据发送线程
	pthread_t tNetwork_server_SPD;
	if (pthread_create(&tNetwork_server_SPD, NULL, NetWork_server_thread_SPD,NULL))
	{
		printf("NetWork SPD create failed!\n");
//		WriteLog("NetWork SPD create failed!\n");
	}
	pthread_detach(tNetwork_server_SPD);

	pthread_mutex_init(&SPDdataHandleMutex,NULL);
}

