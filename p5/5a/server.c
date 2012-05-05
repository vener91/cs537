#include <stdio.h>
#include <sys/stat.h>
#include "udp.h"
#include "mfs.h"

MFS_Protocol_t* rx_protocol;
int mfs_bytes_not_written = 0;
int mfs_free_bytes = 0; // Bytes left empty in memory
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
		void* header_ptr = (void*)header;
		MFS_InodeMap_t* imap = (MFS_InodeMap_t *)(header_ptr + sizeof(MFS_Header_t) + header->map[inode_num / MFS_INODES_PER_BLOCK]);
		//printf("%p %d %p\n", header_ptr, imap->inodes[1], header_ptr + sizeof(MFS_Header_t) + header->map[inode_num / MFS_INODES_PER_BLOCK]);
		//printf("Resolving: %d %d %d\n", inode_num, header->map[inode_num / MFS_INODES_PER_BLOCK], imap->inodes[1]);
		//Check if inodes exist or not
		if(imap->inodes[inode_num % MFS_INODES_PER_BLOCK] != -1){
			return (MFS_Inode_t*)(header_ptr + sizeof(MFS_Header_t) + imap->inodes[inode_num % MFS_INODES_PER_BLOCK]); 	
		}
	}
	return NULL;
}

void* mfs_allocate_space(MFS_Header_t** header, int size, int* offset){
	printf("Allocating Bytes - left %d need %d\n", mfs_free_bytes, size);
	fflush(stdout);
	void *ptr = *header;
	if(mfs_bytes_not_written == 0){
		mfs_bytes_start_ptr = ptr + sizeof(MFS_Header_t) + (*header)->byte_count;
		mfs_bytes_offset = (*header)->byte_count;
	}
	if(mfs_free_bytes > size){
		//I need more space
		mfs_free_bytes -= size;
	}else{
		void* old_header = *header;
		*header = realloc(*header, sizeof(MFS_Header_t) + (*header)->byte_count + size + mfs_free_bytes + MFS_BYTE_STEP_SIZE );
		if (old_header != *header) {
			printf("New header: %p\n", *header);
		}
		mfs_free_bytes = mfs_free_bytes + MFS_BYTE_STEP_SIZE;
	}
	if(offset != NULL){
		*offset = (*header)->byte_count;
	}
	mfs_bytes_not_written += size;
	ptr = (void*)ptr + sizeof(MFS_Header_t) + (*header)->byte_count;
	(*header)->byte_count += size;
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

void mfs_update_inode(MFS_Header_t** header, int inode_num, int new_offset){
	MFS_Header_t* headerp = *header;
	void* header_ptr = (void*)headerp;
	MFS_InodeMap_t* old_imap = (MFS_InodeMap_t *)(header_ptr + sizeof(MFS_Header_t) + headerp->map[inode_num / MFS_INODES_PER_BLOCK]);
	//printf("%p %d %p\n", header_ptr, old_imap->inodes[1], (void*)(header_ptr + sizeof(MFS_Header_t) + headerp->map[inode_num / MFS_INODES_PER_BLOCK]));
	int offset;
	MFS_InodeMap_t* imap = mfs_allocate_space(header, sizeof(MFS_InodeMap_t), &offset);
	memcpy(imap, old_imap, sizeof(MFS_InodeMap_t));
	imap->inodes[inode_num % MFS_INODES_PER_BLOCK] = new_offset; 	
}

void mfs_write_block(int image_fd, void* start_ptr, int offset, int size){
	int rc = pwrite(image_fd, start_ptr, size, sizeof(MFS_Header_t) + offset);	
	if(rc < 0){
		error("Error writing block");
	}
	fsync(image_fd);
}

int mfs_lookup(void* block_ptr, MFS_Inode_t* inode, char* name){
	int i = 0,j;
	if(inode != NULL && inode->type == MFS_DIRECTORY){
		while(i < MFS_INODE_SIZE) {
			if(inode->data[i] != -1){
				j = 0;
				while(j < MFS_BLOCK_SIZE / sizeof(MFS_DirEnt_t)){
					//printf("Parent node %d %d\n", inode->data[i], MFS_BLOCK_SIZE / sizeof(MFS_DirEnt_t) );
					MFS_DirEnt_t* entry = (MFS_DirEnt_t*)(block_ptr + inode->data[i] + (j * sizeof(MFS_DirEnt_t)));			
					/*
					if(entry->inum != -1 ){
						printf("%s - %s\n", entry->name, name);
						fflush(stdout);
					}
					*/
					if(entry->inum != -1 && strcmp(entry->name, name) == 0 ){
						return entry->inum;
					}
					j++;
				}
			}
			i++;
		}
	}
	return -1;
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
	MFS_InodeMap_t* tmp_imap;
	MFS_DirEnt_t* tmp_entry;
	int tmp_offset, tmp_inode_offset, tmp_imap_offset;
	if(fileStat.st_size < sizeof(MFS_Header_t)){
		//Initialize
		image_size = sizeof(MFS_Header_t) + MFS_BYTE_STEP_SIZE;
		header = (MFS_Header_t *)malloc(image_size);

		//Init root dir
		tmp_inode = mfs_allocate_space(&header, sizeof(MFS_Inode_t), &tmp_inode_offset);
		tmp_imap = mfs_allocate_space(&header, sizeof(MFS_InodeMap_t), &tmp_imap_offset);
		tmp_imap->inodes[0] = tmp_inode_offset;
		mfs_init_inode(tmp_inode, MFS_DIRECTORY, NULL);

		//Init header
		for (i = 0; i < MFS_MAX_INODES/MFS_INODES_PER_BLOCK; i++) {
			header->map[i] = -1;	
		}
		//printf("Map entry %p %d\n", tmp_imap, tmp_inode_offset);

		for (i = 0; i < MFS_INODES_PER_BLOCK; i++) {
			tmp_imap->inodes[i] = -1;
		}
		tmp_imap->inodes[0] = tmp_inode_offset;

		//Add . dirs
		tmp_entry = mfs_allocate_space(&header, MFS_BLOCK_SIZE, &tmp_offset);
		tmp_entry->name[0] = '.';
		tmp_entry->name[1] = '\0';
		tmp_entry->inum = tmp_inode_offset; 
		for (i = 1; i < MFS_BLOCK_SIZE/sizeof(MFS_DirEnt_t); i++) {
			tmp_entry++;
			tmp_entry->inum = -1;
		}
		tmp_inode->data[0] = tmp_offset;
		
		//Write to disk
		header->map[0] = tmp_imap_offset;
		mfs_flush(fd);		
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
	void* header_ptr = (void*)header;
	void* block_ptr = header_ptr + sizeof(MFS_Header_t);

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
			} 
			else if(rx_protocol->cmd == MFS_CMD_SHUTDOWN){
				//Close file
				rc = close(fd);
				if(rc < 0){
					error("Cannot open file");
				}
				exit(0);
			} 
			else if(rx_protocol->cmd == MFS_CMD_LOOKUP){
				printf("LOOKUP: pinum: %d name:%s \n", rx_protocol->ipnum, rx_protocol->datachunk);
				MFS_DirEnt_t* entry;
				MFS_Inode_t* parent_inode = mfs_resolve_inode(header, rx_protocol->ipnum);
				rx_protocol->ret = mfs_lookup(block_ptr, parent_inode, &(rx_protocol->datachunk[0]));

				if(UDP_Write(sd, &s, rx_protocol, sizeof(MFS_Protocol_t)) < -1){
					error("Unable to send result");
				}

			} 
			else if(rx_protocol->cmd == MFS_CMD_READ){
				//printf("LOOKUP: pinum: %d name:%s \n", rx_protocol->ipnum, rx_protocol->datachunk);
				rx_protocol->ret = -1;
				MFS_DirEnt_t* entry;
				MFS_Inode_t* read_inode = mfs_resolve_inode(header, rx_protocol->ipnum);
				if(read_inode != NULL && read_inode->type == MFS_REGULAR_FILE){
					int block = (int)rx_protocol->datachunk[0];
					if(read_inode->data[block] != -1) {
						void *data_ptr = read_inode->data[block] + header_ptr;
						strcpy(rx_protocol->datachunk, data_ptr);			
						rx_protocol->ret = 0;
						//printf("datachunk to be returned %s ", rx_protocol->datachunk);
					}
				}
				if(UDP_Write(sd, &s, rx_protocol, sizeof(MFS_Protocol_t)) < -1){
					error("Unable to send result");
				}
			}
			else if(rx_protocol->cmd == MFS_CMD_CREAT){
				printf("CREAT: pinum: %d type:%d name:%s \n", rx_protocol->ipnum, rx_protocol->datachunk[0], rx_protocol->datachunk + sizeof(char));
				//Add a new inode
				rx_protocol->ret = -1;
				MFS_Inode_t* parent_inode = mfs_resolve_inode(header, rx_protocol->ipnum);

				if(parent_inode != NULL && parent_inode->type == MFS_DIRECTORY){
					//Check if the dir is full
					int entry_offset, inode_offset, new_dir_offset;
					MFS_DirEnt_t* entry;
					int done = 0;
					//Initialize new data block for entries
					MFS_DirEnt_t* new_entry =  mfs_allocate_space(&header, MFS_BLOCK_SIZE, &entry_offset);
					MFS_Inode_t* new_inode = mfs_allocate_space(&header, sizeof(MFS_Inode_t), &inode_offset);
					mfs_update_inode(&header, rx_protocol->ipnum, inode_offset);
					mfs_init_inode(new_inode, 0, parent_inode);
					//Copy new stuff	
					done = 0;
					i = 0;
					while(i < MFS_INODE_SIZE) {
						if(parent_inode->data[i] != -1){
							//printf("Parent node %d %d\n", parent_inode->type, MFS_BLOCK_SIZE / sizeof(MFS_DirEnt_t) );
							j = 0;
							while(j < MFS_BLOCK_SIZE / sizeof(MFS_DirEnt_t)){
								entry = (MFS_DirEnt_t*)(block_ptr + parent_inode->data[i] + (j * sizeof(MFS_DirEnt_t)));			
								if(entry->inum == -1){
									//printf("Found space - %d %p\n", j, block_ptr + parent_inode->data[i] + (j * sizeof(MFS_DirEnt_t)));
									//Copy the dir entry
									memcpy(new_entry, block_ptr + parent_inode->data[i], MFS_BLOCK_SIZE);
									new_inode->data[i] = entry_offset;
									entry = (MFS_DirEnt_t*)(block_ptr + parent_inode->data[i] + (j * sizeof(MFS_DirEnt_t)));
									entry->inum = inode_offset;	
									strcpy(entry->name, &(rx_protocol->datachunk[1]));
									//printf("Name: %s - %s\n",entry->name, &(rx_protocol->datachunk[1]));
									done = 1;
									break;
								}
								j++;
							}
							if(done == 1) break;
						}else{	
							printf("Creating new data block\n");
							//Create new node
							new_inode->data[i] = entry_offset;
							entry = (MFS_DirEnt_t*)(new_entry + (j * sizeof(MFS_DirEnt_t)));
							entry->inum = inode_offset;			
							strcpy((new_entry + (0 * sizeof(MFS_DirEnt_t)))->name, &(rx_protocol->datachunk[0]));
							done = 1;
							break;
						}
						i++;
					}
					if(done){
						//Actually create the inode
						new_inode = mfs_allocate_space(&header, sizeof(MFS_Inode_t), &inode_offset);
						mfs_init_inode(new_inode, rx_protocol->datachunk[0], NULL);
						new_entry->inum = inode_offset;

						//Add to map
						i = 0;
						done = 0;
						while(i < MFS_MAX_INODES/MFS_INODES_PER_BLOCK){
							if(header->map[i] != -1){
								tmp_imap = (MFS_InodeMap_t*)(block_ptr + header->map[i]);			
								for (j = 0; j < MFS_INODES_PER_BLOCK; j++) {
									if(tmp_imap->inodes[j] == -1){
										mfs_update_inode(&header, (i * MFS_MAX_INODES/MFS_INODES_PER_BLOCK) + j, inode_offset );
										done = 1;
										break;
									}
								}
								if(done == 1) break;
							}else{
								//Create new node
								tmp_imap = mfs_allocate_space(&header, sizeof(MFS_InodeMap_t), &tmp_offset);
								for (j = 0; j < MFS_INODES_PER_BLOCK; j++) {
									tmp_imap->inodes[j] = -1;	
								}
								tmp_imap->inodes[0] = inode_offset;
								header->map[i] = tmp_offset;
								done = 1;
								break;
							}
							i++;
						}


						//Add .. and . dirs
						if(rx_protocol->datachunk[0] == MFS_DIRECTORY){
							MFS_DirEnt_t* new_dir_entry =  mfs_allocate_space(&header, MFS_BLOCK_SIZE, &new_dir_offset);
							for (i = 0; i < MFS_BLOCK_SIZE/sizeof(MFS_DirEnt_t); i++) {
								(new_dir_entry + i)->inum = -1;
							}
							new_dir_entry->name[0] = '.';
							new_dir_entry->name[1] = '\0';
							new_dir_entry->inum = new_dir_offset; 
							(new_dir_entry + 1)->name[0] = '.';
							(new_dir_entry + 1)->name[1] = '.';
							(new_dir_entry + 1)->name[2] = '\0';
							(new_dir_entry + 1)->inum = inode_offset; 
						}	

						//Write to block
						header->inode_count++;
						mfs_flush(fd);
						mfs_write_header(fd, header);
						rx_protocol->ret = 0;
					}else{
						mfs_reset(header);
					}
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


