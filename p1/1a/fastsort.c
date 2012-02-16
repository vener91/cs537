#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "sort.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int cmprect(const void *p1, const void *p2) {	
	//Naive method
	return (*(rec_t* const*)p1)->key - (*(rec_t* const*)p2)->key;
}

void usage(char *prog) {
	//Removing the path to satisfy the test
	char* prog_name = NULL;
	char* pch = strtok (prog, "/");
	while (pch != NULL) {
		prog_name = pch;
		pch = strtok (NULL, "/");
	}
	fprintf(stderr, "Usage: %s -i inputfile -o outputfile\n", prog_name);
	exit(1);
}

int main(int argc, char *argv[]) {
	// arguments
	char *inFile = NULL;
	char *outFile = NULL;
	
	if(argc != 5){
		usage(argv[0]);
	}
	// input params
	int c;
	opterr = 0;
	while ((c = getopt(argc, argv, "i:o:")) != -1) {
		switch (c) {
			case 'i':
				inFile = strdup(optarg);
				break;
			case 'o':
				outFile = strdup(optarg);
				break;
			default:
				usage(argv[0]);
		}
	}

	//Check for required arguments
	if(inFile == NULL || outFile == NULL){
		usage(argv[0]);
	}

	// open and create output file
	int fd = open(inFile, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error: Cannot open file %s\n", inFile);
		exit(1);
	}

	// get size of file and allocate 
	struct stat fileStat;
	if(fstat(fd,&fileStat) < 0) {
		fprintf(stderr, "Error: Cannot open file %s\n", inFile);
		exit(1);
	}
	// Put text in memory
	int sizeOfArray = fileStat.st_size / sizeof(rec_t);
	rec_t *Rblock;
	Rblock = malloc(fileStat.st_size);
	int rc;
	rc = read(fd, Rblock, fileStat.st_size);
	if(rc < 0){
		fprintf(stderr, "Error: Cannot open file %s\n", inFile);
		exit(1);
	}
	rc = close(fd);
	if(rc < 0){
		fprintf(stderr, "Error: Cannot open file %s\n", inFile);
		exit(1);
	}
	
	rec_t *block[sizeOfArray];
	int i = 0;	
	for(i = 0; i < sizeOfArray; i++){
		block[i] = &Rblock[i];
	}

	

	//Sort
	qsort(block, sizeOfArray, sizeof(rec_t *), cmprect);	

	// open and create output file
	fd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if (fd < 0) {
		fprintf(stderr, "Error: Cannot open file %s\n", outFile);
		exit(1);
	}

	for (i = 0; i < sizeOfArray; i++) {
		// fill in random rest of records
		int rc = write(fd, block[i], sizeof(rec_t));
		if (rc != sizeof(rec_t)) {
			fprintf(stderr, "Error: Cannot open file %s\n", outFile);
			exit(1);
			// should probably remove file here but ...
		}
	}

	rc = close(fd);
	exit(0);
}

