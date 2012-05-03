#include <stdio.h>
#include <sys/stat.h>
#include "udp.h"
#include "mfs.h"

MFS_Protocol_t* rx_protocol;

void usage(char *argv[]){
	fprintf(stderr, "Usage: %s <port> <file-system-image>\n", argv[0]);
	exit(1);
}

void error(char* msg){
	fprintf(stderr, "Error: %s\n", msg);
	exit(1);
}

MFS_Inode_t* mfs_resolve_inode(MFS_Header_t* header, int inode_num){
	if( header->map[inode_num / MFS_INODES_PER_BLOCK] != -1 ){
		int* imap = (int *)(header + sizeof(MFS_Header_t) + header->map[inode_num / MFS_INODES_PER_BLOCK]);
		//Check if inodes exist or not
		if(imap[inode_num % MFS_INODES_PER_BLOCK] != -1){
			return (MFS_Inode_t*)(header + sizeof(MFS_Header_t) + imap[inode_num % MFS_INODES_PER_BLOCK]); 	
		}
	}
	return NULL;
}

void* mfs_allocate_space(MFS_Header_t** header, int* free_bytes, int size, int* offset){
	printf("Bytes left %d need %d\n", *free_bytes, size);
	if(*free_bytes < size){
		//I need more space
		*free_bytes =- size;
	}else{
		*header = realloc(*header, sizeof(MFS_Header_t) + (*header)->byte_count + *free_bytes + MFS_BYTE_STEP_SIZE );
		*free_bytes = *free_bytes - size + MFS_BYTE_STEP_SIZE;
	}
	if(offset != NULL){
		*offset = (*header)->byte_count;
	}
	(*header)->byte_count += size;
	void * ptr = *header;
	ptr = ptr + sizeof(MFS_Header_t) + (*header)->byte_count;
	return ptr;
}

MFS_DirEnt_t* mfs_allocate_entry(MFS_Header_t* header, MFS_Inode_t* inode, int* free_bytes, int* offset){
	int i = 0;
	int j;
	MFS_DirEnt_t* entry;
	while(i < MFS_INODE_SIZE) {
		if(inode->data[i] != -1){
			j = 0;
			while(j < MFS_BLOCK_SIZE / sizeof(MFS_DirEnt_t)){
				entry = (MFS_DirEnt_t*)(header + sizeof(MFS_Header_t) + inode->data[i--] + (j * sizeof(MFS_DirEnt_t)));			
				if(entry->inum == -1){
					return entry;
				}
				j++;
			}
		}else{
			//Initialize new data block for entries
			return mfs_allocate_space(&header, free_bytes, MFS_BLOCK_SIZE, offset);
		}
		i++;
	}
}

void mfs_init_inode(MFS_Inode_t* inode, int type, MFS_Inode_t* old_inode){
	if(old_inode == NULL){	
		inode->size = 0;
		inode->type = type;
		int i;
		for (i = 0; i < MFS_INODE_SIZE; i++) {
			inode->data[i] = -1;
		}
	}else{
		memcpy(inode, old_inode, sizeof(MFS_Inode_t));
	}
}

void mfs_write_block(int image_fd, void* start_ptr, int offset, int size){
	int rc = pwrite(image_fd, start_ptr, size, sizeof(MFS_Header_t) + offset);	
	if(rc < 0){
		error("Error writing block");
	}
	fsync(image_fd);
}

void mfs_write_header(int image_fd, MFS_Header_t* header){
	//Writes the header to the file
	int rc = pwrite(image_fd, header, sizeof(MFS_Header_t), 0);	
	if(rc < 0){
		error("Error writing header");
	}
	fsync(image_fd);
}

int
main(int argc, char *argv[]) {
	if(argc != 3) {
		usage(argv);
	}
	//Get arguments
	int port = atoi(argv[1]);
	char* image_path = argv[2];
	int sd = UDP_Open(port);
	assert(sd > -1);

	//Open up the image file
	int fd = open(image_path, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if (fd < 0) {
		error("Cannot open file");
	}
	// Make a copy in memory
	struct stat fileStat;
	if(fstat(fd,&fileStat) < 0) {
		error("Cannot open file");
	}
	//Put image in memory
	int rc, i, j;
	MFS_Header_t* header;
	int image_size;
	int free_bytes =  MFS_BYTE_STEP_SIZE; // Bytes left empty in memory

	//Temp variables
	MFS_Inode_t* tmp_inode;
	MFS_InodeMap_t* tmp_inodemap;
	int tmp_offset;
	if(fileStat.st_size < sizeof(MFS_Header_t)){
		//Initialize
		image_size = sizeof(MFS_Header_t) + MFS_BYTE_STEP_SIZE;
		header = (MFS_Header_t *)malloc(image_size);
		
		//Init root dir
		tmp_inode = mfs_allocate_space(&header, &free_bytes, sizeof(MFS_Inode_t), NULL);
		tmp_inodemap = mfs_allocate_space(&header, &free_bytes, sizeof(MFS_InodeMap_t), &tmp_offset);
		mfs_init_inode(tmp_inode, MFS_DIRECTORY, NULL);
		
		//Init header
		for (i = 0; i < MFS_MAX_INODES/MFS_INODES_PER_BLOCK; i++) {
			header->map[i] = -1;	
		}

		//Write to disk
		header->map[0] = tmp_offset;
		
		mfs_write_header(fd, header);
		printf("Initializing new file\n");
	}else{
		image_size = fileStat.st_size + MFS_BYTE_STEP_SIZE;
		header = (MFS_Header_t *)malloc(image_size);
		// Put text in memory
		rc = read(fd, header, fileStat.st_size);
		if(rc < 0){
			error("Cannot open file");
		}
	}
	//Set start block location
	void* block_ptr = (void *)header + sizeof(MFS_Header_t);

	rx_protocol = (MFS_Protocol_t*)malloc(sizeof(MFS_Protocol_t));

	printf("Server started listening at port %d\n", port);
	while (1) {
		struct sockaddr_in s;
		rc = UDP_Read(sd, &s, rx_protocol, sizeof(MFS_Protocol_t), 0);
		if (rc > 0) {
			printf("Response cmd: %d\n", rx_protocol->cmd);
			//Special case for shutdown
			if(rx_protocol->cmd == MFS_CMD_INIT){
				printf("Server initialized\n");
			} else if(rx_protocol->cmd == MFS_CMD_SHUTDOWN){
				//Close file
				rc = close(fd);
				if(rc < 0){
					error("Cannot open file");
				}
				exit(0);
			} else if(rx_protocol->cmd == MFS_CMD_CREAT){
				printf("CREAT: pinum: %d type:%d name:%s \n", rx_protocol->ipnum, rx_protocol->datachunk[0], rx_protocol->datachunk + sizeof(char));
				//Add a new inode
				rx_protocol->ret = -1;
				MFS_Inode_t* parent_inode = mfs_resolve_inode(header, rx_protocol->ipnum);
				MFS_DirEnt_t* new_entry = NULL;
				if(parent_inode != NULL && parent_inode->type == MFS_DIRECTORY){
					//Check if the dir is full
					i = 0;
					MFS_DirEnt_t* new_entry = mfs_allocate_entry(header, parent_inode, &free_bytes);
					
					block_ptr;
					MFS_Inode_t* new_inode = mfs_allocate_space(&header, &free_bytes, 2, NULL);
					new_inode->type = rx_protocol->datachunk[0];
					header->map[header->inode_count] = header->byte_count;
					header->byte_count ++;
					//Update parent dir

					header->map[rx_protocol->ipnum] = header->byte_count;
					header->byte_count ++;
					//Write to block
					mfs_write_block(fd, new_inode, header->byte_count, 2);
					header->inode_count++;
					mfs_write_header(fd, header);
					rx_protocol->ret = 0;
				}

				if(UDP_Write(sd, &s, rx_protocol, sizeof(MFS_Protocol_t)) < -1){
					error("Unable to send result");
				}
			} else {
				error("Unknown command");
			}
		}
	}
	//Should not come here
	error("Something is really really wrong");
	return 0;
}


