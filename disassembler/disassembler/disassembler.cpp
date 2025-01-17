#include "pch.h"
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
// #include <sys/uio.h>
// #include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

// #include "printRoutines.h"

#define ERROR_RETURN -1
#define SUCCESS 0

// disable fopen deprecation
#pragma warning(disable:4996)

void resetFilePosition(FILE *machineCode, unsigned long startingOffset);
void printHex(FILE *machineCode, FILE *outputFile);
void posDirective(FILE *machineCode, FILE *outputFile);
void byteDirective(FILE *machineCode, FILE *outputFile);
void quadDirective(FILE *machineCode, FILE *outputFile);
void disassemble(FILE *machineCode, FILE *outputFile, unsigned long fileLength);
int printInstruction(FILE *machineCode, FILE *outputFile);
void halt(unsigned char c, int *res, FILE *out, FILE *machineCode);
void nop(unsigned char c, int *res, FILE *out);
void movq(unsigned char c, int *res, FILE *out, FILE *machineCode);
void irmovq(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress);
void rmmovq(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress);
void mrmovq(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress);
void OPq(unsigned char c, int *res, FILE *out, FILE *machineCode);
void jXX(unsigned char c, int *es, FILE *out, FILE *machineCode, long int startingAddress);
void call(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress);
void ret(unsigned char c, int *res, FILE *out);
void pushq(unsigned char c, int *res, FILE *out, FILE *machineCode);
void popq(unsigned char c, int *res, FILE *out, FILE *machineCode);
const char* condition(unsigned char ifun);
const char* reg(unsigned char reg);
unsigned long long destAddr(FILE *machineCode, int size);

int consecutiveHalt = 0;

int main()
{
	FILE *machineCode, *outputFile;

	unsigned int argc = 3;
	const char *InputFilename = "sort_64.mem";
	const char *OutFilename = "";
	
	unsigned long long startingOffset = 0;

	// Verify that the command line has an appropriate number of arguments
	if (argc < 2 || argc > 4)
	{
		printf("Usage: InputFilename [OutputFilename] [startingOffset]\n");
		return ERROR_RETURN;
	}

	// Attempt to open file for reading and verify that the open occurred
	machineCode = fopen(InputFilename, "rb");
	if (machineCode == NULL)
	{
		printf("Failed to open input file %s\n", InputFilename);
		return ERROR_RETURN;
	}

	// Write to standard output. File will be provided in actual implementation
	outputFile = stdout;
	if (outputFile == NULL)
	{
		fprintf(stderr, "Failed to open output file\n");
		fclose(machineCode);
		return ERROR_RETURN;
	}

	// Obtain offset
	fprintf(stdout, "Opened %s, starting offset 0x%lx\n\n", InputFilename, startingOffset);
	
	// Get file length
	fseek(machineCode, 0, SEEK_END);
	unsigned long fileLength = ftell(machineCode);
	fprintf(outputFile, "size: %ld bytes\n", fileLength);
	
	resetFilePosition(machineCode, startingOffset);

	// Print machine code instructions in hex
	printHex(machineCode, outputFile);

	resetFilePosition(machineCode, startingOffset);

	// Base case
	if (fileLength == 1)
	{
		if (fgetc(machineCode) == 0x0)
		{
			fprintf(outputFile, ".byte 0x0\n");
			fclose(machineCode);
			return SUCCESS;
		}
		fseek(machineCode, -1, SEEK_CUR);
	}

	// Print pos directive
	posDirective(machineCode, outputFile);

	resetFilePosition(machineCode, startingOffset);

	// Disassemble instructions
	disassemble(machineCode, outputFile, fileLength);

	fclose(machineCode);
	fclose(outputFile);

	system("pause");

	return SUCCESS;
}

void resetFilePosition(FILE *machineCode, unsigned long startingOffset)
{
	fseek(machineCode, startingOffset, SEEK_SET);
}

// TODO: Testing purposes. Delete later
void printHex(FILE *machineCode, FILE *outputFile)
{
	unsigned char c;

	while (1)
	{
		c = fgetc(machineCode);
		if (feof(machineCode))
		{
			break;
		}
		fprintf(outputFile, "%02X ", c);
	}
	fprintf(outputFile, "\n\n");
}

void posDirective(FILE *machineCode, FILE *outputFile)
{
	unsigned char c;

	while (1)
	{
		c = fgetc(machineCode);

		if (feof(machineCode))
		{
			long int currLine = ftell(machineCode);
			fprintf(outputFile, ".pos 0x%lx\n", currLine - 0x1);
			fprintf(outputFile, ".byte 0x0\n");
			break;
		}

		if (c)
		{
			long int currLine = ftell(machineCode);
			if (currLine != 0x1)
			{
				fprintf(outputFile, ".pos 0x%lx\n", currLine - 0x1);
			}
			break;
		}

		consecutiveHalt = 1;
	}
}

void byteDirective(FILE *machineCode, FILE *outputFile)
{
	fprintf(outputFile, ".byte %llu\n", destAddr(machineCode, 1));
}

void quadDirective(FILE *machineCode, FILE *outputFile)
{
	fprintf(outputFile, ".quad %llu\n", destAddr(machineCode, 8));
}

void disassemble(FILE *machineCode, FILE *outputFile, unsigned long fileLength)
{
	for (int i = 0; i < fileLength; ++i)
	{
		printInstruction(machineCode, outputFile);
	}
}

int printInstruction(FILE *machineCode, FILE *out)
{
	int res = 1;
	long int startingAddress = ftell(machineCode);
	unsigned char c = fgetc(machineCode);

	if (!feof(machineCode))
	{
		if ((c >> 4) == 0x0)
		{
			halt(c, &res, out, machineCode);
		}
		else {
			consecutiveHalt = 0;
			switch (c >> 4)
			{
				case 0x1:
					nop(c, &res, out);
					break;
				case 0x2:
					movq(c, &res, out, machineCode);
					break;
				case 0x3:
					irmovq(c, &res, out, machineCode, startingAddress);
					break;
				case 0x4:
					rmmovq(c, &res, out, machineCode, startingAddress);
					break;
				case 0x5:
					mrmovq(c, &res, out, machineCode, startingAddress);
					break;
				case 0x6:
					OPq(c, &res, out, machineCode);
					break;
				case 0x7:
					jXX(c, &res, out, machineCode, startingAddress);
					break;
				case 0x8:
					call(c, &res, out, machineCode, startingAddress);
					break;
				case 0x9:
					ret(c, &res, out);
					break;
				case 0xA:
					pushq(c, &res, out, machineCode);
					break;
				case 0xB:
					popq(c, &res, out, machineCode);
					break;
				default:
					res = -1;
			}
		}
	}

	if (res < 0)
	{
		consecutiveHalt = 0;

		fseek(machineCode, -1, SEEK_CUR);

		// Check for quad directive
		for (int i = 0; i < 8; ++i)
		{
			fgetc(machineCode);
			if (feof(machineCode))
			{
				fseek(machineCode, -i, SEEK_CUR);
				byteDirective(machineCode, out);
				return res;
			}
		}

		// There are 8 bytes available AND the starting position
		// of would-be instruction is a multiple of 8
		if (!(startingAddress % 8))
		{
			fseek(machineCode, -8, SEEK_CUR);
			quadDirective(machineCode, out);
		}
		else {
			fseek(machineCode, -8, SEEK_CUR);
			byteDirective(machineCode, out);
		}
	}

	return res;
}

// INSTRUCTION FUNCTIONS. MAKE THESE PRIVATE
void halt(unsigned char c, int *res, FILE *out, FILE *machineCode)
{
	*res = !(c % 0x10) ? 1 : -1;
	if (!consecutiveHalt)
	{
		fprintf(out, "halt\n");
		consecutiveHalt = 1;
		fprintf(out, "\n");
		posDirective(machineCode, out);
	}
}

void nop(unsigned char c, int *res, FILE *out)
{
	*res = !(c % 0x10) ? 1 : -1;
	fprintf(out, "nop\n");
}

void movq(unsigned char c, int *res, FILE *out, FILE *machineCode)
{
	long int startingAddress = ftell(machineCode);

	unsigned char ifun = c % 0x10;
	unsigned char registers = fgetc(machineCode);

	if (feof(machineCode))
	{
		// Instruction is invalid
		fseek(machineCode, -1, SEEK_CUR);
		byteDirective(machineCode, out);
	}

	unsigned char firstRegister, secondRegister;

	// check validity of instruction
	if ((0 <= ifun) && (ifun <= 6))
	{
		firstRegister = registers >> 4;
		secondRegister = registers % 0x10;

		if ((0 <= firstRegister)
			&& (firstRegister <= 0xE)
			&& (0 <= secondRegister)
			&& (secondRegister <= 0xE)
			&& (*res > 0))
		{
			*res = 1;
		}
		else *res = -1;
	}
	else *res = -1;

	// print instruction
	if (*res > 0)
	{
		if (!ifun)
		{
			fprintf(out, "rrmovq");
		}
		else if (ifun)
		{
			fprintf(out, "cmov");
			condition(c % 0x10);
		}

		fprintf(out, "%s %s, %s\n", condition(ifun), reg(firstRegister), reg(secondRegister));
	}
}

void irmovq(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress)
{
	// Check if instruction is invalid
	for (int i = 0; i < 9; ++i)
	{
		fgetc(machineCode);
		if (feof(machineCode))
		{
			// Instruction is invalid
			// There are 8 bytes available AND the starting position
			// of would-be instruction is a multiple of 8
			if ((i >= 7) && (!(startingAddress % 8)))
			{
				fseek(machineCode, -(i+1), SEEK_CUR);
				quadDirective(machineCode, out);	
				return;
			}
			else {
				fseek(machineCode, -(i + 1), SEEK_CUR);
				byteDirective(machineCode, out);
				return;
			}
		}
	}

	fseek(machineCode, -9, SEEK_CUR);

	unsigned char ifun = c % 0x10;
	unsigned char registers = fgetc(machineCode);
	unsigned char firstRegister, secondRegister;

	// check validity of instruction
	if (!ifun)
	{
		firstRegister = registers >> 4;
		secondRegister = registers % 0x10;

		if ((0 <= secondRegister)
			&& (secondRegister <= 0xE)
			&& (firstRegister == 0xF)
			&& (*res > 0))
		{
			*res = 1;
		}
		else *res = -1;
	}
	else *res = -1;

	if (*res > 0)
	{
		fprintf(out, "irmovq $%llu, %s\n", destAddr(machineCode, 8), reg(secondRegister));
	}
}

void rmmovq(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress)
{

	// Check if instruction is invalid
	for (int i = 0; i < 9; ++i)
	{
		fgetc(machineCode);
		if (feof(machineCode))
		{
			// Instruction is invalid
			// There are 8 bytes available AND the starting position
			// of would-be instruction is a multiple of 8
			if ((i >= 7) && (!(startingAddress % 8)))
			{
				fseek(machineCode, -(i+1), SEEK_CUR);
				quadDirective(machineCode, out);	
				return;
			}
			else {
				fseek(machineCode, -(i + 1), SEEK_CUR);
				byteDirective(machineCode, out);
				return;
			}
		}
	}

	fseek(machineCode, -9, SEEK_CUR);

	unsigned char ifun = c % 0x10;
	unsigned char registers = fgetc(machineCode);
	unsigned char firstRegister, secondRegister;

	// check validity of instruction
	if (!ifun)
	{
		firstRegister = registers >> 4;
		secondRegister = registers % 0x10;

		if ((0 <= firstRegister)
			&& (firstRegister <= 0xE)
			&& (0 <= secondRegister)
			&& (secondRegister <= 0xE)
			&& (*res > 0))
		{
			*res = 1;
		}
		else *res = -1;
	}
	else *res = -1;

	if (*res > 0)
	{
		fprintf(out, "rmmovq %s, %llu(%s)\n", reg(firstRegister), destAddr(machineCode, 8), reg(secondRegister));
	}
}

void mrmovq(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress)
{

	// Check if instruction is invalid
	for (int i = 0; i < 9; ++i)
	{
		fgetc(machineCode);
		if (feof(machineCode))
		{
			// Instruction is invalid
			// There are 8 bytes available AND the starting position
			// of would-be instruction is a multiple of 8
			if ((i >= 7) && (!(startingAddress % 8)))
			{
				fseek(machineCode, -(i + 1), SEEK_CUR);
				quadDirective(machineCode, out);
				return;
			}
			else {
				fseek(machineCode, -(i + 1), SEEK_CUR);
				byteDirective(machineCode, out);
				return;
			}
		}
	}

	fseek(machineCode, -9, SEEK_CUR);

	unsigned char ifun = c % 0x10;
	unsigned char registers = fgetc(machineCode);
	unsigned char firstRegister, secondRegister;

	// check validity of instruction
	if (!ifun)
	{
		firstRegister = registers >> 4;
		secondRegister = registers % 0x10;

		if ((0 <= firstRegister)
			&& (firstRegister <= 0xE)
			&& (0 <= secondRegister)
			&& (secondRegister <= 0xE)
			&& (*res > 0))
		{
			*res = 1;
		}
		else *res = -1;
	}
	else *res = -1;

	if (*res > 0)
	{
		fprintf(out, "mrmovq %llu(%s), %s\n", destAddr(machineCode, 8), reg(secondRegister), reg(firstRegister));
	}
}

void OPq(unsigned char c, int *res, FILE *out, FILE *machineCode)
{
	long int startingAddress = ftell(machineCode);

	unsigned char ifun = c % 0x10;
	unsigned char registers = fgetc(machineCode);

	if (feof(machineCode))
	{
		// Instruction is invalid
		fseek(machineCode, -1, SEEK_CUR);
		byteDirective(machineCode, out);
	}

	unsigned char firstRegister, secondRegister;

	// check validity of instruction
	if ((0 <= ifun) && (ifun <= 6))
	{
		firstRegister = registers >> 4;
		secondRegister = registers % 0x10;

		if ((0 <= firstRegister)
			&& (firstRegister <= 0xE)
			&& (0 <= secondRegister)
			&& (secondRegister <= 0xE)
			&& (*res > 0))
		{
			*res = 1;
		}
		else *res = -1;
	}
	else *res = -1;

	// print instruction
	if (*res > 0)
	{
		switch (ifun)
		{
		case 0:
			fprintf(out, "addq");
			break;
		case 1:
			fprintf(out, "subq");
			break;
		case 2:
			fprintf(out, "andq");
			break;
		case 3:
			fprintf(out, "xorq");
			break;
		case 4:
			fprintf(out, "mulq");
			break;
		case 5:
			fprintf(out, "divq");
			break;
		case 6:
			fprintf(out, "modq");
			break;
		default:
			*res = -1;
		}

		fprintf(out, " %s, %s\n", reg(firstRegister), reg(secondRegister));
	}
}

void jXX(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress)
{
	// Check if instruction is invalid
	for (int i = 0; i < 8; ++i)
	{
		fgetc(machineCode);
		if (feof(machineCode))
		{
			// Instruction is invalid
			// There are 8 bytes available AND the starting position
			// of would-be instruction is a multiple of 8
			if ((i == 7) && (!(startingAddress % 8)))
			{
				fseek(machineCode, -8, SEEK_CUR);
				quadDirective(machineCode, out);
				return;
			}
			else {
				fseek(machineCode, -(i+1), SEEK_CUR);
				byteDirective(machineCode, out);
				return;
			}
		}
	}

	fseek(machineCode, -8, SEEK_CUR);
	unsigned char ifun = c % 0x10;
	if ((0 <= ifun) && (ifun <= 6))
	{
		*res = 1;
	}
	else *res = -1;

	// print instruction
	if (*res > 0)
	{
		fprintf(out, "j%s %llu\n", condition(ifun), destAddr(machineCode, 8));
	}
}

void call(unsigned char c, int *res, FILE *out, FILE *machineCode, long int startingAddress)
{
	// Check if instruction is invalid
	for (int i = 0; i < 8; ++i)
	{
		fgetc(machineCode);
		if (feof(machineCode))
		{
			// Instruction is invalid
			// There are 8 bytes available AND the starting position
			// of would-be instruction is a multiple of 8
			if ((i == 7) && (!(startingAddress % 8)))
			{
				fseek(machineCode, -8, SEEK_CUR);
				quadDirective(machineCode, out);
				return;
			}
			else {
				fseek(machineCode, -(i+1), SEEK_CUR);
				byteDirective(machineCode, out);
				return;
			}
		}
	}

	fseek(machineCode, -8, SEEK_CUR);

	*res = !(c % 0x10) ? 1 : -1;
	if (*res > 0)
	{
		fprintf(out, "call %llu\n", destAddr(machineCode, 8));
	}
}

void ret(unsigned char c, int *res, FILE *out)
{
	*res = !(c % 0x10) ? 1 : -1;
	fprintf(out, "ret\n");
}

void pushq(unsigned char c, int *res, FILE *out, FILE *machineCode)
{
	long int startingAddress = ftell(machineCode);

	unsigned char ifun = c % 0x10;
	unsigned char registers = fgetc(machineCode);

	if (feof(machineCode))
	{
		// Instruction is invalid
		fseek(machineCode, -1, SEEK_CUR);
		byteDirective(machineCode, out);
	}

	unsigned char firstRegister, secondRegister;

	// check validity of instruction
	if (!ifun)
	{
		firstRegister = registers >> 4;
		secondRegister = registers % 0x10;

		if ((0 <= firstRegister)
			&& (firstRegister <= 0xE)
			&& (secondRegister == 0xF)
			&& (*res > 0))
		{
			*res = 1;
		}
		else *res = -1;
	}
	else *res = -1;

	if (*res > 0)
	{
		fprintf(out, "pushq %s\n", reg(firstRegister));
	}
}

void popq(unsigned char c, int *res, FILE *out, FILE *machineCode)
{
	long int startingAddress = ftell(machineCode);

	unsigned char ifun = c % 0x10;
	unsigned char registers = fgetc(machineCode);

	if (feof(machineCode))
	{
		// Instruction is invalid
		fseek(machineCode, -1, SEEK_CUR);
		byteDirective(machineCode, out);
	}

	unsigned char firstRegister, secondRegister;

	// check validity of instruction
	if (!ifun)
	{
		firstRegister = registers >> 4;
		secondRegister = registers % 0x10;

		if ((0 <= firstRegister)
			&& (firstRegister <= 0xE)
			&& (secondRegister == 0xF)
			&& (*res > 0))
		{
			*res = 1;
		}
		else *res = -1;
	}
	else *res = -1;

	if (*res > 0)
	{
		fprintf(out, "popq %s\n", reg(firstRegister));
	}
}

const char* condition(unsigned char ifun)
{
	switch (ifun)
	{
		case 0:
			return "";
		case 1:
			return "le";
		case 2:
			return "l";
		case 3:
			return "e";
		case 4:
			return "ne";
		case 5:
			return "ge";
		case 6:
			return "g";
	}
}

const char* reg(unsigned char reg)
{
	switch (reg)
	{
		case 0x0:
			return "%rax";
		case 0x1:
			return "%rcx";
		case 0x2:
			return "%rdx";
		case 0x3:
			return "%rbx";
		case 0x4:
			return "%rsp";
		case 0x5:
			return "%rbp";
		case 0x6:
			return "%rsi";
		case 0x7:
			return "%rdi";
		case 0x8:
			return "%r8";
		case 0x9:
			return "%r9";
		case 0xA:
			return "%r10";
		case 0xB:
			return "%r11";
		case 0xC:
			return "%r12";
		case 0xD:
			return "%r13";
		case 0xE:
			return "%r14";
	}
}

unsigned long long destAddr(FILE *machineCode, int size)
{
	unsigned long long destAddr = 0;
	for (int i = 0; i < size; ++i)
	{
		destAddr += fgetc(machineCode) * (pow(0x10, 2*i));
	}
	return destAddr;
}
