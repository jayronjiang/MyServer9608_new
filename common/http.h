/*************************************************************************
 * File Name: http.h
 * Author:
 * Created Time: 2020-08-19
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __HTTP_H_
#define __HTTP_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <stdint.h>
#include <pthread.h>
#include <string>

using namespace std;
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Class
 ******************************************************************************/

class HTTP {
public:
    typedef enum {
        get,
        post,
    } RequestType_EN;

    /* 回调函数 */
    typedef void (*Callback)(long code,char *data,uint32_t dLen,void *userdata);

private:
    bool slefDelete;
    void *curl, *slist;
    string url, content;
    RequestType_EN type;

    Callback callback;
    void *userdata;

private:
    ~HTTP();
    static bool CurlInit(void);
    static void CurlCleanup(void);
    static size_t CurlWrite(void *ptr, size_t size, size_t nmemb, void *data);
    static void *PerformThread(void *arg);

public:
    HTTP(RequestType_EN type,const string url);

    void setCallback(Callback cb,void *userdata);
    void setContent(string content);
    void addContent(string content);

    void appendHead(const string header,const string para);

    void setBasicAuth(string key);
    void setDigestAuth(string name,string pwd);

    void setTimeout(uint16_t time);
    void setConnTimeout(uint16_t time);

    void setCustomOpt(uint16_t opt,const char *var);
    void setCustomOpt(uint16_t opt,uint32_t var);

    void setSelfDelete(bool isDel);

    uint8_t perform(void);
};

#endif
