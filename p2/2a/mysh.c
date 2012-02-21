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

int main(int argc, char *argv[]) {
	//Constants
	const int MAX_LINE = 1024;
	int batch_mode = 0;
	char error_message[30] = "An error has occurred\n";
	FILE* target_stream;
	//Check should I run in batch mode?
	if(argc >= 3){
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	} else if(argc == 2){
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
			write(1, "mysh> ", 6);
		}

		//Read input variables
		int argsize;
		char in[MAX_LINE];
		char* new_argv[512]; //Will not exceed 256 arguments
		int new_argc;
		int r; // Result for stuff
		char* arg;

		if(fgets(in, MAX_LINE, target_stream) == NULL){
			if(feof(target_stream)){
				//Read till the end of file for batch mode
				break;
			}else{
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
		}
		
		if(batch_mode){ //Print stuff out if batch mode
			write(STDOUT_FILENO, in, strlen(in));
		}

		if(strlen(in) > 513){
			write(STDERR_FILENO, error_message, strlen(error_message));
			continue;
		}
		
		if(strlen(in) == 1){
			continue; //Just a return char
		}

		//Remove old stuff
		memset(&new_argv, 0, sizeof(new_argv));
		strtok(in, "\n"); //Trick to remove new lines
		
		//Redirection detection
		int redirect_mode = 0;	
		int bak, new; //File discriptor variables
		strtok(in, ">");
		arg = strtok (NULL, ">");
		if(arg != NULL) {	
			if(strstr(arg, ">") != NULL){
				//Found > chars 
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}else{
				//Switch stdout
				//Strip front whitespace
				while(arg[0] == ' '){
					arg++;
				}
				char* path =  strtok(arg, " "); //Trick to strip trailling whitespace
				if(strtok(NULL, " ") != NULL){ //Check for multiple redirection files
					write(STDERR_FILENO, error_message, strlen(error_message));
					continue;
				}
				//Enable redirect mode
				redirect_mode = 1;
				
				fflush(stdout);
				bak = dup(1);
				new = open(arg, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
				if (new < 0) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					continue;
				}
				dup2(new, 1);
				close(new);
			}
		}

		new_argc = 0;
		arg = strtok (in, " ");
		while (arg != NULL) {
			new_argv[new_argc] = arg;
			new_argc++;
			arg = strtok (NULL, " ");
		}
		
		//Checking for builtin commands
		if(!strcmp(new_argv[0], "exit")){
			if (new_argc != 1) {
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
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
			if (new_argc == 1) {
				//Code for pwd
				char cwd[1024];
				if (getcwd(cwd, sizeof(cwd)) != NULL){
					write(1, cwd, strlen(cwd));
					write(1, "\n", 1); //Lazy hack to pass test
				}else{
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
			}else{
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
			continue;
		}


		//Forking here
  		pid_t child_pid;
  		pid_t gcc_pid;
		
		//Do fun feature, c compiler
		int fun_mode = 0;
		int arglength = strlen(new_argv[0]);
		if(new_argv[0][arglength - 1] == 'c' && new_argv[0][arglength - 2] == '.'){
			
			char* gcc_argv[5];
			gcc_argv[0] = "gcc";
			gcc_argv[1] = "-o";
			gcc_argv[2] = "/tmp/run";
			gcc_argv[3] = new_argv[0];
			gcc_argv[4] = NULL;
			//Replace running proggy
			new_argv[0] = "/tmp/run";
			//Forking for gcc
			gcc_pid = fork();
			//printf("%d\n", child_pid);
			if(gcc_pid == 0) {
				
				int x = execvp("gcc", gcc_argv);
				write(STDERR_FILENO, error_message, strlen(error_message));
				exit(1);
			}else {
				wait(&gcc_pid);
			}

		}
		child_pid = fork();
		if(child_pid == 0) {
			execvp(new_argv[0], new_argv);
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}else {
			wait(&child_pid);
			if(redirect_mode){	
				fflush(stdout);
				dup2(bak, 1);
				close(bak);
			}

		}
	}

	exit(0);
}
