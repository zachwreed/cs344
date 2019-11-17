#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
// Built in commands

#define EXIT "exit"
#define CD "cd"
#define STATUS "status"
#define COM_ARGS 10
char* command[COM_ARGS];
char* pwd;

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

char* cd_command(char* line) {
 int cd_valid = line_args(line);
 int cd = -5;
 char cwd[PATH_MAX];

 if ((cd_valid != 1 || cd_valid != 2) && strcmp(command[0], CD) != 0) {
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

// int main() {
//    constructor();
//    char *line = NULL;
//    size_t line_size = 256;
//    size_t buff_line;
//    pid_t spawn_pid = -5;
//    int sp_child_exit = -5;
//    int sp_exit_status = -5;
//    int sp_term_sig = -5;
//
//     printf(": ");
//     buff_line = getline(&line, &line_size, stdin);
//     line[strcspn(line, "\n")] = '\0';
//     fflush(stdin);
//
//     if (strncmp(CD, line, 2) == 0) {
//       pwd = cd_command(line);
//       printf("%s\n", pwd);
//     }
//     return 0;
//   }

int main(int argc, char **argv)
{
  int in, out;
  int dupErr;
  char *args[] = {"ls"};

  // open input and output files

  //in = open("test.txt", O_RDONLY);
  out = open("sample.txt", O_WRONLY | O_TRUNC | O_CREAT, 0644);

  // replace standard input with input file
  //dup2(in, 0);

  // replace standard output with output file
  dupErr = dup2(out, 1);
  //printf("Test\n");
  // close unused file descriptors
  // execute grep

  execlp("ls", "ls", NULL);
  close(out);

  return 0;
}
