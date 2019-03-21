// The MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments
/*
  read in the image file, use fseek then fread 5 time and print the values in decimal and hexadecimal (%5x).
  get info working then ls working.

  memcpy

  foo .....txt 8000017K:  017 is cluster num, k is size
    stat should print.
*/
 /*struct _attribute_((_packed_)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  unit8_t Unused1[8];
  unit16_t DIR_FirstClusterHigh;
  unit8_t Unused2[4];
  unit16_t DIR_FirstClusterLow;
  unit32_t DIR_FileSize;


} ;
*/
int main(int argc, char *argv[])
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  //added
  FILE *file_ptr;
  //struct DirectoryEntry dir[16];
  char BS_OEMName[8];
  int16_t BPB_BytsPerSec;
  int8_t BPB_SecPerClus;
  int16_t BPB_RsvdSecCnt;
  int8_t BPB_NumFATs;
  int16_t BPB_RootEntCnt;
  char  BS_VolLab[11];
  int32_t BPB_FATSz32;

  int32_t RootDirSectors =0;
  int32_t FirstDataSector = 0;
  int32_t FirstSectorofClustor =0;


  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    if(strcmp(token[0],"open")==0)
    {
       file_ptr= fopen(token[1],"r");
      if(file_ptr == NULL)
      {
        printf("Error: File system image not found.\n" );
        return 1;
      }
      printf(" file open success\n" );
      // add fun if the file is already opened before.



    }
    if(strcmp(token[0],"close")==0)
    {
      if(file_ptr !=NULL) //if the file was oppened
      {
        fclose(file_ptr);
        printf("file close success\n" );
      }
      if(file_ptr==NULL) //if file is not oppened then print error
      {
        printf("Error: File system not open\n" );
      }


    }
    if(strcmp(token[0],"info")==0)
    {

    }
    if(strcmp(token[0],"stat")==0)
    {

    }
    if(strcmp(token[0],"get")==0)
    {

    }
    if(strcmp(token[0],"put")==0)
    {

    }
    if(strcmp(token[0],"cd")==0)
    {

    }
    if(strcmp(token[0],"ls")==0)
    {

    }
    if(strcmp(token[0],"read")==0)
    {

    }









    free( working_root );

  }
  return 0;
}
