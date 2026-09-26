#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void read_file(FILE *pSource, FILE *pOutput);
void trim(char *line);
void write_push(FILE *pOutput, char *segment, int index);
void translate_line(char *line, FILE *pOutput);
char *segment_lookup_table(char *symbol);

int labelCounter = 0;
char currentFileName[64] = "Foo";

void write_pop(FILE *pOutput, char *segment, int index);
void write_arithmetic(FILE *pOutput, char *command);

int main()
{
  char fileName[256];
  printf("Enter file location: ");
  scanf("%255s", fileName);

  FILE *pSource = fopen(fileName, "r");

  if(pSource == NULL)
  {
    printf("File not found!");
    return 1;
  }

  printf("File opened successfully!");

  FILE *pOutput = fopen("output.asm", "w");

  if(pOutput == NULL)
  {
    printf("File not found!");
    fclose(pSource);
    return 1;
  }

  printf("File created successfully!");

  read_file(pSource, pOutput);

  fclose(pSource);
  fclose(pOutput);



  return 0;
}

void read_file(FILE *pSource, FILE *pOutput)
{
  char line[256];
  while(fgets(line, sizeof(line), pSource) != NULL)
  {
    //? Reads each line
    //? First trim and remove whitespace
    trim(line);

    if(line[0] == '\0')
    {
      continue;
    }

    if(line[0] == '/' && line[1] == '/')
    {
      continue;
    }

    //? write the finished product in output.asm
    translate_line(line, pOutput);
  }
}


void trim(char *line)
{

  //? Triming the end
  int len = strlen(line);

  while (len > 0 && isspace((unsigned char)line[len - 1])) {
    line[len - 1] = '\0';
    len--;
  }

  //? Triming from the start
  int start = 0;
  while (line[start] != '\0' && isspace((unsigned char)line[start])) {
    start++;
  }

  if (start > 0) {
    memmove(line, line + start, strlen(line + start) + 1);
  }
}

void translate_line(char *line, FILE *pOutput)
{
  char lineCopy[256];
  strcpy(lineCopy, line);

  char *command = strtok(lineCopy, " ");

  if(strcmp(command, "push") == 0)
  {
    char *segment = strtok(NULL, " ");
    char *indexStr = strtok(NULL, " ");
    int index = atoi(indexStr);

    write_push(pOutput, segment, index);
  }
  else if(strcmp(command, "pop") == 0)
  {
    char *segment = strtok(NULL, " ");
    int index = atoi(strtok(NULL, " "));
    write_pop(pOutput, segment, index);
  }
  else
  {
    //? Arithmetic command: add, sub, neg, eq ...
  }
}

void write_push(FILE *pOutput, char *segment, int index)
{
  if(strcmp(segment, "constant") == 0)
  {
    fprintf(pOutput, "@%d\n", index);
    fprintf(pOutput, "D=A\n");
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "A=M\n");
    fprintf(pOutput, "M=D\n");
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "M=M+1\n");
  }
  else if(strcmp(segment, "local") == 0 || strcmp(segment, "argument") == 0 || strcmp(segment, "this") == 0 || strcmp(segment, "that") == 0)
  {
    char *symbol = segment_lookup_table(segment);

    fprintf(pOutput, "@%d\n", index);   // A = index
    fprintf(pOutput, "D=A\n");          // D = index
    fprintf(pOutput, "@%s\n", symbol);  // A = address of LCL/ARG/THIS/THAT itself
    fprintf(pOutput, "A=M\n");          // A = the VALUE stored there (the segment's base address)
    fprintf(pOutput, "A=D+A\n");        // A = base + index (the real target address)
    fprintf(pOutput, "D=M\n");          // D = the value stored at that target address

    fprintf(pOutput, "@SP\n");          // ---- same push tail as constant ----
    fprintf(pOutput, "A=M\n");
    fprintf(pOutput, "M=D\n");
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "M=M+1\n");
    }
  else if(strcmp(segment, "temp") == 0)
  {
  // temp has a FIXED base (address 5), no indirection needed
    int address = 5 + index;
    fprintf(pOutput, "@%d\n", address); // A = real address directly
    fprintf(pOutput, "D=M\n");          // D = value there
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "A=M\n");
    fprintf(pOutput, "M=D\n");
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "M=M+1\n");
  }
  else if(strcmp(segment, "pointer") == 0)
  {
    // pointer 0 = THIS, pointer 1 = THAT, direct, no indirection
    char *symbol = (index == 0) ? "THIS" : "THAT";
    fprintf(pOutput, "@%s\n", symbol);
    fprintf(pOutput, "D=M\n");
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "A=M\n");
    fprintf(pOutput, "M=D\n");
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "M=M+1\n");
  }
  else if(strcmp(segment, "static") == 0)
  {
    // becomes symbol "Filename.i" -- your ASSEMBLER resolves this to a real address later
    fprintf(pOutput, "@%s.%d\n", currentFileName, index);
    fprintf(pOutput, "D=M\n");
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "A=M\n");
    fprintf(pOutput, "M=D\n");
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "M=M+1\n");
  }
}

void write_pop(FILE *pOutput, char *segment, int index)
{
    if(strcmp(segment, "local") == 0 || strcmp(segment, "argument") == 0 ||
     strcmp(segment, "this") == 0 || strcmp(segment, "that") == 0)
  {
    char *symbol = segment_lookup_table(segment);

    // THE TRAP: we need the target address computed BEFORE popping,
    // but D can only hold ONE value at a time. If you pop first, you
    // overwrite D with the popped value before you've saved the address.
    // Fix: compute the address, stash it in R13 (a free scratch register),
    // THEN pop, THEN combine.

    fprintf(pOutput, "@%d\n", index);   // A = index
    fprintf(pOutput, "D=A\n");          // D = index
    fprintf(pOutput, "@%s\n", symbol);  // A = the segment's base-pointer variable
    fprintf(pOutput, "D=D+M\n");        // D = index + base = REAL target address
    fprintf(pOutput, "@R13\n");         // R13 = general-purpose scratch register
    fprintf(pOutput, "M=D\n");          // stash target address there

    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");       // SP--, A = popped value's address
    fprintf(pOutput, "D=M\n");          // D = the popped value

    fprintf(pOutput, "@R13\n");
    fprintf(pOutput, "A=M\n");          // A = the address we stashed
    fprintf(pOutput, "M=D\n");          // write popped value into real target
  }
  else if(strcmp(segment, "temp") == 0)
  {
    // fixed base -- no stashing needed, much simpler
    int address = 5 + index;
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");
    fprintf(pOutput, "D=M\n");
    fprintf(pOutput, "@%d\n", address);
    fprintf(pOutput, "M=D\n");
  }
  else if(strcmp(segment, "pointer") == 0)
  {
    char *symbol = (index == 0) ? "THIS" : "THAT";
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");
    fprintf(pOutput, "D=M\n");
    fprintf(pOutput, "@%s\n", symbol);
    fprintf(pOutput, "M=D\n");
  }
  else if(strcmp(segment, "static") == 0)
  {
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");
    fprintf(pOutput, "D=M\n");
    fprintf(pOutput, "@%s.%d\n", currentFileName, index);
    fprintf(pOutput, "M=D\n");
  }
}


void write_arithmetic(FILE *pOutput, char *command)
{
  if(strcmp(command, "add") == 0)
  {
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");   // SP--, A = address of y
    fprintf(pOutput, "D=M\n");      // D = y
    fprintf(pOutput, "A=A-1\n");    // A = address of x, WITHOUT touching SP again
    fprintf(pOutput, "M=M+D\n");    // x = x + y, written in place
  }
  else if(strcmp(command, "sub") == 0)
  {
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");
    fprintf(pOutput, "D=M\n");
    fprintf(pOutput, "A=A-1\n");
    fprintf(pOutput, "M=M-D\n");
  }
  else if(strcmp(command, "neg") == 0)
  {
    // unary -- only ONE value involved, no second pop needed
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "A=M-1\n");    // top value's address (SP itself doesn't move)
    fprintf(pOutput, "M=-M\n");
  }
  else if(strcmp(command, "and") == 0)
  {
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");
    fprintf(pOutput, "D=M\n");
    fprintf(pOutput, "A=A-1\n");
    fprintf(pOutput, "M=M&D\n");
  }
  else if(strcmp(command, "or") == 0)
  {
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");
    fprintf(pOutput, "D=M\n");
    fprintf(pOutput, "A=A-1\n");
    fprintf(pOutput, "M=M|D\n");
  }
  else if(strcmp(command, "not") == 0)
  {
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "A=M-1\n");
    fprintf(pOutput, "M=!M\n");
  }
  else if(strcmp(command, "eq") == 0 || strcmp(command, "gt") == 0 || strcmp(command, "lt") == 0)
  {
    // needs a CONDITIONAL JUMP -> needs a UNIQUE label each time,
    // otherwise two eq's in the same file collide on the same label name
    int myLabel = labelCounter;
    labelCounter++;

    char *jumpType;
    if (strcmp(command, "eq") == 0)      jumpType = "JEQ";
    else if (strcmp(command, "gt") == 0) jumpType = "JGT";
    else                                 jumpType = "JLT";

    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "AM=M-1\n");
    fprintf(pOutput, "D=M\n");        // D = y
    fprintf(pOutput, "A=A-1\n");
    fprintf(pOutput, "D=M-D\n");      // D = x - y  (0 means equal, etc.)

    fprintf(pOutput, "@TRUE_%d\n", myLabel);
    fprintf(pOutput, "D;%s\n", jumpType);   // jump if comparison holds

    fprintf(pOutput, "@SP\n");              // didn't jump -> false
    fprintf(pOutput, "A=M-1\n");
    fprintf(pOutput, "M=0\n");
    fprintf(pOutput, "@END_%d\n", myLabel);
    fprintf(pOutput, "0;JMP\n");            // skip the true case below

    fprintf(pOutput, "(TRUE_%d)\n", myLabel);
    fprintf(pOutput, "@SP\n");
    fprintf(pOutput, "A=M-1\n");
    fprintf(pOutput, "M=-1\n");             // -1 = all 1s = "true" in two's complement

    fprintf(pOutput, "(END_%d)\n", myLabel);
  }
}

char* segment_lookup_table(char *symbol)
{
  if (strcmp(symbol, "local") == 0) return "LCL";
  if (strcmp(symbol, "argument") == 0) return "ARG";
  if (strcmp(symbol, "this") == 0) return "THIS";
  if (strcmp(symbol, "that") == 0) return "THAT";
  return NULL;
}
