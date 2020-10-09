/*************************************************************************
 * File Name: tem_humi.h
 * Author:
 * Created Time: 2020-09-23
 * Last modify:  
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef _TEM_HUMI_H_
#define _TEM_HUMI_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <pthread.h>
#include <stdint.h>


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Class
 ******************************************************************************/
class TemHumi {
public:
    typedef struct {
        float tempture;
        float humidity;
        float dew;
    }Info_S;

    /* 回调函数 */
    typedef void (*Callback)(uint8_t addr,Info_S info,void *userdata);

private:
    uint8_t addr;
    Info_S info;

    Callback callback;
    void *userdata;

    static void UartCallback(uint8_t port, uint8_t *buf, uint32_t len, void *userdata);
    static void *QueryTask(void *arg);
    
public:
    TemHumi(uint8_t addr);
    ~TemHumi();

    void setCallback(Callback cb,void *userdata);

};





#endif


