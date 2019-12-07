#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<time.h>

/***********************************************
** Function: Main
** Prerequisite: args[] contains {command, keySize}
************************************************/
int main(int argc, char* argv[]) {
  if(argc < 2) {
    printf("Error: No keygen size specified");
    return 1;
  }
  char pad[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "; // range of pad values

  time_t t;                       // holds time for rand()
  srand((unsigned) time(&t));     // seed time
  int keyS = atoi(argv[1]);       // parse char[] size arg to int
  char str[keyS + 1];             // add 1 for newline
  memset(str, '\0', sizeof(str)); // clear out str

  int i, v;
  for(i = 0; i < keyS; i++){      // loop through arg size
    v = rand() % (26 - 0 + 1);    // rand range of 0 - 26
    str[i] = pad[v];              // set str to pad char
  }
  printf("%s\n", str);            // print to stdout
  return 0;
}
