/*// The MIT License (MIT)
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
*/
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include<stdint.h>
#include <inttypes.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

#define TRUE 1
#define FALSE 0

/*

  get info working then ls working.
  memcpy
  foo .....txt 8000017K:  017 is cluster num, k is size
    stat should print.

*/
struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];// stat
  uint8_t DIR_Attr; //stat
  uint8_t Unused1[8]; //???
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4]; //???
  uint16_t DIR_FirstClusterLow;//stat
  uint32_t DIR_FileSize;//stat
} ;


int main(int argc, char *argv[])
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  //added
  FILE *file_ptr;
  struct DirectoryEntry dir[16];
  char BS_OEMName[8]; //
  int16_t BPB_BytsPerSec; // Count of bytes per sector
  int8_t BPB_SecPerClus; // Number of sectors per allocation unit.
  int16_t BPB_RsvdSecCnt; // Number of reserved sectors in the Reserved region of the volume starting at the first sector of the volume.  FAT #1 starts at address BPB_RsvdSecCnt * BPB_BytsPerSec
  int8_t BPB_NumFATs; //The count of FAT data structures on the volume. This field should always contain the value 2 for any FAT volume of any type.
  int16_t BPB_RootEntCnt; // For FAT12 and FAT16 volumes, this field contains the count of 32-byte directory entries in the root directory. For FAT32 volumes,this field must be set to 0.
  char  BS_VolLab[11]; // Volume label. This field matches the 11-byte volume label recorded in the root directory.
  int32_t BPB_FATSz32; // This field is only defined for FAT32 media and does not exist on FAT12 and FAT16 media. This field is the FAT32 32-bit count ofsectors occupied by ONE FAT. BPB_FATSz16 must be 0. Total FAT size is BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec


/*
Clusters start at address (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec)
Clusters are each (BPB_SecPerClus * BPB_BytsPerSec) in bytes
Root Directory is at the first cluster.
Address: (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +(BPB_RsvdSecCnt * BPB_BytsPerSec)
*/


  int32_t RootDirSectors = 0;
  int32_t FirstDataSector = 0;
  int32_t FirstSectorofClustor = 0;

  int flag_open;


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

    //catches empty buffer prevents from seg fault
    if (token[0] == NULL)
    {
      continue;
    }
    else if(strcmp(token[0],"open")==0)
    {
      if(token[1]==NULL)
      {
        printf("Error: Please enter the file name to open\n" );
        continue;
      }
      else
      {
        file_ptr= fopen(token[1],"r");
        if(file_ptr == NULL)
          {
             printf("Error: File system image not found.\n" );
             continue;
          }
        if (flag_open == TRUE)
          {
             printf("Error: File system image already open.\n");
             continue;
          }

           printf(" file open success\n" );
           flag_open = TRUE;
      }
    }
    else if(strcmp(token[0],"close")==0) // we are having seg fault if we close without opning the file
    {

      if ( (flag_open == FALSE) || (token_count == 2) ) //if file is not oppened then print error
      {
        printf("Error: File system image must be opened first.\n");
        continue;
      }
      if(file_ptr !=NULL) //if the file was oppened
      {
        fclose(file_ptr);
        printf("file close success\n" );
        flag_open = FALSE;
      }

    }
  /*  read in the image file, use fseek then fread 5 time and
   print the values in decimal and hexadecimal (%5x).
   Q> DO WE NEED TO OPEN IMG FILE BEFORE EXECUTIN INFO?? OR INSIDE INFO()
*/
  else  if(strcmp(token[0],"info")==0) // ASSUMMING IMG FILE IS ALREADY OPEN
    {
      // TO DO: if not open print Error
      fseek(file_ptr, 11,SEEK_SET);
      fread(&BPB_BytsPerSec,2,1,file_ptr);

      fseek(file_ptr, 13,SEEK_SET);
      fread(&BPB_SecPerClus,1,1,file_ptr);

      fseek(file_ptr, 14,SEEK_SET);
      fread(&BPB_RsvdSecCnt,2,1,file_ptr);

      fseek(file_ptr, 16,SEEK_SET);
      fread(&BPB_NumFATs,1,1,file_ptr);

      fseek(file_ptr, 36,SEEK_SET); //make sure of the This
      fread(&BPB_FATSz32,4,1,file_ptr);
      //%x for hexadecimal
      printf(" BPB_BytsPerSec in hexadecimal is 0x%5x and in decimal %d\n", BPB_BytsPerSec, BPB_BytsPerSec );
      printf(" BPB_SecPerClus in hexadecimal is 0x%5x and in decimal %d\n", BPB_SecPerClus, BPB_SecPerClus );
      printf(" BPB_RsvdSecCnt in hexadecimal is 0x%5x and in decimal %d\n", BPB_RootEntCnt, BPB_RootEntCnt );
      printf(" BPB_NumFATs in hexadecimal is 0x %5x and in decimal %d\n", BPB_NumFATs, BPB_NumFATs );
      printf(" BPB_FATSz32 in hexadecimal is 0x%5x and in decimal %d\n", BPB_FATSz32, BPB_FATSz32 );


    }
    /*
    req: print attributes and the starting cluster  num of the file or dir.
      print: name , attrbute, cluster low, size,
      attribute will tell file ( 10 dir )


    */
  else  if(strcmp(token[0],"stat")==0)
    {
      if(token[1]==NULL)
      {
        printf("Error: need file name or dir name\n" );
        continue;
      }
      else // how to file or dir???
      {
            //go to root directory
            fseek(file_ptr, 0x100400, SEEK_SET);

            //read the root directory
            fread(&dir[0], sizeof(struct DirectoryEntry), 16, file_ptr);

            int i;

            printf("token 2:%s \n", token[1]);


            for (i = 0; i < 16; i++)
            {

            if ( strcmp( token[1], dir[i].DIR_Name) == 0 )
              {
                char name[12];

                memset(&name, 0 , 12);
                strncpy(&name, dir[i].DIR_Name, 11);
                printf("%s  attribute: 0x %x file size: %d low: %d\n", name, dir[i].DIR_Attr, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow );
              }
            }

      }


    }
    //
  else  if(strcmp(token[0],"get")==0)
    {

    }
    else if(strcmp(token[0],"put")==0)
    {

    }
    /*
    for relative
    for absolute path: need to tokenize with "/", then go to each dir if exitst go
    to next ....
        it is while loop();
    */
  else  if(strcmp(token[0],"cd")==0)
    {

    }
    /*
    get the add of the rootDir,
     Locate the Root Directory, get the list of fileand folders
     find low cluser num, and replace the offset with the low thing
    */
    /*

    */
  else  if(strcmp(token[0],"ls")==0)
    {

      fseek(file_ptr, 11, SEEK_SET);
      fread(&BPB_BytsPerSec, 2, 1, file_ptr);

      printf("\nbytes per sector %d\n\n", BPB_BytsPerSec);

      //go to root directory
      fseek(file_ptr, 0x100400, SEEK_SET);

      //read the root directory
      fread(&dir[0], sizeof(struct DirectoryEntry), 16, file_ptr);

      int i;


      //while loop (!= EOC) {}
      for (i = 0; i < 16; i++)
      {
        char name[12];
        memset(&name, 0 , 12);

        strncpy(&name, dir[i].DIR_Name, 11);
        printf("%s %d\n", name, dir[i].DIR_FirstClusterLow);
      }


    }
  else  if(strcmp(token[0],"read")==0)
    {

    }
  else  if(strcmp(token[0],"exit")==0)
    {
      printf("EXITING\n");
      exit(0);

    }
  else
    {
      printf("Command not recognized, please enter valid command to execute\n" );
      continue;
    }









    free( working_root );

  }
  return 0;
}
