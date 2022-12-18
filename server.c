#include <stdio.h>
#include "udp.h"
#include "ufs.h"
#include "mfs.h"
#include <sys/mman.h>
#include "sys/stat.h"
#include "message.h"

int sd, fd, port;
struct sockaddr_in addr_receive;
struct sockaddr_in addr_out;
void *file_ptr;
super_t* sb_pointer;
super_t superblock;
char* ibitmap_ptr;
char* dbitmap_ptr;
char* iregion_ptr;
char* dregion_ptr;

int image_size;

typedef struct {
	dir_ent_t entries[BLOCK_SIZE / sizeof(dir_ent_t)];
} dir_block_t;

void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

//from Kai

unsigned int get_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   return (bitmap[index] >> offset) & 0x1;
}

void set_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= 0x1 << offset;
}

int server_init(char* img_name){
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
	//filling sb_pointer
	read(fd, &superblock, sizeof(super_t));
	fsync(fd);
	return 0;
}

int server_create(message_t m){
	int pinum = m.inum;
	int dt = m.dir_type;
	char filename[28];
	strcpy(filename, m.name);
	inode_t* pinode = malloc(sizeof(inode_t));
	pinode = (inode_t*)(iregion_ptr + pinum*sizeof(inode_t));
	if(pinode->type!= MFS_DIRECTORY){
			return -1;
    }
	//find empty inode
	int check_freeinode = 0;
	for(int i = 0; i < sb_pointer->num_inodes; i++) {
		check_freeinode = get_bit((unsigned int *)ibitmap_ptr, i);
		if(check_freeinode==0) {
			break;
		}
	}
	inode_t* f_inode = (inode_t*)(iregion_ptr + check_freeinode*sizeof(inode_t));

	//find data block
	dir_ent_t*  pinum_db_addr = (dir_ent_t*)(file_ptr + (int)(pinode->direct[0])*4096);
	// move ptr
	int datab_ptr;
	datab_ptr = iregion_ptr[pinum].direct[0] - sb_pointer->dregion_ptr;

	for(int i = 0; i< 4096/sizeof(dir_ent_t); i++) {
		if(pinum_db_addr[datab_ptr].entries[i].inum == -1) {
			pinum_db_addr[datab_ptr].entries[i].inum = check_freeinode;
			strcpy(pinum_db_addr[datab_ptr].entries[i].name, filename);
			set_bit((unsigned int *)ibitmap_ptr, check_freeinode);
			iregion_ptr[pinum].size += sizeof(dir_ent_t);
			break;
		}
	}
	iregion_ptr[check_freeinode].type = dt;

	if(dt == MFS_REGULAR_FILE){
		for(int i = 0; i<DIRECT_PTRS; i++) {
			//find free data block
			int new_datab_ptr = 0;
			for(int i = 0; i<sb_pointer->num_inodes; i++) {
				new_datab_ptr = get_bit((unsigned int *)dbitmap_ptr, i);
				if(new_datab_ptr == 0) {
					break;
				}
			}
			iregion_ptr[check_freeinode].direct[i] = new_datab_ptr + sb_pointer->dregion_ptr;
			set_bit((unsigned int *)dbitmap_ptr, new_datab_ptr);
		}
	}
	else{
		// MFS_DIRECTORY
		dir_ent_t nd_entry[128];
		iregion_ptr[check_freeinode].size = 2 * sizeof(dir_ent_t);

		//setting root
		strcpy(nd_entry[0].name, ".");
		nd_entry[0].inum = check_freeinode;
		strcpy(nd_entry[1].name, "..");
		nd_entry[1].inum = pinum;
		for(int i = 1; i<DIRECT_PTRS; i++) {
			iregion_ptr[check_freeinode].direct[i] = -1;
		}
		for(int i = 2; i<4096/sizeof(dir_ent_t); i++) {
			nd_entry[i].inum = -1;
		}

		//find free datablock
		int new_datab_ptr = 0;
		for(int i = 0; i<sb_pointer ->num_inodes; i++) {
			new_datab_ptr = get_bit((unsigned int *)dbitmap_ptr, i);
			if(new_datab_ptr == 0) {
				break;
			}
		}
		iregion_ptr[check_freeinode].direct[0] = new_datab_ptr + sb_pointer->dregion_ptr;
		set_bit((unsigned int *)dbitmap_ptr, new_datab_ptr);

		memcpy(&pinum_db_addr[new_datab_ptr].entries, nd_entry, 4096);
	}

	return 0;
}

int server_lookup(int pinum, char* name, message_t m){
	inode_t* pinode = malloc(sizeof(inode_t));
	//move ptr
	pinode = (inode_t*)(iregion_ptr + pinum*sizeof(inode_t))	;
	if(pinode->type!= MFS_DIRECTORY){
		//failure
		return -1;
  }

	// loop and find
	for(int i = 0; i < DIRECT_PTRS; i++){
		//if initialized
		if(pinode->direct[i] != -1){
			for(int j = 0; j < 128; j++){
				dir_ent_t* check_entry =  (dir_ent_t*)(file_ptr +  (pinode->direct[i] * 4096) + j*sizeof(dir_ent_t));
				//printf(" direntry : %p, entry inum : %d, name : %s\n", check_entry, check_entry->inum, check_entry->name);
				if(strcmp(check_entry->name, name) == 0){
					m.inum = check_entry->inum;
					break;
					}
				}
			}
    }
	fsync(fd);
	return -1;
}


int server_shutdown(message_t* m){
	fsync(fd);
	close(fd);
	UDP_Write(sd, &addr_receive, (char *)&m, sizeof(message_t));
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
	//fd = open(img_path, O_RDWR);
	sd = UDP_Open(port);
	if (sd < 0){
		return -1;
	}
	// loop through, read requests, call appropriate function...
	while (1) {
		int to_do;
		message_t m;
		message_t response;
		printf("server:: waiting...\n");
		int rc = UDP_Read(sd, &addr_receive, (char *) &m, sizeof(message_t));
		printf("server:: read message [size:%d contents:(%d)]\n", rc, m.mtype);
		//printf("rc is : %d\n", rc);
		if (rc > 0) {
			to_do = m.mtype;
			switch(to_do){
				case 1:
					int si = server_init(img_path);
					m.rc = si;
					UDP_Write(sd, &addr_receive, (char*)&response, sizeof(message_t));
					break;
				case 2: 
						int pinum_temp = m.inum;
						char filename[28];
						strcpy(filename, m.name);
						m.rc = server_lookup(pinum_temp, filename);
						UDP_Write(sd, &addr_receive, (char*)&m, sizeof(message_t));
				case 3: //stat
				case 4: //write
				case 5: //read
				case 6: 
					server_create(m);
				case 7:	//unlink			
				case 8:
					server_shutdown(&m);
					break;
				default:
					break;
			} 
    }
}
return 0; 
}
    

 
