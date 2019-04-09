/*
Programming Assignment 3: FAT32 File System
April 3 2019
CONTRIBUTORS
Sudip Ghale
Michael Pena
*/
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
int address = 0x100400;

/* struct declaration for Directory*/
struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
} ;

/*Next three funs are used from the code provided by the professor: function discription is at the end*/
int16_t NextLB(uint32_t sector);
int LBAToOffset(int32_t sector);
int compare(char file_name[50], char img_name[50]);


FILE *file_ptr; // file pointer
char BS_OEMName[8]; //
int16_t BPB_BytsPerSec; // Count of bytes per sector
int8_t BPB_SecPerClus; // Number of sectors per allocation unit.
int16_t BPB_RsvdSecCnt; // Number of reserved sectors in the Reserved region of the volume starting at the first sector of the volume.  FAT #1 starts at address BPB_RsvdSecCnt * BPB_BytsPerSec
int8_t BPB_NumFATs; //The count of FAT data structures on the volume. This field should always contain the value 2 for any FAT volume of any type.
int16_t BPB_RootEntCnt; // For FAT12 and FAT16 volumes, this field contains the count of 32-byte directory entries in the root directory. For FAT32 volumes,this field must be set to 0.
char  BS_VolLab[11]; // Volume label. This field matches the 11-byte volume label recorded in the root directory.
int32_t BPB_FATSz32; // This field is only defined for FAT32 media and does not exist on FAT12 and FAT16 media. This field is the FAT32 32-bit count ofsectors occupied by ONE FAT. BPB_FATSz16 must be 0. Total FAT size is BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec


int main(int argc, char *argv[])
{
  struct DirectoryEntry dir[16]; // using just 16 array for the dir
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

/*
Clusters start at address (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec)
Clusters are each (BPB_SecPerClus * BPB_BytsPerSec) in bytes
Root Directory is at the first cluster.
Address: (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +(BPB_RsvdSecCnt * BPB_BytsPerSec)
*/

  int32_t RootDirSectors = 0;//RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec – 1)) / BPB_BytsPerSec;
  int32_t FirstDataSector = 0;//FirstDataSector = BPB_ResvdSecCnt + (BPB_NumFATs * FATSz) + RootDirSectors;
  int32_t FirstSectorofClustor = 0;//FirstSectorofCluster = ((N – 2) * BPB_SecPerClus) + FirstDataSector;
  int flag_open = FALSE;

  while( 1 )
  {
    printf ("mfs> ");// Print out the mfs prompt

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

    //catches empty buffer prevents from seg fault
    if (token[0] == NULL)
    {
      continue;
    }
    /*if command is open, then open the file, display message enter file name if not privide,
     for open error(if not succes fopen),file already oppened  */
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

          fseek(file_ptr, address, SEEK_SET);//go to root directory
          fread(&dir[0], sizeof(struct DirectoryEntry), 16, file_ptr);  //read the root directory

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

           printf("file open successful\n" );
           flag_open = TRUE;
      }
    }
    /*
    if the command is close then close the oppned file or display file not opped in that case
    */
    else if(strcmp(token[0],"close")==0)
    {

      if ( (flag_open == FALSE)  ) //if file is not oppened then print error
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
  /*
  ASSUMED THE FILE IS OPPNED:
if the command is info then print the five BPB indexed
*/
  else  if(strcmp(token[0],"info")==0)
    {
      //%x for hexadecimal
      printf(" BPB_BytsPerSec in hexadecimal is 0x%x and in decimal %d\n", BPB_BytsPerSec, BPB_BytsPerSec );
      printf(" BPB_SecPerClus in hexadecimal is 0x%x and in decimal %d\n", BPB_SecPerClus, BPB_SecPerClus );
      printf(" BPB_RsvdSecCnt in hexadecimal is 0x%x and in decimal %d\n", BPB_RootEntCnt, BPB_RootEntCnt );
      printf(" BPB_NumFATs in hexadecimal is 0x%x and in decimal %d\n", BPB_NumFATs, BPB_NumFATs );
      printf(" BPB_FATSz32 in hexadecimal is 0x%x and in decimal %d\n", BPB_FATSz32, BPB_FATSz32 );
    }
    /*
     print attributes and the starting cluster  num of the file or dir.
      print: name , attrbute, cluster low, size,
    */
    else if(strcmp(token[0],"stat")==0) // DONE assumed file is oppned
        {
          if(token[1]==NULL)
          {
            printf("Error: need file name or dir name\n" );
            continue;
          }
          else
          {
                int match =0;
                char copyToken[12];
                strncpy( copyToken, token[1], strlen( token[1] ) ); // copy original token

                int i;
                for (i = 0; i < 16; i++) // loop to find the file/dir
                {
                  char name[12];
                  memset(&name, 0 , 12);
                  match =0;
                  strncpy(&name, dir[i].DIR_Name, 11);
                  strncpy( token[1],copyToken, strlen( copyToken ) );// copying original token
                  match = compare(token[1], dir[i].DIR_Name); //fun called to see if the filename matches
                  if (match) // if matched then print dir/file infos
                    {
                      printf("%s, DIR_Attr: 0x%x, DIR_FileSize: %d and DIR_FirstClusterLow: %d\n",name, dir[i].DIR_Attr, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow );
                      break;
                    }
                }
                if(!match) printf("%s file didn't find\n",token[1] );// if not print the message
          }
        }
        /*
        if command is get check the if filenames in the fat file, is so
        read the data and copy into buffer. then open file in the cwd and copy the buffer
        into it.
        */
  else  if(strcmp(token[0],"get")==0)
    {
      if(token[1]==NULL)
      {
        printf("Error: need file name or dir name\n" );
        continue;
      }
      else //ASSUMED ITS FILENAME
      {

        int i, match =0;
        char copyToken[12];
        int cluster_low;
        char name[12];
        FILE  *file_to_cwd;

        strncpy( copyToken, token[1], strlen( token[1] ) );

        for (i = 0; i < 16; i++)
        {

          memset(&name, 0 , 12);
          match =0;
          strncpy(&name, dir[i].DIR_Name, 11);
          strncpy( token[1],copyToken, strlen( copyToken ) );
          match = compare(token[1], dir[i].DIR_Name);
          if (match)
            {
              char *buffer= malloc(sizeof(char)* dir[i].DIR_FileSize); // allocate for the dir entry
              if(buffer == NULL) // print error
              {
                printf("Error: malloc\n" );
                break;
              }

            cluster_low = (int)dir[i].DIR_FirstClusterLow; // get the low cluster number
              while(cluster_low != -1)// while loop low cluster num is -1 ( ie no more data into next cluster)
              {
                address = LBAToOffset(cluster_low); // get the pointer to the data

                fseek(file_ptr, address, SEEK_SET);// point the data region
                fread(buffer, dir[i].DIR_FileSize, 1, file_ptr); // fread the data

                cluster_low = NextLB(cluster_low); //update the low cluster number
              }
              file_to_cwd = fopen(copyToken, "w"); // open the file in cwd in write mode
              if(file_to_cwd)
              {
                fwrite(buffer,sizeof(buffer),1,file_to_cwd);// write from buffer to the file pointer
                printf("File retrived into the cwd\n" ); //display msg
              }
              fclose(file_to_cwd); // close the file
             address = 0x100400; // update the root dir address
              break;
            }
        }
        if(!match) printf("Error: file not found.\n" );// print msg
      }
    }
    /*
    Make a directory entry
    set the attribute to file value
    Change the FAT table
    1. loop from 0 to 16
    2. look at first byte of of fname and check if its 0 or 0xe5 that means it's free
    3. Make a new variable holding the address by passing that directory low cluster number. this is the address of start
    4. fseek into start address and repeat above step to find next deleted or free cluster.
    5. ??? set fat to -1 marks the end of the fwrite
    6. fseek to beginning of address and fwrite until it finds the end.
    */
// *******************SPENT TOO MUCH TIME ON THIS ASG: HOPE YOU WILL GIVE PARTIAL CREDIT ON PUT ************************************
    else if(strcmp(token[0],"put")==0)
    {
      FILE *file_of_cwd;
      int cluster_low, i, size_of_file, next_addr, index;

      if(token[1]==NULL)
      {
        printf("Error: need file name or dir name\n" );
        continue;
      }
      else //ASSUMED ITS FILENAME
      {
        file_of_cwd = fopen(token[1], "r");
        if (file_of_cwd == NULL)
        {
          printf("Error: File not found\n" );
          continue;
        }

        size_of_file = sizeof(file_of_cwd);
        printf("FILE SIZE  %d\n",size_of_file) ;

    for( i =0; i < 16; i++)
    {

      if(dir[i].DIR_Attr == 0x02 || dir[i].DIR_Name[0] == (char)5|| dir[i].DIR_Name[0] == (char)229)
      {
        uint8_t buffer_read[512];
        index =i;
        dir[i].DIR_Attr == (int8_t) 0x20;
        strcpy(&dir[i].DIR_Name, token[1]);
        dir[i].DIR_FileSize = size_of_file;

        cluster_low = dir[i].DIR_FirstClusterLow;
        next_addr = LBAToOffset(cluster_low);
        fseek(file_ptr, next_addr, SEEK_SET);

        if(size_of_file >512)
        {
          printf("size of file is >512\n" );
          fread(&buffer_read, 512, 1, file_of_cwd);
          fwrite(&buffer_read, 512, 1, file_ptr);

          cluster_low = NextLB(cluster_low);
          next_addr = LBAToOffset(cluster_low);
          fseek(file_ptr, next_addr, SEEK_SET);
          size_of_file -= 512;
        }

        fread(&buffer_read, 512, 1, file_of_cwd);
        printf("BUFFER IS A%sA\n",buffer_read );

        fwrite(&buffer_read, 512, 1, file_ptr);
        break;


        }
      }
    }




    }


    /*
    for relative
    for absolute path: need to tokenize with "/", then go to each dir if exitst go
    to next ....
        it is while loop();
    use int LBAToOffset(int32_t sector)  // sector = DIR_FirstClusterLow
    { return ((sector -2) * BPB_BytsPerSec)+(BPB_BytsPerSec* BPB_RsvdSecCnt)+(BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);
    }
    then read in the values like we did for the root dir fseek and fread
    Files and sub-directory entries can be found by going to their first cluster number
    Find first cluster numberin directory entry of the file or directory at hand
    Figure out the sector to read using cluster number and FirstSectorofCluster equation
    Read that cluster
    */

/*
FOR CD:
fist see if passed in .., the compare if .. dir exist
else compare if the dir name exist if so
get the low cluster num of the dir , and passed that into LBAToOffset(cluster) fun to get the address to the data region
and update the address var
*/
  else if(strcmp(token[0],"cd")==0)
    {
      if(token[1]==NULL)
      {
        printf("Error: need dir name\n" );
        continue;
      }
      else
      {
            int i=0, match = 0, directory=0, index=0, notfound=0;
            char copyToken[12];
            char name[12];

          memset(&copyToken,0,12);

            strncpy( copyToken,token[1], strlen( token[1] ) );
            for (i = 0; i < 16; i++)
            {
              match = 0;
              memset(&name, 0 , 12);
              strncpy(&name, dir[i].DIR_Name, 11);
              strncpy( token[1],copyToken, strlen( copyToken ) );
              if(strcmp(token[1],"..")==0)
              {
                if(strncmp(token[1],dir[i].DIR_Name,2) ==0)
                {
                  match =1;
                  index = i;
                  break;
                }
              }
              else
              {
                match = compare(token[1], dir[i].DIR_Name);
                if(match)
                {
                  index=i;
                  break;
                }

              }
            }

              if (match)
                {
                  if(dir[index].DIR_Attr == (int8_t)0x10)
                  {
                    directory =1;
                  }
                }

            if(directory)
            {
              int cluster = dir[index].DIR_FirstClusterLow;
              address = LBAToOffset(cluster);
              if(address==0x100000) address = 0x100400;
              fseek(file_ptr, address, SEEK_SET);
            }
              if(!directory) printf("Error: cd: %s: Not a directory or not found\n",copyToken);
            }

    }
    /*
  for ls :
    check to see if the file is oppened : if so the
      see if .. is passed in, if so see if .. dir exist if so get cluster and update the address variable
      then fseek and fread to that address print the files and dirs
    */
  else  if(strcmp(token[0],"ls")==0)  //assumed file oppened
    {
      int i=0;
    if(flag_open== TRUE)
      {
        if(token[1] !=NULL && strcmp(token[1],"..")==0)
        {
          for(i =0; i <16 ;i++)
          {
          if(strncmp("..",dir[i].DIR_Name,2) ==0)
              {
                int cluster = dir[i].DIR_FirstClusterLow;
                address = LBAToOffset(cluster);
                if(address==0x100000) address = 0x100400;
              }
            }
        }

        fseek(file_ptr, address, SEEK_SET);
        fread(&dir[0], sizeof(struct DirectoryEntry), 16, file_ptr);

        int i;

        for (i = 0; i < 16; i++)
          {
            char name[12];
            memset(&name, 0 , 12);
            if(dir[i].DIR_Name[0] != (char)5 || dir[i].DIR_Name != (char)229) //e5=229 : if deleted files
              {
                if(dir[i].DIR_Attr == (int8_t) 0x1|| dir[i].DIR_Attr ==(int8_t)0x10||dir[i].DIR_Attr == (int8_t) 0x20) // requirement
                 {
                  strncpy(&name, dir[i].DIR_Name, 11);
                  printf("%s %d\n", name, dir[i].DIR_FirstClusterLow);

                 }
              }
          }
      }
      else printf("Error: please open the file first to execute ls\n" );// print error message
    }
/*
IF READ:
valid if all the pareameters are passed in:
search for filename if found: get the cluster and address  then fseek to the add+ position of the data
from that read each bite using loop into the buffer and print it
then update the address variable

*/
  else  if(strcmp(token[0],"read")==0)
    {

       //read <filename> <position> <number of bytes>
      if( token[1] == NULL ||token[2] == NULL || token[3] == NULL)
      {
        printf("Error: parameter error\n" );
        continue;
      }

       int position = *token[2];
       position -= '0'; //converts the postion to int

       int number_of_bytes_to_read = *token[3];
       number_of_bytes_to_read -= '0'; //converts the number of bytes to int

       int match = 0;
       char copyToken[12];
       strncpy( copyToken, token[1], strlen( token[1] ) );

       int i, j, k = 0;
       for (i = 0; i < 16; i++)
       {
        char name[12];
        memset(&name, 0 , 12);
        match = 0;
        strncpy(&name, dir[i].DIR_Name, 11);
        strncpy( token[1],copyToken, strlen( copyToken ) );
        match = compare(token[1], dir[i].DIR_Name);

         if (match)
          {

             int cluster_low = dir[i].DIR_FirstClusterLow;
             address = LBAToOffset(cluster_low);
             char *buffer= malloc(sizeof(char)* (dir[i].DIR_FileSize)); // buffer to read

             //goes to the position specified by the user
             fseek(file_ptr, address + position, SEEK_SET);


             //populates the buffer from position to number_of_bytes_to_read
             for (j = 0; j < (number_of_bytes_to_read*2); j++)
             {
                fread(buffer+j, 1, 1, file_ptr);
             }

              printf("%s\n", buffer);
              address = 0x100400;

             break;
          }
      }
      if(!match) printf("%s file didn't find\n",token[1] );


    }
  else  if(strcmp(token[0],"exit")==0) // if exit then close the file_ptr
    {
      printf("EXITING\n");
      fclose(file_ptr);
      exit(0);

    }
  else
    {
      printf("Command not recognized, please enter valid command to execute\n" );
      continue;
    }

    free( working_root ); // free working root

  }
  return 0;
}
// funnctions ******************************************************************
int LBAToOffset(int sector) // fun to get the offset to the cluster
{
  return ((sector -2) * BPB_BytsPerSec)+(BPB_BytsPerSec* BPB_RsvdSecCnt)+(BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);
}


int16_t NextLB(uint32_t sector) // fun to see if there is data into other cluster
{
  uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4 );
  int16_t val;
  fseek (file_ptr, FATAddress, SEEK_SET);
  fread(&val, 2,1, file_ptr);
  return val;

}


int compare(char file_name[50], char img_name[50]) // compare fun to see file name matches
{

  char expanded_name[12];
  memset( expanded_name, ' ', 12 );
  char *token = strtok( file_name, "." );
  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "/n" );

  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }

  expanded_name[11] = '\0';
  int i;
  for( i = 0; i < 11; i++ )
  {
    expanded_name[i] = toupper( expanded_name[i] );
  }

  if( strncmp( expanded_name, img_name, 11 ) == 0 )
  {
    return 1;
  }
  else return 0;
}
