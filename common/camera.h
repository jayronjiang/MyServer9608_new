/*************************************************************************
 * File Name: camera.h
 * Author:
 * Created Time: 2020-09-25
 * Last modify:  
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __CAMERA_H_
#define __CAMERA_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <stdint.h>
#include <string>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IMA_SAVE_PATH_ROOT                     "/opt/cam_img/"
/*******************************************************************************
 * Class
 ******************************************************************************/

class Camera {
public:
    typedef enum{
        Msg_CaptureSuccess,
        Msg_CaptureFail,
        Msg_IsCapturing,
    }MsgType_EN;

    typedef void (*Callback)(MsgType_EN msg,const char *jpgName,void *userdata);

private:
    bool capturing;
    uint8_t time;
    uint16_t interval;
    std::string url,path;

    void *http;

    Callback callback;
    void *userdata;

    static void *CaptureTask(void *arg);

public:
    Camera(std::string url,std::string dirName);
    ~Camera();

    void capture(void);
    void capture(uint8_t time,uint8_t interval);
    void setCallback(Callback callback,void *userdata);
    void cleanUp(uint8_t fileNum);
};


#endif

