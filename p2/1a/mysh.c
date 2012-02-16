#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

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
	//Constants
	const int MAX_LINE = 512;
	int batch_mode = 0;
	char error_message[30] = "An error has occurred\n";
	FILE* target_stream;
	//Check should I run in batch mode?
	if(argc >= 2){
		//Do batch stuff
		target_stream = fopen(argv[1], "r");
		if (target_stream == NULL) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}
		batch_mode = 1;
	} else {
		target_stream = stdin;
	}


	//Housekeeping
	int done = 1;

	while(done){

		if(!batch_mode){
			printf("mysh> ");
		}

		//Read input variables
		int argsize;
		char in[MAX_LINE];
		char* new_argv[512]; //Will not exceed 256 arguments
		int new_argc = 0;
		int r; // Result for stuff
		char* arg;

		if(fgets(in, MAX_LINE + 1, target_stream) == NULL){
			if(feof(target_stream)){
				//Read till the end of file for batch mode
				break;
			}else{
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
		}
		if(strlen(in) == 512){
			write(STDERR_FILENO, error_message, strlen(error_message));
			char c;
			scanf("%c%*[^\n]%*c", &c);
			continue;
		}

		if(batch_mode){ //Print stuff out if batch mode
			write(STDOUT_FILENO, in, strlen(in));
		}

		if(strlen(in) == 1){
			continue; //Just a return char
		}

		//Remove old stuff
		memset(&new_argv, 0, sizeof(new_argv));

		strtok(in, "\n"); //Trick to remove new lines
		new_argc = 0;
		arg = strtok (in, " ");
		while (arg != NULL) {
			new_argv[new_argc] = arg;
			new_argc++;
			arg = strtok (NULL, " ");
		}
		
		//Checking for builtin commands
		if(!strcmp(new_argv[0], "exit")){
			break;	
		}else if(!strcmp(new_argv[0], "cd")){
			//Code for cd
			char* target_dir;
			if(new_argv[1] == '\0'){
				char * home_dir;
				home_dir = getenv ("HOME");
				if (home_dir == NULL){
					write(STDERR_FILENO, error_message, strlen(error_message));
					continue;	
				}
				target_dir = home_dir;
			}else{
				target_dir = new_argv[1];
			}
			if(chdir(target_dir) != 0){
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
			continue;
		}else if(!strcmp(new_argv[0], "pwd")){
			//Code for pwd
			char cwd[1024];
			if (getcwd(cwd, sizeof(cwd)) != NULL){
				write(1, cwd, strlen(cwd));
				write(1, "\n", 1); //Lazy hack to pass test
			}else{
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
			continue;
		}


		//Forking here
  		pid_t child_pid;
  		pid_t tpid;
    	int child_status;

		//Do fun feature, c compiler
		int fun_feature = 0;
		int arglength = strlen(new_argv[0]);
		if(new_argv[0][arglength - 1] == 'c' && new_argv[0][arglength - 2] == '.'){
			
			char* gcc_args[4];
			gcc_args[0] = "gcc";
			gcc_args[1] = "-o";
			gcc_args[2] = "/tmp/run";
			gcc_args[3] = new_argv[0];

			//Replace running proggy
			new_argv[0] = "/tmp/run";
			//Forking for gcc
			child_pid = fork();
			if(child_pid == 0) {
				execvp("gcc", gcc_args);
				write(STDERR_FILENO, error_message, strlen(error_message));
				exit(1);
			}else {
				wait(&child_status);
			}

		}


		child_pid = fork();
		if(child_pid == 0) {
			execvp(new_argv[0], new_argv);
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}else {
			wait(&child_status);
		}
	}

	exit(0);
}

