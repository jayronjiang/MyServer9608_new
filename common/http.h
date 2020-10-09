/*************************************************************************
 * File Name: http.h
 * Author:
 * Created Time: 2020-08-19
 * Version: V1.1
 *
 * Description:
 ************************************************************************/
#ifndef __HTTP_H_
#define __HTTP_H_

/*******************************************************************************
 * includes
 ******************************************************************************/

#include <pthread.h>
#include <stdint.h>
#include <string>
#include <map>
#include <semaphore.h>

using namespace std;
/*******************************************************************************
 * Definitions
 ******************************************************************************/

namespace Klib {
/*******************************************************************************
 * Class
 ******************************************************************************/
class Http {
public:
    typedef enum {
        Type_Get,
        Type_Post,
        Type_Head,
		Type_Put,
		Type_Delete,
		Type_Options,
		Type_Trace,
		Type_Connect,
		Type_Patch,
        Type_Unknow
    } RequestType_EN;

    typedef enum {
        Cfg_SetContent = 1,
        Cfg_AddContent,
        Cfg_AppendHead,
        Cfg_SetBasicAuth,
        Cfg_SetDigestAuth,
        Cfg_SetTimeout,
        Cfg_SetConnTimeout,
        Cfg_SetCustomOpt,
        Cfg_SetUri,
    } ConfigType_S;

    typedef struct {
        char *data;
        uint32_t size;
        map<string,string> head;
        union {
            long code;
            RequestType_EN type;
        };
    } Value_S;

    /* 回调函数 */
    typedef void (*Callback)(Http *http, Http::Value_S value, void *userdata);

protected:
    pthread_t tid;
    sem_t psem;
    bool cbing;
    string url,ip,port,content;
    
    Callback callback;
    void *userdata;

    Http(){};

public:
    virtual ~Http(){};

    virtual bool perform(void);
    virtual bool response(Value_S *value);
    virtual void config(ConfigType_S cfg, uint16_t u16Val);
    virtual void config(ConfigType_S cfg, const string strVal);
    virtual void config(ConfigType_S cfg, const string strValF,const string strValS);
    virtual void config(ConfigType_S cfg, uint16_t u16Val, const string strVal);
    virtual void config(ConfigType_S cfg, uint16_t u16Val, uint32_t u32Val);

    void setCallback(Callback cb, void *userdata);
};

/* Http Client */
class HttpClient : public Http {
private:
    void *curl, *slist;
    RequestType_EN type;

private:
    static bool CurlInit(void);
    static void CurlCleanup(void);
    static size_t CurlWrite(void *ptr, size_t size, size_t nmemb, void *data);
    static void *PerformThread(void *arg);

public:
    HttpClient(RequestType_EN type, const string url);
    ~HttpClient();

    bool perform(void);
    bool response(Value_S *value);
    void config(ConfigType_S cfg, uint16_t u16Val);
    void config(ConfigType_S cfg, const string strVal);
    void config(ConfigType_S cfg, const string strValF,const string strValS);
    void config(ConfigType_S cfg, uint16_t u16Val, const string strVal);
    void config(ConfigType_S cfg, uint16_t u16Val, uint32_t u32Val);
};

/* Http Server */
class HttpServ : public Http {

private:
    void *evBase,*evHttp,*evbuf;
    long code;
    map<string,string> *header;
    
private:
    static void *PerformThread(void *arg);
    static void SpecifUriCallback(struct evhttp_request *req, void *arg);
    static void GenerCallback(struct evhttp_request *req, void *arg);

public:
    HttpServ(const string ip, const string port);
    ~HttpServ();

    bool perform(void);
    bool response(Value_S *value);
    void config(ConfigType_S cfg, const string strValF,const string strValS);
    void config(ConfigType_S cfg, const string strVal);
};

} // namespace Klib

#endif
