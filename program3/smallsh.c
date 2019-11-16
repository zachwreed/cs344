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

/****************************************
** Reset Command Array
** Description: Resets arrray up to args count
** Prerequisites: called in command function, args contains a valid positive integer
*****************************************/
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
   printf("%s\n", token);
 }
 return i;
}

/****************************************
** Execute Command Function
** Description: Takes command and executes with working directory
** Prerequisites: line points to a valid string, command[] is initialized to NULL
** Postrequisites: command[] is initialized up to [i],
*****************************************/
void exec_command(char* line) {
 int exec_valid = line_args(line);

 if (exec_valid >= 1) {
   execvp(command[0], command);
   exit(0);
 }
 reset_command(exec_valid);
}

/****************************************
** CD Command Function
** Description: Attemps Change of directory. If change, writes current directory to pwd
** Prerequisites:
** Postrequisites:
*****************************************/
char* cd_command(char* line) {
 int cd_valid = line_args(line);
 int cd = -5;
 char cwd[PATH_MAX];
 char *ptr;

 if ((cd_valid != 1 || cd_valid != 2) && strcmp(command[0], CD) != 0) {
   reset_command(cd_valid);
   return NULL;
 }
 char *home = getenv("HOME");
 switch (cd_valid) {
   // if no args ----------------
    case 1:
      chdir(home);
    break;

   // if directory specified ----
    case 2:
      cd = chdir(command[1]);
    break;

    default:
      return NULL;
    break;
 }
 // if wd changed, set pwd to new wd
 reset_command(cd_valid);
 return getcwd(cwd, sizeof(cwd));
}

/****************************************
** Status Command
** Description:
** Prerequisites:
** Postrequisites:
*****************************************/
void status_command(char* line, int term_sig, int exit_status) {
   int st_valid = line_args(line);

   if (st_valid == 1 && strcmp(command[0], STATUS)) {

   }
   reset_command(st_valid);
  }

/****************************************
** Main
** Description: Initializes variables and handles command line loop, calls functions when passed.
** Prerequisites: None
** Postrequisites:
*****************************************/
int main() {
   constructor();
   char *line = NULL;
   size_t line_size = 256;
   size_t buff_line;
   pid_t spawn_pid = -5;
   int sp_child_exit = -5;
   int sp_exit_status = -5;
   int sp_term_sig = -5;

  while(1) {
    printf(": ");
    buff_line = getline(&line, &line_size, stdin);
    line[strcspn(line, "\n")] = '\0';
    fflush(stdin);

    if (strncmp(CD, line, 2) == 0) {
      pwd = cd_command(line);
    }

    else if (strcmp(STATUS, line) == 0) {
      status_command(line, sp_exit_status, sp_term_sig);
    }

    else if (strcmp(EXIT, line) == 0) {
      printf(": exit called\n");
      break;
    }

    // If Non-build in command
    else {
      spawn_pid = fork();

      // IF child spawn, execute function
      if (spawn_pid == 0) {
        exec_command(line);
      }
      waitpid(spawn_pid, &sp_child_exit, 0);
      sp_exit_status = WIFEXITED(sp_child_exit);
      sp_term_sig = WTERMSIG(sp_child_exit);
      // printf("Exit: %d\n", sp_exit_status);
      // printf("Term Sig: %d\n", sp_term_sig);
    }

    // free line pointer
    free(line);
    line = NULL;
    fflush(stdout);
  }
  return 0;
}
