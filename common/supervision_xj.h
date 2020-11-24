
/*************************************************************************
 * File Name: Supervision_xj.h
 * Author:
 * Created Time: 2020-10-28
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __SUPERVISION_XJ_H_
#define __SUPERVISION_XJ_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <stdint.h>
#include <string>
#include <map>
#include <pthread.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ELOCK_NUM (2)
#define AIR_CONDITION_NUM (2)
#define UPS_NUM (2)
#define BMS_NUM (6)

/*******************************************************************************
 * Class
 ******************************************************************************/
/**
 * 动环系统控制类 (Supervision system)  品牌：迅捷
 * @brief 2020-11-09 根据《广中江数据协议》定义数据结构;
 * */
class SupervisionXJ {
public:
    /* 监控 */
    typedef struct {
        std::string smoke1;       /* 1#烟雾报警 */
        std::string smoke2;       /*  2#烟雾报警 */
        std::string humidity1;    /* 1#湿度 */
        std::string humidity2;    /* 2#湿度 */
        std::string temperature1; /* 1#温度 */
        std::string temperature2; /* 2#温度 */
        std::string door1;        /* 1#门禁 */
        std::string door2;        /* 2#门禁 */
        std::string water1;       /* 1#水浸 */
        std::string water2;       /* 2#水浸 */
        std::string do7;          /* 继电器(可控制) */
    } Monitor_S;

    /* 电能表 */
    typedef struct {
        bool isLink;
        std::string pvolt;     /* 电能表电压 */
        std::string pcurr;     /* 电能表电流 */
        std::string ppower;    /* 有功功率 */
        std::string pfact;     /* 功率因素 */
        std::string pfreq;     /* 输出频率 */
        std::string pallpower; /* 总有功电量 */
    } Ammeter_S;

    /* 空调 */
    typedef struct {
        bool isLink;
        std::string tempture;   /* 内温度 */
        std::string vptemp;     /* 内盘管温度 */
        std::string extemp;     /* 外盘管温度 */
        std::string comp_vol;   /* 压缩机电压 */
        std::string comp_cur;   /* 压缩机电流 */
        std::string kout;       /* 继电器输出告警字 */
        std::string sysal;      /* 系统告警字 */
        std::string swork;      /* 工作状态 */
        std::string srun;       /* 运行状态 */
        std::string temp_warn;  /* 系统告警 */
        std::string sysal_warn; /* 温度告警 */
    }Airc_S;

    /* 智能重合闸 */
    typedef struct {
        bool isLink;
        std::string sw;             /* 开关 */
        std::string current;        /* 当前电流 */
        std::string leak_cur;       /* 漏电电流 */
        std::string voltage;        /* 当前电压 */
        std::string ov_cur_cnt;     /* 过流次数 */
        std::string leak_cur_cnt;   /* 漏电次数 */
        std::string under_vol_cnt;  /* 欠压次数 */
        std::string ov_vol_cnt;     /* 过压次数 */
        std::string ov_cur_warn;    /* 过流告警 */
        std::string leak_cur_warn;  /* 漏电告警 */
        std::string under_vol_warn; /* 欠压告警 */
        std::string ov_vol_warn;    /* 过压告警 */
        std::string power_off;      /* 停电告警 */
    }Switch_S;

    typedef struct {
        bool isLink;
        std::string invol;            /* 输入电压 */
        std::string outvol;           /* 输出电压 */
        std::string freq;             /* 输出频率 */
        std::string loadrate;         /* 当前负载 */
        std::string outdc_vol;        /* 直流输出电压 */
        std::string outcur;           /* 输出电流 */
        std::string chgcur;           /* 充电电流 */
        std::string dischgcur;        /* 放电电流 */
        std::string power_rate;       /* 功率 */
        std::string invol_low_warn;   /* 输入电压低 */
        std::string invol_high_warn;  /* 输入电压高 */
        std::string outvol_low_warn;  /* 输出电压低 */
        std::string outvol_high_warn; /* 输出电压高 */
        std::string dischg_stat;      /* 充放电 */
        std::string work_mode;        /* 工作模式 */
        std::string falt_warn;        /* 故障告警 */
        std::string vol;              /* 输入电压 */
        std::string output_cur;       /* 输出电流 */
        std::string load_rate;        /* 当前负载 */
        std::string charge_cur;       /* 充电电流 */
        std::string acfail;           /* 市电中断 */
        std::string over_cur;         /* 过流 */
        std::string overload;         /* 过载 */
    }Ups_S; 

    /* BMS */
    typedef struct {
        bool isLink;
        std::string high_vol;  /* 电池电压高 */
        std::string low_vol;   /* 电池电压低 */
        std::string total_vol; /* 总电池电压 */
        std::string high_temp; /* 电池温度高 */
        std::string low_temp;  /* 电池温度低 */
        std::string discharge; /* 放电过流告警 */
        std::string charge;    /* 充电过流告警 */
        std::string vol;       /* 电压 */
        std::string cur;       /* 电流 */
        std::string subvol;    /* 单体电压 */
        std::string rlcap;     /* 剩余容量 */
        std::string soc;       /* SOC */
        std::string soh;       /* SOH */
        std::string cnt;       /* 充电次数 */
        std::string temp;      /* 电池温度 */
    }Bms_S;

    typedef struct {
        Monitor_S monitor;
        Ammeter_S ammeter;
        Switch_S sw;
        std::string elock[2];
        Airc_S airc[2];
        Ups_S ups[2];
        Bms_S bms[6];
    }State_S;

    typedef void (*Callback)(State_S state,void *userdata);

private:
    pthread_t tid;
    State_S state;
    std::string ip,devId;
    uint16_t interval;
    void *pMqtt; 

    bool isGetParam;
    std::map<std::string,int> params;

    Callback callback;
    void *userdata;

public:
    /**
     * @name: Supervision构造函数
     *
     * @description:传入动环的IP地址、端口号及认证信息构造动环对象；
     *
     * @para: strMqttIp    : string型MQTT服务器IP地址参数；
     *        strDevId     : string型动环设备ID号；
     * 
     * */
    SupervisionXJ(const std::string strMqttIp,const std::string strDevId);

    /**
     * @name: 动环析构函数
     *
     * @description:关闭通讯句柄，释放占用资源；
     *
     * */
    ~SupervisionXJ();

    /**
     * @name: setCallback
     *
     * @description:传入函数指针，当收到数据时，会调用回调函数返回数据；
     *
     * */
    void setCallback(Callback callback,void *userdata);

    /**
     * @name: getState
     *
     * @description:外部主动获取动环参数结构
     *
     * @return: 返回动环::State_S 结构体；
     * 
     * */
    SupervisionXJ::State_S getState(void);

private:
    static void *GetStateThread(void *arg);
    static void MqttSubCallback(uint8_t *msg, uint16_t msgLen, void *data);

    std::string getValByKey(const std::string key);
    std::string getValByKey(const std::string prefix,const std::string key);
    std::string getValByKey(const std::string key, uint32_t divide);
    std::string getValByKey(const std::string prefix,const std::string key, uint32_t divide);
};

#endif

