#include <stdio.h>
#include "udp.h"

#include "message.h"

// client code
int main(int argc, char *argv[]) {
    struct sockaddr_in addrSnd, addrRcv;

    int sd = UDP_Open(20000);
    int rc = UDP_FillSockAddr(&addrSnd, "localhost", 10000);

    message_t m;

    m.mtype = MFS_READ;
    printf("client:: send message %d\n", m.mtype);
    rc = UDP_Write(sd, &addrSnd, (char *) &m, sizeof(message_t));
    if (rc < 0) {
	printf("client:: failed to send\n");
	exit(1);
    }

    printf("client:: wait for reply...\n");
    rc = UDP_Read(sd, &addrRcv, (char *) &m, sizeof(message_t));
    printf("client:: got reply [size:%d rc:%d type:%d]\n", rc, m.rc, m.mtype);
    return 0;
}

