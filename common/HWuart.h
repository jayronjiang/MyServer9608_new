/*************************************************************************
 * File Name: uart.h
 * Author:
 * Created Time: 2020-09-22
 * Last modify:
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __UART_H_
#define __UART_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <pthread.h>
#include <stdint.h>
#include <list>
#include <semaphore.h>

using namespace std;
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define UART_PORT_NUM                               (4)
/*******************************************************************************
 * Class
 ******************************************************************************/
class Uart {
public:
    typedef enum { 
        U0, U1, U2, U3 
    } Port_EN;

    typedef enum {
        BR_9600,
        BR_19200,
        BR_57600,
        BR_115200,
        BR_230400,
        BR_460800,
        BR_921600,
    }BaudRate_EN;

    /* 回调函数 */
    typedef void (*Callback)(uint8_t port, uint8_t *buf, uint16_t len, void *userdata);

private:
    typedef struct {
        Callback cb;
        void *userdata;
    }Callback_S;

    
    int dev;
    Port_EN port;
    pthread_t tid;

    bool isCfg;
    uint8_t stpBit, verBit, dBit;
    uint16_t interval;
    uint32_t baudrate;
    

    list<Callback_S> lstCallback;
    sem_t sem;

public:
    Uart();
    ~Uart(){};
    static Uart *getInstance(Port_EN port);

    void config(uint8_t stpBit, uint8_t verBit, uint8_t dBit);
    void config(BaudRate_EN baudrate);
    void config(uint16_t interval);
    void setCallback(Callback cb, void *userdata);
    void send(uint8_t *buf,uint32_t len);

private:
    void config(void);
    static void *CommunicationTask(void *arg);
};


#endif
