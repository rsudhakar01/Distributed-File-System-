#include <stdio.h>
#include "message.h"
#include "mfs.h"
#include "udp.h"
#include <time.h>
#include <stdlib.h>
#include "sys/stat.h"

int MIN_PORT = 20000;
int MAX_PORT = 40000;
int port_num;
struct sockaddr_in addrSnd, addrRcv;
int sd, rc;

int port_initialize(){
    srand(time(0));
    int port_num;
    port_num = rand() % (MAX_PORT - MIN_PORT) + MIN_PORT;
    return port_num;
}

int MFS_Init(char *hostname, int port) {
    printf("MFS Init2 %s %d\n", hostname, port);
    port_num = port_initialize();
    sd = UDP_Open(port_num);
    if (sd < 0){
		return -1;
	}
    int rc = UDP_FillSockAddr(&addrSnd, hostname, port);
    //tell server to open port?
    message_t to_send;
    to_send.mtype = 1;
    //filler for message
    to_send.message[0] = port;
    to_send.message[sizeof(port)] = '\0';
    //int rc = UDP_Write(sd, &addrSnd,(char *) &to_send, sizeof(message_t));
    //rc = UDP_Write(sd, &addrSnd,(char *) &to_send, sizeof(message_t));
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    //UDP_Read(sd, &addrRcv, (char *)&to_send, sizeof(message_t));
    return to_send.rc;
}

int MFS_Lookup(int pinum, char *name) {
    // network communication to do the lookup to server
    // sending message in format: namepinum
    // buffer for name, pinum, and null terminator
    message_t to_send;
    to_send.mtype = 2;
    strcpy(to_send.message, name);
    to_send.message[sizeof(name)] = pinum;
    to_send.message[sizeof(name) + 4] = '\0';
    int rc = UDP_Write(sd, &addrSnd, (char *)&to_send, sizeof(message_t));
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    //implement timeout
    UDP_Read(sd, &addrRcv, (char *)&to_send, sizeof(message_t));
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
