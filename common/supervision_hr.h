
/*************************************************************************
 * File Name: Supervision_hr.h
 * Author:
 * Created Time: 2020-12-2
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __SUPERVISION_HR_H_
#define __SUPERVISION_HR_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <stdint.h>
#include <string>
#include <map>
#include <pthread.h>
#include <semaphore.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SU_HR_LOCK_NUM                    (2)
#define SU_HR_AC_NUM                    (1)
/*******************************************************************************
 * Class
 ******************************************************************************/
/**
 * 动环系统控制类 (Supervision system)  品牌：华软
 * */
class SupervisionHR {
private:
    typedef enum {
        Sgl_Query = 0x01,     // 取卡状态
        Sgl_QueryPost = 0x02, // 取卡号
        Sgl_Open = 0x03,
        Sgl_OpenPost = 0x04,
    } LkSignal_EN;

    typedef enum {
        Sgl_Setting = 0x000A,
        Sgl_Version = 0x0104,
        Sgl_Control = 0x0200,
        Sgl_Warning = 0x0300,
        Sgl_AcInfo = 0x1000,
        Sgl_AcInfo2 = 0x101C,
        Sgl_Model = 0x1E1A,
    } AcSignal_EN;

public:
    /* 空调参数设置类型 */
    typedef enum {
        Cfg_Refrigeration = 0, /* 制冷点 */
        Cfg_RefriRetDiff,      /* 制冷回差 */
        Cfg_HighTempWarn,      /* 高温告警点 */
        Cfg_LowTempWarn,       /* 低温告警点 */
        Cfg_DcOverVolWarn,     /* 直流过压告警 */
        Cfg_DcUnderVolWarn,    /* 直流欠压告警 */
        Cfg_DcDownVol,         /* 直流下电电压 */
        Cfg_AcOverVolWarn,     /* 交流过压告警 */
        Cfg_AcUnderVolWarn,    /* 交流欠压告警 */
        Cfg_Heating,           /* 加热点 */
        Cfg_HeatRetDiff,       /* 加热回差 */
    } AcCfgType_EN;

    typedef struct {
        std::string state;        /* 设备工作状态 */
        std::string interFan;     /* 室内风机状态 */
        std::string exterFan;     /* 室外风机状态 */
        std::string compressor;   /* 压缩机状态 */
        std::string retAirTemp;   /* 柜内回风温度 */
        std::string pumpSta;      /* 水泵状态 */
        std::string exterTemp;    /* 柜外温度 */
        std::string condTemp;     /* 冷凝器温度 */
        std::string evapTemp;     /* 蒸发器温度 */
        std::string interFanSpd;  /* 内风机转速 */
        std::string exterFanSpd;  /* 外风机转速 */
        std::string acInputVol;   /* 交流输入电压 */
        std::string dcInputVol;   /* 直流输入电压 */
        std::string acCurr;       /* 交流运行电流 */
        std::string devRuntime;   /* 设备运行时间 */
        std::string compRuntime;  /* 压缩机运行时间 */
        std::string inFanRuntime; /* 内风机运行时间 */
        std::string compActCnt;   /* 压缩机动作次数 */
        std::string dcCurr;       /* 直流运行电流 */
        std::string dcPower;      /* 直流功率 */
        std::string acPower;      /* 交流功率 */
        std::string refriOutput;  /* 制冷量 */
    } AcInfo_S;

    typedef struct {
        std::string compRunTemp;  /* 压缩机启动温度 */
        std::string compSlew;     /* 压缩机回差 */
        std::string highTempWarn; /* 高温告警点 */
        std::string lowTempWarn;  /* 低温告警点 */
        std::string dcOverVol;    /* 直流过压告警 */
        std::string dcUnderVol;   /* 直流欠压告警 */
        std::string dcDownVol;    /* 直流下电电压 */
        std::string acOverVol;    /* 交流过压告警 */
        std::string acUnderVol;   /* 交流欠压告警 */
        std::string heatRunTemp;  /* 加热器启动温度设置 */
        std::string heatSlew;     /* 加热器回差设 */
    } AcSetting_S;

    typedef struct {
        std::string highTemp;        /* 高温告警 */
        std::string interFanFault;   /* 内风机故障告警 */
        std::string exterFanFault;   /* 外风机故障告警 */
        std::string compFault;       /* 压缩机故障告警 */
        std::string sensorFault;     /* 柜内回风温度温度探头故障 */
        std::string highPressure;    /* 系统高压力告警 */
        std::string lowTemp;         /* 低温告警 */
        std::string dcOverVol;       /* 直流过压告警 */
        std::string dcUnderVol;      /* 直流欠压告警 */
        std::string acOverVol;       /* 交流过压告警 */
        std::string acUnderVol;      /* 交流欠压告警 */
        std::string acDown;          /* 交流掉电告警 */
        std::string evaSensorFault;  /* 蒸发器温度传感器故障 */
        std::string condSensorFault; /* 冷凝器温度传感器故障 */
        std::string envSenssorFault; /* 环境温度传感器故障 */
        std::string evaFreeze;       /* 蒸发器冻结报警 */
        std::string freqHighPress;   /* 频繁高压力告警 */
    } AcWarn_S;

    typedef struct {
        bool isLink;
        uint8_t linkCnt; 

        uint8_t addr;
        LkSignal_EN signal, sigSave;

        uint16_t status;    // 0：关闭，1：打开
        uint16_t reason;    // 开锁原因
        uint8_t cardId[16]; // ID号，最长16个字节
        uint64_t cardUid;   // 前4个字节ID号
    } LockSta_S;

    typedef struct {
        bool isLink;            //连接标志
        uint8_t linkCnt;    

        uint8_t addr;
        AcSignal_EN signal;

        bool isConfigured;
        bool shutdown;

        std::string model;
        std::string version;

        AcInfo_S info;
        AcSetting_S setting;
        AcWarn_S warning;
    } AcSta_S;

    LockSta_S lock[SU_HR_LOCK_NUM];
    AcSta_S ac[SU_HR_AC_NUM];

private:
    pthread_t tid;
    std::string ip;
    uint16_t port;
    void *uartTcp1, *uartTcp2;
    sem_t semU1,semU2;

    
    LockSta_S *curLock;

public:
    /**
     * @name: Supervision构造函数
     *
     * @description:传入动环的IP地址、端口号及认证信息构造动环对象；
     *
     * @para: strIP    : string型TCP服务器IP地址参数；
     * 
     * */
    SupervisionHR(const std::string strIP);

    /**
     * @name: 动环析构函数
     *
     * @description:关闭通讯句柄，释放占用资源；
     *
     * */
    ~SupervisionHR();



    void openDoor(uint8_t seq);

    void acConfig(AcCfgType_EN type, uint8_t val);

private:
    void valInit(void);
    static void *GetStateThread(void *arg);
#ifdef SUPERVISION_PRIVATE
    static void TcpU1Callback(Tcp *tcp, Tcp::Socket_t soc, Tcp::Event_EN event, uint8_t *data, uint32_t size, void *userdata) ;
    static void TcpU2Callback(Tcp *tcp, Tcp::Socket_t soc, Tcp::Event_EN event, uint8_t *data, uint32_t size, void *userdata) ;
#endif
  
};

#endif

