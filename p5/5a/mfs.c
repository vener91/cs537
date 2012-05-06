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
char* mfs_hostname;
int mfs_port = 0;

//tmp variables
int rc = 0;

void error(char* msg){
	fprintf(stderr, "Error: %s\n", msg);
	exit(1);
}

int MFS_UDP_Read(void *tx, void *rx, int tries, int timeout){
//int MFS_UDP_Read(int fd, struct sockaddr_in *addr, void *buffer, int n, int timeout){
	int sd = UDP_Open(0);
    if(sd < -1) {
        perror("Error opening connection.\n");
        return -1;
    }

    struct sockaddr_in addr, addr2;
    int rc = UDP_FillSockAddr(&addr, mfs_hostname, mfs_port);
    if(rc < 0) {
        perror("Error looking up host.\n");
        return -1;
    }

    fd_set rfds;
    struct timeval tv;
    tv.tv_sec=timeout;
    tv.tv_usec=0;


    do {
        FD_ZERO(&rfds);
        FD_SET(sd,&rfds);
        UDP_Write(sd, &addr, (char*)tx, sizeof(MFS_Protocol_t));
        if(select(sd+1, &rfds, NULL, NULL, &tv))
        {
            rc = UDP_Read(sd, &addr2, (char*)rx, sizeof(MFS_Protocol_t));
            if(rc > 0){
                UDP_Close(sd);
                return 0;
            }
        }else {
            tries -= 1;
        }
    }while(1);

}

int MFS_Init(char *hostname, int port){
	if(port != 0){
		free(tx_protocol);
		free(rx_protocol);
		free(mfs_hostname);
	}
	tx_protocol = (MFS_Protocol_t*)malloc(sizeof(MFS_Protocol_t));
	rx_protocol = (MFS_Protocol_t*)malloc(sizeof(MFS_Protocol_t));
	mfs_hostname = (char*)malloc(strlen(hostname) + 1);
	strcpy(mfs_hostname, hostname);
	mfs_port = port;
	printf("Initializing MFS %s %d \n", mfs_hostname, mfs_port);

	tx_protocol->cmd = MFS_CMD_INIT;

	rc = MFS_UDP_Read(tx_protocol, rx_protocol, 10, 5);
	if (rc >= 0) {
		//Does get something back
		return rx_protocol->ret;
	}

	return -1;
}

int MFS_Lookup(int pinum, char *name){
	tx_protocol->cmd = MFS_CMD_LOOKUP;
	tx_protocol->ipnum = pinum;
	strcpy(tx_protocol->datachunk, name);

	rc = MFS_UDP_Read(tx_protocol, rx_protocol, 10, 5);
	if(rc >= 0){
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Stat(int inum, MFS_Stat_t *m){
	tx_protocol->cmd = MFS_CMD_STAT;
	tx_protocol->ipnum = inum;
	
	rc = MFS_UDP_Read(tx_protocol, rx_protocol, 10, 5);
	if(rc >= 0){
		m->type = rx_protocol->datachunk[0];
		m->size = rx_protocol->block;
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Write(int inum, char *buffer, int block){
	tx_protocol->cmd = MFS_CMD_WRITE;
	tx_protocol->ipnum = inum;
	tx_protocol->block = block;
	memcpy(tx_protocol->datachunk, buffer, MFS_BLOCK_SIZE);

	rc = MFS_UDP_Read(tx_protocol, rx_protocol, 10, 5);
	if(rc >= 0){
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Read(int inum, char *buffer, int block){
	tx_protocol->cmd = MFS_CMD_READ;
	tx_protocol->ipnum = inum;
	tx_protocol->block = block;

	rc = MFS_UDP_Read(tx_protocol, rx_protocol, 10, 5);
	if(rc >= 0){
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

	rc = MFS_UDP_Read(tx_protocol, rx_protocol, 10, 5);
	if(rc >= 0){
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Unlink(int pinum, char *name){
	tx_protocol->cmd = MFS_CMD_UNLINK;
	tx_protocol->ipnum = pinum;
	strcpy(tx_protocol->datachunk, name);

	rc = MFS_UDP_Read(tx_protocol, rx_protocol, 10, 5);
	if(rc >= 0){
		return rx_protocol->ret;
	}
	return -1;
}

int MFS_Shutdown(){
	
	tx_protocol->cmd = MFS_CMD_SHUTDOWN;

	rc = MFS_UDP_Read(tx_protocol, rx_protocol, 10, 5);
	if(rc >= 0){
		return rx_protocol->ret;
	}
	return -1;
}


