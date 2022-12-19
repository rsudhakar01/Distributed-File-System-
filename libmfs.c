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

int MFS_Init(char *hostname, int port) {
    printf("MFS Init2 %s %d\n", hostname, port);
    srand(time(0));
    int port_num;
    port_num = rand() % (MAX_PORT - MIN_PORT) + MIN_PORT;
    sd = UDP_Open(port_num);
    if (sd < 0){
		return -1;
	}
    rc = UDP_FillSockAddr(&addrSnd, hostname, port);
    //tell server to open port?
    message_t to_send, to_receive;
    to_receive.rc = -1;
    to_send.mtype = 1;
    //filler for message
    rc = UDP_Write(sd, &addrSnd,(char *) &to_send, sizeof(message_t));
    if (rc < 0) {
        printf("client:: failed to send\n");
        return -1;
    }
    UDP_Read(sd, &addrRcv, (char *)&to_receive, sizeof(message_t));
    return to_receive.rc;
}

int MFS_Lookup(int pinum, char *name) {
    // network communication to do the lookup to server
    // sending message in format: namepinum
    // buffer for name, pinum, and null terminator
    message_t to_send, to_receive;
    to_send.mtype = 2;
    to_send.inum = pinum;
    to_receive.inum = -98;
    strncpy(to_send.message, name, 28);
    rc = UDP_Write(sd, &addrSnd, (char *)&to_send, sizeof(message_t));
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    UDP_Read(sd, &addrRcv, (char *)&to_receive, sizeof(message_t));
    printf("came lookup %i\n", to_receive.inum);
    return to_receive.inum;
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
    if(name == NULL || strlen(name) > 28){
        return -1;
    }
/*     if(pinum < 0){
        return -1;
    } */
    message_t to_send, to_receive;
    to_send.mtype = 6;
    to_send.inum = pinum;
    to_send.dir_type = type;
    to_receive.rc = -932;
    strncpy(to_send.name, name, 28);
    rc = UDP_Write(sd, &addrSnd,(char *) &to_send, sizeof(message_t));
    if (rc < 0) {
        printf("client: create failed\n");
        return -1;
    }
    
    UDP_Read(sd, &addrRcv, (char*)&to_receive, sizeof(message_t));
    printf("came crelookup %i\n", to_receive.rc);
    return to_receive.rc;
}

int MFS_Unlink(int pinum, char *name) {
    return 0;
}

int MFS_Shutdown() {
    message_t to_send, to_receive;
    to_send.mtype = 8;
    to_receive.rc = -1;

    rc = UDP_Write(sd, &addrSnd, (char*) &to_send, sizeof(message_t));
    if(rc < 0){
        return -1;
    }
    UDP_Read(sd, &addrRcv, (char *)&to_receive, sizeof(message_t));
    return to_receive.rc;
}
