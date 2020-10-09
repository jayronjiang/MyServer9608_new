/*************************************************************************
 * File Name: ipcam.h
 * Author:
 * Created Time: 2020-08-21
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __IPCAM_H_
#define __IPCAM_H_

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
class IPCam {
public:
    typedef struct {
        bool linked;        //连接状态
        uint32_t timestamp; //状态获取时间戳

        string ip;          //摄相机IP
        string factoryid;   //摄相机厂商1-宇视2-海康3-大华4-中威5-哈工大6-华为7-北京智通 8-北京信路威
        string errcode;     //正常时为0，状态异常时为厂商自定义的错误代码
        string devicemodel; //摄相机的设备型号
        string softversion; //摄相机的软件版本号
        string statustime;  //状态时间
        string filllight;   //闪光灯状态 0:正常，1:异常
        string temperature; //摄像枪温度（摄氏度）

        /* 交通部协议的状态内容部分 */
        string picstateid;              //流水号
        string gantryid;                //门架编号,全网唯一编号
        string statetime;               //状态采集时间
        string overstockImageJourCount; //积压图片流水数
        string overstockImageCount;     //积压图片数
        string cameranum;               //相机编号（101~299）
        string lanenum;                 //车道编号
        string connectstatus;           //连接状态
        string workstatus;              //工作状态
        string lightworkstatus;         //补光灯的工作状态
        string recognitionrate;         //识别成功率
        string hardwareversion;         //固件版本
        string softwareversion;         //软件版本
        string runningtime;             //设备从开机到现在的运行时间（秒）
        string brand;                   //厂商型号
        string devicetype;              //设备型号
        string statuscode;              //状态码,详见附录A3 0-正常；其他由厂商自定义
        string statusmsg;               //状态描述 由厂商自定义,最大长度256 例如：正常
    } State_S;

    typedef void (*Callback)(string staCode,string msg,State_S state,void *userdata);

private:
    pthread_t gtid,ctid;
    State_S state;
    string ip,port,key;
    uint16_t interval;

    Callback callback;
    void *userdata;

private:
    static void *GetStateThread(void *arg);
    static void *ControlThread(void *arg);
    // static void HttpCallback(Http *http,Http::void *value, void *userdata);
    static uint32_t GetTickCount(void);
public:
    /**
     * @name: IPCam构造函数
     *
     * @description:传入IPCam的IP地址、端口号及认证信息构造IPCam对象；
     *
     * @para: ip    : string型IP地址参数；
     *        port  : string型端口号；
     *        key   : string型认证信息；
     * */
    IPCam(const string ip, const string port, const string key);

    /**
     * @name: IPCam析构函数
     *
     * @description:关闭通讯句柄，释放占用资源；
     *
     * */
    ~IPCam();

    /**
     * @name: setCallback
     *
     * @description:传入函数指针，当收到数据时，会调用回调函数返回数据；
     *
     * */
    void setCallback(Callback cb,void *userdata);

    /**
     * @name: setInterval
     *
     * @description: 设置数据获取时间间隔为（1~65535秒）；
     * 
     * @para: interval    : 时间间隔；
     *
     * */
    void setInterval(uint16_t interval);

    /**
     * @name: reboot
     *
     * @description: 调用函数重启IP摄像头；
     *
     * */
    void reboot(void);

    /**
     * @name: modify
     *
     * @description:修改IPCam的IP地址、端口号及认证信息；
     *
     * @para: ip    : string型指针指向IP地址参数，如果不需要修改，传入“NULL”；
     *        port  : string型指针指向端口号，如果不需要修改，传入“NULL”；
     *        key   : string型指针指向认证信息，如果不需要修改，传入“NULL”；
     * */
    void modify(string *ip, string *port, string *key);


    /**
     * @name: getState
     *
     * @description:外部主动获取IPCam参数结构
     *
     * @return: 返回IPCam::State_S 结构体；
     * 
     * */
    IPCam::State_S getState(void);
};

#endif
