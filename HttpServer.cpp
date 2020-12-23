#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <evhttp.h>
#include <event.h>
#include "event2/http.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/bufferevent_compat.h"
#include "event2/http_struct.h"
#include "event2/http_compat.h"
#include "event2/util.h"
#include "event2/listener.h"
#include <pthread.h>
#include "http.h"
#include "tea.h"
#include "http.h"
#include <unistd.h> 
#include "registers.h"

#define BUF_MAX  100*1024
#define JSON_LEN 100*1024


using namespace std;

char *messagebuf;
char *jsonPackbuf;

pthread_mutex_t httprebootMutex ;
pthread_mutex_t uprebootMutex ;
int HttpReboot = 0;

extern bool jsonStrReader(char* jsonstrin, int lenin, char* jsonstrout, int *lenout);
extern int WDTfd ;
extern VMCONTROL_CONFIG VMCtl_Config;	//控制器配置信息结构体



int WritepUpdata(unsigned char *pDateBuf,int pDateLen)
{
    FILE* fdd ;
    fdd	= fopen("/opt/tranter", "wb");
    if(fdd == NULL)
    {
       return 1 ;
    }

    //int ret = fwrite(pDateBuf,pDateLen, 1, fdd);
    int ret = fwrite(pDateBuf,1, pDateLen, fdd);
    fflush(fdd);
    sleep(2);
    fclose(fdd);
    printf("WritepUpdata tranter len=%d,ret=%d\r\n",pDateLen,ret);
	system("sync") ;
	
    fdd	= fopen("/opt/tranter", "rb");
    if(fdd == NULL)
    {
       return 1 ;
    }
	fseek(fdd,0,SEEK_END);
	int len=ftell(fdd);
    printf("read tranter len=%d\r\n",len);
    fclose(fdd);
    if(ret == pDateLen && len == pDateLen)
       return 0 ;
    else
       return 1 ;

}


int WriteZipdata(unsigned char *pDateBuf,int pDateLen)
{
	if(access("/tmp/autorun.sh",0)==0)
	{
		printf("/tmp/autorun.sh 存在，删除 /tmp/autorun.sh\r\n");
		system("rm /tmp/autorun.sh") ;
	}
	if(access("/tmp/upgrade.zip",0)==0)
	{
		printf("/tmp/upgrade.zip 存在，删除 /tmp/upgrade.zip\r\n");
		system("rm /tmp/upgrade.zip") ;
	}
	FILE* fdd ;
	fdd = fopen("/tmp/upgrade.zip", "wb");
	if(fdd == NULL)
	{
	   return 1 ;
	}
	
	int ret = fwrite(pDateBuf,1, pDateLen, fdd);
	fflush(fdd);
	sleep(2);
	fclose(fdd);
	printf("WriteZipdata tranter len=%d,ret=%d\r\n",pDateBuf,ret);
	system("sync") ;
    if(ret == pDateLen)
       return 0 ;
    else
       return 1 ;

}


int WritezImagedata(unsigned char *pDateBuf,int pDateLen)
{
    FILE* fdd ;
    fdd	= fopen("/opt/zImage", "wb");
    if(fdd == NULL)
    {
       return 1 ;
    }

    //int ret = fwrite(pDateBuf,pDateLen, 1, fdd);
    int ret = fwrite(pDateBuf,1, pDateLen, fdd);
    fflush(fdd);
    sleep(2);
    fclose(fdd);
    printf("write zImage len=%d,ret=%d\r\n",pDateLen,ret);
    if(ret == pDateLen)
       return 0 ;
    else
       return 1 ;

}



//解析post请求数据
int get_post_message(char *buf, struct evhttp_request *req)
{
    size_t post_size = 0;

    post_size = evbuffer_get_length(req->input_buffer);//获取数据长度
    if (post_size <= 0)
    {
        printf("post msg is empty!\n");
        return 0;
    }
    else
    {
        size_t copy_len = post_size > BUF_MAX ? BUF_MAX : post_size;
        printf("post len:%d, copy_len:%d\n",post_size,copy_len);
        //updata
        unsigned char *pbuf = (unsigned char *)(evbuffer_pullup(req->input_buffer,-1));
        if((pbuf[0] == 0x12) && (pbuf[1] == 0x34) && (pbuf[2] == 0x56) && (pbuf[3] == 0x78)) 
        {
           unsigned int getlen ;
           memcpy(&getlen,pbuf+4,4);
           if(getlen == post_size)
           {
              unsigned short int IntCRC;
              unsigned short int getCrc = GetCrc(pbuf,getlen-2) ;
              memcpy(&IntCRC,pbuf+getlen-2,2);
              if(IntCRC == getCrc)
              {
                 pthread_mutex_lock(&uprebootMutex);
                 write(WDTfd, "\0", 1);

                 system("mv tranter tranter1") ;
                 printf("start updata\r\n");
                 if(WritepUpdata(pbuf+8,getlen-16) != 0)
                 {
                    printf("rm tranter\r\n");
                    system("rm tranter") ;
                    system("sync") ;
                    sleep(2);
//                    system("reboot -f") ;
					system("echo 1 > /proc/sys/kernel/sysrq") ;
					sleep(2);
					system("echo b > /proc/sysrq-trigger") ;
                 }

                 //get version
                 char verbuf[100];
                 string StrNewVersionNo = "";
                 memset(verbuf,0,100);
                 sprintf(verbuf,"%d%d.%d%d.%d%d",*(pbuf+getlen-8)-'0',*(pbuf+getlen-7)-'0',*(pbuf+getlen-6)-'0',*(pbuf+getlen-5)-'0',*(pbuf+getlen-4)-'0',*(pbuf+getlen-3)-'0');
                 StrNewVersionNo = (char *)verbuf ;

                 struct evbuffer *retbuff = NULL;
                 retbuff = evbuffer_new();
                 if(retbuff == NULL)
                 {
                   printf("retbuff is null.");
                   //system("reboot") ;
                   //return;
                 }
                 else
                 {
                   string strjsonback = "";
                   strjsonback = strjsonback + "{\n";
                   strjsonback = strjsonback + "\"result\":\"tranterdata updata success\",\n";
                   strjsonback = strjsonback + "\"new version\":\""+ StrNewVersionNo +"\",\n";
                   strjsonback = strjsonback + "\"old version\":\""+ VMCtl_Config.StrVersionNo +"\",\n";
                   strjsonback = strjsonback + "\"dec\":\"Updata LTKJ Controller success! Don't turn off the power in 10 seconds! device is auto restart! Thank you!\"\n";
                   strjsonback = strjsonback + "}\n";
                   evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type", "application/json");
                   evbuffer_add_printf(retbuff,strjsonback.c_str());
                   evhttp_send_reply(req,HTTP_OK,"Client",retbuff);
                   printf("strjsonback=%s\r\n",strjsonback.c_str());
                 }

                 //sleep(2);
                 system("chmod 777 tranter") ;
                 printf("chmod 777 tranter\r\n");
                 sleep(1);
                 pthread_mutex_lock(&httprebootMutex);
                 HttpReboot = 1;
                 pthread_mutex_unlock(&httprebootMutex);
                 return 1 ;

                 pthread_mutex_unlock(&uprebootMutex);

              }
              else
              {
                  printf("updata CRC erro %d %d\r\n",IntCRC,getCrc) ;
              }

           }
        }
        else if((pbuf[0] == 0x11) && (pbuf[1] == 0x22) && (pbuf[2] == 0x33) && (pbuf[3] == 0x55)) 
        {
           unsigned int getlen ;
           memcpy(&getlen,pbuf+4,4);
           if(getlen == post_size)
           {
              unsigned short int IntCRC;
              unsigned short int getCrc = GetCrc(pbuf,getlen-2) ;
              memcpy(&IntCRC,pbuf+getlen-2,2);
              if(IntCRC == getCrc)
              {
                 pthread_mutex_lock(&uprebootMutex);
                 write(WDTfd, "\0", 1);

                 printf("start updata zImage\r\n");
                 if(WritezImagedata(pbuf+8,getlen-16) != 0)
                 {
                    printf("rm zImage\r\n");
                    system("rm zImage") ;
                    system("sync") ;
                    sleep(1);
//                    system("reboot -f") ;
					system("echo 1 > /proc/sys/kernel/sysrq") ;
					sleep(2);
					system("echo b > /proc/sysrq-trigger") ;
                 }
                 sleep(1);
                 printf("systemupdate start\r\n");
                 system("chmod 777 /opt/systemupdate") ;
                 system("/opt/systemupdate") ;
                 printf("systemupdate end\r\n");
                 struct evbuffer *retbuff = NULL;
                 retbuff = evbuffer_new();
                 if(retbuff == NULL)
                 {
                   printf("retbuff is null.");
                   //system("reboot") ;
                   //return;
                 }
                 else
                 {
                   string strjsonback = "";
                   strjsonback = strjsonback + "{\n";
                   strjsonback = strjsonback + "\"result\":\"zImage updata success\",\n";
                   strjsonback = strjsonback + "\"dec\":\"Updata LTKJ Controller success! Don't turn off the power in 10 seconds! device is auto restart! Thank you!\"\n";
                   strjsonback = strjsonback + "}\n";

                   evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type", "application/json");
                   evbuffer_add_printf(retbuff,strjsonback.c_str());
                   evhttp_send_reply(req,HTTP_OK,"Client",retbuff);
                 }

                 //sleep(3);
                 pthread_mutex_lock(&httprebootMutex);
                 HttpReboot = 1;
                 pthread_mutex_unlock(&httprebootMutex);
                 return 1 ;

                 pthread_mutex_unlock(&uprebootMutex);

              }
              else
              {
                  printf("updata CRC erro %d %d\r\n",IntCRC,getCrc) ;
              }

           }
        }




        memcpy(buf, evbuffer_pullup(req->input_buffer,-1), copy_len);
        buf[post_size] = '\0';
//        printf("post msg:%s\n",buf);
    }

    return 0 ;
}


//解析http头，主要用于get请求时解析uri和请求参数
char *find_http_header(struct evhttp_request *req, struct evkeyvalq *params, const char *query_char)
{
    
	if(req == NULL || params == NULL || query_char == NULL)
	{
		//printf("====line:%d,%s\n",__LINE__,"input params is null.");
		return NULL;
	}
	
	struct evhttp_uri *decoded = NULL;
	char *query = NULL; 
	char *query_result = NULL;
	const char *path;
	const char *uri = evhttp_request_get_uri(req);//获取请求uri
	
	if(uri == NULL)
	{
		//printf("====line:%d,evhttp_request_get_uri return null\n",__LINE__);
		return NULL;
	}
	else
	{
		;//printf("====line:%d,Got a GET request for <%s>\n",__LINE__,uri);
	}
	
	//解码uri
	decoded = evhttp_uri_parse(uri);
	if (!decoded) 
	{
		//printf("====line:%d,It's not a good URI. Sending BADREQUEST\n",__LINE__);
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return NULL;
	}
	
	//获取uri中的path部分
	path = evhttp_uri_get_path(decoded);
	if (path == NULL) 
	{
		path = "/";
	}
	else
	{
		;//printf("====line:%d,path is:%s\n",__LINE__,path);
	}
	
	//获取uri中的参数部分
	query = (char*)evhttp_uri_get_query(decoded);
	if(query == NULL)
	{
		//printf("====line:%d,evhttp_uri_get_query return null\n",__LINE__);
		return NULL;
	}
	
	//查询指定参数的值
	evhttp_parse_query_str(query, params);			
	query_result = (char*)evhttp_find_header(params, query_char);
	
	return query_result;
}


void http_handler_post_msg(struct evhttp_request *req,void *arg)
{
    int jsonPackLen;
//printf("http_handler_post_msg \r\n");	
    if(req == NULL)
    {
        printf("====line:%d,%s\n",__LINE__,"input param req is null.");
        return;
    }

    char *token = NULL;
    struct evkeyvalq token_params = { 0 };
    token = find_http_header(req, &token_params, "token");//获取get请求uri中的token参数
    if (token == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "request uri no param token.\n");
    }
    else
    {
        ;//printf("====line:%d,get request param: token=[%s]\n", __LINE__, token);
    }

    memset(messagebuf,0,BUF_MAX);
    int getret = get_post_message(messagebuf, req);
    if(messagebuf == NULL)
    {
        printf("====line:%d,%s\n",__LINE__,"get_post_message return null.");
        return;
    }
    else
    {
        printf("====line:%d,request data:%s,len: %d\n",__LINE__,messagebuf,strlen(messagebuf));
    }

    if(getret == 1)
      return ;

	//解析发来的JSON请求，打包请求数据
	memset(jsonPackbuf,0,JSON_LEN);
	jsonPackLen=0;
	jsonStrReader(messagebuf,strlen(messagebuf),jsonPackbuf,&jsonPackLen);
	//printf("jsonPack len:%d out:%s\n",jsonPackLen,jsonPackbuf);
	
    //回响\E5\BA?    
    struct evbuffer *retbuff = NULL;
    retbuff = evbuffer_new();
    if(retbuff == NULL)
    {
        printf("retbuff is null.");
        return;
    }
//    evbuffer_add_printf(retbuff,"Receive post request,Thanks for the request!");
    evbuffer_add_printf(retbuff,jsonPackbuf);
    evhttp_add_header(evhttp_request_get_output_headers(req),"tocken", "mjkzq2019");
	evhttp_add_header(evhttp_request_get_output_headers(req),"Access-Control-Allow-Origin","*"); 
    evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type", "application/json");
	evhttp_send_reply(req,HTTP_OK,"Client",retbuff);
    evbuffer_free(retbuff);

}





void *HttpServerhread(void *param)
{
    struct evhttp *http_server = NULL;
    short http_port = 8081;
    char *http_addr = "0.0.0.0";

    event_init();
    http_server = evhttp_start(http_addr,http_port);
    if(http_server == NULL)
    {
        printf("http server start failed.");
        return 0;
    }

    //设置请求超时时间(s)
    evhttp_set_timeout(http_server,15);
    evhttp_set_cb(http_server,"/Device/RemoteCtl",http_handler_post_msg,NULL);


    event_dispatch();
    evhttp_free(http_server);

    return 0;
}


void *HttpReboothread(void *param)
{
  while(1)
  {
     pthread_mutex_lock(&httprebootMutex);             
     if(HttpReboot == 1)
     {
        pthread_mutex_unlock(&httprebootMutex);
		system("sync") ;
        printf("3s affter reboot\r\n");
        sleep(3);
//        system("reboot -f") ;
		system("echo 1 > /proc/sys/kernel/sysrq") ;
		sleep(2);
		system("echo b > /proc/sysrq-trigger") ;
     }
     else
     {
        pthread_mutex_unlock(&httprebootMutex);
        sleep(1);
     }

  }
  
}

int Init_HttpServer(void)
{
   messagebuf = new char[BUF_MAX+10];
   jsonPackbuf = new char[JSON_LEN+10];
   memset(messagebuf,0,BUF_MAX+10);
   memset(jsonPackbuf,0,JSON_LEN+10);

   pthread_mutex_init(&uprebootMutex,NULL);
   pthread_mutex_init(&httprebootMutex,NULL);

   pthread_t m_HttpReboothread ;
   pthread_create(&m_HttpReboothread,NULL,HttpReboothread,NULL);

   pthread_t m_HttpServerhread ;
   pthread_create(&m_HttpServerhread,NULL,HttpServerhread,NULL);


   return 0 ;
}



