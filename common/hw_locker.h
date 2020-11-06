/*************************************************************************
 * File Name: lock.h
 * Author:
 * Created Time: 2020-09-22
 * Last modify:  
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __HW_LOCK_H_
#define __HW_LOCK_H_

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
#define LOCKER_MAX_NUM						4		// 最大支持4把锁
	
	
#define	MBUS_ADDR							0       //字节0：从站地址
#define	MBUS_FUNC							1       //字节1：功能码
#define	MBUS_REFS_ADDR_H 	                2       //字节2：寄存器起始地址高位
#define	MBUS_REFS_ADDR_L					3       //字节3：寄存器起始地址低位
#define	MBUS_REFS_COUNT_H 	                4       //字节4：寄存器数量高位
#define	MBUS_REFS_COUNT_L 	                5       //字节5：寄存器数量低位
#define	MBUS_OPT_CODE_H 	                4       //字节4：操作码高位
#define	MBUS_OPT_CODE_L 	                5       //字节5：操作码低位
	
	
/*REMOTE CONTROL definition*/
#define	SINGLE_WRITE_HW		0x06	// function of the LOCKER
	
/*功能码*/
#define	MBUS_READ_CMD				0x03           //读寄存器

/*******************************************************************************
 * Class
 ******************************************************************************/
class HWLock {
public:

    typedef enum {
        Evt_NorClose = 0x00,	// 闭合
        Evt_CmdOpen = 0x01,		// 命令开锁
        Evt_nullOpen = 0x02,	// 控制量开锁，未使用
        Evt_AbnorClose = 0x03	// 钥匙开锁
    }Event_EN;

	typedef enum {
		Sgl_Query = 0x01,
        Sgl_Open = 0x02,
        Sgl_Close = 0x03,
    } Signal_EN;

    typedef struct{
        uint16_t status;
        uint16_t reason;		// 开锁原因
        uint16_t upload_cnt;	// 上报计数
		uint16_t ID_len;		// ID长度
        uint8_t cardId[16];		// ID号，最长16个字节
        uint32_t card_read;		// 前4个字节ID号
        uint16_t last_cnt;
    }Info_S;

    /* 回调函数 */
    typedef void (*Callback)(uint8_t addr,Info_S info,void *userdata);
	Signal_EN signal;
	Info_S info;
	bool linked;

private:
    uint8_t addr;
	uint8_t fail_cnt;

    Callback callback;
    void *userdata;

public:
    HWLock(uint8_t addr);
    ~HWLock();

    void setCallback(Callback cb,void *userdata);
    void open(void);
	void close(void);

private:
    static void UartCallback(uint8_t port, uint8_t *buf, uint32_t len, void *userdata);
    static void *QueryTask(void *arg);

    void transmit(Signal_EN sgl,uint8_t addr,uint8_t *card);

};

#endif


