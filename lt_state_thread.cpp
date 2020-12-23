#include <stdlib.h>
#include <linux/watchdog.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>  
#include <sys/ioctl.h>
#include <string>
#include <sys/time.h>
#include <csignal>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include "lt_state_thread.h"
#include "registers.h"

using namespace std;

char *infobuf;
extern VMCONTROL_STATE VMCtl_State;	//控制器运行状态结构体
extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体
extern unsigned long GetTickCount();
extern void myprintf(char* str);
extern void WriteLog(char* str);

int ReadLTInfo(char *pbuf)
{
    FILE* Infofd ;
    if((Infofd=fopen("./ltinfo", "rb"))==NULL)
    {
        printf("read ltinfo erro\r\n");
        return 0;
    }
    else
    {
        fread(pbuf,1,1024*5,Infofd);
        fclose(Infofd);
        return 1;
    }

}

int getstrvaule(string &mstr,int mIndex)
{
   int pos = 0 ;
   string strret = "";
   //mstr = "Cpu(s):  0.2%us,  0.3%sy,  0.0%ni, 99.5%id,  0.0%wa,  0.0%hi,  0.0%si,  0.0%st";
   for(int i=0;i<mIndex;i++)
   {
      pos = mstr.find(" ");
      if(pos < 0)
      {
         //printf("return:%d\r\n",i);
         mstr = "";
         return i ;
      }
      strret = mstr.substr(0,pos) ;
      //printf("getmstr:%s\r\n",strret.c_str());
      mstr = mstr.substr(pos+1);
      
      pos = mstr.find_first_not_of(" ");
      if(pos > 0)
      {
        mstr = mstr.substr(pos) ;
        //printf("nextmstr:%s,pos:%d\r\n",mstr.c_str(),pos);
      }
   }
   mstr = strret ;
   //处理最后回车的值
   if(mstr.find("\r\n") == (mstr.size()-2))
   {
       mstr = mstr.substr(0,(mstr.size()-2)) ;
   }
   else if(mstr.find("\n") == (mstr.size()-1))
   {
       mstr = mstr.substr(0,(mstr.size()-1)) ;
   }
   return mIndex ;
}
int getstrvaule2(string &mstr,int mCols,int mIndex)
{
   int i,pos = 0 ;
   string strret = "";
   //mstr = "Cpu(s):  0.2%us,  0.3%sy,	0.0%ni, 99.5%id,  0.0%wa,  0.0%hi,	0.0%si,  0.0%st";
   for(i=0;i<mCols;i++)
   {
	  pos = mstr.find(" ");
	  if(pos < 0)
	  {
		 //printf("return:%d\r\n",i);
		 mstr = "";
		 return i;
	  }
	  if(i==mIndex)
	  {
		  strret = mstr.substr(0,pos) ;
		  //printf("getmstr:%s\r\n",strret.c_str());
	  }
	  mstr = mstr.substr(pos);
	  
	  pos = mstr.find_first_not_of(" ");
	  if(pos > 0)
	  {
		mstr = mstr.substr(pos) ;
		//printf("nextmstr:%s\r\n",mstr.c_str());
	  }
   }
   //printf("strret:%s\r\n",strret.c_str());
   mstr = strret ;
   //处理最后回车的值
   if(mstr.find("\r\n") == (mstr.size()-2))
   {
	   mstr = mstr.substr(0,(mstr.size()-2)) ;
   }
   else if(mstr.find("\n") == (mstr.size()-1))
   {
	   mstr = mstr.substr(0,(mstr.size()-1)) ;
   }
   return i ;
}



int getallstate(void)
{
    char strmsg[256],value[20];
	string strinfo = "";
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	VMCONTROL_STATE *pSta=&VMCtl_State; //控制器运行状态结构体

	pSta->TimeStamp=GetTickCount();
	pSta->Linked=true;
//sprintf(strmsg,"begin getallstate timestamp=0x%x\n",pSta->TimeStamp);
//WriteLog(strmsg);

    //查询系统名称
    system("hostname > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
//WriteLog(infobuf);
    pSta->strhostname = infobuf;
    //处理最后回车的值
    if(pSta->strhostname.find("\r\n") == (pSta->strhostname.size()-2))
    {
        pSta->strhostname = pSta->strhostname.substr(0,(pSta->strhostname.size()-2)) ;
    }
    else if(pSta->strhostname.find("\n") == (pSta->strhostname.size()-1))
    {
        pSta->strhostname = pSta->strhostname.substr(0,(pSta->strhostname.size()-1)) ;
    }
	pSta->strhostname="LTXY-INT-301";
//sprintf(strmsg,"系统名称:%s\n",pSta->strhostname.c_str());
//WriteLog(strmsg);
    printf("系统名称:%s\r\n",pSta->strhostname.c_str());

	//查询主程序占用率
//    system("ps aux |grep /opt/tranter |awk '{ print $3 }' > ./ltinfo");
    system("top -bn1 | grep /opt/tranter |awk '{ print $7 }' > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
    strinfo = infobuf;
//WriteLog(infobuf);
	int posr=strinfo.find("%");
	if(posr>0)
	{
		pSta->strSoftRate=strinfo.substr(0,posr);
	}
	else
	{
		pSta->strSoftRate=strinfo;
	}
	float softRate=atof(pSta->strSoftRate.c_str())+1;
	softRate=softRate<99?softRate+1:softRate;
	sprintf(value,"%.2f",softRate);
	pSta->strSoftRate=value;
//sprintf(strmsg,"主程序占用率:%s%\n",pSta->strSoftRate.c_str());
//WriteLog(strmsg);
	printf("主程序占用率:%s%\r\n",pSta->strSoftRate.c_str());
	
	if(atof(pSta->strSoftRate.c_str())>=50.0)
		pSta->strSoftAlarm = "1";
	else
		pSta->strSoftAlarm = "0";
    printf("主程序运行异常报警:%s\r\n",pSta->strSoftAlarm.c_str());

	//ksoftirqd占用率
    system("top -bn1 |grep ksoftirqd/0 |awk '{ print $7 }' > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
//WriteLog(infobuf);
    strinfo = infobuf;
	posr=strinfo.find("%");
	if(posr>0)
	{
		pSta->strKsoftirqdRate=strinfo.substr(0,posr);
	}
	else
	{
		pSta->strKsoftirqdRate=strinfo;
	}
//sprintf(strmsg,"ksoftirqd占用率:%s%\n",pSta->strKsoftirqdRate.c_str());
//WriteLog(strmsg);
    printf("ksoftirqd占用率:%s%\r\n",pSta->strKsoftirqdRate.c_str());

	//kworker占用率
    system("top -bn1 |grep kworker/0 |awk '{ print $7 }' > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
//WriteLog(infobuf);
    strinfo = infobuf;
	posr=strinfo.find("\n");
	if(posr>0)
	{
		pSta->strkworkerRate=strinfo.substr(0,posr);
	}
	else
	{
		pSta->strkworkerRate=strinfo;
	}
//sprintf(strmsg,"kworker占用率:%s%\n",pSta->strkworkerRate.c_str());
//WriteLog(strmsg);
    printf("kworker占用率:%s%\r\n",pSta->strkworkerRate.c_str());

	//查询网口2广播风暴报警信息
    system("dmesg  | grep \"bogus pkt size\" > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
    strinfo = infobuf;
	if(strinfo!="")
	{
		pSta->strLan2BroadcastAlarm="1";
		
		system("echo 1 > /proc/sys/kernel/sysrq") ;		//reboot
		sleep(2);
		system("echo b > /proc/sysrq-trigger") ;
	}
	else
	{
		pSta->strLan2BroadcastAlarm="0";
	}
	pSta->strLan2BroadcastAlarmMesg=strinfo;
//sprintf(strmsg,"网口2广播风暴报警:%s\n",pSta->strLan2BroadcastAlarm.c_str());
//WriteLog(strmsg);
    printf("网口2广播风暴报警:%s\r\n",pSta->strLan2BroadcastAlarm.c_str());
    printf("网口2广播风暴报警信息:%s\r\n",pSta->strLan2BroadcastAlarmMesg.c_str());


    //查询CPU占用率
    system("top -bn1 | grep CPU: > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
//WriteLog(infobuf);
    strinfo = infobuf;
	int poscpuid=strinfo.find("% idle");
	int poscpuni=strinfo.find("nic ");
	if(poscpuid>0 && poscpuni>0)
	{
		pSta->strcpuRate = strinfo.substr(poscpuni+4,poscpuid-(poscpuni+4));
	}	

    float fcpuRate = atof(pSta->strcpuRate.c_str());
    fcpuRate = 100.0 - fcpuRate;
	fcpuRate = fcpuRate<99?fcpuRate+1:fcpuRate;
	sprintf(value,"%.2f", fcpuRate);
    pSta->strcpuRate = value;
//sprintf(strmsg,"CPU占用率:%s%\n",pSta->strcpuRate.c_str());
//WriteLog(strmsg);
    printf("CPU占用率:%s%\r\n",pSta->strcpuRate.c_str());
	
	if(fcpuRate>=atof(pConf->StrCpuAlarmValue.c_str()))
		pSta->strCpuAlarm = "1";
	else
		pSta->strCpuAlarm = "0";
    printf("CPU使用率报警:%s\r\n",pSta->strCpuAlarm.c_str());

    //查询CPU温度
//    system("npu-smi info -t temp -i 0 | grep LM75A_TE > ./ltinfo");
    system("cat /sys/class/thermal/thermal_zone0/temp > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
//WriteLog(infobuf);
    strinfo = infobuf;
	sprintf(infobuf,"%.1f\0",atoi(infobuf)/1000.0);
	pSta->strcpuTemp=infobuf;   
//sprintf(strmsg,"CPU温度:%s\n",pSta->strcpuTemp.c_str());
//WriteLog(strmsg);
    printf("CPU温度:%s\r\n",pSta->strcpuTemp.c_str());

	if(atof(pSta->strcpuTemp.c_str())>=atof(pConf->StrCpuTempAlarmValue.c_str()))
		pSta->strCpuTempAlarm = "1";
	else
		pSta->strCpuTempAlarm = "0";
    printf("CPU温度报警:%s\r\n",pSta->strCpuTempAlarm.c_str());

    //查询内存大小
    system("free -m | grep Mem > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
    strinfo = infobuf;
//WriteLog(infobuf);
    strinfo = strinfo + " ";
    if(getstrvaule(strinfo,2) == 2)
       pSta->strmenTotal = strinfo;
    else
       pSta->strmenTotal = "" ;
//sprintf(strmsg,"内存总大小%sM\n",pSta->strmenTotal.c_str());
//WriteLog(strmsg);
    printf("内存总大小%sM\r\n",pSta->strmenTotal.c_str());

    strinfo = infobuf;
//WriteLog(infobuf);
    strinfo = strinfo + " ";
    if(getstrvaule(strinfo,3) == 3)
       pSta->strmenUsed = strinfo;
    else
       pSta->strmenUsed = "" ;
//sprintf(strmsg,"内存使用大小%sM\n",pSta->strmenUsed.c_str());
//WriteLog(strmsg);
    printf("内存使用大小%sM\r\n",pSta->strmenUsed.c_str());

    float FmenTotal = (float)atoi(pSta->strmenTotal.c_str());
    float FmenUse = (float)atoi(pSta->strmenUsed.c_str());
    float FmenRate = (FmenUse/FmenTotal)*100 ;

	sprintf(infobuf,"%.1f\0",FmenRate);
	pSta->strmenRate=infobuf;
//sprintf(strmsg,"内存占用率%s%\n",pSta->strmenRate.c_str());
//WriteLog(strmsg);
    printf("内存占用率%s%\r\n",pSta->strmenRate.c_str());

	if(atof(pSta->strmenRate.c_str())>=atof(pConf->StrMemAlarmValue.c_str()))
		pSta->strMemAlarm = "1";
	else
		pSta->strMemAlarm = "0";
//sprintf(strmsg,"内存使用率报警:%s\n",pSta->strMemAlarm.c_str());
//WriteLog(strmsg);
    printf("内存使用率报警:%s\r\n",pSta->strMemAlarm.c_str());

    //查询内核日期
    system("uname -a > ./ltinfo");
    memset(infobuf,0x00,1024*5+1) ;
    ReadLTInfo(infobuf);
//WriteLog(infobuf);
	string verdate=infobuf;
	int pos1=verdate.find("Aug 14 09:24:07 CST 2019");
	int pos2=verdate.find("Aug 18 16:33:37 CST 2020");
	int pos3=verdate.find("Sep 30 03:26:33 PDT 2020");
	int pos4=verdate.find("Oct 26 02:52:36 PDT 2020");
//printf("内核1:pos1=%d,pos2=%d,%s\r\n",pos1,pos2,verdate.c_str());

    //处理最后回车的值
	if(pos1>0)
	{
    	pSta->strzimageVer = "V1.0";
    	pSta->strzimageDate = "2019-08-14 09:24:07";
	}
    else if(pos2>0)
    {
    	pSta->strzimageVer = "V1.1";
    	pSta->strzimageDate = "2020-08-14 16:33:37";
    }
    else if(pos3>0)
    {
    	pSta->strzimageVer = "V1.2";
    	pSta->strzimageDate = "2020-09-30 03:26:33";
    }
    else if(pos4>0)
    {
    	pSta->strzimageVer = "V1.3";
    	pSta->strzimageDate = "2020-10-26 02:52:36";
    }
	else
	{
		pSta->strzimageVer = "";
		int posHead=verdate.find("#4 ");
		int posEnd=verdate.find(" arm");
		if(posHead>0 && posEnd>0 && posEnd>posHead)
			pSta->strzimageDate = verdate.substr(posHead+3,posEnd-(posHead+3));;
	}
//sprintf(strmsg,"内核版本:%s\n",pSta->strzimageVer.c_str());
//WriteLog(strmsg);
    printf("内核版本:%s\r\n",pSta->strzimageVer.c_str());
    printf("内核日期:%s\r\n",pSta->strzimageDate.c_str());


	return 0 ;

}


void *LT_Statusthread(void *param)
 {
	infobuf = new char[1024*5+1];

	 string mStrdata;
	 string mstrkey = ""; //没有用户名和密码：则为“”；
	 while(1)
	 {
		 getallstate();
//		 sleep(300);
  	 	 sleep(20);
	 }

	 return 0 ;
 }

void init_lt_state_struct()
{
	VMCONTROL_STATE *pSta=&VMCtl_State; //控制器运行状态结构体
	//初始化
	pSta->TimeStamp=GetTickCount();
	pSta->Linked=false;
	
	pSta->strhostname = "";
	pSta->strcpuRate = "";
	pSta->strcpuTemp = "";
	pSta->strmenTotal = "";
	pSta->strmenUsed = "";
	pSta->strmenRate = "";
	pSta->strzimageVer = "";
	pSta->strzimageDate = "";
	pSta->strCpuAlarm = "";		//CPU使用率报警
	pSta->strCpuTempAlarm = "";		//CPU温度报警
	pSta->strMemAlarm = "";		//内存使用率报警
	pSta->strSoftRate = "";		//主程序CPU占用率
	pSta->strSoftAlarm = "";	//主程序运行异常报警
	pSta->strKsoftirqdRate = "";	//ksoftirqdrateCPU占用率
	pSta->strkworkerRate = "";	//kworkerCPU占用率
	pSta->strLan2BroadcastAlarm = "";	//网口2广播风暴报警
	pSta->strLan2BroadcastAlarmMesg = "";	//网口2广播风暴报警信息
}

void init_lt_status()
{
	init_lt_state_struct();
	pthread_t m_LT_Statusthread ;
	pthread_create(&m_LT_Statusthread,NULL,LT_Statusthread,NULL);
}






