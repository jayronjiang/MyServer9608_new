#ifndef __CSSHCLIENT_H__
#define __CSSHCLIENT_H__

#include <string> 
#include <pthread.h>


using namespace std; 

typedef struct atlas_state_struct
{
	unsigned long TimeStamp; 		//状态获取时间戳
	bool Linked;					//连接状态 0:断开 1：连接

	string stratlasdata;
}ATLAS_STATE;


class CsshClient
{
public:
       CsshClient(void);
       ~CsshClient(void);
        
       int SetIntervalTime(int mSetIntervalTime);
       int Start(void);

private: 
       pthread_t tid; 
       void run(void);
       static void *Sendthread(void *args);
	   int getallstateatlas(string mstrip,string mstruser,string mstrkey1,string mstrkey2,int mcount);
	   int getyhallstate(string mstrip,string mstruser,string mstrkey1,string mstrkey2,int mcount);
       int get_atlas_vaule(string &mstr,int mIndex);
       string getsshval(string mstrRead,string mstrkey);
       
     
public:
       pthread_mutex_t dataMutex ;
       pthread_mutex_t IntervalTimeMutex ;
       int mIntervalTime ;  
	   int init_atlas_struct(void);
	   string mStrAtlasIP ;//AtlasIP
       string mStrAtlasPasswd ;//Atlas密码
	   string mStrAtlasType; 
	   int CHANNEL_READ_TIMEOUT ;
	   ATLAS_STATE stuAtlasState; //Atlas状态
//	   string mStrAtlasType ;//工控机类型 "1":Atlas;"2":研华工控机
};








#endif



