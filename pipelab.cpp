#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	if (argc == 1) {
		perror("please, use this like pipeline. arguments are required.");
		return 2;
	}
	
	int MAX_LEN_CMD = 0;
	int COUNT_CMDS = 1;
	int tmp = 0;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '|') {
			if (tmp == 0) {
				perror("please, use this like pipeline. arguments are required.");
				return 2;
			}
			
			COUNT_CMDS++;
			if (tmp > MAX_LEN_CMD) {
				MAX_LEN_CMD = tmp;
			}
			tmp = 0;
		}
		else {
			tmp++;
		}
	}
	if (tmp == 0){
		perror("please, use this like pipeline. arguments are required.");
		return 2;
	}
	if (tmp > MAX_LEN_CMD) {
		MAX_LEN_CMD = tmp;
	}
	
	char ***ARGS = new char**[COUNT_CMDS];
	int k = 1;
	for (int i = 0; i < COUNT_CMDS; ++i) {
		ARGS[i] = new char*[MAX_LEN_CMD+1];
		int j = 0;
		while (k < argc && argv[k][0] != '|') {
			ARGS[i][j] = argv[k];

			j++;
			k++;
		}
		ARGS[i][j] = NULL;
		k++;
	}

	int pipefds[COUNT_CMDS-1][2];
	for (int i = 0; i < COUNT_CMDS-1; ++i) {
		if (pipe(pipefds[i])== -1) {
			perror("pipe fail");
			for (int i = 0; i < COUNT_CMDS; ++i) {
				delete [] ARGS[i];
			}
			delete ARGS;
			return 1;
		}
	}

	pid_t children[COUNT_CMDS];

	for (int i = 0; i < COUNT_CMDS; ++i) {
		pid_t pid = fork();

		if (pid == -1) {
			perror("could not perform fork");
			for (int i = 0; i < COUNT_CMDS; ++i) {
				delete [] ARGS[i];
			}
			delete ARGS;
			return 1;
		}
		if (pid == 0) {
			char** args = ARGS[i];
			int in_code=0;

			if (COUNT_CMDS==1){
				
			}
			else if (i==0) {
				in_code = in_code || dup2(pipefds[0][1], STDOUT_FILENO);
				in_code = in_code || close(pipefds[0][0]);

				for (int j = 1; j < COUNT_CMDS-1; ++j) {
					in_code = in_code || close(pipefds[j][0]);
					in_code = in_code || close(pipefds[j][1]);
				}				
			}
			else if (i==COUNT_CMDS-1) {
				in_code = in_code || dup2(pipefds[i-1][0], STDIN_FILENO);
				in_code = in_code || close(pipefds[i-1][1]);

				for (int j = 0; j < COUNT_CMDS-2; ++j) {
					in_code = in_code || close(pipefds[j][0]);
					in_code = in_code || close(pipefds[j][1]);
				}				
			}
			else{
				in_code = in_code || dup2(pipefds[i-1][0], STDIN_FILENO);
				in_code = in_code || dup2(pipefds[i][1], STDOUT_FILENO);

				in_code = in_code || close(pipefds[i-1][1]);
				in_code = in_code || close(pipefds[i][0]);

				for (int j = 0; j < COUNT_CMDS-1; ++j) {
					if (j!=i-1 && j != i) {
						in_code = in_code || close(pipefds[j][0]);
						in_code = in_code || close(pipefds[j][1]);
					}					
				}
			}
			int re_code = 0;
			if (in_code==1) re_code=1;
			else re_code = execvp(args[0], args);
			for (int i = 0; i < COUNT_CMDS; ++i) {
				delete [] ARGS[i];
			}
			delete ARGS;
			if (re_code == -1){
				return 1;
			}			
			return 0;
		} else {
			children[i] = pid;
		}
	}

	for (int i  = 0; i < COUNT_CMDS-1; ++i) {
		for (int j = 0; j < 2; ++j) {
			close(pipefds[i][j]);
		}
	}

	int exitCode = 0;
	int status;
	for (int i = 0; i < COUNT_CMDS; ++i) {
		waitpid(children[i], &status, 0);
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0) {
				exitCode = 1;
			}
		}
	}

	for (int i = 0; i < COUNT_CMDS; ++i) {
		delete [] ARGS[i];
	}
	delete ARGS;
	
	return exitCode;
}
