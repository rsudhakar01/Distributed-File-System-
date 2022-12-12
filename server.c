#include <stdio.h>
#include "udp.h"

#include "message.h"

int sd;
struct sockaddr_in addr_receive, sockaddr_in addr_out;

void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, intHandler);
	sd = UDP_Open(10000);
	assert(sd > -1);
	//opening file image 
	char img_path[1024];
	if (argv[2] == '0'){
		//incomplete args
		return -1;
	}
	else{
		img_path = argv[2];
	}
	int fd = open(img, O_CREAT | O_RDWR | O_EXCL, 0644);
	struct stat st;
	if(fstat(fd, &st) == -1){
		perror("error can't read size");
	};
	//mapping image to file
	char *file_ptr = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	// loop through, read requests, call appropriate function...
	while (1) {
		int to_do;
		message_t m;
		printf("server:: waiting...\n");
		int rc = UDP_Read(sd, &addr_receive, (char *) &m, sizeof(message_t));
		printf("server:: read message [size:%d contents:(%d)]\n", rc, m.mtype);
		if (rc > 0) {
			to_do = m.mtype;
			if (mtype == 0){
				//call init
			}
		} 
    }
    return 0; 
}
    


 
