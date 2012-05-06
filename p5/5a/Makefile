CC   = gcc
OPTS = -Wall

all: server lib client

# this generates the target executables
server: server.o udp.o
	$(CC) -o server server.o udp.o 

client: client.o client2.o udp.o
	$(CC) -o client client.o udp.o 
	$(CC) -o client2 client2.o udp.o 

lib: mfs.o udp.o
	$(CC) -Wall -Werror -shared -fPIC -g -o libmfs.so mfs.c udp.c
	bash ./checkEnv.sh

clean:
# this is a generic rule for .o files 
%.o: %.c 
	$(CC) $(OPTS) -c $< -o $@

clean:
	rm -f server.o client.o client2.o udp.o mfs.o server client client2 lib
	rm libmfs.so
