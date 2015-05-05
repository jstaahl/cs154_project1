#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int encodeRFormat(int op, int func);
int encodeIFormat(int op);
int encodeBFormat(int op);
int encodeJFormat(int op);
int encodeWFormat(int op);
int regToInt(char *reg);

int main(int argc, char *argv[]) {
    FILE *f = fopen(argv[1], "r");
    char *line = NULL;
    size_t len = 0;
        
    char *token;
    while (getline(&line, &len, f) != -1) {
      token = strtok(line, " ");

      if (!strcmp(token, "addi")) {
	printf("%d\n", encodeIFormat(0b110001));
      } else if (!strcmp(token, "add")) {
	printf("%d\n", encodeRFormat(0b100000, 0b100000));
      } else if (!strcmp(token, "and")) {
	printf("%d\n", encodeRFormat(0b100000, 0b100100));
      } else if (!strcmp(token, "sub")) {
	printf("%d\n", encodeRFormat(0b100000, 0b101000));
      } else if (!strcmp(token, "sgt")) {
	printf("%d\n", encodeRFormat(0b100000, 0b110000));
      } else if (!strcmp(token, "lw")) {
	printf("%d\n", encodeWFormat(0b010001));
      } else if (!strcmp(token, "sw")) {
	printf("%d\n", encodeWFormat(0b010010));
      } else if (!strcmp(token, "beq")) {
	printf("%d\n", encodeBFormat(0b001010));
      } else if (!strcmp(token, "jr")) {
	printf("%d\n", encodeRFormat(0b100101, 0));
      } else if (!strcmp(token, "jal")) {
	printf("%d\n", encodeJFormat(0b001000));
      }
    }
}

int encodeRFormat(int op, int func) {
  int rs, rt, rd, shamt;
  char *token;
  int instruction;
  rs = rt = rd = shamt = 0;

  rd = regToInt(strtok(NULL, ", "));
  token = strtok(NULL, ", ");
  if (token != NULL) {
    rs = regToInt(token);
    rt = regToInt(strtok(NULL, ", "));
  } else {
    rs = rd;
    rd = 0;
  }

  //format and concatenate
  instruction = op << 26;
  instruction += (rs & 31) << 21;
  instruction += (rt & 31) << 16;
  instruction += (rd & 31) << 11;
  instruction += (shamt & 0x1f) << 6;
  instruction += (func & 0x3f);

  return instruction;
}

int encodeIFormat(int op) {
  int rs, rt, imm;
  int instruction;
  rs = rt = imm = 0;

  rt = regToInt(strtok(NULL, ", "));
  rs = regToInt(strtok(NULL, ", "));
  imm = atoi(strtok(NULL, ", "));

  //format and concatenate
  instruction = op << 26;
  instruction += (rs & 31) << 21;
  instruction += (rt & 31) << 16;
  instruction += (imm & 0xffff);

  return instruction;
}

// for beq instruction
int encodeBFormat(int op) {
  int rs, rt, imm;
  int instruction;
  rs = rt = imm = 0;

  rs = regToInt(strtok(NULL, ", "));
  rt = regToInt(strtok(NULL, ", "));
  imm = atoi(strtok(NULL, ", "));

  //format and concatenate
  instruction = op << 26;
  instruction += (rs & 31) << 21;
  instruction += (rt & 31) << 16;
  instruction += (imm & 0xffff);

  return instruction;
}

int encodeJFormat(int op) {
  int imm;
  int instruction;
  imm = 0;

  imm = atoi(strtok(NULL, ", "));

  instruction = op << 26;
  instruction += (imm & 0x3ffffff);
  
  return instruction;
}

// for lw and sw instructions
int encodeWFormat(int op) {
  int rs, rt, imm;
  int instruction;
  rs = rt = imm = 0;

  rt = regToInt(strtok(NULL, ", "));
  imm = atoi(strtok(NULL, ", ("));
  rs = regToInt(strtok(NULL, ")"));

  //format and concatenate
  instruction = op << 26;
  instruction += (rs & 31) << 21;
  instruction += (rt & 31) << 16;
  instruction += (imm & 0xffff);

  return instruction;
}

int regToInt(char *reg) {
  return atoi(&(reg[1]));
}
