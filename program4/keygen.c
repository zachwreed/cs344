#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<time.h>

int main(int argc, char* argv[]) {
  char pad[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  if(argc < 2) {
    printf("Error: No keygen size specified");
    return 1;
  }

  // refer ASCII values at https://www.cs.cmu.edu/~pattis/15-1XX/common/handouts/ascii.html
  time_t t;
  srand((unsigned) time(&t));
  int keyS = atoi(argv[1]);
  char str[keyS];
  int i, v;
  for(i = 0; i < keyS; i++){
    v = rand() % (26 - 0 + 1); //range of 0 - 26
    str[i] = pad[v];
  }
  printf("%s\n", str);
  return 0;
}
