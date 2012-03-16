#include "mem.h"
#include <unistd.h>
#include <stdio.h> //Marked for Del
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct __node_t {
	int size;
	struct __node_t *next;
} node_t;

typedef struct __header_t {
	int size;
	int magic;
} header_t;

int m_error;
node_t* startOfAlloc = NULL;

//Temp debug variables
void* start;
int size;

int Mem_Init(int sizeOfRegion, int debug){
	if(sizeOfRegion <= 0 || startOfAlloc){
		m_error = E_BAD_ARGS;
		return -1;	
	}
	// open the /dev/zero device
	int fd = open("/dev/zero", O_RDWR);

	int pageSize = getpagesize();
	size_t length = sizeOfRegion; 
	if(sizeOfRegion % pageSize){
		length = sizeOfRegion + (pageSize - (sizeOfRegion % pageSize));
	}
	void * memPtr;
	memPtr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (memPtr == MAP_FAILED) { perror("mmap"); return -1; }

	//printf("%p\n", memPtr);
	// close the device (don't worry, mapping should be unaffected)
	close(fd);	

	start = memPtr;
	size = length;

	//Put free list pointer
	memPtr = memPtr + length - sizeof(node_t);
	//printf("%p\n", memPtr);
	startOfAlloc = memPtr;
	startOfAlloc->size = length - sizeof(node_t);
	startOfAlloc->next = NULL;

	return 0;
}

void *Mem_Alloc(int size){
	//get best fit
	node_t* currNode = startOfAlloc;
	int currBestSize = 0;
	node_t* currBestNode = NULL;
	int realSize = size + sizeof(header_t);
	if(realSize % 8){
		realSize = size + sizeof(header_t) + (8 - (size % 8)); //Celing to the nearest 8 byte
	}
	while(currNode != NULL){
		if((realSize <= currNode->size) && (currBestSize == 0 || currNode->size < currBestSize)){
			currBestSize = currNode->size;
			currBestNode = currNode;
		}
		currNode = currNode->next;	
	}
	if (currBestSize == 0) {
		m_error = E_NO_SPACE;
		return NULL;
	}

	//There is space, Yeah
	void* ptr = currBestNode;
	ptr = ptr - currBestSize;
	//printf("%p\n", ptr);
	//printf("%p\n", currBestNode);
	//Create new ptr
	header_t* newHeader = ptr;
	newHeader->size = size;
	newHeader->magic = size + sizeof(header_t);
	
	currBestNode->size = currBestSize - realSize;
	return ptr + sizeof(header_t);
}

int Mem_Free(void *ptr, int coalesce){

	return 0;
}

void Mem_Dump(){
	int* startdump = start;	
	int i;
	for (i = 0; i < size/4; i++) {
		printf("%d - %p - %d", i*4, startdump, (int)*startdump);
		printf("\n");
		startdump = startdump + 1;
	}
}
