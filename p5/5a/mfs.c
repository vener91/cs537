#include <stdio.h> //Marked for Del
#include "mfs.h"
#include "udp.h"
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

int sd = -1;
struct sockaddr_in saddr;

MFS_Protocol_t* tx_protocol;
MFS_Protocol_t* rx_protocol;

//tmp variables
int rc = 0;

void error(char* msg){
	fprintf(stderr, "Error: %s\n", msg);
	exit(1);
}

int MFS_Init(char *hostname, int port){
	if(sd != -1){
		//That means a previous sd exists
		if(UDP_Close(sd)){
			//Oh shit
			error("Unable to close the connection");			
		}
		free(tx_protocol);
		free(rx_protocol);
	}
	sd = UDP_Open(-1);
	assert(sd > -1);
	tx_protocol = (MFS_Protocol_t*)malloc(sizeof(MFS_Protocol_t));
	rx_protocol = (MFS_Protocol_t*)malloc(sizeof(MFS_Protocol_t));

	int rc = UDP_FillSockAddr(&saddr, hostname, port);
	assert(rc == 0);
	printf("Initializing MFS %s \n", hostname);

	tx_protocol->cmd = MFS_CMD_INIT;
	rc = UDP_Write(sd, &saddr, tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		return 0;
	}
	return -1;
}

int MFS_Lookup(int pinum, char *name){
	tx_protocol->cmd = MFS_CMD_CREAT;
	tx_protocol->ipnum = pinum;
	tx_protocol->datachunk[0] = (char)MFS_DIRECTORY;
	strcpy(tx_protocol->datachunk + sizeof(char), name);

	rc = UDP_Write(sd, &saddr, tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < -1){
			tries_left--;
			if(!tries_left){
				return -1;
			}
		}
		//Does get something back
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Stat(int inum, MFS_Stat_t *m){
	return 0;
}

int MFS_Write(int inum, char *buffer, int block){
	return 0;
}

int MFS_Read(int inum, char *buffer, int block){
	return 0;
}

int MFS_Creat(int pinum, int type, char *name){

	tx_protocol->cmd = MFS_CMD_CREAT;
	tx_protocol->ipnum = pinum;
	tx_protocol->datachunk[0] = (char)MFS_DIRECTORY;
	strcpy(tx_protocol->datachunk + sizeof(char), name);

	rc = UDP_Write(sd, &saddr, tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < -1){
			tries_left--;
			if(!tries_left){
				return -1;
			}
		}
		//Does get something back
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Unlink(int pinum, char *name){
	return 0;
}

int MFS_Shutdown(){
	tx_protocol->cmd = MFS_CMD_SHUTDOWN;
	rc = UDP_Write(sd, &saddr, &tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		sd = -1;
		return 0;
	}
	return -1;
}
