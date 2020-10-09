#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>  // inet_ntoa
#include <string.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "websocket.h"

#define BUFFER_SIZE 100*1024
#define RESPONSE_HEADER_LEN_MAX 1024
#define RECV_BUFFER_SIZE 10*1024*1024 //升级文件
#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define BACKLOG 	10

int webfd_A[BACKLOG];    // accepted connection fd
int webconn_amount;      // current connection amount
int webdata_conamount;

extern bool jsonStrReader(char* jsonstrin, int lenin, char* jsonstrout, int *lenout);
extern bool SoftwareUpdate(unsigned char *pbuf, int size, string &strjsonback);

/*-------------------------------------------------------------------
0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
--------------------------------------------------------------------*/
typedef struct _frame_head {
    char fin;
    char opcode;
    char mask;
    unsigned long long payload_length;
    char masking_key[4];
}frame_head;

int passive_server(int port,int queue)
{
    ///定义sockfd
	int result,nlen;
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
    int yes = 1;
    int server_sockfd ;
	if( (server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		printf ("ERROR: Failed to obtain WebSocket Despcritor.\n");
		return (0);
	} 
	else 
	{
		printf ("OK: Obtain WebSocket Despcritor sucessfully.\n");
	}
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
    	printf ("ERROR: Failed to setsockopt.\n");
    	return (0);
    }


    ///定义sockaddr_in
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ///bind，成功返回0，出错返回-1
    if(bind(server_sockfd,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr))==-1)
    {
        printf("ERROR: Failed to bind\n");
    	return (0);
    }
	else 
	{
		printf("OK: Bind the Port %d sucessfully.\n",port);
	}
	    ///listen，成功返回0，出错返回-1
    if(listen(server_sockfd,queue) == -1)
    {
        printf("ERROR: Failed to listen potr:%d\n",port);
    	return (0);
    }
	else 
	{
		printf ("OK: Listening the Port %d sucessfully.\n", port);
	}
	
	//hym 2019.05.08
	int nSendBuffLen = BUFFER_SIZE;//100K;
	int nRecvBuffLen = BUFFER_SIZE;
	nlen = 4;
	result = setsockopt(server_sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&nSendBuffLen, nlen);
	result = setsockopt(server_sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&nRecvBuffLen, sizeof(int));

	result = setsockopt(server_sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout,	sizeof(struct timeval));

	int keepAlive = 1;//
	int keepIdle = 5;//
	int keepInterval = 5;//
	int keepCount = 3;//
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*) &keepAlive,
			sizeof(keepAlive)) == -1) {
		printf("SO_KEEPALIVE Error!\n");
	}

	if (setsockopt(server_sockfd, SOL_TCP, TCP_KEEPIDLE, (void *) &keepIdle,
			sizeof(keepIdle)) == -1) {
		printf("TCP_KEEPIDLE Error!\n");
	}

	if (setsockopt(server_sockfd, SOL_TCP, TCP_KEEPINTVL, (void *) &keepInterval,
			sizeof(keepInterval)) == -1) {
		printf("TCP_KEEPINTVL Error!\n");
	}

	if (setsockopt(server_sockfd, SOL_TCP, TCP_KEEPCNT, (void *) &keepCount,
			sizeof(keepCount)) == -1) {
		printf("TCP_KEEPCNT Error!\n");
	}
	//hym end
    return server_sockfd;
}

int base64_encode(char *in_str, int in_len, char *out_str)
{
    BIO *b64, *bio;
    BUF_MEM *bptr = NULL;
    size_t size = 0;

    if (in_str == NULL || out_str == NULL)
        return -1;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, in_str, in_len);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bptr);
    memcpy(out_str, bptr->data, bptr->length);
    out_str[bptr->length-1] = '\0';
    size = bptr->length;

    BIO_free_all(bio);
    return size;
}

/**
 * @brief _readline
 * read a line string from all buffer
 * @param allbuf
 * @param level
 * @param linebuf
 * @return
 */
int _readline(char* allbuf,int level,char* linebuf)
{
    int len = strlen(allbuf);
    for (;level<len;++level)
    {
        if(allbuf[level]=='\r' && allbuf[level+1]=='\n')
            return level+2;
        else
            *(linebuf++) = allbuf[level];
    }
    return -1;
}

int shakehands(int cli_fd)
{
    //next line's point num
    int level = 0;
    //all request data
    char buffer[BUFFER_SIZE];
    //a line data
    char linebuf[256];
    //Sec-WebSocket-Accept
    char sec_accept[32];
    //sha1 data
    char sha1_data[SHA_DIGEST_LENGTH+1]={0};
    //reponse head buffer
    char head[RESPONSE_HEADER_LEN_MAX] = {0};

    if (read(cli_fd,buffer,sizeof(buffer))<=0)
        perror("read");
    printf("request\n");
    printf("%s\n",buffer);

    do {
        memset(linebuf,0,sizeof(linebuf));
        level = _readline(buffer,level,linebuf);
        //printf("line:%s\n",linebuf);

        if (strstr(linebuf,"Sec-WebSocket-Key")!=NULL)
        {
            strcat(linebuf,GUID);
//            printf("key:%s\nlen=%d\n",linebuf+19,strlen(linebuf+19));
            SHA1((unsigned char*)&linebuf+19,strlen(linebuf+19),(unsigned char*)&sha1_data);
//            printf("sha1:%s\n",sha1_data);
            base64_encode(sha1_data,strlen(sha1_data),sec_accept);
//            printf("base64:%s\n",sec_accept);
            /* write the response */
            sprintf(head, "HTTP/1.1 101 Switching Protocols\r\n" \
                          "Upgrade: websocket\r\n" \
                          "Connection: Upgrade\r\n" \
                          "Sec-WebSocket-Accept: %s\r\n" \
                          "\r\n",sec_accept);

            printf("response\n");
            printf("%s",head);
            if (write(cli_fd,head,strlen(head))<0)
                perror("write");

            break;
        }
    }while((buffer[level]!='\r' || buffer[level+1]!='\n') && level!=-1);
    return 0;
}

int recv_frame_head(int fd,frame_head* head)
{
    char one_char;
    /*read fin and op code*/
    if (read(fd,&one_char,1)<=0)
    {
        perror("read fin");
        return -1;
    }
    head->fin = (one_char & 0x80) == 0x80;
    head->opcode = one_char & 0x0F;
    if (read(fd,&one_char,1)<=0)
    {
        perror("read mask");
        return -1;
    }
    head->mask = (one_char & 0x80) == 0X80;

    /*get payload length*/
    head->payload_length = one_char & 0x7F;

    if (head->payload_length == 126)
    {
        char extern_len[2];
        if (read(fd,extern_len,2)<=0)
        {
            perror("read extern_len");
            return -1;
        }
        head->payload_length = (extern_len[0]&0xFF) << 8 | (extern_len[1]&0xFF);
    }
    else if (head->payload_length == 127)
    {
        char extern_len[8],temp;
        int i;
        if (read(fd,extern_len,8)<=0)
        {
            perror("read extern_len");
            return -1;
        }
        for(i=0;i<4;i++)
        {
            temp = extern_len[i];
            extern_len[i] = extern_len[7-i];
            extern_len[7-i] = temp;
        }
        memcpy(&(head->payload_length),extern_len,8);
    }

    /*read masking-key*/
    if (read(fd,head->masking_key,4)<=0)
    {
        perror("read masking-key");
        return -1;
    }

    return 0;
}

/**
 * @brief umask
 * xor decode
 * @param data
 * @param len
 * @param mask
 */
void umask(char *data,int len,char *mask)
{
    int i;
    for (i=0;i<len;++i)
        *(data+i) ^= *(mask+(i%4));
}

int send_frame_head(int fd,frame_head* head)
{
    char *response_head;
    int head_length = 0;
    if(head->payload_length<126)
    {
        response_head = (char*)malloc(2);
        response_head[0] = 0x81;
        response_head[1] = head->payload_length;
        head_length = 2;
    }
    else if (head->payload_length<0xFFFF)
    {
        response_head = (char*)malloc(4);
        response_head[0] = 0x81;
        response_head[1] = 126;
        response_head[2] = (head->payload_length >> 8 & 0xFF);
        response_head[3] = (head->payload_length & 0xFF);
        head_length = 4;
    }
    else
    {
        //no code
        response_head = (char*)malloc(12);
//        response_head[0] = 0x81;
//        response_head[1] = 127;
//        response_head[2] = (head->payload_length >> 8 & 0xFF);
//        response_head[3] = (head->payload_length & 0xFF);
        head_length = 12;
    }

    if(write(fd,response_head,head_length)<=0)
    {
        perror("write head");
        return -1;
    }

    free(response_head);
    return 0;
}

int WebSocketSend(int s,char *pbuffer,int nsize)
{
  int nSendLen = 0;
  int nLen = 0;
  while(nSendLen < nsize) {
	nLen = write(s, pbuffer+nSendLen, nsize - nSendLen);
	if(nLen<=0)
	  break;
	nSendLen += nLen;
  }
  return nSendLen;
}

void *WebSocketthread(void *param)
{
	struct sockaddr_in client_addr;
	//char buffer[BUFFER_SIZE];
	char *buffer=(char*)malloc(RECV_BUFFER_SIZE);
	char retbuf[BUFFER_SIZE];
	int size,retbuflen,sizetmp;
	char *pData = NULL;
	bool ret=false;
	int i,rul;
	frame_head head;
	socklen_t sin_size;
	int newfd;               		// New Socket file descriptor
	int sockfd;                        	// Socket file descriptor
	string mstrdata;
	
    sockfd = passive_server(8082,20);

	fd_set fdsr;
	int maxsock;
	struct timeval tv;

	webconn_amount = 0;
	sin_size = sizeof(client_addr);
	maxsock = sockfd;
	
	//初始化缓冲区
	memset(buffer,0,RECV_BUFFER_SIZE);
	memset(retbuf,0,BUFFER_SIZE);
	pData = buffer;
	size = 0;
	
	while(1)
	{
		FD_ZERO(&fdsr);
		FD_SET(sockfd, &fdsr);

		// timeout setting
		tv.tv_sec = 30;
		tv.tv_usec = 0;

		// add active connection to fd set
		for (i = 0; i < BACKLOG; i++) {
			if (webfd_A[i] != 0) {
				FD_SET(webfd_A[i], &fdsr);
				if(maxsock< webfd_A[i])
					maxsock = webfd_A[i];
			}
		}

		ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
		} else if (ret == 0) {

			continue;
		}

		for (i = 0; i < BACKLOG; i++)
		{
			if (0 < webfd_A[i] && FD_ISSET(webfd_A[i], &fdsr))
			{
				{
			        rul = recv_frame_head(webfd_A[i],&head);
			        if (rul < 0)
					{ // client close
						printf("web client[%d] close\n", i);
						close(webfd_A[i]);
						FD_CLR(webfd_A[i], &fdsr);
						webfd_A[i] = 0;
						webconn_amount--;
			            break;
					}
			        printf("fin=%d opcode=0x%X mask=%d payload_len=%llu\n",head.fin,head.opcode,head.mask,head.payload_length);

					sizetmp=0;
			        do {
			            int rul;
			            rul = read(webfd_A[i],pData,head.payload_length-sizetmp);
			            if (rul<0)
			                break;
						pData+=rul;
			            size+=rul;
						sizetmp+=rul;
			        }while(sizetmp<head.payload_length);

//					for(int j=0;j<size;j++)
//						printf("%02x ",buffer[j]);
					printf("sizetmp=%d, size=: %d \n",sizetmp,size);
					if (sizetmp == head.payload_length)
					{
//						printf("%02x ",head.masking_key);
						umask(pData-sizetmp,sizetmp,head.masking_key);
//						printf("recive:%s\n",buffer);
						//解析发来的JSON请求，打包请求数据
						if (head.fin == 1 && size>0)
						{
							ret=jsonStrReader(buffer,size,retbuf,&retbuflen);
							printf("ret=%d retbuf=%s retbuflen=%d\n",ret,retbuf,retbuflen);
							if(ret==true)
							{
								//echo head
								head.payload_length=retbuflen;
								send_frame_head(webfd_A[i],&head);
								WebSocketSend(webfd_A[i],retbuf,retbuflen);
							}
							else
							{
								for(int j=0;j<10;j++)
									printf("%02x ",buffer[j]);
								printf("\n");
								for(int j=size-10;j<size;j++)
									printf("%02x ",buffer[j]);
								printf("\n");
								
								SoftwareUpdate((unsigned char*)buffer,size,mstrdata);
								//echo head
								head.payload_length=mstrdata.size();
								send_frame_head(webfd_A[i],&head);
								WebSocketSend(webfd_A[i],(char *)(mstrdata.c_str()),mstrdata.size());
							}
							//重新初始化缓冲区
							memset(buffer,0,RECV_BUFFER_SIZE);
							memset(retbuf,0,BUFFER_SIZE);
							pData = buffer;
							size = 0;
						}
					}
				}
			}
		}
		
		if (FD_ISSET(sockfd, &fdsr))
		{
			//printf("sockfd*****\r\n");
			newfd = accept(sockfd, (struct sockaddr *) &client_addr,&sin_size);
			shakehands(newfd);
			printf("newfd%d\r\n",newfd);
			if (newfd <= 0)
			{
				perror("accept");
				continue;
			}

			// add to fd queue
			{
				//find empty
				for (i = 0; i < BACKLOG; i++)
				{
					if (webfd_A[i] == 0)
					{
						webfd_A[i] = newfd;
						webconn_amount++;
						printf("new web connection client[%d] %s:%d\n", i, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						break;
					}
				}

				if(i==BACKLOG)
				{
					printf("[%s:%d]not enough array to accept the socket %d\n", __FILE__, __LINE__, newfd);
					close(newfd);
				}
			}
		}
	}
    close(sockfd);
	free(buffer);
	return 0 ;
} 

/*void *WebSocketthread(void *param)
 {
	 struct sockaddr_in client_addr;
	 //char buffer[BUFFER_SIZE];
	 char *buffer=(char*)malloc(RECV_BUFFER_SIZE);;
	 char retbuf[BUFFER_SIZE];
	 int size,retbuflen;
	 char *pData = NULL;
	 bool ret=false;
	 int i,rul;
	 frame_head head;
	 socklen_t sin_size;
	 int newfd; 					 // New Socket file descriptor
	 int sockfd;						 // Socket file descriptor
	 
	 sockfd = passive_server(8082,20);
 
	 fd_set fdsr;
	 int maxsock;
	 struct timeval tv;
 
	 webconn_amount = 0;
	 sin_size = sizeof(client_addr);
	 maxsock = sockfd;
	 
	 while(1)
	 {
		 FD_ZERO(&fdsr);
		 FD_SET(sockfd, &fdsr);
 
		 // timeout setting
		 tv.tv_sec = 30;
		 tv.tv_usec = 0;
 
		 // add active connection to fd set
		 for (i = 0; i < BACKLOG; i++) {
			 if (webfd_A[i] != 0) {
				 FD_SET(webfd_A[i], &fdsr);
				 if(maxsock< webfd_A[i])
					 maxsock = webfd_A[i];
			 }
		 }
 
		 ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
		 if (ret < 0) {
			 perror("select");
		 } else if (ret == 0) {
 
			 continue;
		 }
 
		 for (i = 0; i < BACKLOG; i++)
		 {
			 if (0 < webfd_A[i] && FD_ISSET(webfd_A[i], &fdsr))
			 {
				 {
					 rul = recv_frame_head(webfd_A[i],&head);
					 if (rul < 0)
					 { // client close
						 printf("web client[%d] close\n", i);
						 close(webfd_A[i]);
						 FD_CLR(webfd_A[i], &fdsr);
						 webfd_A[i] = 0;
						 webconn_amount--;
						 break;
					 }
					 printf("fin=%d opcode=0x%X mask=%d payload_len=%llu\n",head.fin,head.opcode,head.mask,head.payload_length);
 
					 //read payload data
					 memset(buffer,0,BUFFER_SIZE);
					 memset(retbuf,0,BUFFER_SIZE);
					 pData = buffer;
					 size = 0;
					 do {
						 int rul;
						 rul = read(webfd_A[i],pData,head.payload_length-size);
						 if (rul<0)
							 break;
						 pData+=rul;
						 size+=rul;
					 }while(size<head.payload_length);
 
 // 				 for(int j=0;j<size;j++)
//						 printf("%02x ",buffer[j]);
					 printf("size: %d \n",size);
					 if (size == head.payload_length)
					 {
						 umask(buffer,size,head.masking_key);
 // 					 printf("recive:%s\n",buffer);
						 //解析发来的JSON请求，打包请求数据
						 ret=jsonStrReader(buffer,size,retbuf,&retbuflen);
						 printf("ret=%d retbuf=%s retbuflen=%d\n",ret,retbuf,retbuflen);
						 if(ret==true)
						 {
							 //echo head
							 head.payload_length=retbuflen;
							 send_frame_head(webfd_A[i],&head);
							 WebSocketSend(webfd_A[i],retbuf,retbuflen);
						 }
					 }
				 }
			 }
		 }
		 
		 if (FD_ISSET(sockfd, &fdsr))
		 {
			 //printf("sockfd*****\r\n");
			 newfd = accept(sockfd, (struct sockaddr *) &client_addr,&sin_size);
			 shakehands(newfd);
			 printf("newfd%d\r\n",newfd);
			 if (newfd <= 0)
			 {
				 perror("accept");
				 continue;
			 }
 
			 // add to fd queue
			 {
				 //find empty
				 for (i = 0; i < BACKLOG; i++)
				 {
					 if (webfd_A[i] == 0)
					 {
						 webfd_A[i] = newfd;
						 webconn_amount++;
						 printf("new web connection client[%d] %s:%d\n", i, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						 break;
					 }
				 }
 
				 if(i==BACKLOG)
				 {
					 printf("[%s:%d]not enough array to accept the socket %d\n", __FILE__, __LINE__, newfd);
					 close(newfd);
				 }
			 }
		 }
	 }
	 close(sockfd);
	 free(buffer);
	 return 0 ;
 } */
 
 void init_WebSocket()
 {
	pthread_t m_WebSocketthread ;
	pthread_create(&m_WebSocketthread,NULL,WebSocketthread,NULL);
 }


