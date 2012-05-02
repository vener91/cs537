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
	/*
	// Put text in memory
	int sizeOfArray = fileStat.st_size / sizeof(rec_t);
	Rblock = malloc(fileStat.st_size);
	rc = read(fd, Rblock, fileStat.st_size);
	if(rc < 0){
		error("Cannot open file");
	}
	*/
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
				rx_protocol->ret = 0;
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


