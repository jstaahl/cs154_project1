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
PipeNode* insertDataHazStall(PipeNode **wb_node_ptr, int instnum);
void insertControlHazStall(PipeNode *wb_node, int instnum);
void printP2helper(PipeNode*, int);

int instructionsExecuted;

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
	PipeNode *last;

	instructionsExecuted = 0;
	int stall = 0;

	do {

		if (!stall) {
			current->instInfo.inst = 0;
			current->instInfo.pc = -1;
			memset(current->instInfo.string, 0, sizeof(current->instInfo.string));

			fetch(&(current->instInfo));
		}
		decode(&(current->instInfo));
		
		current = current->next;
		
		setPCWithInfo(&(current->instInfo));

		decode(&(current->instInfo));

		current = current->next;
			
		execute(&(current->instInfo));
		current = current->next;
		
		memory(&(current->instInfo));
		current = current->next;
			
		writeback(&(current->instInfo));



		printP2helper(current->next,instnum++);

		if (current->instInfo.inst != 0) {
			instructionsExecuted++;
		}



		last = insertDataHazStall(&current, instnum);
		if (last == current) { //no stall was inserted
			wireDataForwarding(last, instnum);
			insertControlHazStall(last, instnum);
			stall = 0;
		} else {
			stall = 1;
		}

	} while (last->instInfo.pc < maxpc);
	
	printf("Cycles: %d\n", instnum);
	printf("Instructions Executed: %d\n", instructionsExecuted);

	exit(0);

}


void makeNoOp(PipeNode *wb_node_ptr) {
	PipeNode *if_node = wb_node_ptr->next;

	if_node->instInfo.inst = 0;
	if_node->instInfo.pc = -1;
	memset(if_node->instInfo.string, 0, sizeof(if_node->instInfo.string));
}

void insertControlHazStall(PipeNode *wb_node, int instnum) {
	PipeNode *id_node = wb_node->next->next;
	InstInfo *id_instInfo = &(id_node->instInfo);

	if (id_instInfo->fields.op == 0b100101 || id_instInfo->fields.op == 0b001000 ||
			(id_instInfo->fields.op == 0b001010 && (id_instInfo->s1data - id_instInfo->s2data == 0) )) {
		makeNoOp(wb_node);
	}

}

PipeNode *insertStall(PipeNode **wb_node_ptr) {
	PipeNode *wb_node = *wb_node_ptr;

	wb_node->instInfo.inst = 0;
	wb_node->instInfo.pc = -1;
	memset(wb_node->instInfo.string, 0, sizeof(wb_node->instInfo.string));

	PipeNode *id_node = wb_node->next->next;
	PipeNode *mem_node = id_node->next->next;

	mem_node->next = mem_node->next->next;	
	wb_node->next = id_node->next;
	id_node->next = wb_node;


	*wb_node_ptr = mem_node->next;
	return mem_node;
}


// update wb_node_ptr to the logical replacement for wb_node_ptr
// return the last node in the list
PipeNode *insertDataHazStall(PipeNode **wb_node_ptr, int instnum) {
	PipeNode *wb_node = *wb_node_ptr;
	PipeNode *id_node = wb_node->next->next;

	PipeNode *next_node = id_node->next;
	InstInfo *idni = &(next_node->instInfo);
	InstInfo *idnni = &(next_node->next->instInfo);

	PipeNode *ret = wb_node;
	next_node = id_node->next;

	if (id_node->instInfo.signals.btype == 0b11 && idnni->inst != 0) {
		if (idnni->signals.mr == 1) {
			ret = insertStall(wb_node_ptr);
			return ret;
		}
	}

	if (idni->inst == 0) {
		return ret;
	}

	if (id_node->instInfo.signals.btype == 0b11) {
		// id node is beq
		if (idni->destreg == 0b00) {
			if (id_node->instInfo.fields.rs == idni->fields.rt) {
				ret = insertStall(wb_node_ptr);
				return ret;
			}

			if (id_node->instInfo.fields.rt == idni->fields.rt) {
				ret = insertStall(wb_node_ptr);
				return ret;
			}
		} else if (idni->destreg == 0b01) {
			if (id_node->instInfo.fields.rs == idni->fields.rd) {
				ret = insertStall(wb_node_ptr);
				return ret;
			}

			if (id_node->instInfo.fields.rt == idni->fields.rd) {
				ret = insertStall(wb_node_ptr);
				return ret;
			}
		}
	}

	if(id_node->instInfo.signals.mr == 1) {
			// idnode is lw

		if (idni->signals.mr == 1 && id_node->instInfo.fields.rs == idni->fields.rt) {
			// two lw's
			ret = insertStall(wb_node_ptr);
			return ret;
		}

	}	else if (id_node->instInfo.signals.mw == 1) {
		// id node is sw

		if (idni->signals.mr == 1 && id_node->instInfo.fields.rs == idni->fields.rt) {
			// lw then sw
			ret = insertStall(wb_node_ptr);
			return ret;
		}

	} else if (id_node->instInfo.destreg == 0b01) {
		// id node is R format

		if (idni->signals.mr == 1) {
			// lw then R format
			if (id_node->instInfo.fields.rs == idni->fields.rt) {
				ret = insertStall(wb_node_ptr);
				return ret;
			}

			if (id_node->instInfo.fields.rt == idni->fields.rt) {
				ret = insertStall(wb_node_ptr);
				return ret;
			}

		}

	} 

	return ret;

}

void wireDataForwarding(PipeNode *wb_node, int instnum) {
	PipeNode *id_node = wb_node->next->next;
	PipeNode *next_node = id_node->next;
	InstInfo *id_instInfo = &(id_node->instInfo);
	InstInfo *next_instInfo;
	
	int i;
	for (i = 0; i < 3 && i < instnum - 1; i++, next_node = next_node->next) {
		next_instInfo = &(next_node->instInfo);

		if (next_instInfo->inst == 0) {
			continue;
		}

		if (id_instInfo->signals.btype == 0b11) {
			// id node is beq
			if (next_instInfo->destreg == 0b00) {
				if (next_instInfo->signals.mr) {
					if (id_instInfo->fields.rs == next_instInfo->fields.rt) {
						id_instInfo->s1data = next_instInfo->memout;
					}

					if (id_instInfo->fields.rt == next_instInfo->fields.rt) {
						id_instInfo->s2data = next_instInfo->memout;
					}
				} else if (next_instInfo->signals.btype != 0b11) {
					if (id_instInfo->fields.rs == next_instInfo->fields.rt) {
						id_instInfo->s1data = next_instInfo->aluout;
					}

					if (id_instInfo->fields.rt == next_instInfo->fields.rt) {
						id_instInfo->s2data = next_instInfo->aluout;
					}
				}
			} else if (next_instInfo->destreg == 0b01) {
				if (id_instInfo->fields.rs == next_instInfo->fields.rd) {
					id_instInfo->s1data = next_instInfo->aluout;
				}

				if (id_instInfo->fields.rt == next_instInfo->fields.rd) {
					id_instInfo->s2data = next_instInfo->aluout;
				}
			}
		}

		// sw
		if (id_instInfo->signals.mw == 1) {
			// infoNext is I format, and it's not another sw (because sw should have a destreg of -1, b/c it doesn't write)
			if (next_instInfo->destreg == 0b00) {
				// if sw's rt = infoNext's rt
				if (next_instInfo->fields.rt == id_instInfo->fields.rt) {
					if (next_instInfo->signals.mr == 0)
						// sw and addi
						id_instInfo->destdata = next_instInfo->aluout;
					else
						// sw and lw
						id_instInfo->destdata = next_instInfo->memout;						
				} 

				// if sw's rs = infoNext's rt (sw reads two registers)
				if (next_instInfo->fields.rs == id_instInfo->fields.rt) {
					if (next_instInfo->signals.mr == 0)
						// sw and addi, sw reads addi's result for the mem address
						id_instInfo->input2 = next_instInfo->aluout;
					else
						// sw and lw, sw reads from what lw writes to
						//  but does next have its memout yet?
						id_instInfo->input2 = next_instInfo->memout;
				}
			// info is R format
			} else if (next_instInfo->destreg == 0b01) {
				// if sw's rt = infoNext's rd 
				if (id_instInfo->fields.rt == next_instInfo->fields.rd) {
					id_instInfo->destdata = next_instInfo->aluout;
				}
				// if sw's rs = infoNext's rd
				if (id_instInfo->fields.rs == next_instInfo->fields.rd) {
					id_instInfo->input1 = next_instInfo->aluout;
				}
			}
		}

		if (id_instInfo->signals.rw) {
			if (id_instInfo->destreg == 0b00) {
				// write to rt
				// addi, lw
				
				// info is reading from the reg that infoNext wrote to
				if ((id_instInfo->fields.rs == next_instInfo->fields.rt && next_instInfo->destreg == 0b00) ||
				 (id_instInfo->fields.rs == next_instInfo->fields.rd && next_instInfo->destreg == 0b01) ){
					id_instInfo->input1 = next_instInfo->aluout;
				}
				
			} else if (id_instInfo->destreg == 0b01) {
				// write to rd
				// add, or, sub
				if (next_instInfo->destreg == 0b00) {

					if (next_instInfo->signals.mr == 0) {
						if (id_instInfo->fields.rs == next_instInfo->fields.rt) {
							id_instInfo->input1 = next_instInfo->aluout;
						}
						if (id_instInfo->fields.rt == next_instInfo->fields.rt) {
							id_instInfo->input2 = next_instInfo->aluout;
						}
					} else {
						// id_instInfo is an arith
						// next_instInfo is lw
						if (id_instInfo->fields.rs == next_instInfo->fields.rt) {
							id_instInfo->input1 = next_instInfo->memout;
							// printf("  (%s) input1: %d\n", id_instInfo->string, next_instInfo->aluout);
						}
						if (id_instInfo->fields.rt == next_instInfo->fields.rt) {
							id_instInfo->input2 = next_instInfo->memout;
							// printf("  (%s) input2: %d\n", id_instInfo->string, next_instInfo->aluout);
						}
					}


				} else if (next_instInfo->destreg == 0b01) {
					if (id_instInfo->fields.rs == next_instInfo->fields.rd) {
						id_instInfo->input1 = next_instInfo->aluout;
						//printf("**************R format: id_instInfo->input1 = next_instInfo->aluout;");
						// printf("  (%s) aluout3: %d\n", next_instInfo->string, next_instInfo->aluout);
					}
					if (id_instInfo->fields.rt == next_instInfo->fields.rd) {
						id_instInfo->input2 = next_instInfo->aluout;
						// printf("  (%s) aluout4: %d\n", next_instInfo->string, next_instInfo->aluout);
					}
				}
			}
		}
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
