/*************************************************************************
 * File Name: SupervisionZTE.cpp
 * Author:
 * Created Time: 2020-10-28
 * Version: V1.0
 *
 * Description:
 ************************************************************************/
/*******************************************************************************
 * includes
 ******************************************************************************/
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "CJsonObject.hpp"
#include "http.h"
#include "base_64.h"

#define SUPERVISION_PRIVATE
#include "supervision_zte.h"

using namespace neb;
using namespace std;
using namespace Klib;
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DOOR_STATUS_ADDR	9	// 返回帧锁的状态在第几个字节
#define DOOR_ID_ADDR		10	// 返回帧锁的ID在第几个字节
#define DOOR_SOI_ADDR		0
#define DOOR_EOI_ADDR		25

#define DEF_INTERVAL (2)
#define POLL_INTERVAL	(2)	// 2*2=4s轮询一个数据,其实算上延时已经到了17s了

#define ID_ARRAY_TO_INT(_id)                     (((uint32_t)_id[3] << 24) | ((uint32_t)_id[2] << 16) |((uint32_t)_id[1] << 8) |((uint32_t)_id[0]))


/*******************************************************************************
 * Variables
 ******************************************************************************/
// static const string StrDataType[][24] = {
//     /* 动环主机 */
//     {"138101001", "138102001", "138103001", ""},
//     /* 数字温湿度 */
//     {"118201001", "118204001", ""},
//     /* 普通空调 */
//     {"115008001", "115009001", "115010001", "115011001", "115012001", "115013001", "115014001", "115016001", "115018001", "115019001", "115020001", "115021001", "115022001", "115023001", "115024001","115025001","115026001","115027001",""},
//     /* 锂电池 */
//     {"190004001", "190264001", "190265001", "190002001", ""},
//     /* 铁锂电池(暂时复制锂电池) */
//     {"190004001", "190264001", "190265001", "190002001", ""},
//     /* UPS */
//     {"108035001", "108036001", "108011001", "108037001", "108056001", "108304001", "108308001", "108302001", "108303001", "108305001", "108035001", "108012001", "108301001", ""},
//     /* cabinet_lock */
//     {"", "", ""},
//     /* 水浸 */
//     {"", "", ""},
//     /* 门磁 */
//     {"", "", ""},
//     /* 烟感 */
//     {"", "", ""},
//     /* 开关电源 */
    
//     /* 摄像机报警 */
//     {"", "", ""},
// };
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void crc(uint8_t *pucData,uint8_t wLength,uint8_t *pOutData);
static uint8_t cmdpack(uint8_t addr, uint16_t cmd, uint8_t *buf);
static string getResFromJson(CJsonObject res,string id);
static string getWarnFromJson(CJsonObject res,string id,string *time);
static uint32_t getTickCount(void);
static void stateInit(SupervisionZTE::State_S *state);
/*******************************************************************************
 * Code
 ******************************************************************************/
SupervisionZTE::SupervisionZTE(const string ip, const string user, const string key) {
    printf("Creat ZTE ip = %s user = %s key = %s\n",ip.c_str(),user.c_str(),key.c_str());
    this->ip = ip.c_str();
    this->user = user.c_str();
    this->key = key.c_str();

    callback = NULL;
    interval = DEF_INTERVAL;
    isGetObjs = false;
    isGetSensorPort = false;

    stateInit(&state);

    pthread_create(&tid, NULL, GetStateThread, this);
}

SupervisionZTE::~SupervisionZTE() {}

void SupervisionZTE::setCallback(Callback calback, void *userdata) {
    this->callback = calback;
    this->userdata = userdata;
}

void SupervisionZTE::setInterval(uint16_t interval) {
    this->interval = interval;
}

SupervisionZTE::State_S SupervisionZTE::getState(void) {
    return state;
}

void SupervisionZTE::httpRequest(void *para){
    if(para == NULL)
        return;

    ReqPara_S *reqPara = (ReqPara_S *)para;

    Http *http = new HttpClient((Http::RequestType_EN)reqPara->httpType, reqPara->url);

    http->config(Http::Cfg_AppendHead, "Accept: application/json");
    http->config(Http::Cfg_AppendHead, "Content-Type: application/json");
    http->config(Http::Cfg_AppendHead, "Accept-Language: zh-Hans-CN,zh-Hans;q=0.5");
    http->config(Http::Cfg_AppendHead, "Connection: close");
    http->config(Http::Cfg_SetTimeout, 2);
    http->config(Http::Cfg_SetConnTimeout, 2);
    http->config(Http::Cfg_SetSelfDelete, 1);
    http->config(Http::Cfg_SetDigestAuth, user, key);
    if(reqPara->data != NULL)
        http->config(Http::Cfg_SetContent,*reqPara->data);
    http->setCallback(HttpCallback, reqPara);
    http->perform();
}
void SupervisionZTE::reqObjs(void) {
    ReqPara_S *para = new ReqPara_S;

    para->reqType = Req_Objs;
    para->httpType = Http::Type_Get;
    para->su = this;
    para->url = "http://" + ip + "/jscmd/objs";
    para->data = NULL;
    para->objinfo = NULL;
    httpRequest(para);
}

void SupervisionZTE::reqWarning(void){
    ReqPara_S *para = new ReqPara_S;

    para->reqType = Req_Warning;
    para->httpType = Http::Type_Get;
    para->su = this;
    para->url = "http://" + ip + "/jscmd/alarmquery?type=now";
    para->data = NULL;
	printf("warning = %s\r\n",para->url.c_str());
    httpRequest(para);
}

void SupervisionZTE::reqSensorPort(void){
    ReqPara_S *para = new ReqPara_S;

    para->reqType = Req_SensorPort;
    para->httpType = Http::Type_Get;
    para->su = this;
    para->url = "http://" + ip + "/jscmd/sensorports";
    para->data = NULL;
    httpRequest(para);
}

void SupervisionZTE::reqObjData(std::string objid, ObjInfo_S *info) {
    if(info == NULL || info->type == DevType_Undefine)
        return;

    ReqPara_S *para = new ReqPara_S;

    para->objid = objid;
    para->objinfo = info;
    para->reqType = Req_Data;
    para->httpType = Http::Type_Get;
    para->su = this;
    para->url = "http://" + ip + "/jscmd/objdataquery?objId=" + objid + "&mId=0";
    para->data = NULL;
	printf("url = %s\r\n",para->url.c_str());
    httpRequest(para);

}


void SupervisionZTE::ctrlLock(uint16_t cmd, std::string objid, ObjInfo_S *info) {
    uint8_t len, buf[64];

    Base64 Base64Cal;
    ReqPara_S *para = new ReqPara_S;

    para->objid = objid;
    para->objinfo = info;
    para->reqType = Req_Data;
    para->httpType = Http::Type_Post;
    para->su = this;
    para->url = "http://" + ip + "/jscmd/serialsend";
    para->data = new string();
    
    len = cmdpack( info->addr,cmd, buf);
    (*para->data) = Base64Cal.Encode(buf, len); // 加密
    (*para->data) = "{\"jsonrpc\":\"2.0\",\"method\":\"POST_METHOD\",\"id\":\"3\",\"params\":{\"objid\":\"" + objid + "\",\"data\":\"" + (*para->data) + "\",\"endchar\":\"\"}}";
    printf("data = %s\r\n",para->data->c_str());
    httpRequest(para);
}


// 开锁接口
// cmd:ZTE_DOOR_OPEN，开
// pos:DEV_FRONT_DOOR/DEV_BACK_DOOR/POWER_FRONT_DOOR/POWER_BACK_DOOR
void SupervisionZTE::openLock(uint16_t cmd, uint16_t pos) 
{
	map<string,ObjInfo_S>::iterator iter;
	string objid = "";
	ObjInfo_S *info = NULL;
	
	for (iter = objs.begin(); iter != objs.end(); iter++) 
	{
        info = &iter->second;
		if (pos == DEV_FRONT_DOOR)
		{
			if ((info->portId == "0_COM5") && (info->addr == 1))
			{
				objid = iter->first;
				break;
			}
		}
		else if (pos == DEV_BACK_DOOR)
		{
			if ((info->portId == "0_COM5") && (info->addr == 2))
			{
				objid = iter->first;
				break;
			}
		}
		else if (pos == POWER_FRONT_DOOR)
		{
			if ((info->portId == "0_COM3") && (info->addr == 1))
			{
				objid = iter->first;
				break;
			}
		}
		else if (pos == POWER_BACK_DOOR)
		{
			if ((info->portId == "0_COM3") && (info->addr == 2))
			{
				objid = iter->first;
				break;
			}
		}
    }
	if (objid != "")
	{
		printf("open_lock = %s,%s,%d\r\n",objid.c_str(),info->portId.c_str(),info->addr);
		ctrlLock(cmd,objid,info);
		usleep(200*1000);
	}
}



/* static */
void *SupervisionZTE::GetStateThread(void *arg) {
    SupervisionZTE *su = (SupervisionZTE *)arg;
    map<string,ObjInfo_S>::iterator iter;
	static map<string,ObjInfo_S>::iterator data_iter;
	static uint16_t cnt = POLL_INTERVAL-1;
	bool exchg[2] = {false,false};

    printf("Supervision(%s) get state thread start ..\n",su->ip.c_str());
    
    do{
        su->reqObjs();
        sleep(2);
    }while(!su->isGetObjs);

    do{
        su->reqSensorPort();
        sleep(2);
    }while(!su->isGetSensorPort);

		
    for (iter = su->objs.begin(); iter != su->objs.end(); iter++) {
        ObjInfo_S *info = &iter->second;
        printf("objid=%s,(%d) %s addr = %d port = %s\n",iter->first.c_str(),info->index,info->name.c_str(),info->addr,info->portId.c_str());
    }

	data_iter = su->objs.begin();
    for (;;) 
	{
		// 17s钟取一个数据,5分钟一个循环
		if (cnt++ > POLL_INTERVAL)
		{
			cnt = 0;
	        //for(iter = su->objs.begin();iter != su->objs.end();iter++)
			{
	            ObjInfo_S *info = &data_iter->second;
	            string objid = data_iter->first;

	            if(data_iter->second.type == DevType_CabLock)
	                su->ctrlLock(ZTE_DOOR_POLL,objid,info);
	            else
	                su->reqObjData(objid,info);

				//su->state.timestamp = getTickCount();
				//printf("timestampdata=%ld,\n",su->state.timestamp);
	        }
			usleep(500*1000);
			data_iter++;
			if (data_iter == su->objs.end())
			{
				data_iter = su->objs.begin();
			}
	        //su->reqWarning();
		}

		for(iter = su->objs.begin();iter != su->objs.end();iter++)
		{
            ObjInfo_S *info = &iter->second;
            string objid = iter->first;

			// 每秒单独轮询门锁
            if(iter->second.type == DevType_CabLock)
            {
                su->ctrlLock(ZTE_DOOR_POLL,objid,info);
				//su->state.timestamp = getTickCount();
				//printf("timestamp=%ld,\n",su->state.timestamp);
				usleep(500*1000);
            }
            else
                continue;
        }
		su->reqWarning();
		usleep(200*1000);
        su->state.timestamp = getTickCount();
		printf("Warningtimestamp=%ld,\n",su->state.timestamp);
        if(su->callback != NULL)
            su->callback(su->state,su->userdata);
        sleep(su->interval);
    }
}

/* static  */
void SupervisionZTE::HttpCallback(Http *http, Http::Value_S value, void *userdata) {
    ReqPara_S *para = (ReqPara_S *)userdata;
    SupervisionZTE *su = para->su;
    
    if (para == NULL)
        return;

    //printf("code:%ld  data = %s ,para type = %d\n",value.code,value.data,para->reqType);
    if (value.code == 200 || value.code == 202) {
        CJsonObject json(value.data), jsResult;
        uint16_t i,j;
        string key, val;

        if (json.IsEmpty())
                return;

        switch (para->reqType) {
        case Req_Objs: {
            CJsonObject objs;
            string objid, name;

            if (!json.Get("result", jsResult))
                break;

            if (!jsResult.Get("Objs", objs))
                break;

            if (objs.GetArraySize() <= 0)
                break;

            su->objs.clear();
            memset(su->state.devNum, 0, DEV_TYPE_NUM);

            for (i = 0; i < objs.GetArraySize(); i++) {
                if (objs[i].Get("ObjId", objid) && objs[i].Get("Name", name)) {
                    ObjInfo_S info;

                    info.name = name.c_str();
                    info.type = DevType_Undefine;
                    for (j = 0; j < DEV_TYPE_NUM; j++) {
                        if (name == StrDevType[j]){
                            info.index = su->state.devNum[j];
                            su->state.devNum[j] += 1;
                            info.type = (DevType_EN)j;
                            break;
                        }
                    }

                    su->objs[objid.c_str()] = info;
                }
            }

            su->state.linked = true;
            su->isGetObjs = true;
#if 0
            map<string,ObjInfo_S>::iterator iter;
            for(iter = su->objs.begin();iter != su->objs.end();iter++){
                printf("id:%s\tname:%s\n",iter->first.c_str(),iter->second.name.c_str());
            }
#endif

            } break;

        case Req_SensorPort:{
            string objid,portId,addr;
            
            if (!json.Get("result", jsResult))
                break;

            if (jsResult.GetArraySize() <= 0)
                break;

            for (i = 0; i < jsResult.GetArraySize(); i++) {
                if (jsResult[i].Get("objId", objid)) {
                    su->objs[objid.c_str()].portId = "";
                    su->objs[objid.c_str()].addr = 0;

                    if(jsResult[i].Get("portId",portId))
                        su->objs[objid.c_str()].portId = portId;

                    if(jsResult[i].Get("addr",addr))
                        su->objs[objid.c_str()].addr = atoi(addr.c_str());
                }
            }
            su->isGetSensorPort = true;
        }break;

        case Req_Data:{
            ObjInfo_S *info = para->objinfo;

            if (!json.Get("result", jsResult))
                break;

            if(info == NULL)
                break;

            switch (info->type) {
            case DevType_SuHost: {
                su->state.linked = true;
                su->state.cpuRate = getResFromJson(jsResult,"138101001");
                su->state.memRate = getResFromJson(jsResult,"138102001");
                su->state.devDateTime = getResFromJson(jsResult,"138103001");
            } break;
            case DevType_TemHumi: {
                if(info->index < 0 || info->index > 1)
                    break;

                su->state.temhumi[info->index].humity = getResFromJson(jsResult,"118201001");
                su->state.temhumi[info->index].tempture = getResFromJson(jsResult,"118204001");
            } break;
            case DevType_AirCo: {
                if(info->index < 0 || info->index > 1)
                    break;

                su->state.airco[info->index].state = getResFromJson(jsResult,"115008001");
                su->state.airco[info->index].interFanSta = getResFromJson(jsResult,"115009001");
                su->state.airco[info->index].exterFanSta = getResFromJson(jsResult,"115010001");
                su->state.airco[info->index].compressorSta = getResFromJson(jsResult,"115011001");
                su->state.airco[info->index].retTempture = getResFromJson(jsResult,"115012001");
                su->state.airco[info->index].outDoorTempture = getResFromJson(jsResult,"115013001");
                su->state.airco[info->index].condenserTemp = getResFromJson(jsResult,"115014001");
                su->state.airco[info->index].dcVol = getResFromJson(jsResult,"115016001");
                su->state.airco[info->index].hiTempWarn = getResFromJson(jsResult,"115018001");
                su->state.airco[info->index].lowTempWarn = getResFromJson(jsResult,"115019001");
                su->state.airco[info->index].interFanFault = getResFromJson(jsResult,"115020001");
                su->state.airco[info->index].exterFanFault = getResFromJson(jsResult,"115021001");
                su->state.airco[info->index].compressorFault = getResFromJson(jsResult,"115022001");
                su->state.airco[info->index].dcOverVol = getResFromJson(jsResult,"115023001");
                su->state.airco[info->index].dcUnderVol = getResFromJson(jsResult,"115024001");
                su->state.airco[info->index].acOverVol = getResFromJson(jsResult,"115025001");
                su->state.airco[info->index].acUnderVol = getResFromJson(jsResult,"115026001");
                su->state.airco[info->index].acPowDown = getResFromJson(jsResult,"115027001");
            } break;
            case DevType_LiBat: {
                /* 锂电池 */

            } break;
            case DevType_LiFeBat: {
               /* 铁锂电池(暂时复制锂电池) */
            } break;
            case DevType_UPS: {
                     /* UPS */
            } break;
            case DevType_CabLock: {
                string strData;
                uint8_t *data;
                
                if (!jsResult.Get("data", strData))
                    break;

                data = (uint8_t *)malloc(100);

                base64_2_Hex_decode(strData.c_str(), strData.size(), data);

                if ((data[0] == 0x7E) && (data[DOOR_EOI_ADDR] == 0x7E)) // 锁的协议以0x7E开始，0x7E结束
                {
                    uint8_t cardId[4];
                    memcpy(cardId, &data[DOOR_ID_ADDR], 4);
                    // 这个锁读到的卡号是逆序的，比如0xD6E9C160的卡(3605643616)读出来是60 c1 e9 d6
                    su->state.lock[info->index].cardId = ID_ARRAY_TO_INT(cardId);

                    printf("index = %d,card id = %d\n",info->index,su->state.lock[info->index].cardId);
                }

                free(data);

            } break;

            case DevType_PowSupply: {
            } break;
            }
        }break;
        case Req_Warning:{
            // printf("Warning data:%s\n",value.data);

            CJsonObject json(value.data),res;

            if (!json.Get("result", res))
                break;

            if(res.GetArraySize() <= 0)
                break;

            su->state.temhumi[0].warning = getWarnFromJson(res,"22550",&su->state.temhumi[0].warnTime);
            
            su->state.magnetWarn[0].warning = getWarnFromJson(res,"22826",&su->state.magnetWarn[0].warnTime);
            su->state.magnetWarn[1].warning = getWarnFromJson(res,"22827",&su->state.magnetWarn[1].warnTime);
            
            su->state.smokeWarn[0].warning = getWarnFromJson(res,"22412",&su->state.smokeWarn[0].warnTime);
            su->state.smokeWarn[1].warning = getWarnFromJson(res,"41",&su->state.smokeWarn[1].warnTime);
            
            su->state.immerWarn[0].warning = getWarnFromJson(res,"22825",&su->state.immerWarn[0].warnTime);
            su->state.powSupply.warning = getWarnFromJson(res,"15",&su->state.powSupply.warnTime);
           
            su->state.airco[0].warning = getWarnFromJson(res,"21454",&su->state.airco[0].warnTime);

            su->state.lifebat[0].warning = getWarnFromJson(res,"22140",&su->state.lifebat[0].warnTime);
            su->state.lifebat[1].warning = getWarnFromJson(res,"22141",&su->state.lifebat[1].warnTime);
            su->state.lifebat[2].warning = getWarnFromJson(res,"23063",&su->state.lifebat[2].warnTime);
            su->state.lifebat[3].warning = getWarnFromJson(res,"23064",&su->state.lifebat[3].warnTime);
        

        
        }break;
        }
    }

    delete para;
}

static void stateInit(SupervisionZTE::State_S *state){
    if (state == NULL)
        return;

    uint8_t i;

    state->linked = false;    //连接状态
    state->timestamp = 0;; //状态获取时间戳

    state->devNum[DEV_TYPE_NUM]; //各类设备数量

    state->cpuRate = "0"; // CPU利用率
    state->memRate = "0";       //内存利用率
    state->devDateTime = "0";   //设备系统时间

    for (i = 0; i < 2; i++) {
        state->temhumi[i].humity = "2147483647";
        state->temhumi[i].tempture = "2147483647";
        state->temhumi[i].warning = "2147483647";
        state->temhumi[i].warnTime = "2147483647";

        state->airco[i].dcVol = "2147483647";
        state->airco[i].exterFanFault = "2147483647";
        state->airco[i].exterFanSta = "2147483647";
        state->airco[i].hiTempWarn = "2147483647";
        state->airco[i].interFanFault = "2147483647";
        state->airco[i].interFanSta = "2147483647";
        state->airco[i].lowTempWarn = "2147483647";
        state->airco[i].outDoorTempture = "2147483647";
        state->airco[i].retTempture = "2147483647";
        state->airco[i].state = "2147483647";
        state->airco[i].warning = "2147483647";
        state->airco[i].warnTime = "2147483647";

        state->immerWarn[i].warning = "2147483647";
        state->immerWarn[i].warnTime = "2147483647";

        state->magnetWarn[i].warning = "2147483647";
        state->magnetWarn[i].warnTime = "2147483647";

        state->smokeWarn[i].warning = "2147483647";
        state->smokeWarn[i].warnTime = "2147483647";

        state->camAlarmWarn[i].warning = "2147483647";
        state->camAlarmWarn[i].warnTime = "2147483647";
    }

    for (i = 0; i < 4; i++) {
        state->libat[i].capacity = "2147483647";
        state->libat[i].fullCap = "2147483647";
        state->libat[i].monTempture = "2147483647";
        state->libat[i].voltage = "2147483647";
        state->libat[i].warning = "2147483647";
        state->libat[i].warnTime = "2147483647";

        state->lifebat[i].capacity = "2147483647";
        state->lifebat[i].fullCap = "2147483647";
        state->lifebat[i].monTempture = "2147483647";
        state->lifebat[i].voltage = "2147483647";
        state->lifebat[i].warning = "2147483647";
        state->lifebat[i].warnTime = "2147483647";

        state->lock[i].addr = 0;
    }

    state->ups.faultWarn = "2147483647";
    state->ups.inputFreq = "2147483647";
    state->ups.inputVol = "2147483647";
    state->ups.isBypass = "2147483647";
    state->ups.loadPercent = "2147483647";
    state->ups.lowBatVol = "2147483647";
    state->ups.lowBatWarn = "2147483647";
    state->ups.mainsVol = "2147483647";
    state->ups.mainsVolAbnor = "2147483647";
    state->ups.outVol = "2147483647";
    state->ups.runState = "2147483647";
    state->ups.tempture = "2147483647";

    state->powSupply.acInPhaseIa = "2147483647";
    state->powSupply.acInPhaseIb = "2147483647";
    state->powSupply.acInPhaseIc = "2147483647";
    state->powSupply.acInPhaseUa = "2147483647";
    state->powSupply.acInPhaseUb = "2147483647";
    state->powSupply.acInPhaseUc = "2147483647";
    state->powSupply.rectifierOutCurr = "2147483647";
    state->powSupply.rectifierOutTemp = "2147483647";
    state->powSupply.rectifierOutVol = "2147483647";
    state->powSupply.warning = "2147483647";
    state->powSupply.warnTime = "2147483647";

}

static string getResFromJson(CJsonObject res,string id){
    if(res.GetArraySize() <= 0)
        return "";

    uint8_t i;
    string getId,val;
    for(i = 0;i < res.GetArraySize();i++){
        if(res[i].Get("Id",getId) && getId == id){
            res[i].Get("Value",val);
            return val;
        }
    }

    return "";
}

static string getWarnFromJson(CJsonObject res,string id,string *time){
    if(res.GetArraySize() <= 0)
        return "0";

    uint8_t i;
    string getId,val;
    for(i = 0;i < res.GetArraySize();i++){
        if(res[i].Get("DeviceId",getId) && getId == id){
            if(time != NULL){
                res[i].Get("AlarmTime",*time);
            }
            return "1";
        }
    }

    return "0";
}

static void crc(uint8_t *pucData,uint8_t wLength,uint8_t *pOutData)
{
	uint8_t ucTemp;
	uint16_t wValue;
	uint16_t crc_tbl[16]={0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef};  //四位余式表

	wValue=0;

	//本字节的CRC余式等于上一字节的CRC余式的低12位左移4位后，
	//再加上上一字节CRC余式右移4位（也既取高4位）和本字节之和后所求得的CRC码
	while(wLength--!=0)
	{
	//根据四位CRC余式表，先计算高四位CRC余式

	ucTemp=((wValue>>8))>>4;
	wValue<<=4;
	wValue^=crc_tbl[ucTemp^((*pucData)>>4)];
	//再计算低四位余式
	ucTemp=((wValue>>8))>>4;
	wValue<<=4;
	wValue^=crc_tbl[ucTemp^((*pucData)&0x0f)];
	pucData++;
	}
	pOutData[0]=wValue;
	pOutData[1]=(wValue>>8);
}

static uint8_t cmdpack(uint8_t addr, uint16_t cmd, uint8_t *buf) {
    uint8_t *pbuf = buf;
    uint8_t len = 0;
    uint8_t crc_cal[2];

    pbuf[len++] = 0x7E;
    pbuf[len++] = addr;
    pbuf[len++] = 1;
    pbuf[len++] = 0;
    pbuf[len++] = 0;
    pbuf[len++] = 0x00;
    pbuf[len++] = 0x02;
    pbuf[len++] = (cmd >> 8) & 0xFF;
    pbuf[len++] = cmd & 0xFF;
    crc(&buf[1], len - 1, crc_cal);
    pbuf[len++] = crc_cal[1]; // 高位要在前
    pbuf[len++] = crc_cal[0];
    pbuf[len++] = 0x7E;
    pbuf[len++] = 0x7E;

    // printf("zte Locker data begins\r\n ");
    // uint16_t j;
    // for (j = 0; j < len; j++)
    //     printf("0x%02x ", pbuf[j]);
    // printf("\r\n");

    return len;
}

/* 返回时间戳 */
static uint32_t getTickCount(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((ts.tv_sec * 1000 + ts.tv_nsec / 1000000) / 1000);
}


#undef SUPERVISION_PRIVATE


/* 温湿度 */
//"118201001" /* 温度 */
//"118204001"/* 湿度 */

/* 普通空调 */
//"115236001"/*普通空调送风温度
//"115235001"/* 空调开机温度点 */
//"115232001"/* 空调加热开启温度 */
//"115009001"/* 空调内风机状态 */
//"115010001"/* 空调外风机状态 */
//"115276001")//普通空调柜内温度
//"115281001"/* 空调开机温度点 */
//"115282001"/* 空调关机温度点 */
//"115170001"/* 空调制冷状态 */
//"115293001"/* 空调内风机告警 */
//"115273001"/* 空调%d外风机告警 */
//"115275001"/* 空调%d蒸发器冻结告警 */
//"115283001"/* 空调加热开启温度 */
//"115284001"/* 空调加热停止温度 */

/* 开关电源 */
//"106030001" || "106126001" //整流模块输出电压
//"106100001" || "106127001" //整流模块输出电流
//"106101001" || "106128001"//整流模块温度
//"106206001"//交流输入相电压Ua
//"106207001"//交流输入相电压Ub
//"106208001"//交流输入相电压Uc
//"106339001"//交流输出电流Ia
//"106340001"//交流输出电流Ib
//"106341001"//交流输出电流Ic

/* UPS */
//"108035001"//Ups市电电压
//"108036001"//Ups输出电压
//"108011001"//Ups温度
//"108037001"//UPS负载百分比
//"108056001"//UPS输入频率
//"108304001"//UPS是否旁路
//"108308001"//UPS开关机状态
//"108302001"//ups故障状态报警
//"108303001"//电池电压低报警
//"108305001"//Ups市电异常 //交流电源输入停电告警
//"108035001")  //输入电压值 222.2
//"108012001")  //电池电压
//"108301001"//电压下限

/* 锂电池 */
//"190004001"//电池组电压# 49.35
//"190264001"//剩余容量# 101.95
//"190265001"//满充容量# 102.76
//"190002001"//单体温度# 27.7
