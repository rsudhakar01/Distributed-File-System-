#include <stdio.h>
#include "message.h"
#include "mfs.h"
#include <time.h>
#include <stdlib.h>

int MIN_PORT = 20000;
int MAX_PORT = 40000;

srand(time(0));
int port_num = (rand() % (MAX_PORT - MIN_PORT) + MIN_PORT);

struct sockaddr_in addrSnd, addrRcv;
int sd, rc;

int MFS_Init(char *hostname, int port) {
    printf("MFS Init2 %s %d\n", hostname, port);
    sd = UDP_Open(port_num);
    assert(sd > -1);
    UDP_FillSockAddr(&addrSnd, hostname, port);
    //tell server to open port?
    struct message_t to_send;
    to_send.mtype = 1;
    //filler for message
    to_send.message[0] = port;
    to_send.message[sizeof(port)] = '\0';
    int rc_send = UDP_Write(sd, &addrSnd, to_send, 1024);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    UDP_Read(sd, &addrRcv, message, 1024);
    return to_send.rc;
}

int MFS_Lookup(int pinum, char *name) {
    // network communication to do the lookup to server
    // sending message in format: namepinum
    // buffer for name, pinum, and null terminator
    struct message_t to_send;
    to_send.mtype = 2;
    sprintf(to_send.message, name);
    to_send.message[sizeof(name)] = pinum;
    to_send.message[sizeof(name) + 4] = '\0';
    int rc_send = UDP_Write(sd, &addrSnd, to_send, 1024);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    //implement timeout
    UDP_Read(sd, &addrRcv, message, 1024);
    if (to_send.rc < 0){
        return rc;
    }
    return atoi(to_send.message);
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
    return 0;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes) {
    return 0;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes) {
    return 0;
}

int MFS_Creat(int pinum, int type, char *name) {
    return 0;
}

int MFS_Unlink(int pinum, char *name) {
    return 0;
}

int MFS_Shutdown() {
    //printf("MFS Shutdown\n");
    //unsure if we should also call UDP_Close before exit
    //UDP_Close(sd);
    //exit(0);
    message_t client_msg;
    client_msg.mtype = 8;
    rc = UDP_Write(sd, &addrSnd, (char*) &client_msg, sizeof(message_t));
    if(rc < 0){
        return -1;
    }
    return 0;
}
