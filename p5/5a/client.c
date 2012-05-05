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

	MFS_Protocol_t* tx_protocol;
	MFS_Protocol_t* rx_protocol;
	tx_protocol = (MFS_Protocol_t*)malloc(sizeof(MFS_Protocol_t));
	rx_protocol = (MFS_Protocol_t*)malloc(sizeof(MFS_Protocol_t));

	//Test code starts here
	char* name = "Test woot";

	tx_protocol->cmd = MFS_CMD_CREAT;
	tx_protocol->ipnum = 0;
	tx_protocol->datachunk[0] = (char)MFS_DIRECTORY;
	strcpy(tx_protocol->datachunk + sizeof(char), name);
	rc = UDP_Write(sd, &saddr, tx_protocol, sizeof(MFS_Protocol_t));
	if (rc > 0) {
		int tries_left = 10;
		while(UDP_Read(sd, &saddr, rx_protocol, sizeof(MFS_Protocol_t), 5) < -1){
			tries_left--;
			if(!tries_left){
				printf("Timeout \n");
				exit(0);
			}
		}
		//Does get something back
		printf("%d\n", rx_protocol->ret);
	}
	return 0;
}
