#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void read_file(FILE *pSource, FILE *pOutput);
void trim(char *line);
void write_output_file(FILE *pOutput, char *line);

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
    write_output_file(pOutput, line);
  }
}

void write_output_file(FILE *pOutput, char *line)
{
  //? Logic for putting line into pOutput
  fprintf(pOutput, "// %s\n", line);
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