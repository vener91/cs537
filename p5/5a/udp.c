#include "udp.h"

// create a socket and bind it to a port on the current machine
// used to listen for incoming packets
int
UDP_Open(int port)
{
    int fd;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	perror("socket");
	return 0;
    }

    // set up the bind
    struct sockaddr_in myaddr;
    bzero(&myaddr, sizeof(myaddr));

    myaddr.sin_family      = AF_INET;
    myaddr.sin_port        = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
	perror("bind");
	close(fd);
	return -1;
    }

    // give back descriptor
    return fd;
}

// fill sockaddr_in struct with proper goodies
int
UDP_FillSockAddr(struct sockaddr_in *addr, char *hostName, int port)
{
    bzero(addr, sizeof(struct sockaddr_in));
    if (hostName == NULL) {
	return 0; // it's OK just to clear the address
    }
    
    addr->sin_family = AF_INET;          // host byte order
    addr->sin_port   = htons(port);      // short, network byte order

    struct in_addr *inAddr;
    struct hostent *hostEntry;
    if ((hostEntry = gethostbyname(hostName)) == NULL) {
	perror("gethostbyname");
	return -1;
    }
    inAddr = (struct in_addr *) hostEntry->h_addr;
    addr->sin_addr = *inAddr;

    // all is good
    return 0;
}

int
UDP_Write(int fd, struct sockaddr_in *addr, void *buffer, int n){
    int addrLen = sizeof(struct sockaddr_in);
    int rc      = sendto(fd, buffer, n, 0, (struct sockaddr *) addr, addrLen);
    return rc;
}

int
UDP_Read(int fd, struct sockaddr_in *addr, char *buffer, int n, int server)
{
    int len = sizeof(struct sockaddr_in); 
	
    printf("CLIENT start: %d \n", server);
	if(server == 0) {
		int retval = 0;
		while (retval != 1) { 
			fd_set rfds;
			struct timeval tv;

			printf("CLIENT in \n");
			/* Watch stdin (fd 0) to see when it has input. */
			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);

			/* Wait up to five seconds. */
			tv.tv_sec = 1;
			tv.tv_usec = 0;

			retval = select(fd+1, &rfds, NULL, NULL, &tv);
			/* Don't rely on the value of tv now! */

			if (retval == -1)
				perror("select()");
			else if (retval)
				printf("Data is available now.\n");
			/* FD_ISSET(0, &rfds) will be true. */
			else
				printf("No data within %d seconds.\n", tv.tv_sec);
		}
	}

    printf("CLIENT wait \n");
    int rc = recvfrom(fd, buffer, n, 0, (struct sockaddr *) addr, (socklen_t *) &len);
 	
    printf("CLIENT pass \n");
	    // assert(len == sizeof(struct sockaddr_in)); 
    return rc;
}


int
UDP_Close(int fd)
{
    return close(fd);
}

