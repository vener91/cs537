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

void mfs_add_inode(int image_fd){
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
	int rc;
	MFS_Header_t* header;
	int image_size;
	int blocks_left =  MFS_BLOCK_STEP_SIZE; // Bytes left empty in memory
	if(fileStat.st_size < sizeof(MFS_Header_t)){
		//Initialize
		image_size = sizeof(MFS_Header_t) + (4096 * MFS_BLOCK_STEP_SIZE);
		header = (MFS_Header_t *)malloc(image_size);
		MFS_Inode_t* root_inode = (void *)header + sizeof(MFS_Header_t);
		root_inode->type = MFS_DIRECTORY;
		header->map[0] = 0;
		header->block_count = header->file_count = 1; //Root node
		blocks_left --; //Used a block for root

		mfs_write_header(fd, header);
	}else{
		image_size = fileStat.st_size + (4096 * MFS_BLOCK_STEP_SIZE);
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
				//Add a new inode
				rx_protocol->ret = -1;
				if(header->file_count < MFS_BLOCK_SIZE && rx_protocol->ipnum < header->file_count){
					//Check if the dir is empty
					
					if(){
						header->map[header->file_count] = add_inode() ;
						rx_protocol->ret = 0;
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


