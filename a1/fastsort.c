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
	int i;
	for(i = 0; i < NUMRECS; i++){
		if((*(rec_t* const*)p1)->record[i] >  (*(rec_t* const *)p2)->record[i]){
			return 1;
		}else if((*(rec_t* const*)p1)->record[i] <  (*(rec_t* const*)p2)->record[i] == 1){
			return -1;
		}
	}
	return 0;
	/*
	//Assuming 64 bit machines, uses long
	rec_t recA = *(*(rec_t* const*)p1);
	rec_t recB = *(*(rec_t* const*)p2);
	int64_t recRA = (int64_t)recA.record;
	int64_t recRB = (int64_t)recB.record;
	int i;
	int length = NUMRECS / 2; //since size of int64_t is 2x the size of int
	for(i = 0; i < length; i++){	
		if(recRA > recRB ){
			return 1;
		}else if(recRB < recRA){
			return -1;
		}
		recRA = (int64_t)(&recRA + sizeof(int64_t));
		recRB = (int64_t)(&recRB + sizeof(int64_t));
	}
	//Compare last 4 bytes
	if((int64_t)(&recRA - sizeof(int)) > (int64_t)(&recRB - sizeof(int)) ){
		return 1;
	}else if(recRB < recRA){
		return -1;
	}

	return 0;
	*/
}

void usage(char *prog) {
	fprintf(stderr, "Usage: %s -i inputfile -o outputfile", prog);
	exit(1);
}

int main(int argc, char *argv[]) {
	// arguments
	char *inFile = "/no/such/file";
	char *outFile = "/no/such/file";

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

	rec_t r;
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
	return 0;
}

