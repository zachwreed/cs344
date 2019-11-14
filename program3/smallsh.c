/***************************************
 ** Author: Zach Reed
 ** Description: Smallsh
 ** Date:
 ****************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>
#include <limits.h>


// Built in commands
#define EXIT "exit"
#define CD "cd"
#define STATUS "status"
#define COM_ARGS 10
char* command[COM_ARGS];
char* pwd;

/****************************************
** Constructor
** Description: initializes variable pointers to NULL
** Prerequisites: called in main, pwd doesn't store a path, command is empty
*****************************************/

void constructor() {
 pwd = NULL;
 for (int i = 0; i < COM_ARGS; i++) {
   command[i] = NULL;
 }
}

void reset_command(int args) {
  for (int i = 0; i < args; i++) {
    command[i] = NULL;
  }
}

/****************************************
** Line Arguments Function
** Description: Breaks stdin into tokens read into command array based on space delimiter
** Prerequisites: line points to a valid string, command[] is initialized to NULL
** Postrequisites: command[] is initialized up to [i],
*****************************************/

int line_args(char* line) {
 const char ch[2] = " ";
 int i = 0;
 char* token = strtok(line, ch);

 while (token != NULL) {

   // add token to command array
   command[i] = token;
   i++;
   token = strtok(NULL, ch);
   //printf("%s\n", token);
 }
 return i;
}

void exec_command(char* line) {
 int ex_valid = line_args(line);

 if (exec_valid >= 1) {
   execvp(command[0], command);
   exit(0);
 }
 reset_command(ex_valid);
}

void cd_command(char* line) {
 int cd_valid = line_args(line);
 char cwd[PATH_MAX];
 int cd = -5;

 if ((cd_valid != 1 || cd_valid != 2) && strcmp(command[0], CD) != 0)) {
   reset_command(cd_valid);
   return;
 }
 switch (cd_valid) {
   // if no args ----------------
    case 1:
      char *home = getenv("HOME");
      chdir(home);
    break;

   // if directory specified ----
    case 2:
      cd = chdir(command[1]);
    break;

    default:
      return;
    break;
 }
 // if wd changed, set pwd to new wd
 if(cd == 0) {
   getcwd(cwd, sizeof(cwd));
   pwd = cwd;
 }
 reset_command(cd_valid);
}

void status_command(char* line) {
 int st_valid = line_args(line);

 if (st_valid == 1 && strcmp(command[0], STATUS)) {
   printf("Not Implemented\n");
 }
 reset_command(st_valid);
}

int main() {
 constructor();
 char *line = NULL;
 size_t line_size = 256;
 size_t buff_line;
 pid_t spawn_pid = -5;
 int spawn_pid_exit = -5;

while(1) {

  printf(": ");
  buff_line = getline(&line, &line_size, stdin);
  line[strcspn(line, "\n")] = '\0';
  fflush(stdin);

  if (strncmp(CD, line, 2) == 0) {
    cd_command(line);
  }

  else if (strcmp(STATUS, line) == 0) {
    printf(": status called\n");
  }

  else if (strcmp(EXIT, line) == 0) {
    printf(": exit called\n");
    break;
  }

  else {
    spawn_pid = fork();

    // IF child spawn, execute function
    if (spawn_pid == 0) {
      exec_command(line);
    }

    waitpid(spawn_pid, &spawn_pid_exit, 0);
  }

  // free line pointer
  free(line);
  line = NULL;

}
return 0;
}
