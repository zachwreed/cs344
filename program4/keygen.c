#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<time.h>

int main(int argc, char* argv[]) {
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
    v = 90 - rand() % 27; //range of 0 - 26
    if (v == 64) {
      str[i] = 32;
    }
    else {
      str[i] = v;
    }
  }
  str[keyS - 1] = 10;
  printf("%s", str);
  return 0;
}
