/*************************************************************************
 * File Name: air_condition.h
 * Author:
 * Created Time: 2020-09-27
 * Last modify:
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef _AIR_CONDITION_H_
#define _AIR_CONDITION_H_

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
class AirCondition {
public:
    /* 空调参数设置类型 */
    typedef enum{
        Cfg_Refrigeration = 0, /* 制冷点 */
        Cfg_RefriRetDiff,      /* 制冷回差 */
        Cfg_Heating,           /* 加热点 */
        Cfg_HeatRetDiff,       /* 加热回差 */
        Cfg_HighTemp,          /* 高温点 */
        Cfg_LowTemp,           /* 低温点 */
        Cfg_HighHumi,          /* 高湿点 */
    } CfgType_EN;

    /* 运行状态 */
    typedef struct {
        std::string interFan;      /* 内风机 */
        std::string exterFan;      /* 外风机 */
        std::string compressor;    /* 压缩机 */
        std::string heating;       /* 电加热 */
        std::string urgFan;        /* 应急风机 */
    } Runsta_S;

    /* 传感器状态 */
    typedef struct {
        std::string coilTemp;      /* 盘管温度 */
        std::string outdoorTemp;   /* 室外温度 */
        std::string condTemp;      /* 冷凝温度 */
        std::string indoorTemp;    /* 室内温度 */
        std::string airOutTemp;    /* 排气温度 */
        std::string humity;        /* 湿度 */
 
 
        std::string current;       /* 电流 */
        std::string acVoltage;     /* 交流电压 */
        std::string dcVoltage;     /* 直流电压 */
    } SensorSta_S;  

    /* 告警状态 */
    typedef struct {
        std::string highTemp;      /* 高温告警 */
        std::string lowTemp;       /* 低温告警 */
        std::string highHumi;      /* 高湿告警 */
        std::string lowHumi;       /* 低湿告警 */
        std::string coilFreeze;    /* 盘管防冻 */
        std::string airOutHiTemp;  /* 排气高温 */
        std::string coilSensInval; /* 盘管温感失效 */
        std::string outSensInval;  /* 室外温感失效 */
        std::string condSensInval; /* 冷凝温感失效 */
        std::string inSensInval;   /* 内温感失效 */
        std::string airSensInval;  /* 排气温感失效 */
        std::string humiSensInval; /* 湿感失效 */
        std::string inFanFault;    /* 内风机故障 */
        std::string exFanFault;    /* 外风机故障 */
        std::string compresFault;  /* 压缩机故障 */
        std::string heatFault;     /* 电加热故障 */
        std::string urgFanFault;   /* 应急风机故障 */
        std::string highPressure;  /* 高压告警 */
        std::string lowPressure;   /* 低压告警 */
        std::string immersion;     /* 水浸告警 */
        std::string smoke;         /* 烟感告警 */
        std::string accessCtrl;    /* 门禁告警 */
        std::string highPresLock;  /* 高压锁定 */
        std::string lowPresLock;   /* 低压锁定 */
        std::string airOutLock;    /* 排气锁定 */
        std::string acOverVol;     /* 交流过压 */
        std::string acUnderVol;    /* 交流欠压 */
        std::string acOff;         /* 交流掉电 */
        std::string phaseLoss;     /* 缺相 */
        std::string freqErr;       /* 频率异常 */
        std::string phaseInverse;  /* 逆相 */
        std::string dcOverVol;     /* 直流过压 */
        std::string dcUnderVol;    /* 直流欠压 */
    } Warning_S;

    /* 系统参数 */
    typedef struct {
        std::string refrigeratTemp;/* 制冷点 */
        std::string refriRetDiff;  /* 制冷回差 */
        std::string heatTemp;      /* 加热点 */
        std::string heatRetDiff;   /* 加热回差 */
        std::string reserved[2];   /* 保留 */
        std::string highTemp;      /* 高温点 */
        std::string lowTemp;       /* 低温点 */
        std::string highHumi;      /* 高湿点 */
    } SysVal_S;

     /* 空调参数结构 */
    typedef struct {
        std::string version;
        Runsta_S runSta;
        SensorSta_S sensorSta;
        Warning_S warnings;
        SysVal_S sysVal;
    } AirInfo_S;

    typedef void (*Callback)(AirInfo_S info,void *userdata);

private:
    typedef enum {
        Sgl_Version = 0x0000,
        Sgl_RunSta = 0x0100,
        Sgl_SensorSta = 0x0500,
        Sgl_Warning = 0x0600,
        Sgl_SysVal = 0x0700,
        Sgl_StartSta = 0x801,
    } Signal_EN;

    typedef struct {
        uint8_t refrigeratTemp; /* 制冷点 */
        uint8_t refriRetDiff;   /* 制冷回差 */
        uint8_t heatTemp;       /* 加热点 */
        uint8_t heatRetDiff;    /* 加热回差 */
        uint8_t reserved[2];    /* 保留 */
        uint8_t highTemp;       /* 高温点 */
        uint8_t lowTemp;        /* 低温点 */
        uint8_t highHumi;       /* 高湿点 */
    } CfgSytVal_S;

    uint8_t addr;
    Signal_EN signal;
    AirInfo_S info;

    Callback callback;
    void *userdata;

    bool isConfigured;
    CfgSytVal_S sysVal;

public:
    AirCondition(uint8_t addr);
    ~AirCondition();

    void setCallback(Callback cb,void *userdata);
    void config(CfgType_EN type,uint8_t val);

private:
    static void UartCallback(uint8_t port, uint8_t *buf, uint32_t len, void *userdata);
    static void *QueryTask(void *arg);
};

#endif
