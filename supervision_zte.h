
/*************************************************************************
 * File Name: SupervisionZTE.h
 * Author:
 * Created Time: 2020-10-28
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
#ifndef __SUPERVISION_ZTE_H_
#define __SUPERVISION_ZTE_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
/*******************************************************************************
 * Definitions
 ******************************************************************************/
const std::string StrDevType[] = {
    "智能动环监控设备（FSU）","数字温湿度", "普通空调", "锂电池", "铁锂电池", "UPS", "cabinet_lock", "水浸", "门磁", "烟感", "开关电源", "摄像机报警",
};

#ifndef DEV_TYPE_NUM
#define DEV_TYPE_NUM            sizeof(StrDevType)/sizeof(std::string)
#endif 
/*******************************************************************************
 * Class
 ******************************************************************************/
/**
 * 动环系统控制类 (Supervision system) 
 * */

// 定义各门锁的位置
#define DEV_FRONT_DOOR		0	// 设备柜前门锁
#define DEV_BACK_DOOR		1
#define POWER_FRONT_DOOR	2
#define POWER_BACK_DOOR		3

#define ZTE_DOOR_POLL		0x0016		// 锁协议中的控制命令
#define ZTE_DOOR_OPEN		0x0005
#define ZTE_DOOR_CLOSE		0x0006


#pragma pack(push, 1)
class SupervisionZTE {
public:

    /* 动环外设类型 */
    typedef enum {
        DevType_SuHost,     /* 0 动环主机 */ 
        DevType_TemHumi,   /* 1 数字温湿度 */
        DevType_AirCo,        /* 2 普通空调 */
        DevType_LiBat,     /* 3 锂电池 */
        DevType_LiFeBat,   /* 4 铁锂电池 */
        DevType_UPS,       /* 5 UPS */
        DevType_CabLock,   /* 6 cabinet_lock */
        DevType_Immer,     /* 7 水浸 */
        DevType_Magnet,    /* 8 门磁 */
        DevType_Smoke,     /* 9 烟感 */
        DevType_PowSupply, /* 10 开关电源 */
        DevType_CamAlarm,  /* 11 摄像机报警 */
        DevType_Undefine,   /* 未定义 */
    } DevType_EN;

    /* 数字温湿度 */
    typedef struct {
        bool isLink;
        std::string tempture; // 温度
        std::string humity;   // 湿度
        std::string warning;
        std::string warnTime;
        std::string tempUpperLimit;
        std::string tempLowerLimit;
        std::string humiUpperLimit;
        std::string humiLowerLimit;
    } TemHumi_S;

    /* 普通空调 */
    typedef struct {
        bool isLink;
        std::string state;           // 设备工作状态
        std::string interFanSta;     // 内风机状态
        std::string exterFanSta;     // 外风机状态
        std::string compressorSta;   // 压缩机状态
        std::string retTempture;     // 回风温度
        std::string outDoorTempture; // 室外温度
        std::string condenserTemp;   // 冷凝器温度
        std::string dcVol;           // 直流电压
        std::string hiTempWarn;      // 高温告警
        std::string lowTempWarn;     // 低温告警
        std::string interFanFault;   // 内风机故障
        std::string exterFanFault;   // 外风机故障
        std::string compressorFault; // 压缩机故障
        std::string dcOverVol;       // 直流过压
        std::string dcUnderVol;      // 直流欠压
        std::string acOverVol;       // 交流过压
        std::string acUnderVol;      // 交流欠压
        std::string acPowDown;       // 交流掉电
        std::string heatActTemp;     //加热开启点
        std::string aircActTemp;     //空调开启点
        std::string workCurrent;     //工作电流
        std::string refrigWarn;      //制冷告警
        std::string tempSensorWarn;  //温度传感器故障告警
        std::string warning;
        std::string warnTime;
    }AirCo_S;

    /* 锂电池 */
    typedef struct {
        bool isLink;
        std::string voltage;// 电池组电压
        std::string capacity;// 剩余容量
        std::string fullCap;// 满充容量
        std::string monTempture;// 单体温度
        std::string SOH;
        std::string SOC;
        std::string warning;
        std::string warnTime;
    } LiBat_S;

    /* 铁锂电池 */
    typedef struct {
        bool isLink;
        std::string voltage;// 电池组电压
        std::string capacity;// 剩余容量
        std::string fullCap;// 满充容量
        std::string monTempture;// 单体温度
        std::string SOH;
        std::string SOC;
        std::string warning;
        std::string warnTime;
    } LiFeBat_S;

    /* UPS */
    typedef struct {
        bool isLink;
        std::string mainsVol;      // Ups市电电压
        std::string outVol;        // Ups输出电压
        std::string tempture;      // Ups温度
        std::string loadPercent;   // UPS负载百分比
        std::string inputFreq;     // UPS输入频率
        std::string isBypass;      // UPS是否旁路
        std::string runState;      // UPS开关机状态
        std::string faultWarn;     // ups故障状态报警
        std::string lowBatWarn;    //电池电压低报警
        std::string mainsVolAbnor; // Ups市电异常 ,交流电源输入停电告警
        std::string inputVol;      //输入电压值 222.2
        std::string batVol;        //电池电压
        std::string lowBatVol;     //电压下限
    } Ups_S;

    /* 开关电源 */
    typedef struct{
        bool isLink;
        std::string rectifierOutVol;//整流模块输出电压
        std::string rectifierOutCurr;//整流模块输出电流
        std::string rectifierOutTemp;//整流模块温度
        std::string acInPhaseUa;//交流输入相电压Ua
        std::string acInPhaseUb;//交流输入相电压Ub
        std::string acInPhaseUc;//交流输入相电压Uc
        std::string acInPhaseIa;//交流输出电流Ia
        std::string acInPhaseIb;//交流输出电流Ib
        std::string acInPhaseIc;//交流输出电流Ic
        std::string warning;
        std::string warnTime;
    }PowSupply_S;

    /* 水浸*/
    typedef struct{
    	bool isLink;
        std::string warning;
        std::string warnTime;
    }Immer_S;

    /* 门磁 */
    typedef struct{
    	bool isLink;
        std::string warning;
        std::string warnTime;
    }Magnet_S;

    /* 烟感*/
    typedef struct{
    	bool isLink;
        std::string warning;
        std::string warnTime;
    }Smoke_S;

    /* 摄像头警告 */
    typedef struct{
    	bool isLink;
        std::string warning;
        std::string warnTime;
    }CamAlarm_S;

    /* cabinet_lock */
    typedef struct {
        bool isLink;
		bool status;	// 锁是否被开启0:close 1:open
        uint8_t addr;
        uint32_t cardId;
    }CabLock_S;

    

    typedef struct {
        bool linked;        //连接状态
        uint32_t timestamp; //状态获取时间戳
        uint8_t devNum[DEV_TYPE_NUM]; //各类设备数量

        std::string cpuRate;        //CPU利用率
        std::string memRate;        //内存利用率
        std::string devDateTime;    //设备系统时间

        TemHumi_S temhumi[2];
        AirCo_S airco[2];
        LiBat_S libat[4];
        LiFeBat_S lifebat[4];
        Ups_S ups;
        CabLock_S lock[4];
        PowSupply_S powSupply;

        Immer_S immerWarn[2];
        Magnet_S magnetWarn[2];
        Smoke_S smokeWarn[2];
        CamAlarm_S camAlarmWarn[2];

    } State_S;

    typedef void (*Callback)(State_S state,void *userdata);

private:
    typedef enum{
        Req_Objs,
        Req_Data,
        Req_Warning,
        Req_SensorPort,
        Req_Alarm,
    }ReqType_EN;

    typedef struct {
        uint8_t index;
        uint8_t addr;
        DevType_EN type;
        std::string name;
        std::string portId;
    }ObjInfo_S;

    typedef struct {
        uint8_t reqType,httpType;
        std::string url;
        std::string *data;
        SupervisionZTE *su;
        std::string objid;
        ObjInfo_S *objinfo;
    } ReqPara_S;

    pthread_t tid;
    State_S state;
    std::string ip,user,key;
    uint16_t interval;

    bool isGetObjs,isGetSensorPort;

    std::map<std::string,ObjInfo_S> objs;

    Callback callback;
    void *userdata;

public:
    /**
     * @name: Supervision构造函数
     *
     * @description:传入动环的IP地址、端口号及认证信息构造动环对象；
     *
     * @para: type  : 设备类型
     *        ip    : string型IP地址参数；
     *        user  : string型认证信息用户名；
     *        key   : string型认证信息密码;
     * */
    SupervisionZTE(const std::string ip, const std::string user, const std::string key);

    /**
     * @name: 动环析构函数
     *
     * @description:关闭通讯句柄，释放占用资源；
     *
     * */
    ~SupervisionZTE();

    /**
     * @name: setCallback
     *
     * @description:传入函数指针，当收到数据时，会调用回调函数返回数据；
     *
     * */
    void setCallback(Callback calback,void *userdata);

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
     * @name: getState
     *
     * @description:外部主动获取动环参数结构
     *
     * @return: 返回动环::State_S 结构体；
     * 
     * */
    SupervisionZTE::State_S getState(void);

	/**
     * @name: openLock
     *
     * @description:开/关门锁
     *
     * @para: cmd:ZTE_DOOR_OPEN,开，ZTE_DOOR_CLOSE关;
     *		  pos:DEV_FRONT_DOOR:设备柜前门，POWER_FRONT_DOOR：电源柜前门
     * 
     * */
	void openLock(uint16_t cmd, uint16_t pos);

private:
    void reqObjs(void);
    void reqWarning(void);
    void reqSensorPort(void);
    void reqObjData(std::string objid, ObjInfo_S *info);
    void reqAlarmMete(std::string objid, ObjInfo_S *info,std::string mId);
    void ctrlLock(uint16_t cmd, std::string objid, ObjInfo_S *info);
    void httpRequest(void *para);


    static void *GetStateThread(void *arg);
#ifdef SUPERVISION_PRIVATE
    static void HttpCallback(Klib::Http *http,Klib::Http::Value_S value, void *userdata);
#endif
};
#pragma pack(pop)



#endif

