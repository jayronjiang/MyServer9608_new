/*************************************************************************
	> File Name: mqtt.h
	> Author: 
	> Mail: 
	> Created Time:
 ************************************************************************/
#ifndef _MQTT_H
#define _MQTT_H

#include <list>
#include <string>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

#include "MQTTClient.h"

typedef enum QoS Qos_en;

typedef void (*SubscribeMsg)(uint8_t *msg,uint16_t msgLen,void *data);

#ifdef __cplusplus
}
#endif


class Mqtt {

	typedef enum {
		IotSta_Login = 0, IotSta_Connect, IotSta_Drop,
	} IotStatus_en;

	typedef struct {
		string content;
		Qos_en qos;
		bool isSub;
	}Subscribe;

	typedef struct {
		string topic;
		string content;
		Qos_en qos;
		bool retained;
	} Publish;

	char *hostIp;
	uint16_t hostPort;
	char *clientId;

	char *user;
	char *pwd;

	uint8_t *txBuffer;
	uint8_t *rxBuffer;

	Network *network;
	Client *client;

	IotStatus_en iotStatus;

	SubscribeMsg subscribeCb;

	pthread_t thread;

	bool subUpdated;
	list<Subscribe> *subList;
	list<Publish> *pubList;

	static void msgCallback(MessageData *msg, void *para);

	static void *mainThread(void *arg);

public:
	void *userData;

public:
	uint8_t init(const char *hostIp, uint16_t hostPort, const char *clientId,
			SubscribeMsg cb, const char *user, const char *pwd);

	uint8_t init(const char *hostIp, uint16_t hostPort, const char *clientId,
			SubscribeMsg cb);

	uint8_t connect(void);
	int disconnect(void);

	void subscribe(string substr,Qos_en qos);
	void subscribe(const char *substr,Qos_en qos);

	void publish(string topic,string pubstr,Qos_en qos,bool retained);
	void publish(const char *topic,const char *pubstr,Qos_en qos,bool retained);

	string getClientId(void);

};


#endif
