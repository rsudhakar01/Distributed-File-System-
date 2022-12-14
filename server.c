#include <stdio.h>
#include "udp.h"

#include "message.h"

int sd, fd, port;
struct sockaddr_in addr_receive;
struct sockaddr_in addr_out;

void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, intHandler);
	//sd = UDP_Open(10000); Not sure if i should remove this and replace with my line below
	assert(sd > -1);
	//opening file image 
	char img_path[1024];
	if (atoi(argv[2]) == '0'){
		//incomplete args
		return -1;
	}
	else{
		img_path = argv[2];
	}
	port = atoi(argv[1]);
	sd = UDP_Open(port);
	fd = open(img_path, O_CREAT | O_RDWR | O_EXCL, 0644);
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
			if (mtype == 1){
				//call init (init has code 1, not 0 from message.h)
			}
			else if(mtype == 8){
				server_shutdown(m);
			}
			else{
				break;
			}
		} 
    }
    return 0; 
}
    
int server_shutdown(message_t* msg){
	fsync(fd);
	close(fd);
	UDP_Close(port);
	exit(0);
	return 0;
}

 
