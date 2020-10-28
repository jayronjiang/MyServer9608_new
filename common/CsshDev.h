#ifndef __CSSHDEV_H__
#define __CSSHDEV_H__

#include <pthread.h>
#include <string>

using namespace std;

#define SSH_DEV_ITS800 (1)
#define SSH_DEV_CPCI (2)

typedef struct  {
    unsigned long TimeStamp; //状态获取时间戳
    bool Linked;             //连接状态 0:断开 1：连接

    string stratlasdata;
} DEV_STATE;

class CsshDev {

public:
    CsshDev(void);
    ~CsshDev(void);

    int SetIntervalTime(int mSetIntervalTime);
    int Start(void);

private:
    pthread_t tid;
    void run(void);
    static void *Sendthread(void *args);
    int getallstateatlas(string mstrip, string mstruser, string mstrkey1, string mstrkey2, int mcount);
    int getcpciallstate(string mstrip, string mstruser, string mstrkey1, string mstrkey2, int mcount);
    int get_atlas_vaule(string &mstr, int mIndex);
    string getsshval(string mstrRead, string mstrkey);
    int init_atlas_struct(void);

public:
    pthread_mutex_t dataMutex;
    pthread_mutex_t IntervalTimeMutex;
    uint8_t devType; // 1:ITS800  2：CPCI
    int mIntervalTime;
    string mStrAtlasIP;
    string mStrAtlasPasswd;
    int CHANNEL_READ_TIMEOUT;
    DEV_STATE stuAtlasState;
};

#endif
