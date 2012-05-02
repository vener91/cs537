#include <stdio.h>
#include "udp.h"
#include "mfs.h"

int
main(int argc, char *argv[])
{

	int sd = UDP_Open(-1);
	assert(sd > -1);

	struct sockaddr_in saddr;
	int rc = UDP_FillSockAddr(&saddr, "localhost", 3000);
	assert(rc == 0);

	return 0;
}


