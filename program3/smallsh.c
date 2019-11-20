/***************************************
 ** Author: Zach Reed
 ** Description: Smallsh
 ** Date:
 ****************************************/
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>

// Built in commands
#define EXIT "exit"
#define CD "cd"
#define STATUS "status"
// Arg Flags
#define COMMENT "#"
#define BACKGROUND " &"
// Boolean
#define TRUE 1
#define FALSE 0
#define COM_ARGS 10
char* pwd;


/****************************************
** Constructor
** Description: initializes variable pointers to NULL
** Prerequisites: called in main, pwd doesn't store a path, command is empty
*****************************************/
void constructor(char* command[]) {
 pwd = NULL;
 int i;
 for ( i = 0; i < COM_ARGS; i++) {
   command[i] = NULL;
 }
}

// From lecture
void catchSIGINT(int signo){
  char* message = "SIGINT. \n";
  write(STDOUT_FILENO, message, 28);
}

void catchSIGSTP(int signo) {
  char* message = "SIGSTP. \n";
  write(STDOUT_FILENO, message, 25);
  _Exit(0);
}

/****************************************
** Reset Command Array
** Description: Resets arrray up to args count
** Prerequisites: called in command function, args contains a valid positive integer
*****************************************/
void reset_command(int args, char* command[]) {
  int i;
  for (i = 0; i < args; i++) {
    command[i] = NULL;
  }
}

/****************************************
** Line Arguments Function
** Description: Breaks stdin into tokens read into command array based on space delimiter
** Prerequisites: line points to a valid string, command[] is initialized to NULL
** Postrequisites: command[] is initialized up to [i],
*****************************************/
int line_args(char* line, char* command[]) {
 const char ch[2] = " ";
 int i = 0;
 char* token = strtok(line, ch);

 while (token != NULL) {
   // add token to command array
   if (strcmp("&", token) != 0) {
     command[i] = token;
     i++;
   }
   token = strtok(NULL, ch);
   //printf("%s\n", token);
 }
 return i;
}

/****************************************
** Execute Command Function
** Description: Takes command and executes with working directory
** Prerequisites: line points to a valid string, command[] is initialized to NULL
** Postrequisites: command[] is initialized up to [i],
*****************************************/
void exec_command(char* line, int bg, char* command[]) {
 int execArgs = line_args(line, command);
 int dv;
 int in, out;
 int isRedirect = 0;

 if (execArgs >= 1) {
   // command [arg1 arg2 ...] [< input_file] [> output_file] [&]

   if (execArgs > 1) {
     int i;
     for (i = 0; i < execArgs; i++) {
       // if redirect stdout
       if (strcmp(">", command[i]) == 0 && command[i] != NULL && i != 0 && i != execArgs -1) {
        //printf("%s\n", command[i + 1]);
        out = open(command[i+1], O_WRONLY | O_TRUNC | O_CREAT, 0644);
         char* err = command[i+1];
        if (out < 0) {
          perror(command[i+1]);
          exit(1);
        }
        else {
          if (dup2(out, 1) == -1) {
            perror(command[i+1]);
            exit(1);
          }
          fcntl(out, F_SETFD, FD_CLOEXEC);
          isRedirect = 1;
        }
       }
       // if redirect stdin
       else if (strcmp("<", command[i]) == 0 && command[i] != NULL && i != 0 && i != execArgs -1) {
         in = open(command[i+1], O_RDONLY);

         if (in < 0) {
           perror(command[i+1]);
           exit(1);
         }
         else {
           if (dup2(in, 0) == -1) {
             perror(command[i+1]);
             exit(1);
           }
           fcntl(in, F_SETFD, FD_CLOEXEC);
           isRedirect = 1;
         }
       }
     }
   }

   if (bg == TRUE && isRedirect == 0) {
     dv = open("/dev/null", O_RDONLY);
     if (dv < 0) {
       perror("open error");
       exit(1);
     }
     if (dup2(dv, 0) == -1) {
       perror("dup2");
       exit(1);
     }

     fcntl(dv, F_SETFD, FD_CLOEXEC);
   }

   if(isRedirect == 1) {
     if (execlp(command[0], command[0], NULL)) {
       perror(command[0]);
       exit(1);
     }

   }
   else {
     if (execvp(command[0], (char* const*)command)) {
       perror(command[0]);
       exit(1);
     }

     perror("Exec Failure!\n");
     exit(1);
   }
 }
 return;
}

/****************************************
** CD Command Function
** Description: Attemps Change of directory. If change, writes current directory to pwd
** Prerequisites:
** Postrequisites:
*****************************************/
char* cd_command(char* line, char* command[]) {
 int cd_valid = line_args(line, command);
 int cd = -5;
 char cwd[PATH_MAX];
 char *ptr;

 if ((cd_valid != 1 || cd_valid != 2) && strcmp(command[0], CD) != 0) {
   reset_command(cd_valid, command);
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
 return getcwd(cwd, sizeof(cwd));
}

/****************************************
** Status Command
** Description:
** Prerequisites:
** Postrequisites:
*****************************************/
void status_command(char* line, int term_sig, int exit_status, char* command[]) {
   int st_valid = line_args(line, command);

   if (st_valid == 1 && strcmp(command[0], STATUS) == 0) {

     if (exit_status >= 0 && term_sig < 0) {
       printf("exit value %d\n", exit_status);
     }
     if (exit_status < 0 && term_sig >= 0) {
       printf("terminated by signal %d\n", exit_status);
     }
   }
  }


/****************************************
** Main
** Description: Initializes variables and handles command line loop, calls functions when passed.
** Prerequisites: None
** Postrequisites:
*****************************************/
int main() {
  char endOfLine[3];
  char *line = NULL;
  size_t line_size = 256;
  size_t line_n;
  pid_t spawn_pid = -5;
  int sp_child_exit = -5;
  size_t buff_line;
  char* command[COM_ARGS];
  constructor(command);

  int sp_exit_status = 0;
  int sp_term_signal = 0;
  int bg = FALSE;

  struct sigaction SIGINT_action = {0};
  struct sigaction SIGSTP_action = {0};
  struct sigaction SIGIGN_action = {0};

  SIGINT_action.sa_handler = SIG_IGN;
  SIGINT_action.sa_flags = 0;
  sigfillset(&SIGINT_action.sa_mask);
  sigaction(SIGINT, &SIGINT_action, NULL);

  SIGSTP_action.sa_handler = catchSIGSTP;
  SIGSTP_action.sa_flags = 0;
  sigfillset(&SIGSTP_action.sa_mask);
  sigaction(SIGTSTP, &SIGSTP_action, NULL);



  while(1) {
    bg = FALSE;
    printf(": ");
    fflush(stdout);
    buff_line = getline(&line, &line_size, stdin);
    line_n = strlen(line);
    memcpy(endOfLine, &line[line_n - 3], 2);
    endOfLine[2] = '\0';
    line[strcspn(line, "\n")] = '\0';

    // Check Line for Built-In Functions And Handlers

    if (strncmp(CD, line, 2) == 0) {
      pwd = cd_command(line, command);
    }

    else if (strcmp(STATUS, line) == 0) {

      if (WIFEXITED(sp_child_exit)) {
        sp_exit_status = WEXITSTATUS(sp_child_exit);
        sp_term_signal = -1;
      }

      if (WIFSIGNALED(sp_child_exit)) {
        sp_term_signal = WTERMSIG(sp_child_exit);
        sp_exit_status = -1;
      }

      status_command(line, sp_term_signal, sp_exit_status, command);
    }

    else if (strncmp(COMMENT, line, 1) == 0) {
      continue;
    }

    else if (line == NULL) {
      printf("Empty line\n");
      continue;
    }

    else if (strcmp(EXIT, line) == 0) {
      exit(0);
      break;
    }

    // If Non-build in command
    else {
      if (strcmp(BACKGROUND, endOfLine) == 0) {
        bg = TRUE;
      }

      spawn_pid = fork();
      switch(spawn_pid) {
        case -1:
          perror("Hull Breach!\n");
          exit(1);
          break;

        case 0:
          fflush(stdout);
          if (bg == FALSE) {
            SIGINT_action.sa_handler = SIG_DFL;
            sigaction(SIGINT, &SIGINT_action, NULL);
          }
          exec_command(line, bg, command);
          exit(0);
          break;

        default:
        if (bg == FALSE) {
          waitpid(spawn_pid, &sp_child_exit, 0);
        }

        if (bg == TRUE) {
          printf("background pid is %d\n", spawn_pid);
          spawn_pid = waitpid(-1, &sp_child_exit, WNOHANG);
        }
        break;
      }
    }
    reset_command(COM_ARGS, command);

    while ((spawn_pid = waitpid(-1, &sp_child_exit, WNOHANG)) > 0) {
      printf("background pid %d is done:\n", spawn_pid);
      fflush(stdout);
    }
  }
  return 0;
}
