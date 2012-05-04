#include <stdio.h>
#include <sys/stat.h>
#include "udp.h"
#include "mfs.h"

MFS_Protocol_t* rx_protocol;
int mfs_bytes_not_written = 0;
int mfs_free_bytes; // Bytes left empty in memory
int mfs_bytes_offset; // Bytes left empty in memory
void* mfs_bytes_start_ptr; // Bytes left empty in memory

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

void* mfs_allocate_space(MFS_Header_t** header, int size, int* offset){
	printf("Bytes left %d need %d\n", mfs_free_bytes, size);
	void * ptr = *header;
	if(mfs_bytes_not_written == 0){
		mfs_bytes_start_ptr = ptr + sizeof(MFS_Header_t) + (*header)->byte_count;
		mfs_bytes_offset = (*header)->byte_count;
	}
	if(mfs_free_bytes < size){
		//I need more space
		mfs_free_bytes =- size;
	}else{
		*header = realloc(*header, sizeof(MFS_Header_t) + (*header)->byte_count + mfs_free_bytes + MFS_BYTE_STEP_SIZE );
		mfs_free_bytes = mfs_free_bytes - size + MFS_BYTE_STEP_SIZE;
	}
	if(offset != NULL){
		*offset = (*header)->byte_count;
	}
	mfs_bytes_not_written += size;
	(*header)->byte_count += size;
	ptr = ptr + sizeof(MFS_Header_t) + (*header)->byte_count;
	return ptr;
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

void mfs_reset(MFS_Header_t* header){
	header->byte_count -= mfs_bytes_not_written;
	mfs_bytes_not_written = 0;
}

void mfs_flush(int image_fd){
	mfs_write_block(image_fd, mfs_bytes_start_ptr, mfs_bytes_offset, mfs_bytes_not_written );
	mfs_bytes_not_written = 0;
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
	mfs_free_bytes =  MFS_BYTE_STEP_SIZE; // Bytes left empty in memory

	//Temp variables
	MFS_Inode_t* tmp_inode;
	MFS_InodeMap_t* tmp_inodemap;
	int tmp_offset;
	if(fileStat.st_size < sizeof(MFS_Header_t)){
		//Initialize
		image_size = sizeof(MFS_Header_t) + MFS_BYTE_STEP_SIZE;
		header = (MFS_Header_t *)malloc(image_size);
		
		//Init root dir
		tmp_inode = mfs_allocate_space(&header, sizeof(MFS_Inode_t), NULL);
		tmp_inodemap = mfs_allocate_space(&header, sizeof(MFS_InodeMap_t), &tmp_offset);
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
	//void* block_ptr = (void *)header + sizeof(MFS_Header_t);

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
			} else if(rx_protocol->cmd == MFS_CMD_LOOKUP){
				printf("LOOKUP: pinum: %d name:%s \n", rx_protocol->ipnum, rx_protocol->datachunk);
				rx_protocol->ret = -1;
				MFS_DirEnt_t* entry;
				MFS_Inode_t* parent_inode = mfs_resolve_inode(header, rx_protocol->ipnum);
				if(parent_inode != NULL && parent_inode->type == MFS_DIRECTORY){
					i = 0;			
					while(i < MFS_INODE_SIZE) {
						if(parent_inode->data[i] != -1){
							j = 0;
							while(j < MFS_BLOCK_SIZE / sizeof(MFS_DirEnt_t)){
								entry = (MFS_DirEnt_t*)(header + sizeof(MFS_Header_t) + parent_inode->data[i] + (j * sizeof(MFS_DirEnt_t)));			
								if(entry->inum != -1 && strcmp(entry->name, rx_protocol->datachunk) == 0 ){
									rx_protocol->ret = 0;
									break;
								}
								j++;
							}
							if(rx_protocol->ret == 0) break;
						}
						i++;
					}
				}

				if(UDP_Write(sd, &s, rx_protocol, sizeof(MFS_Protocol_t)) < -1){
					error("Unable to send result");
				}

			} else if(rx_protocol->cmd == MFS_CMD_CREAT){
				printf("CREAT: pinum: %d type:%d name:%s \n", rx_protocol->ipnum, rx_protocol->datachunk[0], rx_protocol->datachunk + sizeof(char));
				//Add a new inode
				rx_protocol->ret = -1;
				MFS_Inode_t* parent_inode = mfs_resolve_inode(header, rx_protocol->ipnum);
				if(parent_inode != NULL && parent_inode->type == MFS_DIRECTORY){
					//Check if the dir is full
					int entry_offset, inode_offset, new_dir_offset;
					MFS_DirEnt_t* entry;
					//Initialize new data block for entries
					MFS_DirEnt_t* new_entry =  mfs_allocate_space(&header, MFS_BLOCK_SIZE, &entry_offset);
					MFS_Inode_t* new_inode = mfs_allocate_space(&header, sizeof(MFS_INODE_SIZE), &inode_offset);
					mfs_init_inode(new_inode, 0, parent_inode);
					//Copy new stuff	
					while(i < MFS_INODE_SIZE) {
						if(parent_inode->data[i] != -1){
							j = 0;
							while(j < MFS_BLOCK_SIZE / sizeof(MFS_DirEnt_t)){
								entry = (MFS_DirEnt_t*)(header + sizeof(MFS_Header_t) + parent_inode->data[i] + (j * sizeof(MFS_DirEnt_t)));			
								if(entry->inum == -1){
									//Copy the dir entry
									memcpy(new_entry, header + sizeof(MFS_Header_t) + parent_inode->data[i], MFS_BLOCK_SIZE);
									new_inode->data[i] = inode_offset;
									strcpy((new_entry + (j * sizeof(MFS_DirEnt_t)))->name, &(rx_protocol->datachunk[0]));
									break;
								}
								j++;
							}
						}else{
							//Create new node
							new_inode->data[i] = inode_offset;
							strcpy((new_entry + (0 * sizeof(MFS_DirEnt_t)))->name, &(rx_protocol->datachunk[0]));
						}
						i++;
					}

					//Actually create the inode
					new_inode = mfs_allocate_space(&header, sizeof(MFS_INODE_SIZE), &inode_offset);
					mfs_init_inode(new_inode, rx_protocol->datachunk[0], NULL);
					new_entry->inum = inode_offset;
					//Add to map
					i = 0;
					while(i < MFS_MAX_INODES/MFS_INODES_PER_BLOCK){
						header->map[i];
						i++;
					}
					

					//Add .. and . dirs
					if(rx_protocol->datachunk[0] == MFS_DIRECTORY){
						MFS_DirEnt_t* new_dir_entry =  mfs_allocate_space(&header, MFS_BLOCK_SIZE, &new_dir_offset);
						new_dir_entry->name[0] = '.';
						new_dir_entry->name[1] = '\0';
						new_dir_entry->inum = new_dir_offset; 
						new_dir_entry += sizeof(MFS_DirEnt_t);
						new_dir_entry->name[0] = '.';
						new_dir_entry->name[1] = '.';
						new_dir_entry->name[2] = '\0';
						new_dir_entry->inum = inode_offset; 
					}	

					//Write to block
					header->inode_count++;
					mfs_flush(fd);
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


