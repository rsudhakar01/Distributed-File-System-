#include <stdio.h>
#include "udp.h"

#include "message.h"

// server code
int main(int argc, char *argv[]) {
    int sd = UDP_Open(10000);
    assert(sd > -1);
    while (1) {
	struct sockaddr_in addr;

	message_t m;
	
	printf("server:: waiting...\n");
	int rc = UDP_Read(sd, &addr, (char *) &m, sizeof(message_t));
	printf("server:: read message [size:%d contents:(%d)]\n", rc, m.mtype);
	if (rc > 0) {
	    m.rc = 3;
            rc = UDP_Write(sd, &addr, (char *) &m, sizeof(message_t));
	    printf("server:: reply\n");
	} 
    }
    return 0; 
}
    


 
