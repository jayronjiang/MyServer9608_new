#ifndef __MyCritical_h__
#define __MyCritical_h__

#include <pthread.h>

class CMyCritical
{
public:
	CMyCritical(void);
	~CMyCritical(void);
	void Lock();
	void UnLock();
private:
        pthread_mutex_t m_cs ;
	
};


#endif
