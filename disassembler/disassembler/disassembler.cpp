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

// #include "printRoutines.h"

#define ERROR_RETURN -1
#define SUCCESS 0

// disable fopen deprecation
#pragma warning(disable:4996)

void resetFilePosition(FILE *machineCode, unsigned long startingOffset);
void printHex(FILE *machineCode, FILE *outputFile);
void posDirective(FILE *machineCode, FILE *outputFile);
void disassemble(FILE *machineCode, FILE *outputFile, unsigned long fileLength);
int printInstruction(FILE *machineCode, FILE *outputFile);
void halt(unsigned char c, int *res, FILE *out);
void nop(unsigned char c, int *res, FILE *out);
void movq(unsigned char c, int *res, FILE *out);
void irmovq(unsigned char c, int *res, FILE *out);
void rmmovq(unsigned char c, int *res, FILE *out);
void mrmovq(unsigned char c, int *res, FILE *out);
void OPq(unsigned char c, int *res, FILE *out);
void jXX(unsigned char c, int *es, FILE *out);
void call(unsigned char c, int *res, FILE *out);
void ret(unsigned char c, int *res, FILE *out);
void pushq(unsigned char c, int *res, FILE *out);
void popq(unsigned char c, int *res, FILE *out);

int main()
{
	FILE *machineCode, *outputFile;

	unsigned int argc = 3;
	const char *InputFilename = "nop-ret.mem";
	const char *OutFilename = "";
	
	unsigned long startingOffset = 0;

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

	// Disassembler
	printHex(machineCode, outputFile);

	resetFilePosition(machineCode, startingOffset);

	// Print pos directive
	posDirective(machineCode, outputFile);

	resetFilePosition(machineCode, startingOffset);

	// Disassemble instructions
	disassemble(machineCode, outputFile, fileLength);

	fclose(machineCode);
	return SUCCESS;
}

void resetFilePosition(FILE *machineCode, unsigned long startingOffset)
{
	fseek(machineCode, startingOffset, SEEK_SET);
}

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
			break;
		}

		if (c != 0)
		{
			long int currLine = ftell(machineCode);
			if (currLine != 0x1)
			{
				fprintf(outputFile, ".pos ");
				fprintf(outputFile, "0x%ld\n", currLine - 0x1);
			}
			break;
		}
	}
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
	int res = 0;
	unsigned char c = fgetc(machineCode);

	if (!feof(machineCode))
	{
		switch (c >> 4)
		{
		case 0x0:
			halt(c, &res, out);
			break;
		case 0x1:
			nop(c, &res, out);
			break;
		case 0x2:
			movq(c, &res, out);
			break;
		case 0x3:
			irmovq(c, &res, out);
			break;
		case 0x4:
			rmmovq(c, &res, out);
			break;
		case 0x5:
			mrmovq(c, &res, out);
			break;
		case 0x6:
			OPq(c, &res, out);
			break;
		case 0x7:
			jXX(c, &res, out);
			break;
		case 0x8:
			call(c, &res, out);
			break;
		case 0x9:
			ret(c, &res, out);
			break;
		case 0xA:
			pushq(c, &res, out);
			break;
		case 0xB:
			popq(c, &res, out);
			break;
		default:
			res = -1;
		}
	}

	return res;
}

// INSTRUCTION FUNCTIONS. MAKE THESE PRIVATE
void halt(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "halt\n");
}

void nop(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "nop\n");
}

void movq(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "movq\n");
}

void irmovq(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "irmovq\n");
}

void rmmovq(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "rmmovq\n");
}

void mrmovq(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "mrmovq\n");
}

void OPq(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "OPq\n");
}

void jXX(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "jXX\n");
}

void call(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "call\n");
}

void ret(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "ret\n");
}

void pushq(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "pushq\n");
}

void popq(unsigned char c, int *res, FILE *out)
{
	fprintf(out, "popq\n");
}
