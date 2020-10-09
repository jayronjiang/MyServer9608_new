/*************************************************************************
 * File Name: lock.h
 * Author:
 * Created Time: 2020-09-22
 * Last modify:  
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __LOCK_H_
#define __LOCK_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <pthread.h>
#include <stdint.h>
#include <string>


using namespace std;
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Class
 ******************************************************************************/
class Lock {
public:

    typedef enum {
        Evt_NorOpen = 0xA5,
        Evt_AbnorOpen = 0xA9,
        Evt_NorClose = 0xA6,
        Evt_AbnorClose = 0xA7
    }Event_EN;

    typedef struct{
        bool isOpen;
        bool isAuthorCard;
        uint8_t cardId[4];
        Event_EN event;
    }Info_S;

    /* 回调函数 */
    typedef void (*Callback)(uint8_t addr,Info_S info,void *userdata);

private:
    typedef enum {
        Sgl_Open = 0x05,
        Sgl_Close = 0x06,
        Sgl_Query = 0x16,
        Sgl_ClearCards = 0xE2,
        Sgl_AddCard = 0xE3,
        Sgl_DelCard = 0xEB,
        Sgl_SetAddr = 0xD3,
        Sgl_GetAddr = 0xD4,
        Sgl_InitAddr = 0xD5,
    } Signal_EN;

    uint8_t addr;
    Signal_EN signal;
    Info_S info;

    Callback callback;
    void *userdata;

public:
    Lock(uint8_t addr);
    ~Lock();

    void setCallback(Callback cb,void *userdata);
    void open(void);

private:
    static void UartCallback(uint8_t port, uint8_t *buf, uint32_t len, void *userdata);
    static void *QueryTask(void *arg);

};






#endif


