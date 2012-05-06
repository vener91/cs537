#include <stdio.h>
#include "udp.h"
#include "mfs.c"

int
main(int argc, char *argv[])
{
	printf("Starting Client\n");


	MFS_Init("localhost", 3000);

	char* tx_buffer = malloc(MFS_BLOCK_SIZE);
	char* tx_buffer2 = malloc(MFS_BLOCK_SIZE);
	char* rx_buffer = malloc(MFS_BLOCK_SIZE);
	strcpy(tx_buffer, "This is just a test!");
	strcpy(tx_buffer2, "Is this a test?");
	
	int inum = MFS_Lookup(0, "test");
	if(inum < 0){
		printf("Failed at Lookup\n");
		exit(0);
	}

	MFS_Read(inum, rx_buffer, 1);
	if(rc == -1){
		printf("Failed at Write\n");
		exit(0);
	}
	
	if(strcmp(rx_buffer, tx_buffer) != 0){
		printf("%s - %s\n", rx_buffer, tx_buffer);
		printf("Failed at Write - Strings does not match\n");
		exit(0);
	}

	MFS_Read(inum, rx_buffer, 2);
	if(rc == -1){
		printf("Failed at Write\n");
		exit(0);
	}
	if(strcmp(rx_buffer, tx_buffer2) != 0){
		printf("%s - %s\n", rx_buffer, tx_buffer2);
		printf("Failed at Write - Strings does not match\n");
		exit(0);
	}
	
	//Unlink test2
	/*
	int rc = MFS_Creat(0, MFS_DIRECTORY, "test");
	if(rc < 0){
		printf("Failed at Creat 1\n");
		exit(0);
	}

	int inum = MFS_Lookup(0, "test");
	if(inum < 0){
		printf("Failed at Lookup\n");
		exit(0);
	}

	rc = MFS_Unlink(0, "test");
	if(rc < 0){
		printf("Failed at Unlink 1\n");
		exit(0);
	}
	
	inum = MFS_Lookup(0, "test");
	if(inum >= 0){
		printf("Failed at Lookup 2\n");
		exit(0);
	}

	inum = MFS_Lookup(0, "test");
	if(inum >= 0){
		printf("Failed at Lookup 2\n");
		exit(0);
	}
	*/
	return 0;
}
