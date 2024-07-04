#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <unistd.h>
#include "stream.h"
#include "stream-tcp.h"
#include "iac.h"

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

typedef struct streamTcpStatus_t{
    int st;
    int sck;
    int con;
    char c;
    int n;
    int isOnline;
}streamTcpStatus_t;

static streamTcpStatus_t streamTcpStatus = {
    .st = 0,
    .con = 0,
    .isOnline = 0
};

static int  streamTCPDataAvailable(streamTcpStatus_t *tcpStatus);
static int  streamTCPGetChar      (streamTcpStatus_t *tcpStatus, char *c);
static int  streamTCPSendChar     (streamTcpStatus_t *tcpStatus, char c);
static int  streamTCPWrite        (streamTcpStatus_t *tcpStatus, char *buf, int len);
static int  streamTCPClearInput   (streamTcpStatus_t *tcpStatus);
static int  streamTCPFlushOutput  (streamTcpStatus_t *tcpStatus);
static int  streamTCPEnableRawMode(streamTcpStatus_t *tcpStatus);
static void streamTCPTask         (streamTcpStatus_t *tcpStatus);
static int  streamTCPIsOnline     (streamTcpStatus_t *tcpStatus);

static const stream_t streamTemplate = {
    .getChar       = ( int (*)(void *, char *c)    )streamTCPGetChar,
    .clearInput    = ( int (*)(void *)             )streamTCPClearInput,
    .dataAvailable = ( int (*)(void *)             )streamTCPDataAvailable,
    .flushOutput   = ( int (*)(void *)             )streamTCPFlushOutput,
    .sendChar      = ( int (*)(void *, char)       )streamTCPSendChar,
    .task          = (void (*)(void *)             )streamTCPTask,
    .write         = (int  (*)(void *, char *, int))streamTCPWrite,
    .isOnline      = (int  (*)(void *)             )streamTCPIsOnline,
    .custom        = NULL
};

void streamTcpInit(void){

}

static stream_t *streamTcpNewCli(streamTcpStatus_t *status){
    streamTcpStatus_t *tcpStatus = calloc(1, sizeof(streamTcpStatus_t));
    if(!tcpStatus)
        return NULL;

    stream_t *stream = calloc(1, sizeof(stream_t ));
    if(!stream){
        free(status);
        return NULL;
    }

    *stream = streamTemplate;
    *tcpStatus = *status;
    stream->custom = tcpStatus;
    return stream;
}

static void streamTcpMainTask(void){
    char c;
    int len;
    struct sockaddr_in servaddr, cli;
    unsigned char startCmd[] = {
            IAC,DO,LINEMODE,
            IAC,DONT, ECHO,
            IAC, WILL, ECHO,
            '\n'
    };

    switch(streamTcpStatus.st){
        case 0:
                streamTcpStatus.sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (streamTcpStatus.sck == -1) {
                    sleep(5);
                }
                else
                    streamTcpStatus.st = 1;
            break;
        case 1:
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            servaddr.sin_port = htons(PORT);
            int optval= 1;
            setsockopt(streamTcpStatus.sck, SOL_SOCKET, SO_REUSEADDR,(void *)&optval,sizeof(optval));
            if ((bind(streamTcpStatus.sck, (SA*)&servaddr, sizeof(servaddr))) != 0) {
                sleep(5);
            }
            else
                streamTcpStatus.st = 2;
            break;
        case 2:
            if ((listen(streamTcpStatus.sck, 1)) != 0) {
                sleep(5);
            }
            else
                streamTcpStatus.st = 3;
            break;
        case 3:
            len = sizeof(cli);
            streamTcpStatus.con = accept(streamTcpStatus.sck, (SA*)&cli, (socklen_t *)&len);
            if (streamTcpStatus.con < 0) {
                usleep(10000);
            }
            else {
                streamTcpStatus.st = 4;
                streamTcpStatus.isOnline = 1;
                int yes = 1;
                setsockopt(streamTcpStatus.con, IPPROTO_TCP, TCP_NODELAY, (char *) &yes,  sizeof(int)); // 1 - on, 0 - off
                int status = fcntl(streamTcpStatus.con, F_SETFL, fcntl(streamTcpStatus.con, F_GETFL, 0) | O_NONBLOCK);
            }
            break;
        case 4:
            write(streamTcpStatus.con, startCmd, sizeof(startCmd));
            usleep(500000);
            while(read(streamTcpStatus.con, &c, 1) > 0){
                printf("%02X ", (unsigned char)c);
            };
            printf("----\r\n");
            streamTcpStatus.st = 5;
            break;
        case 5:
            /*
             * Check for connection
             * Restore connection
             * End connection and exit
             * Change blocking and waiting to immediate
             * */
            break;
        case 6:
            close(streamTcpStatus.con);
            close(streamTcpStatus.sck);
            streamTcpStatus.st = 0;
            break;
    }
}

void streamTCPTask(streamTcpStatus_t *tcpStatus){
    return;
}

static int streamTcpGetChar(streamTcpStatus_t *tcpStatus, char *c){
    int recvRet;

    if(streamTcpStatus.isOnline){
        recvRet = recv(streamTcpStatus.con, c, 1, MSG_DONTWAIT);
        if(recvRet == 0){
            streamTcpStatus.st = 6;
            return 0;
        }
        return recvRet;
    }
    return 0;
}

static int streamTcpDataAvailable(streamTcpStatus_t *tcpStatus){
    int bytes;
    int ret, ret2;
    char buf;

    if(streamTcpStatus.isOnline){
        ret = recv(streamTcpStatus.con, &buf, 0, MSG_DONTWAIT);
        ret2 = ioctl(streamTcpStatus.con,FIONREAD,&bytes);
        if(ret == 0 && bytes == 0){
            streamTcpStatus.st = 6;
            return 0;
        }

        if(ret2 >= 0){
            return bytes;
        }
    }
    return 0;
}

static int streamTcpSendChar(streamTcpStatus_t *tcpStatus, char c){
    if(streamTcpStatus.isOnline){
        return send(streamTcpStatus.con, &c, 1, 0);
    }
    return 0;
}

static int streamTcpWrite(streamTcpStatus_t *tcpStatus, char *buf, int len){
    if(streamTcpStatus.isOnline){
        return send(streamTcpStatus.con, buf, (size_t)len, 0);
    }
    return 0;
}

static int streamTcpClearInput(streamTcpStatus_t *tcpStatus){
    return 0;
}

static int streamTcpFlushOutput(streamTcpStatus_t *tcpStatus){

}

static int streamTcpEnableRawMode(streamTcpStatus_t *tcpStatus){

}
static int streamTcpIsOnline(streamTcpStatus_t *tcpStatus){
    return streamTcpStatus.isOnline;
}
