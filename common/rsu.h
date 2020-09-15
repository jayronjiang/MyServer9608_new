/*************************************************************************
 * File Name: rsu.h
 * Author:
 * Created Time: 2020-08-15
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __RSU_H_
#define __RSU_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <stdint.h>
#include <string>
#include <pthread.h>

using namespace std;
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define RSU_CONTROLER_NUM                                       (2)
#define RSU_CTRL_PSMA_NUM                                       (12)
#define RSU_CTRL_ANT_NUM                                        (8)
#define RSU_PSMA_TERMINAL_NUM                                   (8)
/*******************************************************************************
 * Class
 ******************************************************************************/
class RSU {
public:
    /* 数据类型 */
    typedef enum {
        /* 状态信息 */
        stateInfo = 0xB0,
        /* 心跳信息(控制器状态信息) */
        heartbeat = 0xB1,
        /* 复位信息 */
        rstInfo = 0xD1,
    }DataType_EN;

    /* 回调函数 */
    typedef void (*Callback)(DataType_EN type,void *data,void *userdata);

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
        PsamSta_S psamSta[RSU_CTRL_PSMA_NUM];
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

        /* 控制器状态,控制器数量  = RSU_CONTROLER_NUM */
        CtrlerSta_S ctrlSta[RSU_CONTROLER_NUM];

        /* 控制器之间的网络连接状态 */
        uint8_t netLinkSta;

        /* 天线总数 */
        uint8_t totalAnt;

        /* 正常工作天线数量 */
        uint8_t workingAnt;

        /* 天线信息 */
        AntInfo_S antInfo[RSU_CTRL_ANT_NUM];

    } RsuControler_S;

    /* RSU状态 */
    typedef struct {
        /* 路侧单元主状态参数;00:表示正常 ，否则表示异常 */
        uint8_t state;

        /* PSAM数量 */
        uint8_t PsamNum;

        /* PSAM信息 */
        PsamInfo_S psamInfo[RSU_PSMA_TERMINAL_NUM];

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

    } RsuInfo_S;

    /* RSU复位信息 */
    typedef struct {
        /* 重启的天线序号 */
        uint8_t antSerialNum;

        /* 天线是否重启状态  0：没有重启  1：系统已经重启 */
        uint8_t antRstSta;
    } RsuRst_S;


private:
    pthread_t tid;
    bool isSocConnect;
    string ip;
    uint16_t port,localPort;
    int soc;

    Callback callback;
    void *userdata;

private:
    bool netConnect(void);
    void netDisconnect(void);
    static void *NetCommunicatThread(void *arg);
    static uint16_t CRC16_pc(uint8_t *pchMsg,uint16_t wDataLen);
    static uint32_t GetTickCount(void);
public:
    RsuControler_S controler;
    RsuInfo_S info;
    RsuRst_S reset;


    /**
     * @name: RSU构造函数
     * 
     * @description:传入RSU的IP地址及端口号构造RSU对象；
     * 
     * @para: ip : string型IP地址参数；
     *        port ： 无符号短整形或string型端口号；  
     * */
    RSU(const string ip,uint16_t port);
    RSU(const string ip,const string port);

    /**
     * @name: RSU析构函数
     * 
     * @description:关闭通讯句柄，释放占用资源；
     * 
     * */
    ~RSU(void);

    /**
     * @name: 设置回调函数
     * 
     * @description:设置RSU数据接受回调函数；
     * 
     * @para: cb : 外部指定的回调函数指针
     *        *userdata ： 用户自定义数据指针（当回调发生时会作为回调函数userdata参数传参）；
     * 
     * */
    void setCallback(Callback cb,void *userdata);
    /*
     *
     * 回调函数说明及例程
     * 
     * typedef void (*Callback)(DataType_EN type,void *data,void *userdata);
     * 
     * type         : 数据类型 （目前是0xB0 0xB1 0xD1三种）；
     * *data        : 数据指针，根据数据类型转型为对应的数据结构，再进行后续处理；
     * *userdata    : 用户数据，返回setCallback函数设置回调时传入的userdata，可用于识别回调的对象，用于多个RSU对象共用同一个回调函数；
     * 
        void RsuCallback(RSU::DataType_EN type, void *data, void *userdata) {
            //心跳信息
            switch (type) {
            case RSU::tHeartbeat: {
                RSU::RsuControler_S *ctrler = (RSU::RsuControler_S *)data;

                ......
            } break;
            //状态信息
            case RSU::tStateInfo: {
                RSU::RsuInfo_S *info = (RSU::RsuInfo_S *)data;

                ......
            } break;
            //复位信息
            case RSU::tReset: {
                RSU::RsuRst_S *reset = (RSU::RsuRst_S *)data;

                ......
            } break;
            }
        }
    **/

};

#endif
