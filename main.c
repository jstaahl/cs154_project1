#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "functions.h"


typedef struct _pipenode PipeNode;

struct _pipenode {
	InstInfo instInfo;
	PipeNode *next;
};


// helpers
PipeNode* pipeNodes(int);
void wireDataForwarding(PipeNode *current, int instnum);
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

	int stallF = 0;
	int stallD = 0;
	int stallX = 0;
	int stallM = 0;
	int stallW = 0;

	do {

		current->instInfo.inst = 0;
		current->instInfo.pc = -1;
		memset(current->instInfo.string, 0, sizeof(current->instInfo.string));

		//if (stallF == 0) {
			fetch(&(current->instInfo));
			decode(&(current->instInfo));
			current = current->next;
			//}
		
		decode(&(current->instInfo));

		current = current->next;
			
		execute(&(current->instInfo));
		current = current->next;
		
		memory(&(current->instInfo));
		current = current->next;
			
		writeback(&(current->instInfo));

		wireDataForwarding(current, instnum);

		//if (current->instInfo.signals.mtr == 0b01)
			
		printP2helper(current->next,instnum++);
	
	} while (current->instInfo.pc < maxpc);
	
	printf("Cycles: %d\n", instnum);
	printf("Instructions Executed: %d\n", maxpc+1);

	exit(0);

}

void wireDataForwarding(PipeNode *current, int instnum) {
	PipeNode *threeBack = current->next->next;
	InstInfo *info = &(threeBack->instInfo);
	InstInfo *infoNext;
	InstInfo *infoNextNext;
	
	int i;
	for (i = 0; i < 3; i++) {
		infoNext = &(threeBack->next->instInfo);
		infoNextNext = &(threeBack->next->next->instInfo);

		// sw
		if (info->signals.mw == 1) {
			// infoNext is I format
			if (infoNext->destreg == 0b00) {
				// if sw's rt = infoNext's rt
				if (infoNext->fields.rt == info->fields.rt) {
					info->destdata = infoNext->aluout;
				}
				// if sw's rs = infoNext's rt (sw reads two registers)
				// (only if infoNext is not a sw operation, because it's not writing to rt in that case)
				if (infoNext->fields.rs == info->fields.rt && info->signals.mw == 0) {
					info->input2 = infoNext->aluout;
					printf("$$$$$$$$$$$$$3 set input2 = %d%s\n", info->input2);
				}
			// info is R format
			} else if (infoNext->destreg == 0b01) {
				// if sw's rt = infoNext's rd 
				if (info->fields.rt == infoNext->fields.rd) {
					info->destdata = infoNext->aluout;
				}
				// if sw's rs = infoNext's rd
				if (info->fields.rs == infoNext->fields.rd) {
					info->input1 = infoNext->aluout;
				}
			}
		}

		if (info->signals.rw && instnum > 1 + i) {
			if (info->destreg == 0b00) {
				// write to rt
				// addi, lw
				
				// info is reading from the reg that infoNext wrote to
				if ((info->fields.rs == infoNext->fields.rt && infoNext->destreg == 0b00) ||
				 (info->fields.rs == infoNext->fields.rd && infoNext->destreg == 0b01) ){
					info->input1 = infoNext->aluout;
				}

				// info is reading from the memory location that infoNext wrote to
				// i.e. mem read after mem write
				// i think this is off... info's execute hasn't happend yet...
				if (infoNextNext->aluout == infoNext->aluout && infoNextNext->signals.mw == 1 && infoNext->signals.mr == 1) {
					infoNext->memout = infoNextNext->destdata;
				}


				if (infoNextNext->signals.mw == 1 && infoNext->signals.mr == 1) {
					printf("*************HIT TEST*************\n");
					printf("infoNextNext->aluout: %d\n", infoNextNext->aluout);
					printf("infoNext->aluout: %d\n", infoNext->aluout);
				}
				
			} else if (info->destreg == 0b01) {
				// write to rd
				// add, or, sub
				if (infoNext->destreg == 0b00) {
					if (info->fields.rs == infoNext->fields.rt) {
						info->input1 = infoNext->aluout;
						//printf("**************R format: info->input1 = infoNext->aluout;");
						//printf("  (%s) aluout: %d\n", infoNext->string, infoNext->aluout);
					}
					if (info->fields.rt == infoNext->fields.rt) {
						info->input2 = infoNext->aluout;
						printf("$$$$$$$$$$$$$1 set input2 = %d%s\n", info->input2);
						//printf("**************R Format: info->input2 = infoNext->aluout;");
					}
				}
				else if (infoNext->destreg == 0b01) {
					if (info->fields.rs == infoNext->fields.rd) {
						info->input1 = infoNext->aluout;
						//printf("**************R format: info->input1 = infoNext->aluout;");
						//printf("  (%s) aluout: %d\n", infoNext->string, infoNext->aluout);
					}
					if (info->fields.rt == infoNext->fields.rd) {
						info->input2 = infoNext->aluout;
						printf("$$$$$$$$$$$$$2 set input2 = %d%s\n", info->input2);
						//printf("**************R Format: info->input2 = infoNext->aluout;");
					}
				}
			}
		}

		threeBack = threeBack->next;
	}
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
