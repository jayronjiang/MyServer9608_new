
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
#include "tsPanel.h"
//#include "global.h"


#define LOCKER_CLOSED			0
#define LOCKER_OPEN				1
#define BOX_NULL_VALUE  2147483647

/*      DECLARATION              */
extern CabinetClient *pCabinetClient[HWSERVER_NUM];//华为机柜状态
extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体

/*      DECLARATION      global.h        */
/****************************************************************/
extern void IPGetFromDevice(uint8_t seq,char *ip);

// 取CAN控制板的电压值
extern float voltTrueGetFromDev(uint8_t seq);

// 取CAN控制板的电压值
extern float ampTrueGetFromDev(uint8_t seq);

// 该机柜还是在线?
// seq: 0:设备柜
// 		1:电池柜
extern uint16_t linkStGetFrombox(uint8_t seq);

// 该设备离线还是在线?
// 1：在线，2：离线
extern uint16_t linkStGetFromDevice(uint8_t seq);

// 该设备离线还是在线?
// 1：有电，2：没电
extern uint16_t powerStGetFromDevice(uint8_t seq);

// 该通道是否配置?
extern bool isChannelNull(uint8_t seq);

// 单柜还是双柜?返回柜子个数
extern uint8_t getBoxNum(void);

// 从配置文件中读取CAN控制器版本号,只读地址为1的控制器
extern string getCanVersion(uint8_t seq);

// 获取机柜n的IP地址
extern string IPGetFromBox(uint8_t seq);

// 获取该机柜是离线还是在线?
extern string linkStringGetFromBox(uint8_t seq);

extern void GetIPinfo(IPInfo *ipInfo);

extern void GetIPinfo2(IPInfo *ipInfo);

// 取8个字节的ID号
extern unsigned long long IDgetFromConfig(void);

// seq:DO序号, DeviceName：返回的设备名称，ip:返回的ip地址,port:返回的端口号
extern void IPgetFromDevice(uint8_t seq,string& DeviceName, string& ip, string& port);

extern void VAgetFromDevice(uint8_t seq, string& volt, string& amp);

extern uint16_t DoorStatusFromLocker(void);
extern unsigned long GetTickCount(); //返回秒

// 获取机柜温度 seq:0:设备柜，1：电源柜
extern string TempGetFromBox(uint8_t seq);
// 获取机柜湿度 seq:0:设备柜，1：电源柜
extern string HumiGetFromBox(uint8_t seq);


/****************************************************************/



const uint8_t BOX_SINGLE = 1;
const uint8_t BOX_BI= 2;

const uint8_t BOX_DEV= 0;	// 设备柜
const uint8_t BOX_BAT= 1;	// 电池柜

const uint8_t PAGE1_0= 0;	// 设备页,第1页上半页
const uint8_t PAGE1_1= 1;	// 设备页,第1页下半页
const uint8_t PAGE2_0= 2;	// 设备页,第2页上半页
const uint8_t PAGE2_1= 3;	// 设备页,第2页下半页
const uint8_t PAGE3_0= 4;	// 设备页,第3页上半页
const uint8_t PAGE3_1= 5;	// 设备页,第3页下半页
const uint8_t PAGE4_0= 6;	// 设备页,第4页上半页
const uint8_t PAGE4_1= 7;	// 设备页,第4页下半页

const string Dev_list[MAX_CHANNEL] = 
{
	{"设备1:"},
	{"设备2:"},
	{"设备3:"},
	{"设备4:"},
	
	{"设备5:"},
	{"设备6:"},
	{"设备7:"},
	{"设备8:"},

	{"设备9:"},
	{"设备10:"},
	{"设备11:"},
	{"设备12:"},

	{"设备13:"},
	{"设备14:"},
	{"设备15:"},
	{"设备16:"}
};


/**************************    类函数      ******************************/
// 打开串口1
tsPanel::tsPanel(CabinetClient **pCab,VMCONTROL_CONFIG *pConfig)
{
	// 默认2s钟发送一次页面刷新
	time_interval = WAIT_SECOND_2;
	isConnect = false;				// 默认未连接
	timestamp = timestamp_get();	// 初始化取时间戳
	screen_poll_flag = 0;
	dev_box = 0;
	bat_box = 0;

	CabClientSet(pCab);
	VM_ConfigSet(pConfig);

	mComPort = new CComPort((char *)"/dev/ttyO1",115200);
	callback = NULL;
	pthread_create(&gtid, NULL, &NetPanelGetThread, this);
	pthread_detach(gtid);

	pthread_create(&stid, NULL, &NetPanelSendThread, this);
	pthread_detach(stid);
	DEBUG_PRINTF("touchScreen constructed.\r\n");
}


tsPanel::~tsPanel(void)
{
	pthread_cancel(gtid);
	pthread_cancel(stid);
	DEBUG_PRINTF("touchScreen destructed.\r\n");
}


// 回调函数
void tsPanel::setCallback(Callback cb,void *userdata){
    callback = cb;
    this->userdata = userdata;
}


// 设置标志位的处理函数，外部调用
void tsPanel::ScreenFlagSet(SREEN_SET_LIST sFlag)
{
	screen_poll_flag |= BIT(sFlag);
}


void tsPanel::CabClientSet(CabinetClient **pCab)
{
	for(int i=0; i<HWSERVER_NUM;i++)
	{
		tsCabClient[i] = *(pCab+i);
		printf("tsCabClientaddr=%d\r\n",tsCabClient[i]);
	}
}

void tsPanel::VM_ConfigSet(VMCONTROL_CONFIG *pConfig)
{
	tsVM_Config = pConfig;
	printf("tsVM_Config=%d\r\n",tsVM_Config);
}


// 配置电池柜和设备柜属于哪个动环？0：动环1,1：动环2
void tsPanel::Box_Config(uint16_t dev_name,uint16_t bat_name)
{
	dev_box = dev_name;
	bat_box = bat_name;
}



/***********************    private ***********************************/
// 这里开始都是private函数
void tsPanel::IP1getFromConfig(void)
{
	IPInfo IPDadaForCom;	// 只用于RS232传输IP地址

	memset(&IPDadaForCom,0,sizeof(IPInfo));
	GetIPinfo(&IPDadaForCom);
	//printf("IP1s0x%s \r\n",IPDadaForCom.ip);
	inet_aton(IPDadaForCom.ip,&IPaddr[0]);
}

// 从配置文件中读取IP2地址，放在IPaddr2中
void tsPanel::IP2getFromConfig(void)
{
	IPInfo IPDadaForCom;	// 只用于RS232传输IP地址

	memset(&IPDadaForCom,0,sizeof(IPInfo));
	GetIPinfo2(&IPDadaForCom);
	//printf("IP1s0x%s \r\n",IPDadaForCom.ip);
	inet_aton(IPDadaForCom.ip,&IPaddr[1]);
}


// 开锁关锁，亮屏逻辑
void tsPanel::backlightLinkedToLocker(void)
{
	// 默认是关的
	static uint16_t door_status_last = LOCKER_CLOSED;
	uint16_t door_status_now = LOCKER_CLOSED;		// 现在的门状态
	
	door_status_now = DoorStatusFromLocker();
	//printf("door_status = %d\r\n",door_status_now);
	if (door_status_now != door_status_last)
	{
		// 开门亮屏
		if ((door_status_now == LOCKER_OPEN))
		{
			screen_poll_flag |= BIT(BACKLIGHT_ON_SET);
		}
		else
		{
			// 关门息屏
			screen_poll_flag |= BIT(BACKLIGHT_OFF_SET);
		}
	}
	door_status_last = door_status_now;
}


//文本输出函数
#define TEXT_MAX_LEN			80		//20个汉字基本要60的长度
// 把string合在一起，并转成GBK
// text_len 这个string的固定长度
uint16_t tsPanel::Get_3PartString(char *pbuf,string str_init,uint16_t TEXT_LEN)
{
	string str_all="";
	const char *c_str = NULL;
	uint16_t len = 0;
	char c_utf[TEXT_MAX_LEN]={"",};
	char c_gbk[TEXT_MAX_LEN]={"",};	// GBK后的字符
	char *str_get = pbuf;	// 真正获取的返回值
	uint16_t i=0;

	// 空判断
	if (str_init !="")
	{
		str_all=str_init;
		c_str = str_all.c_str();
		len = strlen(c_str);
		DEBUG_PRINTF("string_len = 0x%02x \r\n",len);
		if (len < TEXT_MAX_LEN)
		{
			strcpy(c_utf,c_str);
		}
		Utf8ToGbk(c_utf,len+1,c_gbk,TEXT_MAX_LEN);
		len = strlen(c_gbk);
		DEBUG_PRINTF("gbk_len = 0x%02x \r\n",len);

		if (len > TEXT_LEN)
		{
			len = TEXT_LEN;
		}
		for(i =0; i<len; i++)
	    {
	    	str_get[i]=c_gbk[i];
		}

		for (i=len; i<TEXT_LEN; i++)
		{
			// 0x20是清除文本显示
			str_get[i] = 0x20;
		}
		str_get[i] = '\0';			// 字符串的长度
		DEBUG_PRINTF("realstr = %s\r\n",str_get);
	}
	else
	{
		for(i =0; i<TEXT_LEN; i++)
	    {
	    	// 0x20是输出空白,即不显示
	    	str_get[i]=0x20;
		}
		str_get[i]='\0';
		DEBUG_PRINTF("string_=NULL\r\n");
	}

	DEBUG_PRINTF("return_len=%d\r\n",i);
	return i;	// 返回实际长度,每次都是TEXT_LEN
}

// 和Get_3PartString一样，但是不转GBK
uint16_t tsPanel::Get_AsciiString(char *pbuf,string str_init,uint16_t TEXT_LEN)
{
	string str_all="";
	const char *c_str = NULL;
	uint16_t len = 0;
	char c_utf[TEXT_MAX_LEN]={"",};
	char *str_get = pbuf;	// 真正获取的返回值
	uint16_t i=0;

	// 空判断
	if (str_init !="")
	{
		str_all=str_init;
		c_str = str_all.c_str();
		len = strlen(c_str);
		DEBUG_PRINTF("string_len = 0x%02x \r\n",len);
		strcpy(c_utf,c_str);

		if (len > TEXT_LEN)
		{
			len = TEXT_LEN;
		}
		for(i =0; i<len; i++)
	    {
	    	str_get[i]=c_utf[i];
		}

		for (i=len; i<TEXT_LEN; i++)
		{
			// 0x20是清除文本显示
			str_get[i] = 0x20;
		}
		str_get[i] = '\0';			// 字符串的长度
		DEBUG_PRINTF("asciistr = %s\r\n",str_get);
	}
	else
	{
		for(i =0; i<TEXT_LEN; i++)
	    {
	    	// 0x20是输出空白,即不显示
	    	str_get[i]=0x20;
		}
		str_get[i]='\0';
		DEBUG_PRINTF("asciistr=NULL\r\n");
	}

	DEBUG_PRINTF("return_len=%d\r\n",i);
	return i;	// 返回实际长度,每次都是TEXT_LEN
}



// 把string合在一起，并转成GBK
// 同时第二行加空格
#define LONG_TEXT_MAX_LEN			160
#define BYTE_CNT_IF_X_FONT			50	// 32的字体每行可以显示50个字节文本
uint16_t tsPanel::GetSpaceString(char *pbuf,string str_init,uint16_t TEXT_LEN,UINT16 n_space)
{
	string str_all="";
	const char *c_str = NULL;
	uint16_t len = 0;
	char c_utf[LONG_TEXT_MAX_LEN]={"",};
	char c_gbk[LONG_TEXT_MAX_LEN]={"",};	// GBK后的字符
	char *str_get = pbuf;	// 真正获取的返回值
	uint16_t i=0;
	uint16_t true_xspace = n_space;

	// 空判断
	if (str_init !="")
	{
		str_all=str_init;
		c_str = str_all.c_str();
		len = strlen(c_str);
		DEBUG_PRINTF("string_len = 0x%02x \r\n",len);
		if (len < LONG_TEXT_MAX_LEN)
		{
			strcpy(c_utf,c_str);
		}
		Utf8ToGbk(c_utf,len+1,c_gbk,LONG_TEXT_MAX_LEN);
		len = strlen(c_gbk);
		/*
		for(int i =0; i<len;i++)
		{
			DEBUG_PRINTF("%02x  ",c_gbk[i]);
		}
		DEBUG_PRINTF("\r\n");*/
		// 看第50个字节是否是汉字，如果是，移到下一个字符，防止出现乱码
		// 汉字是大于80开头
		if ((len >= BYTE_CNT_IF_X_FONT)&&(((uint8_t)c_gbk[BYTE_CNT_IF_X_FONT-1]) >= 0x80))
		{
			for (i=len;i >=(BYTE_CNT_IF_X_FONT-1);i--)
			{
				c_gbk[i+1]= c_gbk[i];
			}
			c_gbk[BYTE_CNT_IF_X_FONT-1] = 0x20;	// 空值
			len = len+1;
		}

		/*
		DEBUG_PRINTF("space_gbk_len = 0x%02x \r\n",len);
		for(int i =0; i<len;i++)
		{
			DEBUG_PRINTF("%02x  ",c_gbk[i]);
		}
		DEBUG_PRINTF("\r\n");*/

		if (len > TEXT_LEN)
		{
			len = TEXT_LEN;
		}
		// 如果是2行2行显示，第二行要加n个空格缩进
		if (TEXT_LEN > BYTE_CNT_IF_X_FONT)
		{
			// 先全部清除
			for (i=0; i<TEXT_LEN; i++)
			{
				// 0x20是清除文本显示，即空格
				str_get[i] = 0x20;
			}
			// 结束符
			str_get[TEXT_LEN] = '\0';
			// 如果本身显示就小于1行，就不需要加空格了
			if (len <= BYTE_CNT_IF_X_FONT)
			{
				for(i =0; i<len; i++)
			    {
			    	str_get[i]=c_gbk[i];
				}
			}
			// 第二行的开头要加空格
			else
			{
				// 如果加上空格就已经超出最大显示长度
				if (len+n_space > TEXT_LEN)
				{
					true_xspace = (len+n_space -TEXT_LEN);
				}
				else
				{
					true_xspace = n_space;
				}

				// 第一行全部拷贝
				for(i =0; i<BYTE_CNT_IF_X_FONT; i++)
			    {
			    	str_get[i]=c_gbk[i];
				}
				// 第二行开头填几个空格
				for(i =BYTE_CNT_IF_X_FONT; i<len; i++)
			    {
			    	str_get[i+true_xspace]=c_gbk[i];
				}
			}
		}
		// 如果是一行一行显示
		else
		{
			// 先全部清除
			for (i=0; i<TEXT_LEN; i++)
			{
				// 0x20是清除文本显示，即空格
				str_get[i] = 0x20;
			}
			// 结束符
			str_get[TEXT_LEN] = '\0';
			// 加一个判断
			if (len <= BYTE_CNT_IF_X_FONT)
			{
				for(i =0; i<len; i++)
			    {
			    	str_get[i]=c_gbk[i];
				}
			}
		}
		DEBUG_PRINTF("devrealstr = %s\r\n",str_get);
	}
	else
	{
		for(i =0; i<TEXT_LEN; i++)
	    {
	    	// 0x20是输出空白,即不显示
	    	str_get[i]=0x20;
		}
		str_get[i]='\0';
		DEBUG_PRINTF("string_=NULL\r\n");
	}
	DEBUG_PRINTF("return_len=%d\r\n",TEXT_LEN);
	return TEXT_LEN;	// 返回实际长度,每次都是TEXT_LEN
}



// 在线离线状态转成成触摸屏的状态
uint8_t tsPanel::linkValueTrans(bool linked)
{
	uint8_t re_val = 0;

	if (linked)
	{
		re_val = 1;
	}
	else
	{
		re_val = 2;
	}
	return re_val;
}


// 断线判断函数
void tsPanel::disconnctProcess(void)
{
	// 有15s未刷新，断线
	if ((timestamp_delta(timestamp) > PANEL_DISC_TIMEOUT) && isConnect)
	{
		isConnect = false;
	}
}


string tsPanel::tsPanelDevSeqFunc(uint8_t seq)
{
	uint8_t len = 0;
	string str_name = "";
	string str_ip = "";
	string str_port = "";
	string str_volt = "";
	string str_amp = "";
	
	// 名称,IP，断开号
	IPgetFromDevice(seq,str_name, str_ip, str_port);
	VAgetFromDevice(seq,str_volt,str_amp);
	str_name = Dev_list[seq]+str_name+str_ip+str_port+str_volt+str_amp;

	return str_name;
}


#define DEV_TEXT_LEN		100	// 2行2行显示
#define INDENT_LEN			4	// 缩进几个字符
// 设备列表页面更新
uint8_t tsPanel::tsPanelDevListPack(uint8_t page,uint8_t *pbuf,uint16_t addr)
{
	uint8_t len = 0;
	uint8_t i = 0;
	uint8_t *cbuf = pbuf;
	string str_name = "";
	uint16_t address = 0;
	
	address = addr+page*100;
	
	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2;		// 帧头为0x5AA5
	cbuf[len++] = 0x05;				// 长度先临时写入5，后面再更新
	cbuf[len++] = CMD_WRITE;
	// 串口屏的地址是以16位为单位的
	cbuf[len++] = ((address>>8)&0xFF);
	cbuf[len++] = address&0xFF;

	for (i=(page*2); i <= (page*2+1); i++)
	{
		str_name = tsPanelDevSeqFunc(i);
		DEBUG_PRINTF("str_naem=%s\r\n",str_name.c_str());
		len += GetSpaceString((char*)(pbuf+len),str_name,DEV_TEXT_LEN,INDENT_LEN);
	}
	pbuf[BUF_LENTH] = len-3;

	return len;
}



#define FLAGID_TEXT_LEN		33
#define BOX_TEXT_LEN		40
// 对门架ID号/温度等进行打包
uint8_t tsPanel::tsPanelBoxValuePack(uint8_t *pbuf,uint16_t addr)
{
	VMCONTROL_CONFIG *pConf=tsVM_Config;	//控制器配置信息结构体
	CabinetClient *pCab=tsCabClient[0];//华为机柜状态
	
	uint8_t len = 0;
	uint8_t *cbuf = pbuf;
	string str_name = "";
	string str_value = "";
	string str_unit = "";
	string str_temp = "";
	
	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2;		// 帧头为0x5AA5
	cbuf[len++] = 0x05;				// 长度先临时写入5，后面再更新
	cbuf[len++] = CMD_WRITE;
	// 串口屏的地址是以16位为单位的
	cbuf[len++] = ((addr>>8)&0xFF);
	cbuf[len++] = addr&0xFF;

	// flag id
	str_name = pConf->StrFlagID;
	DEBUG_PRINTF("StrFlagID = %s\r\n",str_name.c_str());
	len += Get_3PartString((char*)(pbuf+len),str_name,FLAGID_TEXT_LEN);
	//len += GetFlagIDString((char*)(cbuf+len));	// 英文字符和数字GB2312编码只占1个字节
	cbuf[len++] = 0x20;	// 空出一个地址0xF021

	// 设备柜IP地址
	str_name = "设备柜IP:";
	str_value = IPGetFromBox(0);
	str_unit = ",";
	//str_unit= str_unit+linkStringGetFromBox(HUAWEIDevValue.hwLinked);
	str_unit= str_unit+linkStringGetFromBox(0);
	str_name = str_name+str_value +str_unit;
	// 实际会返回40的长度
	len += Get_3PartString((char*)(pbuf+len),str_name,BOX_TEXT_LEN);


	// 柜1温度
	str_name = "设备柜温度:";
	str_value = TempGetFromBox(0);
		//pCab->HUAWEIDevValue.strhwEnvTemperature[0];
	str_unit = "℃,";
	if ((str_value == TO_STR(BOX_NULL_VALUE)) || (str_value == ""))
	{
		str_value ="无效,";
		str_unit = "";
	}
	str_name = str_name+str_value+str_unit;

	// 柜1湿度
	str_temp = "设备柜湿度:";
	str_value = HumiGetFromBox(0);
	str_unit = "%";
	if ((str_value == TO_STR(BOX_NULL_VALUE)) || (str_value == ""))
	{
		str_value ="无效";
		str_unit = "";
	}
	str_temp = str_temp+str_value+str_unit;
	str_name += str_temp;
	// 实际会返回BOX_TEXT_LEN的长度
	len += Get_3PartString((char*)(pbuf+len),str_name,BOX_TEXT_LEN);	// 英文字符和数字GB2312编码只占1个字节

	if (getBoxNum()==BOX_BI)
	{
		// 电池柜IP地址
		str_name = "电池柜IP:";
		str_value = IPGetFromBox(1);
		str_unit = ",";
		str_unit= str_unit+linkStringGetFromBox(1);
		str_name = str_name+str_value +str_unit;
		// 实际会返回40的长度
		len += Get_3PartString((char*)(pbuf+len),str_name,BOX_TEXT_LEN);
		
		// 柜2温度
		str_name = "电池柜温度:";
		str_value = TempGetFromBox(1);
		str_unit = "℃,";
		if ((str_value == TO_STR(BOX_NULL_VALUE)) || (str_value == ""))
		{
			str_value ="无效,";
			str_unit = "";
		}
		str_name = str_name+str_value+str_unit;

		// 柜2湿度
		str_temp = "湿度:";
		str_value = HumiGetFromBox(1);
		str_unit = "%";
		if ((str_value == TO_STR(BOX_NULL_VALUE)) || (str_value == ""))
		{
			str_value ="无效";
			str_unit = "";
		}
		str_temp = str_temp+str_value+str_unit;
		str_name += str_temp;
		// 实际会返回BOX_TEXT_LEN的长度
		len += Get_3PartString((char*)(pbuf+len),str_name,BOX_TEXT_LEN);	// 英文字符和数字GB2312编码只占1个字节
	}
	else
	{
		// 电池柜IP地址
		str_name = "";
		// 实际会返回40的长度
		len += Get_3PartString((char*)(pbuf+len),str_name,BOX_TEXT_LEN);

		// 温湿度
		str_name = "";
		len += Get_3PartString((char*)(pbuf+len),str_name,BOX_TEXT_LEN);
	}
	// 主程序版本号,版本号只使用12个字节长度
	str_name = pConf->StrVersionNo;
	printf("StrVersionNo= %s\r\n",str_name.c_str());
	printf("pConf->StrVersionNo= %s\r\n",pConf->StrVersionNo.c_str());
	// 实际会返回12的长度
	len += Get_AsciiString((char*)(pbuf+len),str_name,12);

	// 地址为1CAN控制器版本号,版本号只使用12个字节长度
	str_name = getCanVersion(0);
	// 实际会返回12的长度
	len += Get_AsciiString((char*)(pbuf+len),str_name,12);
	

	pbuf[BUF_LENTH] = len-3;

	return len;
}



// 对门架ID号/温度等进行打包
uint8_t tsPanel::tsPanelVarMsgStringPack(uint8_t *pbuf,uint16_t addr)
{
	uint8_t len = 0;
	uint8_t *cbuf = pbuf;
	unsigned long long IDInfo = 0;
	uint16_t t_value = 0;
	int i;

	IDInfo = IDgetFromConfig();
	IP1getFromConfig();
	IP2getFromConfig();
	
	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2;		// 帧头为0x5AA5
	cbuf[len++] = 0x05;				// 长度先临时写入5，后面再更新
	cbuf[len++] = CMD_WRITE;
	// 串口屏的地址是以16位为单位的
	cbuf[len++] = ((addr>>8)&0xFF);
	cbuf[len++] = addr&0xFF;

	// IP地址转换过来顺序是反的，IP高位在s_addr的低位
	// IP地址1
	cbuf[len++] = 0;
	cbuf[len++] = IPaddr[0].s_addr&0xFF;;
	cbuf[len++] = 0;
	cbuf[len++] = (IPaddr[0].s_addr >>8)&0xFF;;
	cbuf[len++] = 0;
	cbuf[len++] = (IPaddr[0].s_addr >>16)&0xFF;
	cbuf[len++] = 0;
	cbuf[len++] = (IPaddr[0].s_addr >>24) &0xFF;

	// IP地址2 默认192.192.1.136，需要改
	cbuf[len++] = 0;
	cbuf[len++] = IPaddr[1].s_addr&0xFF;;
	cbuf[len++] = 0;
	cbuf[len++] = (IPaddr[1].s_addr >>8)&0xFF;;
	cbuf[len++] = 0;
	cbuf[len++] = (IPaddr[1].s_addr >>16)&0xFF;
	cbuf[len++] = 0;
	cbuf[len++] = (IPaddr[1].s_addr >>24) &0xFF;

	// ID号
	cbuf[len++] = (IDInfo>>56)&0xFF;
	cbuf[len++] = (IDInfo>>48)&0xFF;
	cbuf[len++] = (IDInfo>>40)&0xFF;
	cbuf[len++] = (IDInfo>>32)&0xFF;
	cbuf[len++] = (IDInfo>>24)&0xFF;
	cbuf[len++] = (IDInfo>>16)&0xFF;
	cbuf[len++] = (IDInfo>>8)&0xFF;
	cbuf[len++] = IDInfo&0xFF;

	// 机柜是否离线在线
	// 断电/通电 在jasonpackege 4224行
	// 2个柜子的动环连接状态
	// 柜1
	t_value = linkStGetFrombox(dev_box);
	cbuf[len++] =t_value>>8;
	cbuf[len++] =t_value;
	if (getBoxNum() == BOX_BI)
	{
		t_value = linkStGetFrombox(bat_box);
		cbuf[len++] =t_value>>8;
		cbuf[len++] =t_value;
	}
	else
	{
		// 没有配置就不显示
		cbuf[len++] = 0;
		cbuf[len++] = 0;
	}

	// 是否在线
	for (i=0; i<MAX_CHANNEL; i++)
	{
		if(isChannelNull(i) == false)
		{
			// 这里要改
			t_value = linkStGetFromDevice(i);
			cbuf[len++] =t_value>>8;
			cbuf[len++] =t_value;
		}
		else
		{
			// 没有配置就不显示
			cbuf[len++] = 0;
			cbuf[len++] = 0;
		}
	}

	// 电源是否断电,断不断电和有没有配置没有关系
	for (i=0; i<MAX_CHANNEL; i++)
	{
		{
			// 这里要改
			t_value = powerStGetFromDevice(i);
			cbuf[len++] =t_value>>8;
			cbuf[len++] =t_value;
		}
	}
	// 实际长度要减去前面3个字节
	cbuf[BUF_LENTH] = len-3;

	return len;
}


// RTC打包
uint8_t tsPanel::tsPanelRTCStringPack(uint8_t *pbuf,uint16_t addr)
{
	uint8_t len = 0;
	uint8_t *cbuf = pbuf;
	time_t nSeconds;
	struct tm * pTM;

	// 取得当前时间
	time(&nSeconds);
	pTM = localtime(&nSeconds);
	
	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2; 	// 帧头为0x5AA5
	cbuf[len++] = 0x05; 			// 长度先临时写入5，后面再更新
	cbuf[len++] = CMD_WRITE;
	// 时间的地址是固定的0x009C
	cbuf[len++] = ((addr>>8)&0xFF);
	cbuf[len++] = addr&0xFF;
	// 还要加一帧5A A5
	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2;
	// 触摸屏年份从2000年开始, 而这里是从1900年开始
	cbuf[len++] = (uint8_t)(pTM->tm_year-100);
	// 月份0~11，要加1
	cbuf[len++] = (uint8_t)(pTM->tm_mon+1);
	cbuf[len++] = (uint8_t)pTM->tm_mday;
	cbuf[len++] = (uint8_t)pTM->tm_hour;
	cbuf[len++] = (uint8_t)pTM->tm_min;
	cbuf[len++] = (uint8_t)pTM->tm_sec;
	cbuf[BUF_LENTH] = len-3;

	return len;
}

uint8_t tsPanel::tsPanelBackLightEnablePack(uint8_t *pbuf,uint16_t addr)
{
	uint8_t len = 0;
	uint8_t *cbuf = pbuf;

	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2;		// 帧头为0x5AA5
	cbuf[len++] = 0x07;				// 长度先临时写入7，后面再更新
	cbuf[len++] = CMD_WRITE;
	// 串口屏的地址是以16位为单位的
	cbuf[len++] = ((addr>>8)&0xFF);
	cbuf[len++] = addr&0xFF;
	cbuf[len++] = 0x5A;		// 写的时候要是0x5A，配置生效后会清0
	cbuf[len++] = 0x00;		// 不关心
	cbuf[len++] = 0x00;		// 不关心
	cbuf[len++] = 0x3C;		// 开启背光位是bit3,1:开启，0:关闭
	cbuf[BUF_LENTH] = len-3;
	
	return len;
}


uint8_t tsPanel::tsPanelBackLightCfgPack(uint8_t *pbuf,uint16_t addr)
{
	uint8_t len = 0;
	uint8_t *cbuf = pbuf;

	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2;		// 帧头为0x5AA5
	cbuf[len++] = 0x07;				// 长度先临时写入7，后面再更新
	cbuf[len++] = CMD_WRITE;
	// 串口屏的地址是以16位为单位的
	cbuf[len++] = ((addr>>8)&0xFF);
	cbuf[len++] = addr&0xFF;
	cbuf[len++] = LIGHT_100_ON;		// 背光打开时的亮度，100%
	cbuf[len++] = LIGHT_0_OFF;		// 背光关闭时的亮度，0%
	cbuf[len++] = ((SLEEP_ENTER_TIM>>8)&0xFF);
	cbuf[len++] = SLEEP_ENTER_TIM&0xFF;	// 进入休眠的时间
	cbuf[BUF_LENTH] = len-3;
	
	return len;
}


uint8_t tsPanel::tsPanelBackLightOnPack(uint8_t *pbuf,uint16_t addr)
{
	uint8_t len = 0;
	uint8_t *cbuf = pbuf;

	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2; 	// 帧头为0x5AA5
	cbuf[len++] = 0x05; 			// 长度先临时写入5，后面再更新
	cbuf[len++] = CMD_WRITE;
	// 串口屏的地址是以16位为单位的
	cbuf[len++] = ((addr>>8)&0xFF);
	cbuf[len++] = addr&0xFF;
	cbuf[len++] = LIGHT_100_ON; 	// 背光打开时的亮度，100%
	cbuf[len++] = LIGHT_100_ON; 	// 背光关闭时的亮度，0%
	cbuf[BUF_LENTH] = len-3;
	
	return len;
}


uint8_t tsPanel::tsPanelBackLightOffPack(uint8_t *pbuf,uint16_t addr)
{
	uint8_t len = 0;
	uint8_t *cbuf = pbuf;

	cbuf[len++] = FRAME_HEAD_1;
	cbuf[len++] = FRAME_HEAD_2;		// 帧头为0x5AA5
	cbuf[len++] = 0x05;				// 长度先临时写入5，后面再更新
	cbuf[len++] = CMD_WRITE;
	// 串口屏的地址是以16位为单位的
	cbuf[len++] = ((addr>>8)&0xFF);
	cbuf[len++] = addr&0xFF;
	cbuf[len++] = LIGHT_100_ON;		// 背光打开时的亮度，100%
	cbuf[len++] = LIGHT_0_OFF;		// 背光关闭时的亮度，0%
	cbuf[BUF_LENTH] = len-3;
	
	return len;
}


// 屏幕数据打包，返回长度
uint8_t tsPanel::tsMessagePack(uint16_t address,uint8_t msg_type,uint8_t *buf)
{
	/*取得目标串口对应的发送缓存*/
	uint8_t *pbuf = buf;	//buf->pTxBuf;
	uint8_t len = 0;
	unsigned long long IDInfo = 0;
	unsigned long long flagIDInfo = 0;
	//UINT16 temp_v[2];
	//UINT16 moist_v[2];
	//char letter = 0;

	switch (msg_type)
	{
	case WRITE_VAR_MSG:
		len = tsPanelVarMsgStringPack(pbuf,address);
		break;

	case WRITE_TIME_MSG:
		len = tsPanelRTCStringPack(pbuf,address);
		break;

	case BACKLIGHT_EN_MSG:
		len = tsPanelBackLightEnablePack(pbuf,address);
		break;

	case BACKLIGHT_CFG_MSG:
		len = tsPanelBackLightCfgPack(pbuf,address);
		break;

	case BACKLIGHT_ON_MSG:
		len = tsPanelBackLightOnPack(pbuf,address);
		break;

	case BACKLIGHT_OFF_MSG:
		len = tsPanelBackLightOffPack(pbuf,address);
		break;

	case WRITE_FLAG_BOX_MSG:
		len=tsPanelBoxValuePack(pbuf,address);
		break;
	
	case WRITE_DEV_PAGE1_0_MSG:
		len=tsPanelDevListPack(PAGE1_0,pbuf,address);
		break;

	case WRITE_DEV_PAGE1_1_MSG:
		len=tsPanelDevListPack(PAGE1_1,pbuf,address);
		break;

	case WRITE_DEV_PAGE2_0_MSG:
		len=tsPanelDevListPack(PAGE2_0,pbuf,address);
		break;

	case WRITE_DEV_PAGE2_1_MSG:
		len=tsPanelDevListPack(PAGE2_1,pbuf,address);
		break;

	case WRITE_DEV_PAGE3_0_MSG:
		len=tsPanelDevListPack(PAGE3_0,pbuf,address);
		break;

	case WRITE_DEV_PAGE3_1_MSG:
		len=tsPanelDevListPack(PAGE3_1,pbuf,address);
		break;

	case WRITE_DEV_PAGE4_0_MSG:
		len=tsPanelDevListPack(PAGE4_0,pbuf,address);
		break;

	case WRITE_DEV_PAGE4_1_MSG:
		len=tsPanelDevListPack(PAGE4_1,pbuf,address);


	case NOT_USED_MSG:
		break;
	default:
		break;
	}
	return len;
}



// 写屏幕数据
void tsPanel::TouchScreenWrite(uint16_t Addr, uint8_t Func)
{
	ComCri.Lock();
    uint8_t i;
	uint8_t bytSend[256];
	uint8_t len;

	len = tsMessagePack(Addr,Func,bytSend);

	// debug测试打印
	DEBUG_PRINTF("screen data=%d\r\n",len);
	for(i=0;i<len;i++) 
	{
		DEBUG_PRINTF("%02x ",bytSend[i]);
	}
	DEBUG_PRINTF("\r\n");

	mComPort->SendBuf(bytSend,len);
    ComCri.UnLock();
	usleep(5000);//delay 5ms
}



// 接收线程
void *tsPanel::NetPanelGetThread(void *arg) 
{
    tsPanel *p_tsPanel = (tsPanel *)arg;
    int buffPos=0;
	int len ;
	unsigned char buf[256] ;

	while(1)
	{
		len = read(p_tsPanel->mComPort->fd, buf+buffPos, 256);
		buffPos = buffPos+len;
		if(buffPos<5) continue;
	
		// 这里有个小bug,当屏幕热插拔后，有回复，当时数据帧可能会不完整，会出现
		// FRAM error, 可能半分钟后才会恢复, 导致的结果就是屏幕进入休眠很慢
		if((buf[BUF_HEAD1] != FRAME_HEAD_1) || (buf[BUF_HEAD2] != FRAME_HEAD_2))
		{
			printf("FRAM error\r\n");
			if(buffPos>=256) buffPos=0;
			continue ;
		}
	
		if ((buf[BUF_CMD] == CMD_WRITE) || (buf[BUF_CMD] == CMD_READ))
		{
			if ((len == 6) && (buf[4] == 0x4F) && (buf[5] == 0x4B))
			{
				p_tsPanel->timestamp = GetTickCount();
				p_tsPanel->isConnect = true;
			}
		}

		if(p_tsPanel->callback != NULL)
		{
			p_tsPanel->userdata = (void *)buf;
            p_tsPanel->callback(p_tsPanel->userdata,len);
		}
		buffPos=0;
		usleep(5000);//delay 5ms
	}
}

// 发送线程
void *tsPanel::NetPanelSendThread(void *arg)
{
	tsPanel *p_tsPanel = (tsPanel *)arg;
	static bool entry = false;	// 刚连上屏幕要发背光休眠设置命令
	static uint8_t entry_cnt = 0;
	static uint16_t screen_poll_cnt = 0;
	static uint16_t s_cnt = (uint16_t)VAR_SET;
	// 默认是关的
	//static uint16_t door_status_last = LOCKER_CLOSED;
	//uint16_t door_status_now = LOCKER_CLOSED;		// 现在的门状态

	while(1)
	{
		screen_poll_cnt++;
		// 2s钟1个项目
		if (screen_poll_cnt >= p_tsPanel->time_interval)
		{
			screen_poll_cnt = 0;
			p_tsPanel->screen_poll_flag |= BIT(s_cnt);
			s_cnt++;
			if (s_cnt >=(uint16_t)SCREEN_SET_NUM)
			{
				s_cnt = (uint16_t)VAR_SET;
			}
		}

		/************断线逻辑,这个是屏幕背光休眠需要的，不然换屏后失效********/
		if (p_tsPanel->isConnect == false)
		{
			DEBUG_PRINTF("com break!\r\n");
			entry = false;
			entry_cnt = 0;
		}
		else		// 否则发送
		{
			// 第一次进来,默认是一个新屏幕
			if (entry == false)
			{
				DEBUG_PRINTF("new display!\r\n");
				// 多发一次，防止失败
				entry_cnt++;
				if (entry_cnt >= 2)
				{
					entry_cnt = 2;
					entry = true;
					printf("com recovery!\r\n");
				}
				p_tsPanel->screen_poll_flag |= BIT(BACKLIGHT_EN_SET);
				// 对时一次
				p_tsPanel->screen_poll_flag |= BIT(SCREEN_TIME_SET);
			}
		}
		/*******************断线逻辑,完*********************/
		/*-----------------------------------------------------------------------------*/

		// 等待电子锁库完成
		p_tsPanel->backlightLinkedToLocker();
		
		DEBUG_PRINTF("screen_poll_flag=%02x!\r\n",p_tsPanel->screen_poll_flag);
		// 断线判断
		p_tsPanel->disconnctProcess();

		// 以上是置标志，现在是真正的发送数据
		// 设置时序，休眠具有最高时序
		if (p_tsPanel->screen_poll_flag&BIT(BACKLIGHT_EN_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(BACKLIGHT_EN_SET));
			// 先使能背光, 再设置时间，直接一起, 不然发2次不好处理
			p_tsPanel->TouchScreenWrite(SYS_CFG_ADDR,BACKLIGHT_EN_MSG);
			usleep(MSG_SEND_INTERVAL);
			p_tsPanel->TouchScreenWrite(LED_CFG_ADDR,BACKLIGHT_CFG_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(BACKLIGHT_ON_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(BACKLIGHT_ON_SET));
			p_tsPanel->TouchScreenWrite(LED_CFG_ADDR,BACKLIGHT_ON_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(BACKLIGHT_OFF_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(BACKLIGHT_OFF_SET));
			p_tsPanel->TouchScreenWrite(LED_CFG_ADDR,BACKLIGHT_OFF_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(SCREEN_TIME_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(SCREEN_TIME_SET));
			p_tsPanel->TouchScreenWrite(TIME_REG_ADD,WRITE_TIME_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(VAR_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(VAR_SET));
			p_tsPanel->TouchScreenWrite(VAR_REG_ADD,WRITE_VAR_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(FLAG_STRING_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(FLAG_STRING_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(FLAGID_BOX_REG_ADD,WRITE_FLAG_BOX_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(FLAG_STRING_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(FLAG_STRING_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(FLAGID_BOX_REG_ADD,WRITE_FLAG_BOX_MSG);
		}
		
		else if (p_tsPanel->screen_poll_flag&BIT(DEV_PAGE_1_0_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(DEV_PAGE_1_0_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(DEV_PAGE_REG_ADD,WRITE_DEV_PAGE1_0_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(DEV_PAGE_1_1_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(DEV_PAGE_1_1_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(DEV_PAGE_REG_ADD,WRITE_DEV_PAGE1_1_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(DEV_PAGE_2_0_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(DEV_PAGE_2_0_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(DEV_PAGE_REG_ADD,WRITE_DEV_PAGE2_0_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(DEV_PAGE_2_1_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(DEV_PAGE_2_1_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(DEV_PAGE_REG_ADD,WRITE_DEV_PAGE2_1_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(DEV_PAGE_3_0_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(DEV_PAGE_3_0_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(DEV_PAGE_REG_ADD,WRITE_DEV_PAGE3_0_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(DEV_PAGE_3_1_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(DEV_PAGE_3_1_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(DEV_PAGE_REG_ADD,WRITE_DEV_PAGE3_1_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(DEV_PAGE_4_0_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(DEV_PAGE_4_0_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(DEV_PAGE_REG_ADD,WRITE_DEV_PAGE4_0_MSG);
		}
		else if (p_tsPanel->screen_poll_flag&BIT(DEV_PAGE_4_1_SET))
		{
			p_tsPanel->screen_poll_flag &= ~(BIT(DEV_PAGE_4_1_SET));
			// 门架号,以及温湿度等
			p_tsPanel->TouchScreenWrite(DEV_PAGE_REG_ADD,WRITE_DEV_PAGE4_1_MSG);
		}
		else
		{
			p_tsPanel->screen_poll_flag = 0;
		}
		usleep(MSG_SEND_INTERVAL);	// 周期400ms
	}
	return 0 ;
}
 

