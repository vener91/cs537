#ifndef __REQUEST_H__

typedef struct request{
  int fd;
  int failed;
  struct stat sbuf;
  int size;
  char cgiargs[8192];
  char filename[8192];
  int is_static;
} request_t;


void requestHandle(request_t* r);
request_t* requestInit(request_t* r);
#endif
