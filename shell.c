/*
        Implementing a Shell in C
        COP4610 Project 1
        Stanley Vossler, Matthew Echenique, Carlos Pantoja-Malaga
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

typedef struct {
        int size;
        char **items;
} tokenlist;

typedef struct {
	int flag;
	pid_t pid;
	tokenlist *cmd;
} job;

// parser : part 1
char *get_input(void);

tokenlist *get_tokens(char *input);
tokenlist *new_tokenlist(void);

void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

// environment variable replacement : part 2
char *capture_env(char * str);
void replace_env(tokenlist *tokens);

// tilde expansion : part 4
void expand_tilde(tokenlist *tokens);
char * capture_tilde(char * str);

// path search : part 5
tokenlist *tokenize_path(char * path);

// external command execution : part 6 
void execute(job *jobs, tokenlist *paths, tokenlist *tokens, int commandCounter, char** commandlist);

// input/output redirection : part 7
int checkRedirect(char **args_list);
char* getIOName(char **args_list);
int outputIndex(char **args_list);

// piping : part 8
int countPipes(char **args_list);

// background processing : part 9
void init_jobs(job *jobs);
int in_background(tokenlist *tokens);
int available_job(job *jobs);
int push_job(job *jobs);

int add_job(job *jobs, pid_t pid, tokenlist *tokens);
void print_cmd(job j);
void print_jobs(job *jobs);
void update_jobs(job *jobs);
void wake_job(job *jobs);

// built-in functions : part 10
int checkBuiltInExecute(job *jobs, char **args_list, int commandCounter, char **commandlist, tokenlist *tokens);

int main() {
	
	printf("[!] You are currently running [shell.c].\n");
	
	tokenlist *paths = tokenize_path(getenv("PATH")); // capture $PATH directories and tokenize
	
	job jobs[30]; // initialize jobs
	init_jobs(jobs);
	
	int commandCounter = 0; // vars to keep track of executed commands
	char *commandlist[1000];
	
	while(1) { // loop which runs shell environment
		printf("%s@%s:%s> ", getenv("USER"), getenv("MACHINE"), getenv("PWD"));
		
		char *input = get_input(); // capture input and tokenize
		tokenlist *tokens = get_tokens(input);
		
		replace_env(tokens); // replace environment variables and expand tilde within tokens
		expand_tilde(tokens);
		
		execute(jobs, paths, tokens, commandCounter, commandlist); // execute command, take into account: jobs, paths, the command parsed as tokens, current counter and command history
		commandCounter++;
		
		free(input); // clean up
		free(tokens);
	}
	
	free_tokens(paths); // more clean up
	return 0;
}

// built-in commands : part 10
int checkBuiltInExecute(job *jobs, char **args_list, int commandCounter, char **commandlist, tokenlist *tokens) {
	int j = 1;
	if(strcmp(args_list[0], "exit") == 0) {
		if(commandCounter == 3) {
			printf("The following are the last three valid commands executed:\n");
			
			for(int i = commandCounter - 1; i > commandCounter - 4; i--) {
				printf("%d. %s\n", j, commandlist[i]);
				j++;
			}
		} else if(commandCounter == 2) {
			printf("The following are the last two valid commands executed:\n");
			
			for(int i = commandCounter - 1; i > commandCounter - 3; i--) {
				printf("%d. %s\n", j, commandlist[i]);
				j++;
			}
		} else if(commandCounter == 1) {
			printf("The last command executed was:\n");
			printf("%d. %s\n", j, commandlist[commandCounter - 1]);
		} else {
			printf("No valid commands were executed in this shell.\n");
		}
		wake_job(jobs);
		printf("exiting\n");
		exit(0);
	} else if(strcmp(args_list[0], "echo") == 0) {
		for(int i = 1; args_list[i] != NULL; i++)
			printf("%s ", args_list[i]);
		printf("\n");
		return 2;
	} else if(strcmp(args_list[0], "cd") == 0) {
		if(args_list[2] != NULL) {
			printf("Too many arguments!\n");
		}
		
		if(args_list[1] == NULL)
			chdir(getenv("HOME"));
		else {
			DIR * directory = opendir(args_list[1]);
			if(errno == ENOTDIR)
				printf("This is not a directory\n");
			else if(errno == ENOENT)
				printf("This directory does not exist\n");
			else {
				closedir(directory);
				chdir(args_list[1]);
			}
		}
		
		return 3;
	} else if(strcmp(args_list[0], "jobs") == 0) {
		
		print_jobs(jobs);
		return 4;
	}
	return 0;
}

// background processing : part 9

// function that will return whether a & is found in the correct place for background processing
int in_background(tokenlist *tokens) {
	int size = tokens->size;
	int b = 0; // boolean
	
	if(strcmp(tokens->items[size - 1], "&") == 0) // check for &
		b = 1; // return true
	
	if(b == 1) { // take out & from tokens, we already know its going to be run in the background we can relax
		tokens->size-=1;
		free(tokens->items[size - 1]);
	}
	
	return b; // return boolean value
}

// function that initializes jobs with flag as a 0, this means the jobs spot is open
void init_jobs(job *jobs) {
	for(int i = 0; i < 30; i++) {
		jobs[i].flag = 0;
	}
}

// function that will add job to jobs array
int add_job(job *jobs, pid_t pid, tokenlist *tokens) {
	int index = push_job(jobs); // index of open job
	
	jobs[index].flag = 1; // flag that job is being worked on
	jobs[index].pid = pid; // job pid
	jobs[index].cmd = new_tokenlist(); // job command 
	
	for(int i = 0; i < tokens->size; i++)
		add_token(jobs[index].cmd, tokens->items[i]); // add command to job 
	
	return index; // return index of latest job
}

// function to find index of available job
int push_job(job *jobs) {
	int k; // index of available job
	for(int i = 0; i < 30; i++)
		if(jobs[i].flag == 0) { // find first index of an available job
			k = i;
			break;
		}
		
	return k;
}

// function that will return boolean on whether there is a job slot available
int available_job(job *jobs) {
	for(int i = 0; i < 30; i++)
		if(jobs[i].flag == 0) {
			return 1;
		}
		
	return 0;
}

// function that wil print job command
void print_cmd(job j) {
	for(int i = 0; i < j.cmd->size; i++)
		printf("%s ", j.cmd->items[i]);
	printf("\n");
}

// function that will print running jobs
void print_jobs(job *jobs) {
	for(int i = 0; i < 30; i++) {
		if(jobs[i].flag != 0) {
				printf("[%d] Running ", (i + 1));
				print_cmd(jobs[i]);
		}
	}
}


// function check up on jobs
void wake_job(job *jobs) {
	for(int i = 0; i < 30; i++) {
		if(jobs[i].flag != 0) {
			int status;
			jobs[i].pid = waitpid(jobs[i].pid, &status, WNOHANG);
			
			if(jobs[i].pid == 0) {
				printf("[%d] Done ", (i + 1));
				print_cmd(jobs[i]);
			}
		}
	}
}

// piping : part 8
int countPipes(char **args_list) { // simply adds to counter whenver "|" is found
	int pipes = 0;
	for(int i = 0; args_list[i] != NULL; i++) {
		if(strcmp(args_list[i], "|") == 0)
			pipes++;
	}
	
	return pipes;
}

// input/output redirection : part 7

int checkRedirect(char **args_list) {
	int redirect = 0;
	for(int i = 0; args_list[i] != NULL; i++) {
		if(strcmp(args_list[i], ">") == 0)
			redirect = 2; // 2 == output needed
		else if(strcmp(args_list[i], "<") == 0)
			redirect = 1; // 1 == input needed
	}
	
	return redirect;
}

char* getIOName(char **args_list) {
	char *fileName;
	for(int i = 0; args_list[i] != NULL; i++) {
		if(strcmp(args_list[i], ">") == 0 || strcmp(args_list[i], "<") == 0) {
			fileName = malloc(strlen(args_list[i + 1]));
			fileName = args_list[i + 1];
		}
	}
	
	return fileName;
}

int outputIndex(char **args_list) {
	int index;
	
	for(int i = 0; args_list[i] != NULL; i++) {
		if(strcmp(args_list[i], ">") == 0 || strcmp(args_list[i], "<") == 0)
			index = i;
	}
	
	return index;
}

// external command execution : part 6

void execute(job *jobs, tokenlist *paths, tokenlist *tokens, int commandCounter, char** commandlist) {
	int inOut = 0; // ternary to represent: 0 = no i/o, 1 = in, 2 = out
	int numPipes = 0; // number of pipes
	int builtin; // bool to represent if built in is needed or not
	char *ioFileName; //name of the input or output redirection filename
	char *args_list[tokens->size + 1]; //  **char version of tokenlist tokens
	char *path_cmd; // used for path search
	const char *cmd = tokens->items[0];
	
	// background process handling
	int background = in_background(tokens);
	int available = available_job(jobs);
	int status;
	
	for(int i = 0; i < tokens->size; i++)
		args_list[i] = tokens->items[i]; // initialized args_list from tokens
	
	args_list[tokens->size] = NULL; // args_list must end with NULL for execv()
	
	commandlist[commandCounter] = malloc(strlen(args_list[0]));
	strcpy(commandlist[commandCounter], args_list[0]);
	
	for(int i = 1; args_list[i] != NULL; i++) {
		if(args_list[i] != NULL) {
			commandlist[commandCounter] = (char *) realloc(commandlist[commandCounter], strlen(commandlist[commandCounter]) + strlen(args_list[i]) + 2);
			
			strcat(commandlist[commandCounter], " ");
			strcat(commandlist[commandCounter], args_list[i]);
		}
	}
	
	//check for built in commands before execution, if there are built in commands execute them, if not return to this function.
	builtin = checkBuiltInExecute(jobs, args_list, commandCounter, commandlist, tokens);
	//allocate memory for path_cmd which is used for path search
	path_cmd = (char *) malloc(2 * sizeof(char *));
	
	if(builtin == 0) {
		inOut = checkRedirect(args_list);
		numPipes = countPipes(args_list);
		
		// Single Piping Implementation
		if(numPipes > 0) {
			
			//intialize command path 1 & args
			char *cmd_path1 = (char*) malloc(strlen("/bin") + strlen(args_list[0]));
			char **cmd1_args = malloc(sizeof(args_list) / sizeof(args_list[0]) * sizeof(char *));
			cmd1_args[0] = args_list[0];
			int j = 0;
			for(int i = 0; strcmp(args_list[i], "|") != 0; i++) {
				cmd1_args[i] = (char *) malloc(strlen(args_list[i]));
				cmd1_args[i] = args_list[i];
				j++;
			}
			cmd1_args[j] = NULL;
			
			strcpy(cmd_path1, "/bin/");
			strcat(cmd_path1, args_list[0]);
			
			//intialize command path 2 & args
			char *cmd_path2 = (char*) malloc(strlen("/bin") + strlen(args_list[2]));
			char **cmd2_args = malloc(sizeof(args_list) / sizeof(args_list[0]) * sizeof(char *));
			cmd2_args[0] = args_list[j + 1];
			int k = 0;
			for(int i = j + 1; args_list[i] != NULL; i++) {
				cmd2_args[k] = (char *) malloc(strlen(args_list[i]));
				cmd2_args[k] = args_list[i];
				k++;
			}
			cmd2_args[k] = NULL;
			
			strcpy(cmd_path2, "/bin/");
			strcat(cmd_path2, cmd2_args[0]);
			
			//This portion is based off "Project 1 - Slides #3.pdf (pg. 6)
			
			int p_fds[2];
			pipe(p_fds);
			
			int pid1 = fork();
			if(pid1 == 0) {
				close(p_fds[0]);
				dup2(p_fds[1], 1);
				execv(cmd_path1, cmd1_args);
					perror("First execv in pipe implementation");
				close(p_fds[1]);
				exit(1);
			}
			
			int pid2 = fork();
			if(pid2 == 0) {
				close(p_fds[1]);
				dup2(p_fds[0], 0);
				execv(cmd_path2, cmd2_args);
					perror("Second execv in pipe implementation");
				close(p_fds[0]);
				exit(1);
			}
			
			close(p_fds[0]);
			close(p_fds[1]);
			
			waitpid(pid1, NULL, 0);
			waitpid(pid2, NULL, 0);
		} // end of piping
		
		FILE *fp;
		path_cmd = (char *) malloc(2 * sizeof(char *));
		
		pid_t pid = fork();
		int fd; // file descriptor for i/o redirection
		
		
		if(pid == 0) { // child process
			// file redirection
			if(inOut == 1) { // if input redirection is needed
				ioFileName = getIOName(args_list);
				fd = open(ioFileName, O_RDONLY, 0);
				dup2(fd, 0);
				
				inOut = 0;
				
				fp = fopen(ioFileName, "r"); // check if file exists
				if(fp == NULL) {
					perror("\nError\n");
				}
				
				fclose(fp);
			}
			
			if(inOut == 2) { // if output redirection is needed
				ioFileName = getIOName(args_list);
				fd = creat(ioFileName, 0644);
				dup2(fd, 1);
				
				// gets rid of uncessary args for execv(), only needed for redirection
				for(int i = outputIndex(args_list); args_list[i] != NULL; i++) {
					args_list[i] = NULL;
				}
				
				inOut = 0;
			}
			
			if(!numPipes) { //do not execute if there are pipes present
				//this for loop itereates through every path in the $PATH variable
				for(int i = 0; i < paths->size; i++) {
					path_cmd = (char *) realloc(path_cmd, strlen(paths->items[i]) + strlen(cmd) + 1);
					char appendCmd[sizeof(char *)];
					strcpy(appendCmd, cmd);
					strcpy(path_cmd, paths->items[i]);
					strcat(path_cmd, appendCmd);
					execv(path_cmd, args_list);
					path_cmd = NULL;
				}
			}
		} else {
			if(background == 0) {
				waitpid(pid, &status, 0);
			} else if(available == 1) {
				int id = add_job(jobs, pid, tokens);
				
				printf("[%d] [%d]\n", (id + 1), pid);
				wake_job(jobs);
			} else {
				printf("Error: The maximum amount of allowed background processes are currently running... Executing in foreground.\n");
				waitpid(pid, &status, 0);
			}
		}
	}
}

// path search : part 5

// function that will add missing / to tokenized $PATH directory
void fix_path(tokenlist *paths) {
        const char* slash = "/";
		
        for(int i = 0; i < paths->size; i++) { // iterate through each potential path directory
                paths->items[i] = (char *) realloc(paths->items[i], strlen(paths->items[i]) + 2); // allocate enough memory to support adding the slash
                strcat(paths->items[i], slash); // concatenate the slash
        }
}

// function that will tokenize $PATH environment variable, forked from get_tokens()
tokenlist *tokenize_path(char* path) {
        char *buf = (char *) malloc(strlen(path) + 2);
        strcpy(buf, path);
        
        tokenlist *paths = new_tokenlist();
        char* tok = strtok(buf, ":");
        while(tok != NULL) {
                add_token(paths, tok);
                tok = strtok(NULL, ":");
        }
        free(buf);
        fix_path(paths);
        
        return paths;
}

// tilde expansion : part 4

// function that will return complete path with tilde expanded
char * capture_tilde(char *str) {
        char *path = malloc(strlen(str)); // char array that will hold rest of the path
        char *home = malloc(strlen(getenv("HOME") + 1)); // char array that holds deep copy of home path
        strcpy(home, getenv("HOME"));
        
        for(int i = 0; i < strlen(str); i++) { // copy the path without the ~
                path[i] = str[i + 1];
        }
		
		const char * c_path = path; // need const char array to avoid memory leak
        char *expanded =  malloc(strlen(home) + strlen(path) + 2); // concatenate home directory and path with the tilde removed
        strcat(expanded, home);
        strcat(expanded, c_path);
		
        free(home); // clean up
        free(path);
		
        return expanded;
}

// function that will update tokenlist with expanded tilde
void expand_tilde(tokenlist *tokens) {
        for(int i = 0; i < tokens->size; i++) { // loop through tokens
                if(tokens->items[i][0] == '~') { // find tokens with ~ at first char
                        tokens->items[i] = (char *) realloc(tokens->items[i], strlen(capture_tilde(tokens->items[i])) + 1); // allocate enough memory to support new path with home directory
                        strcpy(tokens->items[i], capture_tilde(tokens->items[i])); // copy over expanded path
						
                        //printf("debug> tilde expanded token: %s\n", tokens->items[i]);
                }
        }
}

// environment variable replacement : part 2

// function that will return environment variable as a char array
char * capture_env(char *str) {
        char * substr = malloc(strlen(str) + 1); // char array that holds environment variable substring
        
        for(int i = 0; i < sizeof(substr); i++) // populate char array with substring
                substr[i] = str[i + 1];
                
        char * envvar = getenv(substr); // capture value of environment variable
        
        free(substr); // clean up
        return envvar; // return environment variable actual value as a char array
}

// function that will iterate through the tokenlist and replace environment variables with actual values
void replace_env(tokenlist *tokens) {
        
        char* envvar; // char array will hold captured environment variable value
        envvar = (char *) malloc(2 * sizeof(char)); // allocate base mem, unsure if necessary
        
        for(int i = 0; i < tokens->size; i++) { // iterate through tokens
                if(tokens->items[i][0] == '$') { // if token begins with $ 
                        envvar = (char *) realloc(envvar, strlen(capture_env(tokens->items[i])) + 1); // allocate enough memory to support environment variable transfer
                        strcpy(envvar, capture_env(tokens->items[i])); // hold environment variable
                        
                        tokens->items[i] = (char *) realloc(tokens->items[i], strlen(envvar) + 1); // reallocate token memory to support environment variable value size ex: PATH is large string
                        strcpy(tokens->items[i], envvar); // copy over value
                }
        }
        
        free(envvar); // clean up
}

// parser : part 1

tokenlist *new_tokenlist(void) {
        tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
        tokens->size = 0;
        tokens->items = (char **) malloc(sizeof(char *));
        tokens->items[0] = NULL;
        
        return tokens;
}


void add_token(tokenlist *tokens, char *item) {
        int i = tokens->size;
        
        tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char * ));
        tokens->items[i] = (char *) malloc(strlen(item) + 1);
        tokens->items[i + 1] = NULL;
        strcpy(tokens->items[i], item);
        
        tokens->size += 1;
}

char *get_input(void) {
        char *buffer = NULL;
        int bufsize = 0;
        
        char line[5];
        while(fgets(line, 5, stdin) != NULL) {
                int addby = 0;  
                char *newln = strchr(line, '\n');
                if (newln != NULL)
                        addby = newln - line;
                else
                        addby = 5 - 1;
                
                buffer = (char *) realloc(buffer, bufsize + addby);
                memcpy(&buffer[bufsize], line, addby);
                bufsize += addby;
                
                if (newln != NULL)
                        break;
        }

        buffer =  (char *) realloc(buffer, bufsize + 1);
        buffer[bufsize] = 0;
        
        return buffer;
}

tokenlist *get_tokens(char *input) {
        char *buf = (char *) malloc(strlen(input) + 1);
        strcpy(buf, input);
        
        tokenlist *tokens = new_tokenlist();
        
        char *tok = strtok(buf, " ");
        while(tok != NULL) {
                add_token(tokens, tok);
                tok = strtok(NULL, " ");
        }
        
        free(buf);
        
        return tokens;
}

void free_tokens(tokenlist *tokens) {
        for(int i = 0; i < tokens->size; i++)
                free(tokens->items[i]);
        free(tokens->items);
        free(tokens);
}
