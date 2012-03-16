#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "mem.h"

int main(int argc, const char *argv[]){
	printf("Testing out malloc...\n");

   assert(Mem_Init(4096, 0) == 0);
   void* ptr = Mem_Alloc(8);
   assert(ptr != NULL);
   exit(0);

}
