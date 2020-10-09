/*************************************************************************
 * File Name: io_dev.h
 * Author:
 * Created Time: 2020-09-30
 * Last modify:  
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef _IO_DEV_H_
#define _IO_DEV_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <stdint.h>
#include <pthread.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Class
 ******************************************************************************/
class IODev {
public:
    typedef enum{
        Dev_SomkeSensor,
        Dev_ImmersionSensor,
        Dev_GateMagnetism,
    }DevType_EN; 

    typedef void (*Callback)(DevType_EN type,bool sta,void *userdata);

private:
    static void *AcquisitionSignalTask(void *arg);

private:
    IODev(){};
    ~IODev(){};

    
    bool smoke,immersion,gateMagnet;
    Callback callback;
    void *userdata;

public:
    static IODev *getInstance(void);

    void setCallback(Callback calllback,void *userdata);

};


#endif


