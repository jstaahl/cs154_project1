
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "functions.h"

// Constants

#define OP_CODE_ADD 0b100000
#define OP_CODE_ADDI 0b110001
#define OP_CODE_AND 0b100000
#define OP_CODE_SUB 0b100000
#define OP_CODE_SGT 0b100000
#define OP_CODE_LW 0b010001
#define OP_CODE_SW 0b010010
#define OP_CODE_BEQ 0b001010
#define OP_CODE_JR 0b100101
#define OP_CODE_JAL 0b001000

#define FUNC_CODE_ADD 0b100000
#define FUNC_CODE_AND 0b100100
#define FUNC_CODE_SUB 0b101000
#define FUNC_CODE_SGT 0b110000

#define ALU_AND 0b000
#define ALU_ADD 0b010
#define ALU_SUB 0b011
#define ALU_SGT 0b111


// these are the structures used in this simulator


// global variables
// register file
int regfile[32];
// instruction memory
int instmem[100];	// only support 100 static instructions
// data memory
int datamem[1024];
// program counter
int pc;

// these are the different functions you need to write
int load(char *filename);
void fetch(InstInfo *);
void decode(InstInfo *);
void execute(InstInfo *);
void memory(InstInfo *);
void writeback(InstInfo *);

/* load
 *
 * Given the filename, which is a text file that 
 * contains the instructions stored as integers 
 *
 * You will need to load it into your global structure that 
 * stores all of the instructions.
 *
 * The return value is the maxpc - the number of instructions
 * in the file
 */
int load(char *filename)
{
	FILE *f = fopen(filename, "r");
	int instruction;
	int line = 0;
	while(fscanf(f, "%d[^\n]",&instruction) == 1) {
	instmem[line] = instruction;
	line++;
	}
	pc = 0;
	return line;
}

/* fetch
 *
 * This fetches the next instruction and updates the program counter.
 * "fetching" means filling in the inst field of the instruction.
 */
void fetch(InstInfo *instruction)
{
	instruction->inst = instmem[pc];
	instruction->pc = pc;
	pc++;
}

/* decode
 *
 * This decodes an instruction.	It looks at the inst field of the 
 * instruction.	Then it decodes the fields into the field's data 
 * members.	The first one is given to you.
 *
 * Then it checks the op code.	Depending on what the opcode is, it
 * fills in all of the signals for that instruction.
 */
void decode(InstInfo *instruction)
{

	// fill in the signals and fields
	int val = instruction->inst;
	instruction->fields.op = (val >> 26) & 0x03f;
	// fill in the rest of the fields here
	instruction->fields.rs = (val >> 21) & 0x01f;
	instruction->fields.rt = (val >> 16) & 0x01f;
	instruction->fields.rd = (val >> 11) & 0x01f;
	instruction->fields.imm = (val << 16) >> 16;
	instruction->fields.func = val & 0x03f;
	// now fill in the signals

	instruction->signals.asrc = -1;
	instruction->signals.rdst = -1;
	instruction->signals.mtr = -1;

	int func = instruction->fields.func;

	if (instruction->fields.op == OP_CODE_ADDI) {

		instruction->signals.aluop = 2;
		
		instruction->signals.asrc = 1;
		instruction->signals.rw = 1;
		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.mtr = 0;
		instruction->signals.btype = 0;
		instruction->signals.rdst = 0;

		sprintf(instruction->string,"addi $%d, $%d, %d",
				instruction->fields.rt, instruction->fields.rs, 
				instruction->fields.imm);

		instruction->destreg = instruction->fields.rt;

		instruction->input1 = instruction->fields.imm;
		instruction->s2data = regfile[instruction->fields.rs];
		instruction->input2 = instruction->s2data;

	} else if (instruction->fields.op == OP_CODE_ADD) {

		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.mtr = 0;
		instruction->signals.asrc = 0;
		instruction->signals.btype = 0;
		instruction->signals.rdst = 1;
		instruction->signals.rw = 1;

		instruction->destreg = instruction->fields.rd;

		if (func == FUNC_CODE_ADD) {

			instruction->signals.aluop = 2;
			
			sprintf(instruction->string,"add $%d, $%d, $%d",
				instruction->fields.rd, instruction->fields.rs, 
				instruction->fields.rt);

		} else if (func == FUNC_CODE_AND) {

			instruction->signals.aluop = 0;

			sprintf(instruction->string,"and $%d, $%d, $%d",
				instruction->fields.rd, instruction->fields.rs, 
				instruction->fields.rt);

		} else if (func == FUNC_CODE_SUB) {

			instruction->signals.aluop = 3;

			sprintf(instruction->string,"sub $%d, $%d, $%d",
				instruction->fields.rd, instruction->fields.rs, 
				instruction->fields.rt);
			
		} else if (func == FUNC_CODE_SGT) {

			instruction->signals.aluop = 7;
			sprintf(instruction->string,"sgt $%d, $%d, $%d",
				instruction->fields.rd, instruction->fields.rs, 
				instruction->fields.rt);
		}

		instruction->s1data = regfile[instruction->fields.rs];
		instruction->s2data = regfile[instruction->fields.rt];

		instruction->input1 = instruction->s1data;
		instruction->input2 = instruction->s2data;

	} else if (instruction->fields.op == OP_CODE_LW) {

		instruction->signals.asrc = 1;
		instruction->signals.aluop = 2;
		instruction->signals.btype = 0;
		instruction->signals.mw = 0;
		instruction->signals.mr = 1;
		instruction->signals.mtr = 1;
		instruction->signals.rdst = 0;
		instruction->signals.rw = 1;

		instruction->input1 = instruction->fields.imm;
		instruction->s2data = regfile[instruction->fields.rs];
		instruction->input2 = instruction->s2data;


		sprintf(instruction->string,"lw $%d, %d($%d)",
				instruction->fields.rt, instruction->fields.imm, 
				instruction->fields.rs);

		instruction->destreg = instruction->fields.rt;

	} else if (instruction->fields.op == OP_CODE_SW) {

		instruction->signals.asrc = 1;
		instruction->signals.aluop = 2;
		instruction->signals.btype = 0;
		instruction->signals.mw = 1;
		instruction->signals.mr = 0;
		instruction->signals.rw = 0;

		instruction->input1 = instruction->fields.imm;
		instruction->s2data = regfile[instruction->fields.rs];
		instruction->input2 = instruction->s2data;

		instruction->destdata = regfile[instruction->fields.rt];

		sprintf(instruction->string,"sw $%d, %d($%d)",
				instruction->fields.rt, instruction->fields.imm, 
				instruction->fields.rs);

	} else if (instruction->fields.op == OP_CODE_BEQ) {

		instruction->signals.aluop = 0b11;
		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.btype = 0b11;
		instruction->signals.rw = 0;
		instruction->signals.asrc = 0;

		instruction->s1data = regfile[instruction->fields.rs];
		instruction->s2data = regfile[instruction->fields.rt];

		instruction->input1 = instruction->s1data;
		instruction->input2 = instruction->s2data;		

		sprintf(instruction->string,"beq $%d, $%d, %d",
				instruction->fields.rs, instruction->fields.rt,
				instruction->fields.imm);

	} else if (instruction->fields.op == OP_CODE_JR) {

		instruction->signals.mw = 0;
		instruction->signals.mr = 0;	
		instruction->signals.btype = 0b10;
		instruction->signals.rw = 0;
		instruction->signals.aluop = -1;

		instruction->s2data = regfile[instruction->fields.rs];

		sprintf(instruction->string,"jr $%d",
				instruction->fields.rs);

	} else if (instruction->fields.op == OP_CODE_JAL) {

		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.btype = 0b01;
		instruction->signals.mtr = 0b10;
		instruction->signals.rw = 1;
		instruction->signals.rdst = 0b10;

		instruction->signals.aluop = -1;

		instruction->fields.imm = 0x03FFFFFF & instruction->inst;

		instruction->destdata = instruction->pc + 1;
		instruction->destreg = 31;

		sprintf(instruction->string,"jal %d",
				instruction->fields.imm);

	}
}

/* execute
 *
 * This fills in the aluout value into the instruction and destdata
 */

void execute(InstInfo *instruction) {
	
	int aluop = instruction->signals.aluop;

	int in1 = instruction->input1;
	int in2 = instruction->input2;

	if (aluop == ALU_AND) {
		instruction->aluout = in1 & in2;
	} else if (aluop == ALU_ADD) {
		instruction->aluout = in1 + in2;
	} else if (aluop == ALU_SUB) {
		instruction->aluout = in1 - in2;
	} else if (aluop == ALU_SGT) {
		instruction->aluout = (in1 > in2);
	}

}

/* memory
 *
 * If this is a load or a store, perform the memory operation
 */
void memory(InstInfo *instruction) {
	
	if (instruction->signals.mr) {
		// lw
		instruction->memout = datamem[instruction->aluout];	
	} else if (instruction->signals.mw) {
		// sw
		datamem[instruction->aluout] = instruction->destdata;	
	}
}

/* writeback
 *
 * If a register file is supposed to be written, write to it now
 */
void writeback(InstInfo *instruction) {

	// lw - instruction->memout to register
	if (instruction->signals.rw) {
		switch (instruction->signals.mtr) {

			case 0b00:
				// choose ALU output
				regfile[instruction->destreg] = instruction->aluout;
				break;
			case 0b01:
				// choose memory output
				regfile[instruction->destreg] = instruction->memout;
				break;
			case 0b10:
				// 10 choose PC+4
				regfile[instruction->destreg] = instruction->destdata;
				break;
		}
	}
}

/* setPCWithInfo
 *
 * branch instruction will overwrite PC
*/
void setPCWithInfo( InstInfo *instruction) {

	// BType - 00 if not a branch, 11 if conditional jump, 01 if jump to an address encoded in instruction 
	//(concatenated w/ top 4 bits of PC), 10 if jump to a register-specified location 

	switch (instruction->signals.btype) {

			case 0b01:
				// jump to an address encoded in instruction
				pc = instruction->fields.imm;
				break;
			case 0b10:
				// if jump to a register-specified location
				pc = instruction->s2data;
				break;
			case 0b11:
				// conditional jump
				if (!instruction->aluout) {
					pc = instruction->fields.imm;
				}
				break;

	}

}
