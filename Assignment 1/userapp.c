// userapp.c - the process to use ioctl's to control the kernel module

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              // open 
#include <unistd.h>             // exit 
#include <sys/ioctl.h>          // ioctl 
#include <time.h>        


 // The major device number. We can't rely on dynamic
 // registration any more, because ioctls need to know
 // it.
 
#define MAJOR_NUM 100

 //Select the channel of the adc 

#define IOCTL_SELECT_CHANNEL _IOR(MAJOR_NUM, 0, int *)

 //Select the alignment of the adc

#define IOCTL_SELECT_ALIGNMENT _IOR(MAJOR_NUM, 1, char *)
 
 //Select the mode of operation of the adc

#define IOCTL_SELECT_MODE _IOR(MAJOR_NUM, 2, char *)

 // The name of the device file is adc-dev
 
#define DEVICE_FILE_NAME "/dev/adc-dev"

uint16_t number;
int j=50;

 // Functions for the ioctl calls

int ioctl_select_channel(int file_desc,int channel)
{
int ret_val;
ret_val=ioctl(file_desc,IOCTL_SELECT_CHANNEL,channel);
if(ret_val<0)
{
printf("Failed to select channel");
exit(-1);
}
return 0;
}

int ioctl_select_alignment(int file_desc,char alignment)
{
int ret_val;
ret_val=ioctl(file_desc,IOCTL_SELECT_ALIGNMENT,alignment);
if(ret_val<0)
{
printf("Failed to select alignment");
exit(-1);
}
return 0;
}

int ioctl_select_mode(int file_desc,char mode)
{
int ret_val;
ret_val=ioctl(file_desc,IOCTL_SELECT_MODE,mode);
if(ret_val<0)
{
printf("Failed to select mode");
exit(-1);
}
return 0;
}

 //delay function for continuous conversion mode

void delay(int number_of_seconds)
{
 // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;
  
 // Storing start time
    clock_t start_time = clock();
  
 // looping till required time is not achieved
    while (clock() < start_time + milli_seconds);
}


 // Main - Call the ioctl functions
 
int main()
{
    int file_desc, ret_val,read_1;
    int channel;
    char alignment;
    char mode;
    
 //Opening device file
    
    file_desc = open(DEVICE_FILE_NAME, 0);
    
    if (file_desc < 0) {
        printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
        exit(-1);
    }

 //Accepting the desired inputs from user for ADC data register
 
printf("Select the Channel of ADC from 0-7\n");
scanf("%d",&channel);
printf("Select the mode of operation (Type S for Single Shot and C for continuous)\n");
scanf(" %c",&mode);
printf("Select the alignment left or right (Type L for left and R for right)\n");
scanf(" %c",&alignment);

 //Respective IOCTL calls

ioctl_select_channel(file_desc,channel);
ioctl_select_mode(file_desc,mode);
ioctl_select_alignment(file_desc,alignment);

 //Printing the final adc data register based on single shot conversion mode or continuous conversion mode

  if(mode=='S')
  {
  read_1=read(file_desc,&number,sizeof(number));
  printf("The requested adc data register is %x\n",number); 
  }
  else if(mode=='C')
  {
  while(j!=0)
  {
  read_1=read(file_desc,&number,sizeof(number));
  printf("The requested adc data register is %x\n",number); 
  delay(100);
  j=j-1;
  }
  }
  
 //Closing the device file
 
    close(file_desc);
    return 0;
}

