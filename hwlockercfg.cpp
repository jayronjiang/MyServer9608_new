
#include "hw_locker.h"
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include "debug.h"


using namespace std;

#define CARD_NUM		5	// 暂时为5张卡

//extern void Locker_Stream_To_Net(uint16_t addr,uint32_t card_read);

const uint32_t locker_id[CARD_NUM] =
{
	2035400,
	13838538,
	1547012,
	10863352,
	2857740885u
};


// 锁的回调函数，上传卡号
void LockCallback(uint8_t seq, Lock::Info_S info, void *userdata) 
{
	Lock *lock = Locks[seq];
    DEBUG_PRINTF("Lock(%d) isOpen:%02X Card{%08x} \r\n", seq, info.status, info.card_read);

	// 测试是否能打开
	for (int j=0;j<CARD_NUM;j++)
	{
		if (info.card_read == locker_id[j])
		{
			lock->signal = Lock::Sgl_Open;
			break;
		}
	}
	//Locker_Stream_To_Net(lock.addr,info.card_read);
}


void Init_HwLocker(uint16_t seq,uint16_t address)
{
	if (seq < LOCKER_MAX_NUM)
	{
	    Locks[seq] =  new Lock(address);
		Locks[seq]->setCallback(LockCallback,NULL);
	}
}


