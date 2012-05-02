#ifndef __MFS_h__
#define __MFS_h__

#define MFS_DIRECTORY    (0)
#define MFS_REGULAR_FILE (1)

#define MFS_BLOCK_SIZE   (4096)
#define MFS_BLOCK_STEP_SIZE   (1024)

//Comands
const char MFS_CMD_INIT 	= 0;
const char MFS_CMD_SHUTDOWN = 1;
const char MFS_CMD_LOOKUP 	= 2;
const char MFS_CMD_STAT 	= 3;
const char MFS_CMD_WRITE 	= 4;
const char MFS_CMD_READ 	= 5;
const char MFS_CMD_CREAT 	= 6;
const char MFS_CMD_UNLINK 	= 7;

typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t {
    char name[28];  // up to 28 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;

typedef struct __MFS_Protocol_t {
    char cmd;   // Command type
    int  ipnum; // inode | parent inode
	int  ret;
	char datachunk[4096];
} MFS_Protocol_t;

typedef struct __MFS_Inode_t{
	int size;
	int type;
	int data[14];
} MFS_Inode_t;

typedef struct __MFS_Header_t{
	int file_count;
	int block_count;
	int map[MFS_BLOCK_SIZE];
} MFS_Header_t;

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);

#endif // __MFS_h__