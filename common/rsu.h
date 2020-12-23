/*************************************************************************
 * File Name: rsu.h
 * Author:
 * Created Time: 2020-08-15
 * Last modify:  2020-09018
 * Version: V1.1
 *
 * Description:
 ************************************************************************/
#ifndef __RSU_H_
#define __RSU_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <pthread.h>
#include <stdint.h>
#include <string>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Class
 ******************************************************************************/
class Artc {
public:
    static const uint8_t CONTROLER_NUM = 2;
    static const uint8_t CTRL_PSMA_NUM = 12;
    static const uint8_t CTRL_ANT_NUM = 8;
    static const uint8_t PSMA_TERMINAL_NUM = 8;

    /* 回调函数 */
    typedef void (*Callback)(void *data, void *userdata);

    /* PSMA状态 */
    typedef struct {
        /* PSAM通道号 */
        uint8_t channel;
        /* PSAM状态 */
        uint8_t status;
        /* PSAM授权状态 (0x00:已授权 0x01:未授权) */
        uint8_t auth;
    } PsamSta_S;

    /* PSMA信息 */
    typedef struct {
        /* PSAM通道号 */
        uint8_t channel;
        /* PSAM版本号 */
        uint8_t version;
        /* PSAM授权状态 (0x00:已授权 0x01:未授权) */
        uint8_t auth;
        /* PSAM终端机编号 */
        uint8_t id[6];
    } PsamInfo_S;

    /* 控制器信息 */
    typedef struct {
        /* 控制器状态 */
        uint8_t state;
        /* PSAM数量 */
        uint8_t psamNum;
        /* PSMA状态 */
        PsamSta_S psamSta[CTRL_PSMA_NUM];
    } CtrlerSta_S;

    /* 天线信息 */
    typedef struct {
        /* 天线ID编号 */
        uint8_t id;
        /* 天线运行状态 */
        uint8_t sta;
        /* 天线信道 */
        uint8_t channel;
        /* 天线功率 */
        uint8_t power;
    } AntInfo_S;

    /* RSU控制器状态信息 */
    typedef struct {
        /* 连接状态 */
        bool linked;

        /* 状态获取时间戳 */
        uint32_t timestamp;

        /* 控制器状态,控制器数量  = CONTROLER_NUM */
        CtrlerSta_S ctrlSta[CONTROLER_NUM];

        /* 控制器之间的网络连接状态 */
        uint8_t netLinkSta;

        /* 天线总数 */
        uint8_t totalAnt;

        /* 正常工作天线数量 */
        uint8_t workingAnt;

        /* 天线信息 */
        AntInfo_S antInfo[CTRL_ANT_NUM];

    } Controler_S;

    /* RSU状态 */
    typedef struct {
        /* 路侧单元主状态参数;00:表示正常 ，否则表示异常 */
        uint8_t state;

        /* PSAM数量 */
        uint8_t PsamNum;

        /* PSAM信息 */
        PsamInfo_S psamInfo[PSMA_TERMINAL_NUM];

        /* 算法标识，默认填写00H */
        uint8_t algId;

        /* 路侧单元厂商代码 */
        uint8_t manuID;

        /* 路侧单元编号 */
        uint8_t id[3];

        /* 路侧单元软件版本号 */
        uint8_t ver[2];

        /* 工作模式返回状态，默认填写00H */
        uint8_t workSta;

        /* ETC门架编号（由C0帧中获取,失败填充00H) */
        uint8_t flagId[3];

        /* 保留数据段 */
        uint8_t reserved[4];

    } State_S;

    /* RSU复位信息 */
    typedef struct {
        /* 重启的天线序号 */
        uint8_t antSerialNum;

        /* 天线是否重启状态  0：没有重启  1：系统已经重启 */
        uint8_t antRstSta;
    } Reset_S;

    /*** RSU所有参数信息数据结构 ***/
    typedef struct {
        Controler_S controler;
        State_S sta;
        Reset_S reset;
    } RsuInfo_S;

private:
    pthread_t tid;
    bool isSocConnect;
    std::string ip;
    uint16_t port, localPort;
    int soc;

    Callback callback;
    void *userdata;

private:
    bool netConnect(void);
    void netDisconnect(void);
    static void *NetCommunicatThread(void *arg);
    static uint16_t CRC16_pc(uint8_t *pchMsg, uint16_t wDataLen);
    static uint32_t GetTickCount(void);

    RsuInfo_S info;

    void sendC0(void);

public:
    /**
     * @name: RSU构造函数
     *
     * @description:传入RSU的IP地址及端口号构造RSU对象；
     *
     * @para: ip : std::string型IP地址参数；
     *        port ： 无符号短整形或std::string型端口号；
     * */
    Artc(const std::string ip, uint16_t port);
    Artc(const std::string ip, const std::string port);

    /**
     * @name: RSU析构函数
     *
     * @description:关闭通讯句柄，释放占用资源；
     *
     * */
    ~Artc(void);

    /**
     * @name: 设置回调函数
     *
     * @description:设置RSU数据接受回调函数；
     *
     * @para: cb : 外部指定的回调函数指针
     *        *userdata ： 用户自定义数据指针（当回调发生时会作为回调函数userdata参数传参）；
     *
     * */
    void setCallback(Callback cb, void *userdata);

    /**
     * @name: 获取RSU参数
     *
     * @description:获取RSU参数；
     *
     * @return:当前最新的RSU参数结构；
     * */
    RsuInfo_S getRsuInfo(void);
    
};

class Huawei {

public:
    typedef struct{
        /* 连接状态 */
        bool linked;

        /* 状态获取时间戳 */
        uint32_t timestamp;

        /* Cpu占用率 */
        std::string cpuRate;

        /* 内存占用率 */
        std::string memRate;

        /* 运行状态 0：正常  非0：异常 */
        std::string status;

        /* 温度 */
        std::string temperture;

        /* 软件版本 */
        std::string softVer;

        /* 硬件版本 */
        std::string hardVer;

        /* 序列号 */
        std::string seriNum;

        /* 设备位置经度 */
        std::string longitude;

        /* 设备位置纬度 */
        std::string latitude;
    }RsuInfo_S;


    /* 回调函数 */
    typedef void (*Callback)(void *data, void *userdata);
private:
    std::string ip,port, uri;
    pthread_t tid;
    char *xmlRes, *xmlInq;
    std::string cwmpID,seriNum;

    Callback callback;
    void *userdata;

    RsuInfo_S info;

public:
    /**
     * @name: Huwawei构造函数
     *
     * @description:传入RSU的IP地址及Rui号构造RSU对象；
     *
     * @para: ip : std::string型IP地址参数；
     *        uri ： 华为RSU配置里的HTTP请求Uri；
     * */
    Huawei(std::string ip,std::string port, std::string uri);

    /**
     * @name: Huwawei析构函数
     *
     * @description:关闭通讯句柄，释放占用资源；
     *
     * */
    ~Huawei();

    /**
     * @name: 设置回调函数
     *
     * @description:设置RSU数据接受回调函数；
     *
     * @para: cb : 外部指定的回调函数指针
     *        *userdata ： 用户自定义数据指针（当回调发生时会作为回调函数userdata参数传参）；
     *
     * */
    void setCallback(Callback cb, void *userdata);


    /**
     * @name: 获取RSU参数
     *
     * @description:获取RSU参数；
     *
     * @return:当前最新的RSU参数结构；
     * */
    RsuInfo_S getRsuInfo(void);

private:
    static void *NetCommunicatThread(void *arg);
};

#endif
