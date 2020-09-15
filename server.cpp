#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
//#include "comport.h"
#include "server.h"
#include <unistd.h> // fork, close
#include <arpa/inet.h>  // inet_ntoa
#include <pthread.h>
#include <netinet/tcp.h>
#include <linux/rtc.h>
//#include "comserver.h"
#include <string>

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
#include <unistd.h>
#include <string.h>
//#include "comport.h"
//#include "comserver.h"
#include "MyCritical.h"
#include <string>
#include <semaphore.h>
#include "registers.h"
#include "rs485server.h"
//#include "net_spd.h"
//#include "zte.h"
#include "base_64.h"
//#include "gsyjbox.h"
#include "jsonPackage.h"
#include "CabinetClient.h"


using namespace std;//寮??ユ?翠釜??绌洪??

#define PORT 		5000       		// The port which is communicate with server
#define BACKLOG 	10

int fd_A[BACKLOG];    // accepted connection fd
string fd_ClientIP[BACKLOG];    // accepted connection client ip
int conn_amount;      // current connection amount
int data_conamount;

void* NetWork_server_thread(void *param);
void Client_CmdProcess(int fd, char *cmdbuffer,void *arg);
int NetSend(int s,char *pbuffer,int nsize);
void RemoteControl(UINT8* pRCtrl);
extern void WriteLog(char* str);
extern void myprintf(char* str);
int RebootIpCam(int Ipcamindex);
bool SoftwareUpdate(unsigned char *pbuf, int size, string &str);
extern unsigned short GetCrc(unsigned char *buf,int len);
extern int WritepUpdata(unsigned char *pDateBuf,int pDateLen);
extern int WritezImagedata(unsigned char *pDateBuf,int pDateLen);
extern int WriteZipdata(unsigned char *pDateBuf,int pDateLen);

extern REMOTE_CONTROL *stuRemote_Ctrl;	//遥控寄存器结构体
extern CabinetClient *pCabinetClient;//华为机柜状态
extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体

extern string zteLockDevID[4];
extern int openatslock(void);

extern int Writeconfig(void);
extern int Setconfig(string StrKEY,string StrSetconfig);
extern bool jsonstrRCtrlReader(char* jsonstr, int len, UINT8 *pstuRCtrl);
extern bool jsonComputerReader(char* jsonstr, int len);
extern UINT16 SendCom3Test(char *buf,int len);
extern unsigned long GetTickCount();

pthread_mutex_t PostGetMutex ;
pthread_mutex_t litdataMutex ;
extern int litdataTime ;
extern pthread_mutex_t uprebootMutex ;
extern pthread_mutex_t httprebootMutex ;
extern int WDTfd ;
extern int HttpReboot;


void init_TCPServer()
{
	//获取RTC时钟
//	gRTCfd=rtc_init();

	pthread_t tNetwork_server;
	if (pthread_create(&tNetwork_server, NULL, NetWork_server_thread,NULL))
	{
		printf("NetWork server create failed!\n");
	}
	pthread_detach(tNetwork_server);
}

void* NetWork_server_thread(void *param)
{
	param=NULL;
	int result,nlen;
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	int sockfd;                        	// Socket file descriptor
	int newfd;               		// New Socket file descriptor
	int num;
	socklen_t sin_size;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
    int yes = 1;
	char buf[NETPACKET_MAXLEN],*pData;
	int ret;
	int i,j;
	SocketPara sockpara;
	sockpara.IsQuit = 0;
	char * jsonPack=(char *)malloc(JSON_LEN);
	if(jsonPack==NULL)
	{
		myprintf("NetWork_server_thread jsonPack malloc error!\n");
		return 0;
	}
	int jsonPackLen=0;

	/* Get the Socket file descriptor */
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
    	printf ("ERROR: Failed to obtain Socket Despcritor.\n");
    	return (0);
	} else {
    	printf ("OK: Obtain Socket Despcritor sucessfully.\n");
	}

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
    	printf ("ERROR: Failed to setsockopt.\n");
    	return (0);
    }

	/* Fill the local socket address struct */
	server_addr.sin_family = AF_INET;           		// Protocol Family
	server_addr.sin_port = htons (PORT);         		// Port number
	server_addr.sin_addr.s_addr  = htonl (INADDR_ANY);  	// AutoFill local address
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

	/*  Blind a special Port */
	if( bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1 )
	{
	  	printf ("ERROR: Failed to bind Port %d.\n",PORT);
		close(sockfd);
		return (0);
	} else {
    	printf("OK: Bind the Port %d sucessfully.\n",PORT);
	}

	/*  Listen remote connect/calling */
	if(listen(sockfd,BACKLOG) == -1)
	{
    	printf ("ERROR: Failed to listen Port %d.\n", PORT);
		close(sockfd);
		return (0);
	} else {
    	printf ("OK: Listening the Port %d sucessfully.\n", PORT);
	}

	//hym 2019.05.08
	int nSendBuffLen = 1048576;//1M //1000000;
	int nRecvBuffLen = NETPACKET_MAXLEN;
	nlen = 4;
	result = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&nSendBuffLen, nlen);
	result = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&nRecvBuffLen, sizeof(int));

	result = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout,	sizeof(struct timeval));

	int keepAlive = 1;//
	int keepIdle = 5;//
	int keepInterval = 5;//
	int keepCount = 3;//
	if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*) &keepAlive,
			sizeof(keepAlive)) == -1) {
		printf("SO_KEEPALIVE Error!\n");
	}

	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void *) &keepIdle,
			sizeof(keepIdle)) == -1) {
		printf("TCP_KEEPIDLE Error!\n");
	}

	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (void *) &keepInterval,
			sizeof(keepInterval)) == -1) {
		printf("TCP_KEEPINTVL Error!\n");
	}

	if (setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (void *) &keepCount,
			sizeof(keepCount)) == -1) {
		printf("TCP_KEEPCNT Error!\n");
	}
	//hym end

	fd_set fdsr;
	int maxsock;
	struct timeval tv;

	conn_amount = 0;
	sin_size = sizeof(client_addr);
	maxsock = sockfd;
	while (1) {
		FD_ZERO(&fdsr);
		FD_SET(sockfd, &fdsr);

		// timeout setting
		tv.tv_sec = 30;
		tv.tv_usec = 0;

		// add active connection to fd set
		for (i = 0; i < BACKLOG; i++) {
			if (fd_A[i] != 0) {
				FD_SET(fd_A[i], &fdsr);
				if(maxsock< fd_A[i])
					maxsock = fd_A[i];
			}
		}

		ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
		} else if (ret == 0) {

			continue;
		}

		for (i = 0; i < BACKLOG; i++)
		{
            if (0 < fd_A[i] && FD_ISSET(fd_A[i], &fdsr))
			{
				ret = recv(fd_A[i], buf, NETCMD_HEADERLEN, 0);
//				printf("Client_CmdProcessbuff: 000 ret=%d\r\n",ret);
				if (ret <= 0)
				{ // client close
					printf("client[%d] close\n", i);
					close(fd_A[i]);
					FD_CLR(fd_A[i], &fdsr);
					fd_A[i] = 0;
					fd_ClientIP[i]="";		//改重连时相同ip重复连接问题
					conn_amount--;
				}
				else
				{
					if(ret==NETCMD_HEADERLEN)
					{

						NETCMD_HEADER *pheader = (NETCMD_HEADER *) buf;
						if (pheader->magic == NETCMD_MAGIC)
						{
//							printf("Client_CmdProcessbuff: datalen=%d\r\n",pheader->datalen);
							pData=pheader->data;
							if (pheader->datalen > 0)
							{
								do
								{
//									ret += recv(fd_A[i], pheader->data, pheader->datalen, 0);
									ret += recv(fd_A[i], pData, pheader->datalen, 0);
									pData=pheader->data+ret-NETCMD_HEADERLEN;
								}while(ret<NETCMD_HEADERLEN + pheader->datalen);
							}

//							printf("Client_CmdProcessbuff: ret=%d\r\n",ret);
							if (ret == NETCMD_HEADERLEN + pheader->datalen)
							{
								//printf("");
								//("Client_CmdProcessbuff:%s\r\n",buf);
								//printf("cmd=%d,len=%d",pheader->cmd,pheader->datalen);
								Client_CmdProcess(fd_A[i], buf,	(void *) &sockpara);
//								printf("len=%d,data=%s",pheader->datalen,pheader->data );
								NetSend(fd_A[i], buf,pheader->datalen + NETCMD_HEADERLEN);
							}
						}
						else
						{
							//printf("client[%d] not a current client force closed\n",i);
							close(fd_A[i]);
							FD_CLR(fd_A[i], &fdsr);
							fd_A[i] = 0;
							fd_ClientIP[i]="";		//改重连时相同ip重复连接问题
							conn_amount--;

						}
					}

				}
			}
		}
		//printf("sockfd%d1212\r\n",sockfd);

		if (FD_ISSET(sockfd, &fdsr))
		{
			//printf("sockfd*****\r\n");
			newfd = accept(sockfd, (struct sockaddr *) &client_addr,&sin_size);
			//printf("newfd%d\r\n",newfd);
			if (newfd <= 0)
			{
				perror("accept");
				continue;
			}

			// add to fd queue
			//if (conn_amount < BACKLOG)
			{
				//find empty
				for (i = 0; i < BACKLOG; i++)
				{
					if (fd_A[i] == 0)
					{
						fd_A[i] = newfd;
                        conn_amount++;
                        printf("new connection client[%d] %s:%d\n", i, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						fd_ClientIP[i]==inet_ntoa(client_addr.sin_addr);
						break;
					}
				}

				if(i==BACKLOG)
				{
					printf("[%s:%d]not enough array to accept the socket %d\n", __FILE__, __LINE__, newfd);
                    close(newfd);
				}
/*				//有新客户端连接，主动发门架状态信息
				memset(jsonPack,0,JSON_LEN);
                string mstrdata;
                SetjsonFlagRunStatusStr(NETCMD_FLAGRUNSTATUS,mstrdata);
				//printf("%s",jsonPack);
                NetSendParm(NETCMD_FLAGRUNSTATUS,(char *)(mstrdata.c_str()),mstrdata.size());*/

			}
		}
	}
	free(jsonPack);
}

void Client_CmdProcess(int fd, char *cmdbuffer,void *arg)
{
	NETCMD_HEADER *pCMD = (NETCMD_HEADER *) cmdbuffer;

	SocketPara *socketpara = (SocketPara *) arg;
	IPInfo ipinfo;
	VMCONTROL_CONFIG *pConf=&VMCtl_Config;	//控制器配置信息结构体
	string mstrdata;
	//SYSTEMTIME system_time;
	char tmpStringData[100],tmpstr[50];
	struct rtc_time rtc_tm;
	int ret;
	char * jsonPack=(char *)malloc(JSON_LEN);
	if(jsonPack==NULL)
	{
		myprintf("Client_CmdProcess jsonPack malloc error!\n");
		return ;
	}
	int jsonPackLen=0;
	char * pRecvBuf=(char *)malloc(JSON_LEN);
	if(pRecvBuf==NULL)
	{
		myprintf("Client_CmdProcess pRecvBuf malloc error!\n");
		return ;
	}

	unsigned char  regAddr;
	unsigned short regValue;
	int status;
	if(pCMD->cmd!=NETCMD_PING)
	{
		//printf("Client_CmdProcess cmd =%d \r\n",pCMD->cmd);
		sprintf(tmpStringData,"Client_CmdProcess cmd =%d size =%d\r\n",pCMD->cmd,pCMD->datalen);
		myprintf(tmpStringData);
//		WriteLog(tmpStringData);
	}
	switch (pCMD->cmd)
	{
		case NETCMD_CONTROLERID:		//6 保留
			sprintf(tmpStringData,"LTKJ-CONTROLER-V1.0");
			memcpy( (char *) pCMD->data,tmpStringData,strlen(tmpStringData));
			pCMD->datalen = strlen(tmpStringData);

			break;
		case NETCMD_DATETIME: 			//1 设置日期时间
			if(pCMD->status==SFLAG_WRITE)
			{
				memcpy(tmpstr,pCMD->data,pCMD->datalen);
//				sprintf(tmpStringData,"date %s\n",tmpstr);	//A287
				sprintf(tmpStringData,"date -s \"%s\"\n",tmpstr);		//IoT 9100
				printf("setSystemTimer %s",tmpStringData);
				system(tmpStringData);		//设置日期时间
				system("hwclock -w");		//写入硬时钟
				pCMD->datalen =  0;

			}
			break;

		case NETCMD_REBOOT: 			//4 重启设备指令
			pCMD->datalen = 0;
			printf("System Reboot\n");
//			system("reboot -f") ;
			system("echo 1 > /proc/sys/kernel/sysrq") ;
			sleep(2);
			system("echo b > /proc/sysrq-trigger") ;
			break;

		case NETCMD_CONFIG_NETWORK:		//5 读取/设置网口1
			if(pCMD->status==SFLAG_READ)
			{
				//printf("Get IP1 Addr\n");
				SetjsonIPStr(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			else if(pCMD->status==SFLAG_WRITE)
			{
				//printf("NETCMD_CONFIG_NETWORK write param = %s length=%d \n",pCMD->data,pCMD->datalen);
				string iptmp,masktmp,gatewaytmp;
				iptmp=pConf->StrIP;masktmp=pConf->StrMask;gatewaytmp=pConf->StrGateway;//保存原IP设置
				mstrdata=pCMD->data;
				jsonstrIpInfoReader((char*)mstrdata.c_str(),mstrdata.size(),(UINT8*)&ipinfo);//将json字符串转换成结构体*/
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
				if(iptmp!=pConf->StrIP || masktmp!=pConf->StrMask || gatewaytmp!=pConf->StrGateway)	//IP设置更改
					stuRemote_Ctrl->SysReset=SYSRESET; 	//等待重启
			}
			break;
		case NETCMD_CONFIG_NETWORK2: 	//25 读取/设置网口2
			if(pCMD->status==SFLAG_READ)
			{
				//printf("Get IP2 Addr\n");
				SetjsonIP2Str(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			else if(pCMD->status==SFLAG_WRITE)
			{
				//printf("NETCMD_CONFIG_NETWORK2 write param = %s length=%d \n",pCMD->data,pCMD->datalen);
				string iptmp,masktmp,gatewaytmp;
				iptmp=pConf->StrIP2;masktmp=pConf->StrMask2;gatewaytmp=pConf->StrGateway2;//保存原IP设置
				mstrdata=pCMD->data;
				jsonstrIpInfoReader((char*)mstrdata.c_str(),mstrdata.size(),(UINT8*)&ipinfo);//将json字符串转换成结构体
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
				if(iptmp!=pConf->StrIP2 || masktmp!=pConf->StrMask2 || gatewaytmp!=pConf->StrGateway2)	//IP设置更改
					stuRemote_Ctrl->SysReset=SYSRESET;	//等待重启
			}
			break;

		case NETCMD_SEND_ENVI_PARAM: 			//9 环境寄存器参数
			break;

		case NETCMD_SEND_UPS_PARAM: 			//10 UPS参数
			break;

		case NETCMD_SEND_DEV_PARAM: 			//12 控制器参数
			if(pCMD->status==SFLAG_READ)
			{
				jsonStrVMCtlParamWriter(pCMD->cmd,(char*)&VMCtl_Config,mstrdata);
//				jsonStrVMCtlParamWriterXY(pCMD->cmd,(char*)&VMCtl_Config,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			else if(pCMD->status==SFLAG_WRITE)
			{
				mstrdata = pCMD->data;
				jsonstrVmCtlParamReader((char*)mstrdata.c_str(),mstrdata.size(),(UINT8*)&VMCtl_Config);//将json字符串转换成结构体
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
			}
			break;
		case NETCMD_SEND_AIR_PARAM: 			//13 空调参数
			break;
		case NETCMD_SEND_RSU_PARAM: 			//14 RSU天线状态
			if(pCMD->status==SFLAG_READ)
			{
				jsonStrRsuWriterXY(pCMD->cmd,mstrdata);
                memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
		case NETCMD_SEND_VEHPLATE_PARAM: 			//15 车牌识别仪状态
			if(pCMD->status==SFLAG_READ)
			{
				jsonStrVehPlateWriter(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
		case NETCMD_SEND_SWITCH_INFO: 			//16 交换机状态
			if(pCMD->status==SFLAG_READ)
			{
				SetjsonIPSwitchStatusStr(pCMD->cmd,mstrdata);
                memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
		case NETCMD_FLAGRUNSTATUS:			//17 门架运行状态
			break;
		case NETCMD_REMOTE_CONTROL: 			//18 遥控设备
		case NETCMD_HWCABINET_PARMSET: 			//21 华为机柜参数设置
			if(pCMD->status==SFLAG_WRITE)
			{
				char *strtmp=(char*)malloc(pCMD->datalen+1);
				memcpy(strtmp,pCMD->data,pCMD->datalen);
				strtmp[pCMD->datalen]='\0';
//				WriteLog(strtmp);	//记录日志
				REMOTE_CONTROL *pRCtrl=stuRemote_Ctrl;
				jsonstrRCtrlReader(strtmp,pCMD->datalen+1,(UINT8 *)pRCtrl);//将json字符串转换成结构体
				RemoteControl((UINT8*)pRCtrl);
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
				free(strtmp);
			}
			break;
		case NETCMD_SWITCH_STATUS: 			//19 回路开关状态
			if(pCMD->status==SFLAG_READ)
			{
				jsonStrSwitchStatusWriter(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
			
		case NETCMD_HWCABINET_STATUS: 			//20  华为机柜状态
			if(pCMD->status==SFLAG_READ)
			{
				jsonStrHWCabinetWriter(pCMD->cmd,(char*)pCabinetClient,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
		case NETCMD_DEAL_LOCKER:			//22  门禁开关锁请求
			if(pCMD->status==SFLAG_WRITE)
			{
				char *strtmp=(char*)malloc(pCMD->datalen+1);
				memcpy(strtmp,pCMD->data,pCMD->datalen);
				strtmp[pCMD->datalen]='\0';
//				WriteLog(strtmp);	//记录日志

				REMOTE_CONTROL *pRCtrl=stuRemote_Ctrl;
				memset(pRCtrl,0,sizeof(REMOTE_CONTROL));
//				jsonstrRCtrlReader(pRecvBuf,pCMD->datalen,(UINT8 *)pRCtrl);//将json字符串转换成结构体
				jsonstrRCtrlReader(strtmp,pCMD->datalen+1,(UINT8 *)pRCtrl);//将json字符串转换成结构体
				RemoteControl((UINT8*)pRCtrl);
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
				free(strtmp);
			}
			break;
		case NETCMD_SEND_FIREWALL_INFO:			//23 防火墙状态
			if(pCMD->status==SFLAG_READ)
			{
				SetjsonFireWallStatusStr(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
		case NETCMD_SEND_ATLAS_INFO: 		//24 ATLAS状态
			if(pCMD->status==SFLAG_READ)
			{
				SetjsonAtlasStatusStr(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
		case NETCMD_SOFTWARE_UPDATE:		//26 软件升级
			if(pCMD->status==SFLAG_WRITE)
			{
//printf("NETCMD_SOFTWARE_UPDATE len=%d\n",pCMD->datalen);
				unsigned char *strtmp=(unsigned char*)malloc(pCMD->datalen);
				memcpy(strtmp,pCMD->data,pCMD->datalen);
				SoftwareUpdate(strtmp,pCMD->datalen,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
				free(strtmp);
			}
			break;
		case NETCMD_SEND_SPD_AI_PARAM:		//27 防雷器参数
			if(pCMD->status==SFLAG_READ)
			{
				SetjsonSpdAIStatusStr(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			else if(pCMD->status==SFLAG_WRITE)
			{
				char *strtmp=(char*)malloc(pCMD->datalen+1);
				memcpy(strtmp,pCMD->data,pCMD->datalen);
				strtmp[pCMD->datalen]='\0';
//				WriteLog(strtmp);	//记录日志

				REMOTE_CONTROL *pRCtrl=stuRemote_Ctrl;
				memset(pRCtrl,0,sizeof(REMOTE_CONTROL));
				jsonstrSPDReader(strtmp,pCMD->datalen+1,(UINT8 *)pRCtrl);//将json字符串转换成结构体
				RemoteControl((UINT8*)pRCtrl);
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
				free(strtmp);
			}
			break;
		case NETCMD_SEND_SPD_RES_PARAM:		//28 接地电阻参数
			if(pCMD->status==SFLAG_READ)
			{
				SetjsonSpdResStatusStr(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
		case NETCMD_SEND_VEHPLATE900_PARAM: 			//29 300万全景车牌识别参数
			if(pCMD->status==SFLAG_READ)
			{
				jsonStrVehPlate900Writer(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
		case NETCMD_SEND_VMCTRL_STATE: 			//30 控制器运行状态
			if(pCMD->status==SFLAG_READ)
			{
				jsonStrVMCtrlStateWriter(pCMD->cmd,mstrdata);
				memcpy((char *) pCMD->data,(char *)(mstrdata.c_str()),mstrdata.size());
				pCMD->datalen = mstrdata.size();
			}
			break;
/*		case NETCMD_TEST_NEST: 		//200 测试嵌套
			if(pCMD->status==SFLAG_READ)
			{
				GetConfig(&vmctrl_param);
				memset(jsonPack,0,JSON_LEN);
				jsonPackLen=0;
				jsonStrVMCtlParamWriterXY(pCMD->cmd,(char*)&vmctrl_param,jsonPack,&jsonPackLen);
				memcpy((char *) pCMD->data,jsonPack,jsonPackLen);
				pCMD->datalen = jsonPackLen;
			}
			break;
		case NETCMD_TEST_485:			//100 测试485
			if(pCMD->status==SFLAG_WRITE)
			{
				testFlag |= LBIT(RS485_1_T);
				//SendCom3Test(pCMD->data,pCMD->datalen);
				printf("485_TEST_BEGIN\n");
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
			}
			break;
		case NETCMD_TEST_232_1: 		//101 测试232-1
			if(pCMD->status==SFLAG_WRITE)
			{
				testFlag |= LBIT(RS232_2T);
				printf("232-1_TEST_BEGIN\n");
				//SendCom3Test(pCMD->data,pCMD->datalen);
				//printf("NETCMD_TEST_485 %s\n",pCMD->data);
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
			}
			break;
		case NETCMD_TEST_232_2: 		//101 测试232-1
			if(pCMD->status==SFLAG_WRITE)
			{
				testFlag |= LBIT(RS232_1T);
				printf("232-2_TEST_BEGIN\n");
				//SendCom3Test(pCMD->data,pCMD->datalen);
				//printf("NETCMD_TEST_485 %s\n",pCMD->data);
				sprintf(pCMD->data,"执行命令->执行成功!\n");
				pCMD->datalen = strlen(pCMD->data);
			}
			break;*/
		default:
			break;

	}
	free(jsonPack);
	free(pRecvBuf);
 }

 int NetSend(int s,char *pbuffer,int nsize)
 {
   int nSendLen = 0;
   int nLen = 0;
   while(nSendLen < nsize) {
	 // printf("nsize :%d\r\n",nsize);
	 nLen = send(s, pbuffer+nSendLen, nsize - nSendLen, MSG_NOSIGNAL);
	 //printf("nLen :%d\r\n",nLen);
	 if(nLen<=0)
	   break;
	 nSendLen += nLen;
   }
   return nSendLen;
 }


 int  MySendMessage(char *pBuf)
 {
	 int nsendlen;
	 int i,nlen;
	 char done = 0;

	 NETCMD_HEADER netcmd;
	 netcmd.cmd 	= NETCMD_SEND_DATA;
	 netcmd.magic	= 0x12345678;
	 netcmd.status	= SFLAG_READ;
	 netcmd.datalen = strlen(pBuf);
//	 netcmd.data    = pBuf;


	 for (i = 0; i < BACKLOG; i++) {
		 if (fd_A[i] > 0) {
			 nlen = NetSend(fd_A[i], (char *) &netcmd,NETCMD_HEADERLEN);

		 	 nsendlen=strlen(pBuf);
			 if (nsendlen > 0) {
				 nlen = NetSend(fd_A[i], (char *) pBuf, nsendlen);
				 if (nlen != nsendlen) {
					 printf("send data error, avisock=%d\n", fd_A[i]);
					 return -1;
				 }else
					 done = 1;
			 }
		 }
//		 else
//			 printf("soket %d is unable=%d\n", i,fd_A[i]);

	 }

	 return 0;
 }

  int  NetSendParm(NETCMD_TYPE cmd,char *pBuf,int len)
  {
	  int nsendlen;
	  int i,nlen;
	  char done = 0;

	  NETCMD_HEADER netcmd;
	  netcmd.cmd	 = cmd;
	  netcmd.magic	 = 0x12345678;
	  netcmd.status  = SFLAG_READ;
	  netcmd.datalen = len;


	  for (i = 0; i < BACKLOG; i++) {
		  if (fd_A[i] > 0) {
			  nlen = NetSend(fd_A[i], (char *) &netcmd,NETCMD_HEADERLEN);

			  nsendlen=len;
			  if (nsendlen > 0) {
				  nlen = NetSend(fd_A[i], (char *) pBuf, nsendlen);
				  if (nlen != nsendlen) {
					  printf("send data error, avisock=%d\n", fd_A[i]);
					  return -1;
				  }else
					  done = 1;
			  }
		  }
 // 	  else
 // 		  printf("soket %d is unable=%d\n", i,fd_A[i]);

	  }

	  return 0;
  }

 int Net_close()
 {
	 int i;
	 fd_set fdsr;

	 for (i = 0; i < BACKLOG; i++) {
		 if (fd_A[i] != 0)
		 {
			 close(fd_A[i]);
			 FD_CLR(fd_A[i], &fdsr);
			 fd_A[i] = 0;
		 }
	 }
	 return 0;
 }

 void RemoteControl(UINT8* pRCtrl)
 {
	  int i,j;
	  REMOTE_CONTROL *pstuRCtrl=(REMOTE_CONTROL *)pRCtrl;
	  THUAWEIGantry *pHWDev=&pCabinetClient->HUAWEIDevValue;//华为机柜状态
	  int seq = 0;
	  UINT16 seq_temp = 0;
	  char value[10];
//	  FDATA dummy;
	  UINT8 temp = 0;
	  UINT8 byteSend[BASE64_HEX_LEN]={0x00,};
	  string mStrUser = "admin";
	  string mStrkey = "admin";
printf("000\n");
#if 0	  
	   for (i = 0; i < SWITCH_COUNT; i++)
	   {
  //	   printf("do_seqx=0x%02x\r\n",pstuRCtrl->doseq[i]);
		   if(pstuRCtrl->doseq[i]==ACT_CLOSE)		   //do分闸
		   {
			   seq_temp = DoSeq[i];
			   printf("DoSeq=0x%02x\r\n",DoSeq[i]);
			   //如果未被配置,则不控制
			   if (seq_temp != NULL_VAR)
			   {
				   seq_temp = i%SINGLE_REMOTE_NUM;
				   seq = i/SINGLE_REMOTE_NUM;
				   printf("seq_temp0x%02x=0x%02x,seq=%d\r\n",i,seq_temp,seq);
				   power_ctrl_flag[seq] |= LBIT(POWER_1_CTRL_CLOSE+2*seq_temp);
				   // 不能同时置位2同样的位，同时要开或者关
				   power_ctrl_flag[seq] &= ~(LBIT(POWER_1_CTRL_OPEN+2*seq_temp));
				   printf("power_ctrl_flagseq=0x%04x\r\n",power_ctrl_flag[seq]);
			   }
		   }
		   if(pstuRCtrl->doseq[i]==ACT_OPEN)		   //DO合闸
		   {
			   seq_temp = DoSeq[i];
			   printf("DoSeq=0x%02x\r\n",DoSeq[i]);
			   //如果未被配置,则不控制
			   if (seq_temp != NULL_VAR)
			   {
				   seq_temp = i%SINGLE_REMOTE_NUM;
				   seq = i/SINGLE_REMOTE_NUM;
				   printf("seq_temp0x%02x=0x%02x,seq=%d\r\n",i,seq_temp,seq);
				   power_ctrl_flag[seq] |= LBIT(POWER_1_CTRL_OPEN+2*seq_temp);
				   power_ctrl_flag[seq] &= ~(LBIT(POWER_1_CTRL_CLOSE+2*seq_temp));
				   printf("power_ctrl_flagseq=0x%04x\r\n",power_ctrl_flag[seq]);
			   }
		   }
	   }
#endif
	  string strtmp=pstuRCtrl->systemtime;
	  if(strtmp!="")					  //设置日期时间
	  {
	  	  string strcmd;
		  strcmd="date -s \""+strtmp+"\"\n";
		  printf("setSystemTimer %s",strcmd.c_str());		//IoT 9100
		  system(strcmd.c_str());	  //设置日期时间
		  system("hwclock -w"); 	  //写入硬时钟
	  }
#if 0	  
	 if(pstuRCtrl->FrontDoorCtrl==ACT_UNLOCK)					 //开锁
	 {
	 	printf("FrontDoorCtrl ACT_UNLOCK");
		 // 如果是华为机柜
	#if (CABINETTYPE == 1)
		 //if (CabinetTypeGet() <= CABIN_HUAWEI_1_1)
		 {
			locker_ctrl_flag |= LBIT(LOCKER_1_CTRL_UNLOCK);
		 }
		 //else if (CabinetTypeGet == CABIN_ZTE)
	#elif ((CABINETTYPE == 5)  || (CABINETTYPE == 6) )
		 {
		 	if (zteLockDevID[0] != "")
		 	{
				memset(byteSend,0,BASE64_HEX_LEN);
				// 开锁
			   	zte_jsa_locker_process(0,DOOR_OPEN_CMD,byteSend,mStrUser,mStrkey);
		 	}
		 }
	#elif (CABINETTYPE == 7)
		openatslock();
	#elif (CABINETTYPE == 9)
		GsyjLockerContrl_Func(DEV_DOOR_IC,true);
	#endif
		 usleep(2000);
	 }
	 if(pstuRCtrl->FrontDoorCtrl==ACT_LOCK) 				 //关锁
	 {
		 printf("FrontDoorCtrl ACT_LOCK");
		 // 如果是华为机柜
		 #if (CABINETTYPE == 1)
		 {
		 	locker_ctrl_flag |= LBIT(LOCKER_1_CTRL_LOCK);
		 }
		 #elif ((CABINETTYPE == 5)  || (CABINETTYPE == 6) )
		 {
		 	if (zteLockDevID[0] != "")
		 	{
				memset(byteSend,0,BASE64_HEX_LEN);
				// 开锁
			   	zte_jsa_locker_process(0,DOOR_CLOSE_CMD,byteSend,mStrUser,mStrkey);
		 	}
		 }
		 #elif (CABINETTYPE == 9)
			GsyjLockerContrl_Func(DEV_DOOR_IC,false);
		 #endif
		 usleep(2000);
	 }
	 if(pstuRCtrl->BackDoorCtrl==ACT_UNLOCK)				 //开锁
	 {
	 	printf("BackDoorCtrl ACT_UNLOCK");
		 // 如果是华为机柜
		 #if(CABINETTYPE == 1) //华为
		 {
		 	locker_ctrl_flag |= LBIT(LOCKER_2_CTRL_UNLOCK);
		 }
		 //else if (CabinetTypeGet == CABIN_ZTE)
		 #elif ((CABINETTYPE == 5)  || (CABINETTYPE == 6) )
		 {
		 	if (zteLockDevID[1] != "")
		 	{
				memset(byteSend,0,BASE64_HEX_LEN);
				// 开锁
			   	zte_jsa_locker_process(1,DOOR_OPEN_CMD,byteSend,mStrUser,mStrkey);
		 	}
		 }
		 #elif (CABINETTYPE == 7)
			 openatslock();
		 #endif
		 usleep(2000);
	 }
	 if(pstuRCtrl->BackDoorCtrl==ACT_LOCK)					 //关锁
	 {
		 printf("BackDoorCtrl ACT_LOCK") ;
		 // 如果是华为机柜
		 #if (CABINETTYPE == 1)
		 {
		 	locker_ctrl_flag |= LBIT(LOCKER_2_CTRL_LOCK);
		 }
		 #elif ((CABINETTYPE == 5)  || (CABINETTYPE == 6) )
		 {
		 	if (zteLockDevID[1] != "")
		 	{
				memset(byteSend,0,BASE64_HEX_LEN);
			   	zte_jsa_locker_process(1,DOOR_CLOSE_CMD,byteSend,mStrUser,mStrkey);
		 	}
		 }
		 #endif
		 usleep(2000);
	 }
	 if(pstuRCtrl->SideDoorCtrl==ACT_UNLOCK)				 //开锁
	 {
	 	printf("SideDoorCtrl ACT_UNLOCK");
	     //CABINETTYPE  1：华为（包括华为单门 双门等） 5：中兴; 6：金晟安; 7：爱特斯 StrVersionNo
   	#if(CABINETTYPE == 1) //华为
		 // 如果是华为机柜
		 {
		 	locker_ctrl_flag |= LBIT(LOCKER_3_CTRL_UNLOCK);
		 }
	#elif ((CABINETTYPE == 5)  || (CABINETTYPE == 6) )
		 //else if (CabinetTypeGet == CABIN_ZTE)
		// else
		 {
		 	if (zteLockDevID[2] != "")
		 	{
				memset(byteSend,0,BASE64_HEX_LEN);
				// 开锁
			   	zte_jsa_locker_process(2,DOOR_OPEN_CMD,byteSend,mStrUser,mStrkey);
		 	}
		 }
	#elif (CABINETTYPE == 9)
		GsyjLockerContrl_Func(DEV_DOOR_IC_POWER,true);
	#endif
		 usleep(2000);
	 }
	 if(pstuRCtrl->SideDoorCtrl==ACT_LOCK)					 //关锁
	 {
 		 printf("SideDoorCtrl ACT_LOCK");
		 // 如果是华为机柜
		 #if (CABINETTYPE == 1)
		 {
		 	locker_ctrl_flag |= LBIT(LOCKER_3_CTRL_LOCK);
		 }
		 #elif ((CABINETTYPE == 5)  || (CABINETTYPE == 6) )
		 {
		 	if (zteLockDevID[2] != "")
		 	{
				memset(byteSend,0,BASE64_HEX_LEN);
			   	zte_jsa_locker_process(2,DOOR_CLOSE_CMD,byteSend,mStrUser,mStrkey);
		 	}
		 }
		 #elif (CABINETTYPE == 9)
			GsyjLockerContrl_Func(DEV_DOOR_IC_POWER,false);
		 #endif
		 usleep(2000);
	 }
	 if(pstuRCtrl->RightSideDoorCtrl==ACT_UNLOCK)				 //开锁
	 {
	 	printf("SideDoorCtrl ACT_UNLOCK");
		 // 如果是华为机柜
		 #if (CABINETTYPE == 1)
		 {
		 	locker_ctrl_flag |= LBIT(LOCKER_4_CTRL_UNLOCK);
		 }
		 #elif ((CABINETTYPE == 5)  || (CABINETTYPE == 6) )
		 {
		 	if (zteLockDevID[3] != "")
		 	{
				memset(byteSend,0,BASE64_HEX_LEN);
				// 开锁
			   	zte_jsa_locker_process(3,DOOR_OPEN_CMD,byteSend,mStrUser,mStrkey);
		 	}
		 }
		 #endif
		 usleep(2000);
	 }
	 if(pstuRCtrl->RightSideDoorCtrl==ACT_LOCK)					 //关锁
	 {
 		 printf("SideDoorCtrl ACT_LOCK");
		  // 如果是华为机柜
		 #if (CABINETTYPE == 1)
		 {
		 	locker_ctrl_flag |= LBIT(LOCKER_4_CTRL_LOCK);
		 }
		 #elif ((CABINETTYPE == 5)  || (CABINETTYPE == 6) )
		 {
		 	if (zteLockDevID[3] != "")
		 	{
				memset(byteSend,0,BASE64_HEX_LEN);
				// 开锁
			   	zte_jsa_locker_process(3,DOOR_CLOSE_CMD,byteSend,mStrUser,mStrkey);
		 	}
		 }
		 #endif
		 usleep(2000);
	 }
	 if(pHWDev->hwLinked && GetTickCount()-pHWDev->hwTimeStamp>5*60) //超过5分钟没更新，认为没有连接)
	 	pHWDev->hwLinked=false;
 	 //控制单板复位 0：保持；1：热复位；
	 if(pHWDev->hwLinked && pstuRCtrl->hwctrlmonequipreset!=ACT_HOLD)
	 {
		 sprintf(value,"%d",pstuRCtrl->hwctrlmonequipreset);
		 printf("RemoteControl 控制单板复位=%s\n",value);
		 SnmpSetOid(hwCtrlMonEquipReset,value,1);
	 }
 	 //AC过压点设置 0:保持；50-600（有效）；280（缺省值）
	 if(pHWDev->hwLinked && pstuRCtrl->hwsetacsuppervoltlimit!=ACT_HOLD)
	 {
		 sprintf(value,"%d",pstuRCtrl->hwsetacsuppervoltlimit);
		 printf("RemoteControl AC过压点设置=%s\n",value);
		 SnmpSetOid(hwSetAcsUpperVoltLimit,value,1);
	 }
	 //AC欠压点设置 0:保持；50-600（有效）；180（缺省值）
	 if(pHWDev->hwLinked && pstuRCtrl->hwsetacslowervoltlimit!=ACT_HOLD)
	 {
		 sprintf(value,"%d",pstuRCtrl->hwsetacslowervoltlimit);
		 printf("RemoteControl AC欠压点设置=%s\n",value);
		 SnmpSetOid(hwSetAcsLowerVoltLimit,value,1);
	 }
	 //设置DC过压点 0:保持；53-600（有效）；58（缺省值）
	 if(pHWDev->hwLinked && pstuRCtrl->hwsetdcsuppervoltlimit!=ACT_HOLD)
	 {
		 sprintf(value,"%d",pstuRCtrl->hwsetdcsuppervoltlimit*10);
		 printf("RemoteControl 设置DC过压点=%s\n",value);
		 SnmpSetOid(hwSetDcsUpperVoltLimit,value,1);
	 }
	 //设置DC欠压点 0:保持；35 - 57（有效）；45（缺省值）
	 if(pHWDev->hwLinked && pstuRCtrl->hwsetdcslowervoltlimit!=ACT_HOLD)
	 {
		 sprintf(value,"%d",pstuRCtrl->hwsetdcslowervoltlimit*10);
		 printf("RemoteControl 设置DC欠压点=%s\n",value);
		 SnmpSetOid(hwSetDcsLowerVoltLimit,value,1);
	 }
	 //环境温度告警上限 0:保持；25-80（有效）；55（缺省值）
	 for(i=0;i<2;i++)
	 {
		 if(pHWDev->hwLinked && pstuRCtrl->hwsetenvtempupperlimit[i]!=ACT_HOLD)
		 {
			 sprintf(value,"%d",pstuRCtrl->hwsetenvtempupperlimit[i]);
			 printf("RemoteControl 环境温度告警上限%d=%s\n",i,value);
			 SnmpSetOid(hwSetEnvTempUpperLimit,value,i+1);
		 }
	 }
	 //环境温度告警下限255:保持；-20-20（有效）；-20（缺省值）
	 for(i=0;i<2;i++)
	 {
		 if(pHWDev->hwLinked && pstuRCtrl->hwsetenvtemplowerlimit[i]!=ACT_HOLD_FF)
		 {
			 sprintf(value,"%d",pstuRCtrl->hwsetenvtemplowerlimit[i]);
			 printf("RemoteControl 环境温度告警下限%d=%s\n",i,value);
			 SnmpSetOid(hwSetEnvTempLowerLimit,value,i+1);
		 }
	 }
	 //环境湿度告警上限 255:保持；0-100（有效）；95（缺省值）
	 for(i=0;i<2;i++)
	 {
		 if(pHWDev->hwLinked && pstuRCtrl->hwsetenvhumidityupperlimit[i]!=ACT_HOLD_FF)
		 {
			 sprintf(value,"%d",pstuRCtrl->hwsetenvhumidityupperlimit[i]);
			 printf("RemoteControl 环境湿度告警上限%d=%s\n",i,value);
			 SnmpSetOid(hwSetEnvHumidityUpperLimit,value,i+1);
		 }
	 }
	 //环境湿度告警下限 255:保持；0-100（有效）；5（缺省值）
	 for(i=0;i<2;i++)
	 {
		 if(pHWDev->hwLinked && pstuRCtrl->hwsetenvhumiditylowerlimit[i]!=ACT_HOLD_FF)
		 {
			 sprintf(value,"%d",pstuRCtrl->hwsetenvhumiditylowerlimit[i]);
			 printf("RemoteControl 环境湿度告警下限%d=%s\n",i,value);
			 SnmpSetOid(hwSetEnvHumidityLowerLimit,value,i+1);
		 }
	 }
	 //温控模式 	 0：保持；1：纯风扇模式；2：纯空调模式；3：智能模式；
     if(pHWDev->hwLinked && pstuRCtrl->hwcoolingdevicesmode!=ACT_HOLD)
	 {
         sprintf(value,"%d",pstuRCtrl->hwcoolingdevicesmode);
         printf("RemoteControl 温控模式=%s\n",value);
         SnmpSetOid(hwCoolingDevicesMode,value,1);
	 }
	 //空调开机温度点		 255:保持； -20-80（有效）；45(缺省值)
	 for(i=0;i<2;i++)
	 {
	     if(pHWDev->hwLinked && pstuRCtrl->hwdcairpowerontemppoint[i]!=ACT_HOLD_FF)
		 {
	         sprintf(value,"%d",pstuRCtrl->hwdcairpowerontemppoint[i]);
	         printf("RemoteControl 空调开机温度点%d=%s\n",i,value);
	         SnmpSetOid(hwDcAirPowerOnTempPoint,value,i+1);
		 }
	 }
	 //空调关机温度点		   255:保持； -20-80（有效）；37(缺省值)
	 for(i=0;i<2;i++)
	 {
	     if(pHWDev->hwLinked && pstuRCtrl->hwdcairpowerofftemppoint[i]!=ACT_HOLD_FF)
		 {
	         sprintf(value,"%d",pstuRCtrl->hwdcairpowerofftemppoint[i]);
	         printf("RemoteControl 空调关机温度点%d=%s\n",i,value);
	         SnmpSetOid(hwDcAirPowerOffTempPoint,value,i+1);
		 }
	 }
	 //空调控制模式 0：保持；1：自动；2：手动
	 for(i=0;i<2;i++)
	 {
		 if(pHWDev->hwLinked && pstuRCtrl->hwdcairctrlmode[i]!=ACT_HOLD)
		 {
			 sprintf(value,"%d",pstuRCtrl->hwdcairctrlmode[i]);
			 printf("RemoteControl 空调控制模式%d=%s\n",i,value);
			 SnmpSetOid(hwDcAirCtrlMode,value,i+1);
		 }
	 }
	 //控制烟感复位 0：保持；1：不需复位；2：复位
	 for(i=0;i<2;i++)
	 {
		 if(pHWDev->hwLinked && pstuRCtrl->hwctrlsmokereset[i]!=ACT_HOLD)
		 {
			 sprintf(value,"%d",pstuRCtrl->hwctrlsmokereset[i]);
			 printf("RemoteControl 控制烟感复位%d=%s\n",i,value);
			 SnmpSetOid(hwCtrlSmokeReset,value,i+1);
		 }
	 }
	 //SPD控制
	 for(i=0;i<SPD_NUM;i++)
	 {
		 if(pstuRCtrl->DO_spdcnt_clear[i]!=ACT_HOLD)
		 {
			 sprintf(value,"%d",pstuRCtrl->DO_spdcnt_clear[i]);
			 printf("RemoteControl SPD%d雷击计数清零=%s\n",i+1,value);
			 //雷击计数清零
			 if (SPD_Type == TYPE_LEIXUN)
			 {
				//if (i == 0)
				{
					Ex_SPD_Set_Process(i,SPD_DO_SET,DO_ADDR_CNT_CLR,dummy,DO_ON_CMD);
				}
			 }
			 else if ((SPD_Type == TYPE_KY) ||(SPD_Type == TYPE_KY0M))
			 {
				Ex_SPD_Set_Process(i,SPD_DO_SET,KY_CLEAR_ADDR,dummy,DO_ON_CMD_KY);	// 这里是1:清除记录
			 }
		 }
		 if(pstuRCtrl->DO_totalspdcnt_clear[i]!=ACT_HOLD)
		 {
			 sprintf(value,"%d",pstuRCtrl->DO_totalspdcnt_clear[i]);
			 printf("RemoteControl SPD%d总雷击计数清零=%s\n",i+1,value);
			 //总雷击计数清零
			 if (SPD_Type == TYPE_LEIXUN)
			 {
				//if (i == 0)
				{
					Ex_SPD_Set_Process(i,SPD_DO_SET,DO_ADDR_TOTAL_CLR,dummy,DO_ON_CMD);
				}
			 }
		 }
		 if(pstuRCtrl->DO_psdtime_clear[i]!=ACT_HOLD)
		 {
			 sprintf(value,"%d",pstuRCtrl->DO_psdtime_clear[i]);
			 printf("RemoteControl SPD%d雷击时间清零=%s\n",i+1,value);
			 //雷击时间清零
			 if (SPD_Type == TYPE_LEIXUN)
			 {
				//if (i == 0)
				{
					Ex_SPD_Set_Process(i,SPD_DO_SET,DO_ADDR_STRUCK_TIME_CLR,dummy,DO_ON_CMD);
				}
			 }
		 }
		 if(pstuRCtrl->DO_daytime_clear[i]!=ACT_HOLD)
		 {
			 sprintf(value,"%d",pstuRCtrl->DO_daytime_clear[i]);
			 printf("RemoteControl SPD%d在线时间清零=%s\n",i+1,value);
			 //在线时间清零
			 if (SPD_Type == TYPE_LEIXUN)
			 {
				//if (i == 0)
				{
					Ex_SPD_Set_Process(i,SPD_DO_SET,DO_ADDR_WORK_TIME_CLR,dummy,DO_ON_CMD);
				}
			 }
		 }
		 if(pstuRCtrl->DO_leak_type[i]!=ACT_HOLD)
		 {
			 sprintf(value,"%d",pstuRCtrl->DO_leak_type[i]);
			 printf("RemoteControl SPD%d外接漏电流控制=%s\n",i+1,value);//0:保持，1:内置漏电流，2：外接漏电流
			 //外接漏电流控制	 注意:设置时 0:内置漏电流，1：外接漏电流
			 if (SPD_Type == TYPE_LEIXUN)
			 {
				//if (i == 0)
				{
					// 把1,2转换成0,1
					temp = ((pstuRCtrl->DO_leak_type[i]-1) == 0)?0:1;
					Ex_SPD_Set_Process(i,SPD_DO_SET,DO_ADDR_LEAK_SET,dummy,temp);
				}
			 }
		 }
		 if(pstuRCtrl->spdleak_alarm_threshold[i]!=0.000 && pstuRCtrl->spdleak_alarm_threshold[i]!=stuSpd_Param->rSPD_data[i].leak_alarm_threshold) //漏电流报警阈值改变
		 {
			 sprintf(value,"%0.3f",pstuRCtrl->spdleak_alarm_threshold[i]);
			 printf("RemoteControl SPD%d漏电流报警阈值=%s\n",i+1,value);//漏电流报警阈值
			 //漏电流报警阈值
			 if (SPD_Type == TYPE_LEIXUN)
			 {
				//if (i == 0)
				{
					dummy.f = pstuRCtrl->spdleak_alarm_threshold[i];
					Ex_SPD_Set_Process(i,SPD_AI_SET,AI_LEAK_THRESHOLD_ADDR,dummy,0);
				}
			 }
		 }
	 }
#endif
 }

 bool SoftwareUpdate(unsigned char *pbuf, int size, string &strjsonback)
 {
 #if 0
 	 strjsonback="";
	 if(size>4 && (pbuf[0] == 0x12) && (pbuf[1] == 0x34) && (pbuf[2] == 0x56) && (pbuf[3] == 0x78)) 
	 {
		unsigned int getlen ;
		memcpy(&getlen,pbuf+4,4);
		if(getlen == size)
		{
		   unsigned short int IntCRC;
		   unsigned short int getCrc = GetCrc(pbuf,getlen-2) ;
		   memcpy(&IntCRC,pbuf+getlen-2,2);
		   if(IntCRC == getCrc)
		   {
			  pthread_mutex_lock(&uprebootMutex);
			  write(WDTfd, "\0", 1);
	 
			  system("mv tranter tranter1") ;
			  printf("start updata\r\n");
			  if(WritepUpdata(pbuf+8,getlen-16) != 0)
			  {
				 printf("rm tranter\r\n");
				 system("rm tranter") ;
				 system("sync") ;
				 sleep(2);
//				 system("reboot -f") ;
				 system("echo 1 > /proc/sys/kernel/sysrq") ;
				 sleep(2);
				 system("echo b > /proc/sysrq-trigger") ;
			  }
	 
			  //get version
			  char verbuf[100];
			  string StrNewVersionNo = "";
			  memset(verbuf,0,100);
			  sprintf(verbuf,"%d%d.%d%d.%d%d",*(pbuf+getlen-8)-'0',*(pbuf+getlen-7)-'0',*(pbuf+getlen-6)-'0',*(pbuf+getlen-5)-'0',*(pbuf+getlen-4)-'0',*(pbuf+getlen-3)-'0');
			  StrNewVersionNo = (char *)verbuf ;
	 
				strjsonback = strjsonback + "{\n";
				strjsonback = strjsonback + "\"result\":\"升级成功！\",\n";
				strjsonback = strjsonback + "\"new version\":\""+ StrNewVersionNo +"\",\n";
				strjsonback = strjsonback + "\"old version\":\""+ StrVersionNo +"\",\n";
				strjsonback = strjsonback + "\"dec\":\"升级利通控制器成功！请不要关闭电源，10秒后设备自动重启！\"\n";
				strjsonback = strjsonback + "}\n";
				printf("strjsonback=%s\r\n",strjsonback.c_str());
	 
			  //sleep(2);
			  system("chmod 777 tranter") ;
			  printf("chmod 777 tranter\r\n");
			  sleep(1);
			  pthread_mutex_lock(&httprebootMutex);
			  HttpReboot = 1;
			  pthread_mutex_unlock(&httprebootMutex);
			  return 1 ;
	 
			  pthread_mutex_unlock(&uprebootMutex);
	 
		   }
		   else
		   {
			   printf("updata CRC erro %d %d\r\n",IntCRC,getCrc) ;
		   }
	 
		}
	 }
	 else if(size>4 && (pbuf[0] == 0x11) && (pbuf[1] == 0x22) && (pbuf[2] == 0x33) && (pbuf[3] == 0x55)) 
	 {
		unsigned int getlen ;
		memcpy(&getlen,pbuf+4,4);
		if(getlen == size)
		{
		   unsigned short int IntCRC;
		   unsigned short int getCrc = GetCrc(pbuf,getlen-2) ;
		   memcpy(&IntCRC,pbuf+getlen-2,2);
		   if(IntCRC == getCrc)
		   {
			  pthread_mutex_lock(&uprebootMutex);
			  write(WDTfd, "\0", 1);
	 
			  printf("start updata zImage\r\n");
			  if(WritezImagedata(pbuf+8,getlen-16) != 0)
			  {
				 printf("rm zImage\r\n");
				 system("rm zImage") ;
				 system("sync") ;
				 sleep(1);
//				 system("reboot -f") ;
				 system("echo 1 > /proc/sys/kernel/sysrq") ;
				 sleep(2);
				 system("echo b > /proc/sysrq-trigger") ;
			  }
			  sleep(1);
			  printf("systemupdate start\r\n");
			  system("chmod 777 /opt/systemupdate") ;
			  system("/opt/systemupdate") ;
			  printf("systemupdate end\r\n");

				strjsonback = strjsonback + "{\n";
				strjsonback = strjsonback + "\"result\":\"zImage updata success\",\n";
				strjsonback = strjsonback + "\"dec\":\"Updata LTKJ Controller success! Don't turn off the power in 10 seconds! device is auto restart! Thank you!\"\n";
				strjsonback = strjsonback + "}\n";
	 
			  //sleep(3);
			  pthread_mutex_lock(&httprebootMutex);
			  HttpReboot = 1;
			  pthread_mutex_unlock(&httprebootMutex);
			  return 1 ;
	 
			  pthread_mutex_unlock(&uprebootMutex);
	 
		   }
		   else
		   {
			   printf("updata CRC erro %d %d\r\n",IntCRC,getCrc) ;
		   }
	 
		}
	 }
	 else if(size>6 && pbuf[0] == 0x50 && pbuf[1] == 0x4b && pbuf[2] == 0x03 && pbuf[3] == 0x04 && pbuf[4] == 0x14 && pbuf[5] == 0x00) 
	 {
		printf("start updata\r\n");
		pthread_mutex_lock(&uprebootMutex);
		write(WDTfd, "\0", 1);

		if(WriteZipdata(pbuf,size) == 0)	//写文件成功
		{
			system("mv tranter tranter1") ;
			printf("unzip -o upgrade.zip\r\n");
			system("unzip -o upgrade.zip") ;
			system("sync") ;

			sleep(2);
			if(access("9100.sh",0)==0)
			{
				printf("9100.sh 存在，运行9100.sh\r\n");
				system("chmod 777 9100.sh") ;
				system("chmod 777 tranter") ;
				system("./9100.sh") ;
			}
			else
			{
				printf("9100.sh 不存在，自动重启\r\n");
				system("chmod 777 tranter") ;
			}
			sleep(1);

			strjsonback = strjsonback + "{\n";
			strjsonback = strjsonback + "\"result\":\"升级成功！\",\n";
			strjsonback = strjsonback + "\"dec\":\"升级利通控制器成功！请不要关闭电源，10秒后设备自动重启！\"\n";
			strjsonback = strjsonback + "}\n";
			printf("strjsonback=%s\r\n",strjsonback.c_str());
			pthread_mutex_lock(&httprebootMutex);
			HttpReboot = 1;
			pthread_mutex_unlock(&httprebootMutex);
		}
		else								//写文件失败
		{
			strjsonback = strjsonback + "{\n";
			strjsonback = strjsonback + "\"result\":\"升级失败！\",\n";
			strjsonback = strjsonback + "\"dec\":\"文件解压失败！\"\n";
			strjsonback = strjsonback + "}\n";
			printf("strjsonback=%s\r\n",strjsonback.c_str());
		}
		return 1 ;

		pthread_mutex_unlock(&uprebootMutex);
	 }
	 else
	 {
		 strjsonback = strjsonback + "{\n";
		 strjsonback = strjsonback + "\"result\":\"升级失败！\",\n";
		 strjsonback = strjsonback + "\"dec\":\"文件格式错误！\"\n";
		 strjsonback = strjsonback + "}\n";
		 printf("strjsonback=%s\r\n",strjsonback.c_str());
	 }
#endif	 
	 return 0;
 }

 void DealDoReset(REMOTE_CONTROL *pstuRCtrl)
 {
	 int i,j;
	 string StrDev;
	 char dev[30];
	 int seq = 0;
	 UINT16 seq_temp = 0;

#if 0
	 for (i = 0; i < SWITCH_COUNT; i++)
	 {
		 if(pstuRCtrl->doseq[i]==ACT_HARDWARERST)	 //断电重启
		 {
			 seq_temp = DoSeq[i];
			 printf("DoSeq=0x%02x\r\n",DoSeq[i]);
			 //如果未被配置,则不控制
			 if (seq_temp != NULL_VAR)
			 {
				 seq_temp = i%SINGLE_REMOTE_NUM;
				 seq = i/SINGLE_REMOTE_NUM;
				 printf("seq_temp0x%02x=0x%02x,seq=%d\r\n",i,seq_temp,seq);
				 power_ctrl_flag[seq] |= LBIT(POWER_1_CTRL_CLOSE+2*seq_temp);
				 power_ctrl_flag[seq] &= ~(LBIT(POWER_1_CTRL_OPEN+2*seq_temp));
				 printf("power_ctrl_flagseq=0x%04x\r\n",power_ctrl_flag[seq]);
				 sleep(5);
				 printf("seq_temp0x%02x=0x%02x,seq=%d\r\n",i,seq_temp,seq);
				 power_ctrl_flag[seq] |= LBIT(POWER_1_CTRL_OPEN+2*seq_temp);
				 power_ctrl_flag[seq] &= ~(LBIT(POWER_1_CTRL_CLOSE+2*seq_temp));
				 printf("power_ctrl_flagseq=0x%04x\r\n",power_ctrl_flag[seq]);
			 }
			 pstuRCtrl->doseq[i]=ACT_HOLD;
		 }
		 if(pstuRCtrl->doseq[i]==ACT_SOFTWARERST)	 //软件重启
		 {
		 	 //摄像机重启
			 for(j=0;j<atoi(StrVehPlateCount.c_str());j++)
			 {
				 sprintf(dev,"vehplate%d_do",j+1);
				 StrDev=dev;
				 if(StrDeviceNameSeq[i]==StrDev)
				 {
					 printf("do%d  reboot %s\n",i,dev);
					 RebootIpCam(j);
				 }
			 }
			 //RSU控制器重启
			 for(j=0;j<atoi(StrRSUCount.c_str());j++)
			 {
				 sprintf(dev,"rsu%d_do",j+1);
				 StrDev=dev;
				 if(StrDeviceNameSeq[i]==StrDev)
				 {
					 printf("do%d  reset %s\n",i,dev);
					 //重启RSU 0x1D
					 send_RSU(j,0x1D,false,0,0);
				 }
			 }
			 pstuRCtrl->doseq[i]=ACT_HOLD;
		 }
	 }
	 //antenna重启
	 for(i=0;i<ANTENNA_NUM;i++)
	 {
		 if(pstuRCtrl->antenna[i] ==ACT_SOFTWARERST)	//软件重启
		 {
			 printf("antenna%d  reset\n",i+1);
			 //重启天线0xC4
			 send_RSU(0,0x1D,false,0,i+1);
			 pstuRCtrl->antenna[i] = ACT_HOLD;
		 }
	 }
#endif
 }

  void *LTKJ_DataPostthread(void *param)
  {
	  string mStrdata;
	  string mstrkey = ""; //没有用户名和密码：则为“”；
	  int ret = 0 ;
	  while(1)
	  {
/*		 if(StrServerURL1.length()>0)
		 {
			 mStrdata = "";
			 //原20 /(新部标准2.3)机柜状态数据
			 SetjsonCabinetStatus("CABINETINFOUPLOAD", mStrdata);
			 //printf("LTKJ_DataPostthread CABINETINFOUPLOAD=\n%s\n",mStrdata.c_str());
			 if(StrServerURL1!="")
			 {
                ret = HttpPostParm(StrServerURL1,mStrdata,mstrkey,HTTPPOST);
				 if(ret == 1)
				 {
					 pthread_mutex_lock(&litdataMutex);
					 litdataTime = 0;
					 pthread_mutex_unlock(&litdataMutex);
				 }
			 }
			 mStrdata = "";
			 //原17/(新部标准2.1) 门架关键设备状态数据
			 SetjsongantryRunStatus("gantryRunStatus", mStrdata);
			 //printf("LTKJ_DataPostthread gantryRunStatus=\n%s\n",mStrdata.c_str());
             ret = HttpPostParm(StrServerURL1+"batch",mStrdata,mstrkey,HTTPPOST);
			 if(ret == 1)
			 {
				 pthread_mutex_lock(&litdataMutex);
				 litdataTime = 0;
				 pthread_mutex_unlock(&litdataMutex);
			 }

		 }*/
	 	 sleep(60*3);
 //		 sleep(10);
	  }

	  return 0 ;
  }

 void init_LTKJ_DataPost()
 {
	 pthread_mutex_init(&litdataMutex,NULL);
	 pthread_t m_LTKJ_DataPostthread ;
	 pthread_create(&m_LTKJ_DataPostthread,NULL,LTKJ_DataPostthread,NULL);
 }

 void *XY_DataPostthread(void *param)
 {
#if 0
	 string mStrdata;
	 string mstrkey = ""; //没有用户名和密码：则为“”；
	 while(1)
	 {
		 //SetjsonFlagRunStatusStr(NETCMD_FLAGRUNSTATUS,mStrdata);
		 if(StrServerURL2.length()>0)
             HttpPostParm(StrServerURL2,mStrdata,mstrkey,HTTPPOST);

		 sleep(300);
 // 	 sleep(1);
	 }
#endif
	 return 0 ;
 }

 void init_XY_DataPost()
 {
	 pthread_t m_XY_DataPostthread ;
	 pthread_create(&m_XY_DataPostthread,NULL,XY_DataPostthread,NULL);
 }

 void *SocketNetSendthread(void *param)
 {

	 string mStrdata;
	 while(1)
	 {
		 //memset(jsonPack,0,JSON_LEN);
		 //SetjsonFlagRunStatusStr(NETCMD_FLAGRUNSTATUS,mStrdata);
		 //NetSendParm(NETCMD_FLAGRUNSTATUS,(char *)(mStrdata.c_str()),mStrdata.size());

		 sleep(10);
 // 	 sleep(1);
	 }

	 return 0 ;
 }

 void init_SocketNetSend()
 {
	 pthread_t m_SoketNetSendthread ;
	 pthread_create(&m_SoketNetSendthread,NULL,SocketNetSendthread,NULL);
 }


 void *DealDoResetthread(void *param)
 {

	 string mStrdata;
	 while(1)
	 {
		 DealDoReset(stuRemote_Ctrl);							 //处理DO重启

		 sleep(1);
	 }

	 return 0 ;
 }

 void *DealSysResetthread(void *param)
 {

	 string mStrdata;
	 while(1)
	 {
		 if(stuRemote_Ctrl->SysReset==SYSRESET) 				 //系统重启
		 {
			 printf("DealSysResetthread reboot\n");
//			 system("reboot -f") ;
			 system("echo 1 > /proc/sys/kernel/sysrq") ;
			 sleep(2);
			 system("echo b > /proc/sysrq-trigger") ;
		 }
		 sleep(1);
	 }

	 return 0 ;
 }

 void init_DealDoReset()
 {
	 pthread_t m_DealDoResetthread ;
	 pthread_create(&m_DealDoResetthread,NULL,DealDoResetthread,NULL);

	 pthread_t m_DealSysResetthread ;
	 pthread_create(&m_DealSysResetthread,NULL,DealSysResetthread,NULL);
 }


