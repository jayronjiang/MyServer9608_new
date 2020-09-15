#ifndef __LT_STATE_THREAD_h__
#define __LT_STATE_THREAD_h__

#include <string>

using namespace std;//引入整个名空间

//控制器运行状态结构体
typedef struct vmctl_state_struct
{
	unsigned long TimeStamp; 		//状态获取时间戳
	bool Linked;					//连接状态 0:断开 1：连接

	string strhostname;
	string strcpuRate;
	string strcpuTemp;
	string strmenTotal;
	string strmenUsed;
	string strmenRate;
	string strzimageVer;
	string strzimageDate;
	string strCpuAlarm;		//CPU使用率报警
	string strCpuTempAlarm;		//CPU温度报警
	string strMemAlarm;		//内存使用率报警
	string strSoftRate;		//主程序CPU占用率
	string strSoftAlarm;	//主程序运行异常报警
	string strKsoftirqdRate;	//ksoftirqdrateCPU占用率
	string strkworkerRate;	//kworkerCPU占用率
	string strLan2BroadcastAlarm;	//网口2广播风暴报警
	string strLan2BroadcastAlarmMesg;	//网口2广播风暴报警信息
}VMCONTROL_STATE;
	

void init_lt_status();
void init_lt_state_struct();


#endif

