#ifndef __MONINTERFACE_H_
#define __MONINTERFACE_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <string>
#include <pthread.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ETH_ADAPTER_LIST_SIZE                         (8)
/*******************************************************************************
 * Class
 ******************************************************************************/
class Moninterface
{
public:
    typedef struct
    {
        std::string name;
        std::string ip;
        std::string mask;
    }EthAdapter_S;
    
    typedef struct{
        std::string cpuUsageRate;
        std::string memTotalSize;
        std::string memFreeSize;
        std::string memUsage;
        std::string memUsageRate;
        std::string diskTotalSize;
        std::string diskFreeSize;
        std::string diskUsage;
        std::string diskUsageRate;

        uint8_t ethAdapterNum;
        EthAdapter_S ethAdapters[ETH_ADAPTER_LIST_SIZE];
    }State_S;

    typedef void (*Callback)(State_S state,void *userdata);
    
public:
    Moninterface(std::string ip);
    ~Moninterface(){};

    State_S getState(void);
    void setCallback(Callback callback,void *userdata);

private:
    pthread_t tid;
    std::string ip;
    State_S state;

    Callback callback;
    void *userdata;

    static void *CommunicationTask(void *arg);
    
    
};



#endif

