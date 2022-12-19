#include <stdio.h>
#include "udp.h"
#include "ufs.h"
#include "mfs.h"
#include <sys/mman.h>
#include "sys/stat.h"
#include "message.h"

int sd, fd, port;
struct sockaddr_in addr_send;
void *file_ptr;
super_t* sb_pointer;
super_t superblock;
char* ibitmap_ptr;
char* dbitmap_ptr;
char* iregion_ptr;
char* dregion_ptr;
int image_size;
struct sockaddr_in addr_send;

typedef struct {
	dir_ent_t entries[4096 / sizeof(dir_ent_t)];
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

int server_init(char* file_ptr_name, message_t m){
	fd = open(file_ptr_name, O_RDWR);
	if (fd < 0){
		m.rc = -1;
		fsync(fd);
		UDP_Write(sd, &addr_send, (char*)&m, sizeof(message_t));
		return -1;
	}
	read(fd, &superblock, sizeof(super_t));
	struct stat sbuf;
	int rc = fstat(fd, &sbuf);
	if (rc < 0){
		m.rc = -1;
		fsync(fd);
		UDP_Write(sd, &addr_send, (char*)&m, sizeof(message_t));
		return -1;
	}
	image_size = (int) sbuf.st_size;
	file_ptr = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
/* 	if (file_ptr == MAP_FAILED){
		m.rc = -1;
		fsync(fd);
		return -1;
	} */

	//set other pointers
	sb_pointer = (super_t*)file_ptr;
	ibitmap_ptr = (char*)sb_pointer + sb_pointer->inode_bitmap_addr*4096;
	dbitmap_ptr = (char*)sb_pointer + sb_pointer->data_bitmap_addr*4096;
	iregion_ptr = (char*)sb_pointer + sb_pointer->inode_region_addr*4096;
	dregion_ptr = (char*)sb_pointer + sb_pointer->data_region_addr*4096;
	//filling sb_pointer
	m.rc = 0;
	fsync(fd);
	UDP_Write(sd, &addr_send, (char*)&m, sizeof(message_t));
	return 0;
}

int server_create(message_t m){
	int db_new = -1;
	//if mfs_directory vars
	dir_ent_t* total_entries;

	int pinum = m.inum;
	int dt = m.dir_type;
	m.rc = -1;
	m.inum = -1;
	inode_t* pinode = malloc(sizeof(inode_t));
	pinode = (inode_t*)(iregion_ptr + pinum*sizeof(inode_t));
	if(pinode->type!= MFS_DIRECTORY){
			fsync(fd);
			UDP_Write(sd, &addr_send, (char*)&m, sizeof(message_t));
			return -1;
    }
	//find empty inode
	int check_freeinode = 0;
	for(int i = 0; i < sb_pointer->num_inodes; i++) {
		check_freeinode = get_bit((unsigned int *)ibitmap_ptr, i);
		if(check_freeinode==0) {
			check_freeinode = i;
			break;
		}
	}
	inode_t* f_inode = (inode_t*)(iregion_ptr + check_freeinode*sizeof(inode_t));

	//find data block
	dir_ent_t*  pinum_db_addr = (dir_ent_t*)((char*)file_ptr+ (int)(pinode->direct[0])*4096);
	// move ptr
	//int datab_ptr;
	//datab_ptr = iregion_ptr[pinum].direct[0] - sb_pointer->dregion_ptr;

	for(int i = 0; i < 4096/sizeof(dir_ent_t); i++) {
		if((pinum_db_addr + i)->inum == -1) {
			m.inum = 0;
			m.rc = 0;
			(pinum_db_addr + i)->inum = check_freeinode;
			for(int z = 0; z != 28; z++){
        (pinum_db_addr + i)->name[z] = m.name[z];
      }
			set_bit((unsigned int *)ibitmap_ptr, check_freeinode);
			f_inode->type = dt;
			pinode->size += sizeof(dir_ent_t);
			break;
		}
	}

	if(dt == MFS_DIRECTORY){
		// MFS_DIRECTORY

		//set all to uninitialized
		f_inode-> size = 2 * sizeof(dir_ent_t);
		//switch to i = 1 maybe if error 
		for(int i = 0; i < DIRECT_PTRS; i++){
        f_inode->direct[i] = -1;
      }
		//find free datablock
		int new_datab_ptr = -1;
		for(int i = 0; i < sb_pointer->num_inodes; i++) {
			new_datab_ptr = get_bit((unsigned int *)dbitmap_ptr, i);
			if(new_datab_ptr == 0) {
				new_datab_ptr = i;
				break;
			}
		}
		db_new = new_datab_ptr + sb_pointer->data_region_addr;
		f_inode->direct[0] = db_new;
		total_entries = (dir_ent_t*)((char*)file_ptr + db_new*4096);

		//setting root
		strcpy(total_entries->name, ".");
		strcpy((total_entries+1)->name,"..");
		total_entries->inum = check_freeinode;
    (total_entries+1)->inum = m.inum;
		for(int i = 2; i<4096/sizeof(dir_ent_t); i++) {
			(total_entries+i)->inum= -1;
		}
		set_bit((unsigned int *)dbitmap_ptr, db_new-sb_pointer->data_region_addr);
	}
	else{
		//MFS REG FILE
		for(int i = 0; i < DIRECT_PTRS; i++) {
			//find free data block
			int new_datab_ptr = 0;
			for(int j = 0; i < sb_pointer->num_inodes; j++) {
				new_datab_ptr = get_bit((unsigned int *)dbitmap_ptr, j);
				if(new_datab_ptr == 0) {
					new_datab_ptr = j;
					break;
				}
			}
			db_new = new_datab_ptr;
			//maybe swap order if error
			set_bit((unsigned int *)dbitmap_ptr, db_new);
			f_inode->direct[i] = new_datab_ptr + sb_pointer->data_region_addr;
		}
	}
	m.rc = 0;
	fsync(fd);
	UDP_Write(sd, &addr_send, (char*)&m, sizeof(message_t));
	return 0;
}

int server_lookup(int pinum, char* name, message_t m){
	inode_t* pinode = malloc(sizeof(inode_t));
	//move ptr
	pinode = (inode_t*)(iregion_ptr + pinum*sizeof(inode_t));
	if(pinode->type!= MFS_DIRECTORY){
		//failure
		m.inum = -1;
		m.rc = -1;
		fsync(fd);
		UDP_Write(sd, &addr_send, (char*)&m, sizeof(message_t));
		return -1;
  }
	// loop and find
	for(int i = 0; i < DIRECT_PTRS; i++){
		//if initialized
		if(pinode->direct[i] != -1){
			for(int j = 0; j < 128; j++){
				dir_ent_t* check_entry =  (dir_ent_t*)((char*)file_ptr+  (pinode->direct[i] * 4096) + j*sizeof(dir_ent_t));
				//printf(" direntry : %p, entry inum : %d, name : %s\n", check_entry, check_entry->inum, check_entry->name);
				if(strcmp(check_entry->name, name) == 0 && check_entry->inum != -1){
					m.inum = check_entry->inum;
					m.rc = 0;
					fsync(fd);
					UDP_Write(sd, &addr_send, (char*)&m, sizeof(message_t));
					return m.inum;
					}
				}
			}
    }
	fsync(fd);
	UDP_Write(sd, &addr_send, (char*)&m, sizeof(message_t));
	return -1;
}

int server_read(message_t m){
	m.rc = -1;
	//checks
	//unsigned long err_Test = get_bit((unsigned int*)ibitmap_ptr, m.inum); != 1
	if(m.bytes>4096 || get_bit((unsigned int*)ibitmap_ptr, m.inum) !=1){
		fsync(fd);
		UDP_Write(sd, &addr_send, (char *)&m, sizeof(message_t));
		return -1;
	}
	inode_t* pinode = malloc(sizeof(inode_t));
	pinode = (inode_t*)(iregion_ptr + m.inum*sizeof(inode_t));
	if(pinode->size < m.offset + m.bytes){
		fsync(fd);
		UDP_Write(sd, &addr_send, (char *)&m, sizeof(message_t));
		return -1;
	}

	int offset_start = m.offset % 4096;
	unsigned long current = pinode->direct[m.offset/4096]; // in blocks current writing block
	unsigned long currentAddress = current*4096;

	dir_ent_t*  pinum_db_addr = (dir_ent_t*)((char*)file_ptr+ (int)(pinode->direct[0])*4096);
	if(offset_start + m.bytes <= 4096) {
		char* oneblock = (char*)file_ptr + currentAddress;
		memcpy((void*)m.buffer, (void*)(oneblock + offset_start), m.bytes);
	}
	else {
		char* block_first= (char*)file_ptr + currentAddress;
		char* block_two = (char*)file_ptr + (pinode->direct[(m.offset/4096) + 1])*4096;
		memcpy((void*)m.buffer, (void*)(block_first+offset_start), 4096 - offset_start);
		memcpy((void*)(m.buffer + 4096 - offset_start), (void*)(block_two),m.bytes - (4096 - offset_start));
	}
	m.rc = 0;
	fsync(fd);
	UDP_Write(sd, &addr_send, (char *)&m, sizeof(message_t));
	return 0;
}
int server_shutdown(message_t m){
	m.rc = 0;
	fsync(fd);
	close(fd);
	UDP_Write(sd, &addr_send, (char *)&m, sizeof(message_t));
	UDP_Close(port);
	exit(0);
	return 0;
}

int main(int argc, char *argv[]) {
	signal(SIGINT, intHandler);
	printf("herebass\n");
	if (argc !=3) {
		perror("Error");
	}

	port = atoi(argv[1]);
	
	char file_ptr_path[4096];
	strcpy(file_ptr_path, argv[2]);
	sd = UDP_Open(port);
	if (sd <= -1){
		return -1;
	}
	// loop through, read requests, call appropriate function...
	while (1) {
		int to_do;
		message_t m;
		printf("server:: waiting...\n");
		int rc = UDP_Read(sd, &addr_send, (char *) &m, sizeof(message_t));
		printf("server:: read message [size:%d contents:(%d)]\n", rc, m.mtype);
		//printf("rc is : %d\n", rc);
		if (rc > 0) {
			to_do = m.mtype;
			switch(to_do){
				case 1:{
					server_init(file_ptr_path, m);
					break;
				}
				case 2:{
					server_lookup(m.inum, m.name, m);
					break;
				}
				case 3: //stat
				case 4: //write
				case 5: {
					server_read(m);
					break;
				}
				case 6: {
					server_create(m);
					break;
				}
				case 7:	break;//unlink			
				case 8:
					server_shutdown(m);
					break;
				default:
					break;
			} 
    }
}
return 0; 
}
    

 
