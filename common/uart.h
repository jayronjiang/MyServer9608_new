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
#include <list>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

using namespace std;
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define UART_PORT_NUM (4)
/*******************************************************************************
 * Class
 ******************************************************************************/
class Uart {
public:
    typedef enum { U0, U1, U2, U3 } Port_EN;

    typedef enum {
        BR_9600,
        BR_19200,
        BR_57600,
        BR_115200,
        BR_230400,
        BR_460800,
        BR_921600,
    } BaudRate_EN;

    typedef enum {
        /* 半双工 */
        Type_HalfDuplex,
        /* 全双工 */
        Type_FullDuplex,
    }TransType_EN;

    /* 回调函数 */
    typedef void (*Callback)(uint8_t port, uint8_t *buf, uint32_t len, void *userdata);

private:
    typedef struct {
        Callback cb;
        void *userdata;
    } Callback_S;

    int dev;
    Port_EN port;
    pthread_t stid,rtid;
    int pifd[2];

    uint8_t stpBit, verBit, dBit;
    uint32_t baudrate;
    TransType_EN transType;
    uint16_t sendIntervalms;

    list<Callback_S> lstCallback;
    sem_t semSend,semRecv;

    Uart();
    ~Uart(){};

public:
    static Uart *getInstance(Port_EN port);

    void config(BaudRate_EN baudrate, uint8_t stpBit, uint8_t verBit, uint8_t dBit);
    void setCallback(Callback cb, void *userdata);
    void setTransmitType(TransType_EN type,uint16_t intervalMs);
    void send(uint8_t *buf, uint32_t len);

private:
    void config(void);
    static void *ReceiveTask(void *arg);
    static void *SendTask(void *arg);
};

#endif
