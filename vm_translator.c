#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void read_file(FILE *pSource, FILE *pOutput);
void trim(char *line);
void write_push(FILE *pOutput, char *segment, int index);

void translate_line(char *line, FILE *pOutput);

char *segment_lookup_table(char *symbol);

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
}

char* segment_lookup_table(char *symbol)
{
  if (strcmp(symbol, "local") == 0) return "LCL";
  if (strcmp(symbol, "argument") == 0) return "ARG";
  if (strcmp(symbol, "this") == 0) return "THIS";
  if (strcmp(symbol, "that") == 0) return "THAT";
  return NULL;
}