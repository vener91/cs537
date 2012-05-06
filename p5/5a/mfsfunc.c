#include "mfs.h"
#include "udp.h"
#include "mfsfunc.c"
int MFS_UDP_Read(int fd, struct sockaddr_in *addr, void *buffer, int n, int timeout){
	int len = sizeof(struct sockaddr_in); 
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
