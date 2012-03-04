all:
		gcc -Wall -Werror -g -fpic -c mem.c
		gcc -shared -o libmem.so mem.o
		gcc -lmem -L. -o memtest memtest.c -Wall -Werror
		export LD_LIBRARY_PATH=`pwd`
