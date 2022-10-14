// Modify this file for your assignment
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "shell.h"

#define MAX_BUFFER_SIZE 80

// user interupt ctrl-c
void sigint_handler(int sig) {
	write(1,"Terminating through signal handler\n",35); 
	exit(0);
}


// get the prompt and user input
void showprompt(char* command) { //how to get user input

	char* buffer = NULL;
	size_t temp;

	printf("mini-shell> ");                // mini-shell prompt
	// fgets(line,MAX_BUFFER_SIZE,stdin); //get line
	getline(&buffer, &temp, stdin);       // get user input
	strcpy(command, buffer);              // coppy buffer value to command
}	

void parse_command(char* command, char* cmd[]) {
	char* temp = command;
	int i;

	for (i = 0; i < MAX_BUFFER_SIZE;i++) {
		cmd[i] = strsep(&temp, " ");

		if (cmd[i] == NULL) {
			break;
		}

		if (strlen(cmd[i]) == 0)
			i--; 
	}

}

void cd(char* command1[]) {

	if (command1[0] == NULL) {
		perror("Command is empty");
	} else {
		if (chdir (command1[1]) != 0) {
			perror("Cannot execute change directory\n");
		}
	}
}

void history(linkedlist_t* history);

// command1[0], command1[1]   --> cd(command1[0] <directory>(command1[1])
int check_com(char* command1[], linkedlist_t* hist) { //checking command

	char* function[4];
	int temp = 0;        // default to 0
	int i;

	function[0] = "cd";        // temp = 1
	function[1] = "help";      // temp = 2
	function[2] = "history";   // temp = 3
	function[3] = "exit";      // temp = 4

	if (strcmp(function[0], command1[0]) == 0) {
		printf("directory changed!\n");
		cd(command1);
		temp = 3;
	} else if (strcmp(function[1], command1[0]) == 0) {
		printf("cd - change directory\n");
		printf("help - explains all the built-in functions\n");
		printf("history - shows all previous commands used\n");
		printf("exit - exits the program\n");
		temp = 3;
	} else if (strcmp(function[2], command1[0]) == 0) {
		history(hist);
		temp = 3;
	} else if (strcmp(function[3], command1[0]) == 0) {
		printf("exit\n");
		FreeLinkedList(hist);
		exit(0);
	}

	return temp;
}

int process_command(char* command, char* command1[], char* command2[], linkedlist_t* history) {

	char* cmd = NULL; 
	char* temp_cmd;

	int i;
	char* temp[2];
	int pipe_flag = 0;
	int builtin_command;

	cmd = strchr(command, '|');
	temp_cmd = command;

	// separating command between pipe
	for (i = 0; i < 2; i++) {
		temp[i] = strsep(&temp_cmd, "|");
		if (temp[i] == NULL)
			break;
	}

	if (cmd) { // with pipe
		//printf("inside cmd\n");              // ls -l | nl  ;  command1 | command2
		parse_command(temp[0], command1);          // ls -l: temp[0]  
		parse_command(temp[1], command2);           // nl: temp[1]

		strtok(command1[0], "\n");
		strtok(command2[0], "\n");
		strtok(command1[1], "\n");
		strtok(command2[1], "\n");

		pipe_flag = 1;

	} else { // without pipe
		//printf("outside cmd\n");
		parse_command(command, command1);

		strtok(command1[0], "\n"); 
		strtok(command1[1], "\n"); 
	}

	builtin_command = check_com(command1, history);

	if (pipe_flag == 1 && builtin_command == 0)   // executing pipe with flag
		return 1;
	else if (pipe_flag == 0 && builtin_command == 0) // executing without pipe
		return 2;
	else                                            // executing builtin function
		return 3; 
}


void shell_without_pipe(char* command1[]) {

	// executing parent and child process
	pid_t pid = fork();
	char temp[MAX_BUFFER_SIZE];
	char *env[] = {(char*)"PATH=/bin", 0};

	if (pid == -1) {
		perror("failing in FORK fork() command");
		return;
	} else if (pid == 0) {
		printf("child id: %d\n", getpid());

		char* trial[4];
		trial[0] = command1[0];
		trial[1] = command1[1];
		trial[2] = command1[2];
		trial[3] = NULL;

		if (execvp(command1[0], trial) == -1)
			perror("Command not found!");
		exit(1);

	} else {
		wait(NULL);
		return;	
	}
}

void shell_with_pipe(char* command, char* command1[], char* command2[]) {

	pid_t pid1 = fork();
	pid_t pid2;
	int fd[2]; // file descriptor 1: write; 0: read
	char temp1[MAX_BUFFER_SIZE];
	char temp2[MAX_BUFFER_SIZE];
	char *env[] = {(char*)"PATH=/bin", 0};
	// int p = pipe(fd);    // executing
	int s1;  // status pid1
	int s2; // status pid2

	if (pipe(fd) < 0) {
		perror("Pipe not succesful!\n");
		return;
	}

	char* myTemp1[4];
	char* myTemp2[4];

	myTemp1[0] = command1[0];
	myTemp1[1] = command1[1];
	myTemp1[2] = command1[2];
	myTemp1[3] = NULL;
	myTemp2[0] = command2[0];
	myTemp2[1] = command2[1];
	myTemp2[2] = command2[2];
	myTemp2[3] = NULL;

	if (pid1 == -1) {
		perror("failing in FORK1 fork() command");
		return;
	} else if (pid1 == 0) {
		printf("child1 id: %d\n", getpid());
		close(fd[0]);         // close read, write the command to pipe

		if (dup2(fd[1], 1) == -1) {
			perror("FAIL file descriptor");
			return;
		}

		close(fd[1]);         // close write

		if (execvp(command1[0], myTemp1) == -1)
			perror("Command1 not found!");

		exit(0);

	} else {
		waitpid(pid1, &s1, 0);  
		pid2 = fork();

		if (pid2 == -1) {
			perror("failing in FORK2 fork() command");
			return;
		} else if (pid2 == 0) {
			printf("child2 id: %d\n", getpid());
			close(fd[1]);         // close write read the command to pipe

			if (dup2(fd[0], 0) == -1) {
				perror("FAIL file descriptor");
				return;
			}

			close(fd[0]);         // close read

			if (execvp(command2[0], myTemp2) == -1)
				perror("Command2 not found!");

			exit(0);
		} else {

			close(fd[0]);   // close pipe read
			close(fd[1]);   // close pipe write
			//                waitpid(pid1, &s1, 0);
			waitpid(pid2, &s2, 0);
		}
	}
}

node_t* create_node(char* data) {

	// heap memory allocation for new node
	node_t* new_node = (node_t*)malloc(sizeof(node_t));

	// heap memory allocation for data - deep copy
	new_node->data = (char*)malloc(strlen(data) * sizeof(char) + 1);
	strcpy(new_node->data, data);
	new_node->next = NULL;

	return new_node;
}

// returns an empty linkedlist
linkedlist_t* create_empty_hist() {
	linkedlist_t* history = (linkedlist_t*)malloc(sizeof(linkedlist_t));
	if (history == NULL)
		exit(1);
	history->head = NULL;
	return history;
}

void FreeLinkedList(linkedlist_t* list){

//	if (list==NULL){
//		return;
//	}

	if (list->head == NULL) {
		free(list);
	} else {
		node_t* currentnode = list->head;

		node_t* nextnode = currentnode->next;
		free(list);
		while(nextnode != NULL){
			free(currentnode->data);
                        free(currentnode);
			currentnode = nextnode;
			nextnode = nextnode->next;
		}
                free(currentnode->data);
		free(currentnode);
                free(list);
	}
}    

void update_history(linkedlist_t* history, char* command) {

	node_t* newnode = create_node(command);

	if (history->head == NULL) { 
		history->head = newnode;
	} else {
		node_t* iter = history->head;

		while(iter->next != NULL){
			iter=iter->next;
		}

		iter->next = newnode;
	}
}

void history(linkedlist_t* history) {
	node_t* currentnode = history->head;
	int i = 1;

	while(currentnode != NULL) {
		printf("%d: %s\n", i, currentnode->data);
		currentnode = currentnode->next;
		i++;
	}
}

//linkedlist_t* history;

int main() {
	alarm(180); // Please leave in this line as the first statement in your program.
	// This will terminate your shell after 180 seconds,
	// and is useful in the case that you accidently create a 'fork bomb'

	signal(SIGINT, sigint_handler);

	char command[MAX_BUFFER_SIZE];
	char duplicate_command[MAX_BUFFER_SIZE];
	char* command1[MAX_BUFFER_SIZE];    //ls -l
	char* command2[MAX_BUFFER_SIZE];
	int num_pipe; // for pipe and without pipe switching
	linkedlist_t* history = create_empty_hist();

	// A loop that runs forever.
	while(1){
		// Read in 1 line of text
		// The line is coming from 'stdin' standard input
		showprompt(command);
		//strcpy(duplicate_command, command);

		update_history(history, command);

		// check if num_pipe: 1 - command with pipe; 2 - command without pipe; 3 - builtin function
		num_pipe = process_command(command, command1, command2, history);

		if (num_pipe == 1) {
			//	printf("with pipe:\n");  
			//	printf("command %s\n", command);
			//	printf("command1: %s %s\n", command1[0], command1[1]);
			//	printf("command2: %s %s\n", command2[0], command2[1]);
			shell_with_pipe(command, command1, command2);
		} else if (num_pipe == 2) {
			shell_without_pipe(command1);
			//		} else {
			//			printf("builtin functions"); // cd, help, history, exit
	}
	}		

	return 0;
}
