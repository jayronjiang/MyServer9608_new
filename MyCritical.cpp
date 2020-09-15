
#include "MyCritical.h"

CMyCritical::CMyCritical(void)
{
	pthread_mutex_init(&m_cs,NULL);
}

CMyCritical::~CMyCritical(void)
{
	pthread_mutex_destroy (&m_cs);
}

void CMyCritical::Lock()
{
	pthread_mutex_lock(&m_cs);
}

void CMyCritical::UnLock()
{
	pthread_mutex_unlock(&m_cs);
}
