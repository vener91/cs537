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

	//Put free list pointer
	memPtr = memPtr + length - sizeof(node_t);
	startOfAlloc = memPtr;
	startOfAlloc->size = length - sizeof(node_t);
	startOfAlloc->next = NULL;

	return 0;
}

void *Mem_Alloc(int size){
	//Check if there is freespace left at all
	if(startOfAlloc == NULL){
		m_error = E_NO_SPACE;
		return NULL;
	}

	//get best fit
	node_t* currNode = startOfAlloc;
	node_t* currBestNode = NULL;
	int realSize = size + sizeof(header_t);
	if(realSize % 8){
		realSize = size + sizeof(header_t) + (8 - (size % 8)); //Celing to the nearest 8 byte
	}
	while(currNode != NULL){
		if((realSize <= currNode->size + sizeof(node_t)) && (currBestNode == NULL || currNode->size <= currBestNode->size)){
			currBestNode = currNode;
		}
		currNode = currNode->next;	
	}
	if (currBestNode == NULL) {
		m_error = E_NO_SPACE;
		return NULL;
	}

	int margin = currBestNode->size + sizeof(node_t) - realSize; //If margin is 8 or less, that means no freeNode
	if(margin <= 8){ //tightfit
		size = size + margin;
		//Change pointers
		currNode = startOfAlloc;
		if(startOfAlloc == currBestNode){ //If best node is the head node
			startOfAlloc = startOfAlloc->next;
		}else{
			while(currNode->next != NULL){
				if(currNode->next == currBestNode){
					currNode->next = currBestNode->next; //Remove currBestNode
					break;
				}
				currNode = currNode->next;
			}
		}

	}
	//There is space, Yeah
	void* ptr = currBestNode;
	ptr = ptr - currBestNode->size;
	//Create new ptr
	header_t* newHeader = ptr;
	newHeader->size = realSize - sizeof(header_t); //Will have problems if sizeof(header_t) != 8
	newHeader->magic = realSize;
	if(margin > 8){	
		currBestNode->size = currBestNode->size - realSize;
	}
	return ptr + sizeof(header_t);
}

int Mem_Free(void *ptr, int coalesce){
	//Check if it is legit
	header_t* currAlloc = ptr - sizeof(header_t);	
	if(ptr == NULL){
		return 0;
	}
	if(currAlloc->magic != currAlloc->size + sizeof(header_t)){	
		return -1;
	}
	
	currAlloc->magic = 0; //Render it invalid
	void* newFreeNodePtr = ptr + currAlloc->size - sizeof(node_t);
	node_t* newFreeNode = (node_t*)newFreeNodePtr;
	newFreeNode->size = currAlloc->size - sizeof(node_t) + sizeof(header_t);
	node_t* currNode;

	currNode = startOfAlloc;
	//Check if it is newhead
	if((void*)currNode <= newFreeNodePtr){
		//Update head
		newFreeNode->next = (node_t*)startOfAlloc;
		startOfAlloc = (node_t*)newFreeNode;
	}else{
		while(currNode != NULL){
			if(currNode->next == NULL || (void*)currNode->next < newFreeNodePtr){
				newFreeNode->next = currNode->next;
				currNode->next = newFreeNode;	
				break;
			}
			currNode = currNode->next;
		}
	}

	if(coalesce){
		//Start coalescing
		currNode = startOfAlloc;
		while(currNode != NULL){
			if (currNode->next == NULL) {
				break;
			}
			if((void*)currNode - currNode->size - sizeof(node_t) == currNode->next){ //Check if need to be coalesce
				currNode->size += currNode->next->size + sizeof(node_t); //Modify new size
				currNode->next = currNode->next->next; //Delete it
			}else{
				currNode = currNode->next;
			}
		}
	}

	return 0;
}
