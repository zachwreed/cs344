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
#include <math.h>

// Built in commands
#define EXIT "exit"
#define CD "cd"
#define STATUS "status"
#define PID "$$"
// Arg Flags
#define COMMENT "#"
#define BACKGROUND " &"
// Boolean
#define TRUE 1
#define FALSE 0
#define COM_ARGS 512
char* pwd;
int isBG = TRUE;

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
void catchSIGSTP(int signo){
  fflush(stdout);
  if (isBG == FALSE) {
    write(STDOUT_FILENO, "Exiting foreground-only mode\n", 25);
    isBG = TRUE;
    fflush(stdout);
  }
  else {
    write(STDOUT_FILENO, "Entering foreground-only mode (& is now ignored)\n", 49);
    isBG = FALSE;
    fflush(stdout);
  }
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
 int iSize;
 int idx;
 int pid;
 char* pidS = NULL;
 char *idxS = NULL;
 char* tokenNP = NULL;

 char* token = strtok(line, ch);

 while (token != NULL) {
   // add token to command array
   if (strcmp("&", token) != 0) {
     if((idxS = strstr(token, PID)) != NULL) {
       idx = (int)(idxS - token);

       pid = getpid();
       int iSize = floor(log10(abs(pid))) + 1;
       //printf("PID: %d, Size: %d\n", pid, iSize);
       pidS = malloc(iSize);
       tokenNP = malloc(sizeof(token) - 2);
       sprintf(pidS, "%d", pid);

       int j = 0;
       while(token[j] != '$') {
         tokenNP[j] = token[j];
         j++;
       }
       //tokenNP[j] = '\0';

       command[i] = strncat(tokenNP, pidS, idx);

       free(pidS);
       free(tokenNP);
       pidS = NULL;
       tokenNP = NULL;
     }
     else {
       command[i] = token;
     }
     i++;
   }
   idxS = NULL;
   pidS = NULL;
   token = strtok(NULL, ch);
 }
 int j;
 // for (j = 0; j < 2; j++){
 //   printf("%s ", command[j]);
 // }
 // printf("\n");
 return i;
}

/****************************************
** Execute Command Function
** Description: Takes command and executes with working directory
** Prerequisites: line points to a valid string, command[] is initialized to NULL
** Postrequisites: command[] is initialized up to [i],
*****************************************/
void exec_command(char* line, int bg, char* command[], int execArgs) {
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
char* cd_command(char* line, char* command[], int cd_valid) {
 int cd = -5;
 char cwd[PATH_MAX];

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
void status_command(char* line, char* command[], int st_valid, int isBG, int childExit) {

   if (isBG == TRUE) {
     if (WIFEXITED(childExit)) {
       int exitStatus = WEXITSTATUS(childExit);
       printf("exit value %d\n", exitStatus);
     }
     else {
       int termSig = WTERMSIG(childExit);
       printf("terminated by signal %d\n", termSig);
     }
   }
   else {
     if (st_valid == 1 && strcmp(command[0], STATUS) == 0) {
       if (WIFEXITED(childExit)) {
         int exitStatus = WEXITSTATUS(childExit);
         printf("exit value %d\n", exitStatus);
       }
       else {
         int termSig = WTERMSIG(childExit);
         printf("terminated by signal %d\n", termSig);
       }
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
  char* pidS;
  int bg;

  struct sigaction SIGINT_action = {0};
  struct sigaction SIGSTP_action = {0};
  struct sigaction SIGIGN_action = {0};

  // Ignore Cntr-C when Signaled
  SIGINT_action.sa_handler = SIG_IGN;
  SIGINT_action.sa_flags = 0;
  sigfillset(&SIGINT_action.sa_mask);
  sigaction(SIGINT, &SIGINT_action, NULL);

  // Redirect Cntr-Z to Function
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
    int args = 0;

    if (strncmp(CD, line, 2) == 0) {
      args = line_args(line, command);
      pwd = cd_command(line, command, args);
    }

    else if (strncmp(STATUS, line, 6) == 0) {
      args = line_args(line, command);
      if (strcmp(BACKGROUND, endOfLine) != 0) {
        status_command(line, command, args, FALSE, sp_child_exit);
      }
      else {
        status_command(line, command, args, TRUE, sp_child_exit);
      }
    }

    else if (strncmp(COMMENT, line, 1) == 0) {
      continue;
    }

    else if (line == NULL) {
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
      args = line_args(line, command);
      spawn_pid = fork();
      switch(spawn_pid) {
        case -1:
          perror("Hull Breach!\n");
          exit(1);
          break;

        case 0:

          if(bg == FALSE) {
            SIGINT_action.sa_handler = SIG_DFL;
            sigaction(SIGINT, &SIGINT_action, NULL);
          }
          exec_command(line, bg, command, args);
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
      printf("background pid %d is done: ", spawn_pid);
      status_command(line, command, args, TRUE, sp_child_exit);
      reset_command(COM_ARGS, command);
      fflush(stdout);
    }
  }
  return 0;
}
