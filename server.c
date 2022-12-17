#include <stdio.h>
#include "udp.h"
#include <sys/mman.h>
#include "sys/stat.h"
#include "message.h"

int sd, fd, port;
struct sockaddr_in addr_receive;
struct sockaddr_in addr_out;
void *file_ptr;
super_t* sb_pointer;
char* ibitmap_ptr;
char* dbitmap_ptr;
char* iregion_ptr;
char* dregion_ptr;

int image_size;

void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

int server_init(int port, char* img_name){
	sd = UDP_Open(port);
	if (sd < 0){
		return -1;
	}
	fd = open(img_name, O_RDWR);
	if (fd < 0){
		return -1;
	}
	struct stat sbuf;
	int rc = fstat(fd, &sbuf);
	if (rc < 0){
		return -1;
	}
	image_size = (int) sbuf.st_size;
	file_ptr = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (file_ptr == MAP_FAILED){
		return -1;
	}

	//set other pointers
	sb_pointer = (super_t*)file_ptr;
	ibitmap_ptr = (char*)file_ptr + sb_pointer->inode_bitmap_addr*4096;
	dbitmap_ptr = (char*)file_ptr + sb_pointer->data_bitmap_addr*4096;
	iregion_ptr = (char*)file_ptr + sb_pointer->inode_region_addr*4096;
	dregion_ptr = (char*)file_ptr + sb_pointer->data_region_addr*4096;
	fsync(fd);
	return 0;
}

int server_shutdown(message_t* msg){
	fsync(fd);
	close(fd);
	UDP_Close(port);
	exit(0);
	return 0;
}

int main(int argc, char *argv[]) {
	signal(SIGINT, intHandler);
	if (argc !=3) {
		perror("Error");
	}

	port = atoi(argv[1]);
	char img_path[1024];
	strcpy(img_path, argv[2]);

	// loop through, read requests, call appropriate function...
	while (1) {
		int to_do;
		message_t m;
		printf("server:: waiting...\n");
		int rc = UDP_Read(sd, &addr_receive, (char *) &m, sizeof(message_t));
		printf("server:: read message [size:%d contents:(%d)]\n", rc, m.mtype);
		if (rc > 0) {
			to_do = m.mtype;
			switch(to_do){
				case 1:
					{
						server_init(port, img_path);
						break;
					}
				default:
					break;
			} 
    }
}
return 0; 
}
    

 
