#include "./goahead/inc/goahead.h"

void *WebServerthread(void *param)
{
	if (websOpen("/opt/index", "route.txt") < 0) 
	{
		printf("Can't open the web server runtime\r\n");
		return 0;
	}
	if (websListen(":80") < 0) 
	{
		printf("Can't listen on port 8080\r\n");
		return 0;
	}
	websServiceEvents(NULL); 
	websClose();
	return 0 ;
} 

 void init_WebServer()
 {
	pthread_t m_WebServerthread ;
	pthread_create(&m_WebServerthread,NULL,WebServerthread,NULL);
 }


