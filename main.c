#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "functions.h"


typedef struct _pipenode PipeNode;

struct _pipenode {
  InstInfo instInfo;
  PipeNode *next;
};


// helpers
PipeNode* pipeNodes(int);
void printP2helper(PipeNode*, int);

int main(int argc, char *argv[])
{
  
  InstInfo curInst;
  InstInfo *instPtr = &curInst;
  int instnum = 0;
  int maxpc;
  FILE *program;
  if (argc != 2)
    {
      printf("Usage: sim filename\n");
		exit(0);
    }

  maxpc = load(argv[1]) - 1;
  //printLoad(maxpc);
  
  PipeNode *head = pipeNodes(5);
  PipeNode *current = head;

  do {

    current->instInfo.inst = 0;
    current->instInfo.pc = -1;
    memset(current->instInfo.string, 0, sizeof(current->instInfo.string));

      fetch(&(current->instInfo));
      decode(&(current->instInfo));
      current = current->next;

      decode(&(current->instInfo));
      current = current->next;

      execute(&(current->instInfo));
      current = current->next;

      memory(&(current->instInfo));
      current = current->next;

      writeback(&(current->instInfo));

      printP2helper(current->next,instnum++);
  } while (current->instInfo.pc < maxpc);

  printf("Cycles: %d\n", instnum);
  printf("Instructions Executed: %d\n", maxpc+1);

  exit(0);

}

void printP2helper(PipeNode* head, int cycle) {
  InstInfo *fetchInst = &(head->instInfo);
  InstInfo *decodeInst = &((head = head->next)->instInfo);
  InstInfo *executeInst = &((head = head->next)->instInfo);
  InstInfo *memoryInst = &((head = head->next)->instInfo);
  InstInfo *writebackInst = &((head = head->next)->instInfo);

  printP2(fetchInst, decodeInst, executeInst, memoryInst, writebackInst, cycle);

}
/* Print
 *
 * prints out the state of the simulator after each instruction
 */
void print(InstInfo *inst, int count)
{
	int i, j;
	printf("Instruction %d: %d\n",count,inst->inst);
	printf("%s\n",inst->string);
	printf("Fields: {rd: %d, rs: %d, rt: %d, imm: %d}\n",
		inst->fields.rd, inst->fields.rs, inst->fields.rt, inst->fields.imm);
	printf("Control Bits:\n{alu: %d, mw: %d, mr: %d, mtr: %d, asrc: %d, bt: %d, rdst: %d, rw: %d}\n",
		inst->signals.aluop, inst->signals.mw, inst->signals.mr, inst->signals.mtr, inst->signals.asrc,
		inst->signals.btype, inst->signals.rdst, inst->signals.rw);
	printf("ALU Result: %d\n",inst->aluout);
	if (inst->signals.mr)
		printf("Mem Result: %d\n",inst->memout);
	else
		printf("Mem Result: X\n");
	for(i=0;i<8;i++)
	{
		for(j=0;j<32;j+=8)
			printf("$%d: %4d ",i+j,regfile[i+j]);
		printf("\n");
	}
	printf("\n");
}
/*
 * print out the loaded instructions.  This is to verify your loader
 * works.
 */
void printLoad(int max)
{
	int i;
	for(i=0;i<max;i++)
		printf("%d\n",instmem[i]);
}

PipeNode* pipeNodes(int numNodes) {
   PipeNode *first;
  first = (PipeNode *)malloc(sizeof(PipeNode));
  first->instInfo.inst = 0;
  first->next = first;
  PipeNode *cur = first;
  int i;
  for (i = 0; i < numNodes-1; i++) {
    PipeNode *start = cur->next;
    PipeNode *new = (PipeNode *)malloc(sizeof(PipeNode));
    new->instInfo.inst = 0;
    new->instInfo.pc = -1;
    cur->next = new;
    new->next = start;
    cur = cur->next;
  }

  return first;

}
