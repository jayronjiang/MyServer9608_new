#ifndef __TCP_H_
#define __TCP_H_

/*******************************************************************************
 * includes
 ******************************************************************************/
#include <map>
#include <pthread.h>
#include <stdint.h>
#include <string>

/*******************************************************************************
 * class
 ******************************************************************************/
/* Base class */
class Tcp {
public:
    typedef enum {
        Event_DataRecv = 0,
        Event_SocConnect = 1,
        Event_SocDisconnect = 2,
    } Event_EN;

    typedef uint16_t Socket_t;

    /* 回调函数 */
    typedef void (*Callback)(Tcp *tcp, Socket_t soc, Event_EN event, uint8_t *data, uint32_t size, void *userdata);

protected:
    typedef struct {
        Socket_t soc;
        int conn;
        uint16_t port;
        std::string ip;
        pthread_t tid;

        Callback cb;
        void *userdata;

        Tcp *tcp;
    } Socket_S;

    typedef enum { Type_Server, Type_Client } TcpType_EN;

    std::string ip;
    uint16_t port;
    int soc;
    pthread_t tid;

    Callback callback;
    void *userdata;

    const TcpType_EN type;

protected:
    Tcp(TcpType_EN t) : type(t){};

    static void *SocketTask(void *arg);

public:
    virtual ~Tcp(){};
    virtual void transmit(Socket_t soc, const uint8_t *buf, uint16_t size){};
    virtual void transmit(Socket_t soc, std::string str){};
    virtual void transmit(const uint8_t *buf, uint16_t size){};
    virtual void transmit(std::string str){};
    virtual void closeSocket(Socket_t soc){};
    virtual void closeSocket(){};
    virtual int isSocketConnect() {
        return 0;
    };

    void setCallback(Callback cb, void *userdata);
};

/* Tcp Server */
class TcpServer : public Tcp {

public:
    TcpServer(uint16_t port);
    TcpServer(std::string ip, uint16_t port);
    ~TcpServer();

    void transmit(Socket_t soc, const uint8_t *buf, uint16_t size);
    void transmit(Socket_t soc, std::string str);
    void transmit(const uint8_t *buf, uint16_t size);
    void transmit(std::string str);

    void closeSocket(Socket_t soc);
    void closeSocket();

    int isSocketConnect();

private:
    uint16_t socCount;
    std::map<uint16_t, Socket_S *> *socketList;

    static void *ServTask(void *arg);
};

/* Tcp Client */
class TcpClient : public Tcp {

public:
    TcpClient(std::string ip, uint16_t port);
    ~TcpClient();

    void transmit(Socket_t soc, const uint8_t *buf, uint16_t size);
    void transmit(Socket_t soc, std::string str);
    void transmit(const uint8_t *buf, uint16_t size);
    void transmit(std::string str);

    void closeSocket(Socket_t soc);
    void closeSocket();

    int isSocketConnect();

private:
    Socket_S *connSoc;

    static void *ClientTask(void *arg);
};

/**
 * DEMO

int main(int argc,char **argv){
    Tcp *serv = new Server(50001);
    serv->setCallback(Callback,NULL);
    serv->transmit(1,"hello client");
    serv->closeSocket(1);

    Tcp *client = new Client("128.8.82.189",49999);
    client->setCallback(Callback,NULL);
    client->transmit("bey server\n");
    client->closeSocket();
}



void Callback(Tcp *tcp,Tcp::Socket_t soc,Tcp::Event_EN event,uint8_t *data,uint32_t size,void *userdata){
    printf("Server soc(%d) event = %d\n",soc,event);

    if(event == Tcp::Event_DataRecv){
        printf("%s\n",data);

        tcp->transmit(soc,"Acknowage!!\n");
    }
}

*/

#endif