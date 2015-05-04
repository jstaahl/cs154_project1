#include "stdio.h"

int main() {
    FILE *f = fopen("testinput.txt", "r");
    int instruction;
    int line = 0;
    while(fscanf(f, "%d[^\n]",&instruction) == 1) {
      printf("%d\n", instruction);
      line++;
    }
}
