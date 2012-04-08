#include "cs537.h"
#include "request.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)> <pool_size>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too

void usage(char* argv[]){
	fprintf(stderr, "Usage: %s <port> <pool_size> <buffers> <schedalg>\n", argv[0]);
	exit(1);
}
typedef struct conn_node{
	int conn; //Connection fd
	int size; //For use in SFF
	struct conn_node* next;
} conn_node_t;

typedef struct threadpool_t {
	int pool_size;	//number of active threads
	pthread_t *threads;	//pointer our threads
	conn_node_t *buffer_head;	//pointer out buffer head
	conn_node_t *buffer_tail;	//pointer out buffer tail (for use in FIFO)
	int buffer_size;
	pthread_mutex_t buffer_lock;		//lock on the queue list
	pthread_cond_t buffer_not_empty;	//non empty and empty condidtion vairiables
	pthread_cond_t buffer_empty;
} threadpool_t;

void* worker_thread(void* p){
	threadpool_t* pool = (threadpool_t *) p;
	conn_node_t* curr;
	while(1){
		pthread_mutex_lock(&(pool->buffer_lock));  //get the buffer lock.

		while( pool->buffer_size == 0) {	//if the size is 0 that means there is no connection to serve 
			pthread_mutex_unlock(&(pool->buffer_lock));
			pthread_cond_wait(&(pool->buffer_not_empty),&(pool->buffer_lock));
		}

		curr = pool->buffer_head;	//set the cur variable.  
		printf("Size: %d ", pool->buffer_size);
		pool->buffer_size--; //Reduce 1

		if(pool->buffer_size == 0) {
			pool->buffer_head = NULL;
			pool->buffer_tail = NULL;
		} else {
			pool->buffer_head = curr->next; //Pop one off the buffer  
		}

		if(pool->buffer_size == 0) {
			//the q is empty again, now signal that its empty.
			pthread_cond_signal(&(pool->buffer_empty));
		}
		pthread_mutex_unlock(&(pool->buffer_lock));
		requestHandle(curr->conn);
		Close(curr->conn);
	}
}

int main(int argc, char *argv[]){
    int listenfd, port, clientlen, pool_size, max_buffer_size;
    struct sockaddr_in clientaddr;
	int i;

    if(argc != 5) {
		usage(argv);
    }
    port = atoi(argv[1]);
    pool_size = atoi(argv[2]);
    max_buffer_size = atoi(argv[3]);
	if(pool_size < 1 || max_buffer_size < 1 || !(!strcmp("FIFO", argv[4]) || !strcmp("SFF", argv[4]))){
		usage(argv);
	}
	int isFIFO = 0;
	if(strcmp("FIFO", argv[4]) == 0){
		isFIFO = 1;
	}
	
	//Creating thread pool
	threadpool_t* pool;
	pool = (threadpool_t *)malloc(sizeof(threadpool_t));
	if(pool == NULL){
		app_error("Not enough memory");
	}
	
	//Initialize buffer queue
	pool->buffer_head = NULL;
	pool->buffer_tail = NULL;
	pool->buffer_size = 0;

	pool->threads = (pthread_t*) malloc(sizeof(pthread_t) * pool_size);
	if(pool->threads == NULL){
		app_error("Not enough memory");
	}
	pool->pool_size = pool_size;

	//initialize mutex and condition variables.  
	if(pthread_mutex_init(&pool->buffer_lock,NULL)) {
		app_error("Unable to initialize lock");
	}
	if(pthread_cond_init(&(pool->buffer_empty),NULL)) {
		app_error("Unable to initialize CV");
	}
	if(pthread_cond_init(&(pool->buffer_not_empty),NULL)) {
		app_error("Unable to initialize CV");
	}

	//Spawn threads
	for (i = 0;i < pool_size;i++) {
		if(pthread_create(&(pool->threads[i]), NULL, worker_thread, pool)) {
			app_error("Unable to create threads");
		}
	}
	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);

		//Create nodes
		
		conn_node_t* curr = (conn_node_t*) malloc(sizeof(conn_node_t));
	
		//Add to buffer
		if(isFIFO){
			curr->conn = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
			curr->next = NULL; //Since it is always the last
			pthread_mutex_lock(&(pool->buffer_lock));
			while(pool->buffer_size >= max_buffer_size){ //Full buffer, blocking connection
				pthread_mutex_unlock(&(pool->buffer_lock));
				pthread_cond_wait(&(pool->buffer_empty),&(pool->buffer_lock));
			}
			if(pool->buffer_size == 0) {
				pool->buffer_head = curr;  //set to only one
				pool->buffer_tail = curr;
				pthread_cond_signal(&(pool->buffer_not_empty));  //I am not empty.  
			} else {
				pool->buffer_tail->next = curr;	//add to end of buffer;
				pool->buffer_tail = curr;			
			}
			pool->buffer_size++;
			pthread_mutex_unlock(&(pool->buffer_lock));
		}else{
				
		}	

	}
	return 0;
}
