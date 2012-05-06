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

int MFS_UDP_Read(int fd, struct sockaddr_in *addr, void *buffer, int n, int timeout){
	// added timeout code
	fd_set rfds;	
	struct timeval tv;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	int rc = -1;

	FD_ZERO(&rfds);
	FD_SET(fd,&rfds);
	if(select(fd+1, &rfds, NULL, NULL, &tv)){
		rc = UDP_Read(fd, addr, buffer, n);
		return rc;
	}else {
		//Timeout;
		return -2;
	}

	return rc;
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
	rc = UDP_Write(sd, &saddr, (char*)tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(MFS_UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < 0){
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

int MFS_Lookup(int pinum, char *name){
	tx_protocol->cmd = MFS_CMD_LOOKUP;
	tx_protocol->ipnum = pinum;
	strcpy(tx_protocol->datachunk, name);

	rc = UDP_Write(sd, &saddr, (char*)tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(MFS_UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < 0){
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
	tx_protocol->cmd = MFS_CMD_STAT;
	tx_protocol->ipnum = inum;
	
	int rc = UDP_Write(sd, &saddr, (char*)tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(MFS_UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < 0){
			tries_left--;
			if(!tries_left){
				return -1;
			}
		}
		m->type = rx_protocol->datachunk[0];
		m->size = rx_protocol->block;
		//Does get something back
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Write(int inum, char *buffer, int block){
	tx_protocol->cmd = MFS_CMD_WRITE;
	tx_protocol->ipnum = inum;
	tx_protocol->block = block;
	memcpy(tx_protocol->datachunk, buffer, MFS_BLOCK_SIZE);

	rc = UDP_Write(sd, &saddr, (char*)tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(MFS_UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < 0){
			tries_left--;
			if(!tries_left){
				return -1;
			}
		}
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Read(int inum, char *buffer, int block){
	tx_protocol->cmd = MFS_CMD_READ;
	tx_protocol->ipnum = inum;
	tx_protocol->block = block;
	rc = UDP_Write(sd, &saddr, (char*)tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(MFS_UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < 0){
			tries_left--;
			if(!tries_left){
				return -1;
			}
		}
		memcpy(buffer, rx_protocol->datachunk, MFS_BLOCK_SIZE);
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Creat(int pinum, int type, char *name){

	tx_protocol->cmd = MFS_CMD_CREAT;
	tx_protocol->ipnum = pinum;
	strcpy(tx_protocol->datachunk + sizeof(char), name);
	tx_protocol->datachunk[0] = (char)type;
	rc = UDP_Write(sd, &saddr, (char*)tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(MFS_UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < 0){
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
	tx_protocol->cmd = MFS_CMD_UNLINK;
	tx_protocol->ipnum = pinum;
	strcpy(tx_protocol->datachunk, name);

	rc = UDP_Write(sd, &saddr, (char*)tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(MFS_UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < 0){
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

int MFS_Shutdown(){
	
	tx_protocol->cmd = MFS_CMD_SHUTDOWN;
	rc = UDP_Write(sd, &saddr, (char*)tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(MFS_UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < 0){
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


