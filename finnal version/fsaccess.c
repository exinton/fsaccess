#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <signal.h>

/*
  Function Declarations for builtin shell commands:
 */

int sh_test(char **);
int sh_exit(char **);
int sh_initfs(char **);
int sh_ls(char **);
int sh_cpin(char **);
int sh_cpout(char **);
int sh_mkdir(char **);
int sh_cd(char **);
int sh_test(char **);
int get_block(int x);
int get_Block_Large_Copy(int , int , int );
int get_Block_Huge_Copy(int , int , int );
int get_inode(int);
void initFileInode(int,int);
int get_Directory_Entry_Offset(int , int);
void init_directory(int,int,int);
void printDirLayout(int,int);
void clear_Block(int, int);

/*
  Global Variables:
 */

static int  g_currentDirectoryInode=1;
char static filletypeMappingTable[4]="FCDB";
int static g_fd=-1;						//file descriptor for the file, default value is -1.


struct super_Block {						//struct for SuperBlock
	unsigned short isize;
	unsigned short fsize;
	unsigned short nfree;
	unsigned short free[100];
	unsigned short ninode;
	unsigned short inode[100];
	char flock;
	char ilock;
	char fmod;
	unsigned short time[2];
	unsigned char pad[97];
	}SB_empty={0,0,0,{0},0,{0},0,0,0,{0},{0}};

struct inode_Flag{						//struct for InodeFlag
	unsigned short alloc:1;
	unsigned short fileType:2;
	unsigned short largeFile:1;
	unsigned short setUid:1;
	unsigned short setGrp:1;
	unsigned short unalloc:1;
	unsigned short readOwner:1;
	unsigned short writeOwner:1;
	unsigned short exeOwner:1;
	unsigned short wregrp:3;
	unsigned short wreothers:3;
}park;

struct inode{  							//Struct for Inode
	struct inode_Flag inodeFlagStruct;
	char nlinks;				//number of links for files
	char uid;
	char gid;
	unsigned char size0;
	unsigned short size1;
	unsigned short address[8];	//address of data blocks pointing to
	unsigned short acttime[2];	//time of last access
	unsigned short modtime[2];	//time of last modification
}inode_empty={{0},0,0,0,0,0,{0},{0},{0}};

struct inode_List{						//struct for inode list ,which contains 16 inode struct

	struct inode inodeblock[16];

};


struct file_Size{					//struct for the 25th bit to indicate the file exceeding 16MB
	unsigned short size1;
	unsigned char size0;
	unsigned char mostsignificant;

}filesize_empty={0,0,0};


struct directory_Entry_Single{				//

	unsigned short inode0;
	char	filename0[14];

}dirEntry_empt={0,{0}};

struct directory_Entries_Combination{

	struct directory_Entry_Single superDir[32];

}superDire_empty={0};

struct indirect_Data_Block_Layout{

	unsigned short address[256];


}indirectBlock_empty={0};








/*
  List of builtin commands, followed by their corresponding functions.
 */

char *builtin_str[] = {  //pointer to command strings
  "q",
  "test",
  "initfs",
  "ls",
  "cpin",
  "cpout",
  "mkdir",
  "cd"
};


int (*builtin_func[]) (char **) = {  //pointer to  function
  &sh_exit,
  &sh_test,
  &sh_initfs,
  &sh_ls,
  &sh_cpin,
  &sh_cpout,
  &sh_mkdir,
  &sh_cd
};

int sh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}


/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: exit.
   @param args List of args.
   @return Always returns 0, to terminate execution.
 */

int sh_exit(char **args) //Exit the shell
{
	close(g_fd);
  return 0;
}



int sh_test(char **args)	//For debug purpose only
{

printf("For debug purpose only! \n");

  return 1;


}

/**
 copy_DataBlock
 copy the size oarray buffer's content to the destination inode's availabe datablock

 */



void copy_DataBlock(int fd, int inodeFileNum,char buffer[],int copied_Size ){


	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	struct file_Size filesize0,*ptr_filesize;
	ptr_filesize=&filesize0;
	struct indirect_Data_Block_Layout indirectBlock0,*ptr_indirecrBlock;
	ptr_indirecrBlock=&indirectBlock0;
	indirectBlock0=indirectBlock_empty;
	struct super_Block superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;

	lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);		//read in the inode struct from file
	read (fd,ptr_inode,32);

	lseek(fd,512,SEEK_SET);								//read in the superblock struct from file
	read(fd,ptr_superblock,512);

	unsigned int *ptr_fileSizeValue;					//use struct to convert the 25th bit  as length of the file
	ptr_fileSizeValue=(unsigned int *)ptr_filesize;
	filesize0.mostsignificant=inode0.inodeFlagStruct.unalloc;
	filesize0.size0=inode0.size0;
	filesize0.size1=inode0.size1;




if ((*ptr_fileSizeValue/512)<8 )							//if the destfile is still small file

{

		//assign the datablock to inode
		int newFileDB=get_block(fd);

		//calculate the address field in Inode to fill
		int inodeAddrNum=*ptr_fileSizeValue/512;
		inode0.address[inodeAddrNum]=newFileDB;


		//copy the copied_Size of byte into dest datablock
		lseek(fd,512*newFileDB,SEEK_SET);
		write(fd,buffer,copied_Size);

		//increment the file size by the copied byte
		*ptr_fileSizeValue=*ptr_fileSizeValue+copied_Size;


		//updated the fileszie into disk
		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		inode0.inodeFlagStruct.fileType=0;
		inode0.modtime[0]=(unsigned short) time(NULL);
		inode0.gid=1;
		inode0.uid=1;
		inode0.inodeFlagStruct.alloc=1;
		inode0.inodeFlagStruct.exeOwner=1;
		inode0.inodeFlagStruct.readOwner=1;
		inode0.inodeFlagStruct.setGrp=1;
		inode0.inodeFlagStruct.setUid=1;
		inode0.inodeFlagStruct.wregrp=1;
		inode0.inodeFlagStruct.writeOwner=1;


		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);

	}

else if((*ptr_fileSizeValue/512)==8)				//change from small file to large file as soon as receiving the the 9th block
	{
	    //store the previous 8 data block temperately
		unsigned short temp[8];
		int i;
		for (i=0;i<8;i=i+1){
			temp[i]=inode0.address[i];
				}

		//clean the address area
		for (i=1;i<8;i=i+1){
			inode0.address[i]=0;
			}

		//update the address in 1st indirect block
		for (i=0;i<8;i=i+1){
		indirectBlock0.address[i]=temp[i];
		}

		//update the address 0 with the new assigned indirect block
		inode0.address[0]=get_block(fd);;
		indirectBlock0.address[8]=get_block(fd);
		lseek(fd,512*inode0.address[0],SEEK_SET);
		write(fd,ptr_indirecrBlock,512);


		//copy content
		lseek(fd,512*indirectBlock0.address[8],SEEK_SET);
		write(fd,buffer,copied_Size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+copied_Size;

		//update copied_Size
		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		inode0.inodeFlagStruct.largeFile=1;
		inode0.inodeFlagStruct.fileType=0;

		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);


	}


else if((*ptr_fileSizeValue/512)>8 && (*ptr_fileSizeValue/(512*256))<7 )		//large file
	{

		int newBlock = get_Block_Large_Copy(fd, *ptr_fileSizeValue,inodeFileNum);
		lseek(fd,512*newBlock,SEEK_SET);
		write(fd,buffer,copied_Size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+copied_Size;

		lseek(fd, 512*2+(inodeFileNum-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);

		//update copied_Size
		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		inode0.inodeFlagStruct.fileType=0;

		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);
		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);

		}


else if(*ptr_fileSizeValue >= 7*256*512)						//extra large file
		{
		int blocktowrite=get_Block_Huge_Copy(fd,*ptr_fileSizeValue,inodeFileNum);
		lseek(fd,blocktowrite*512,SEEK_SET);
		write(fd,buffer,copied_Size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+copied_Size;

		//update copied_Size
		lseek(fd, 512*2+(inodeFileNum-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);
		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);
		}

}

int get_Block_Large_Copy(int fd, int filesize,int inodeNum){
	struct indirect_Data_Block_Layout indirectBlock0,*ptr_indirecrBlock;
	ptr_indirecrBlock=&indirectBlock0;

	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;

	lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
	read(fd,ptr_inode,32);

	int blocknum=filesize/512;
	int Level1Num=blocknum/256;
	int Level2Num=blocknum%256;


	lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
	read(fd,ptr_inode,32);

	if(inode0.address[Level1Num]==0){	//if inode0.address[Level1Num] no assigned with data blocks
		int blockid=get_block(fd);
		inode0.address[Level1Num]=blockid;		//?
		lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);
		lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);
	}


	lseek(fd,512*inode0.address[Level1Num],SEEK_SET);
	read(fd,ptr_indirecrBlock,512);	//read in struct of 1st level indirect block

	if(indirectBlock0.address[Level2Num]==0){
		int blockid=get_block(fd);
		indirectBlock0.address[Level2Num]=blockid;
		lseek(fd,512*inode0.address[Level1Num],SEEK_SET);
		write(fd,ptr_indirecrBlock,512);
	}
	lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
	read(fd,ptr_inode,32);

	return indirectBlock0.address[Level2Num];


}




int get_Block_Huge_Copy(int fd, int filesize,int inodeNum){ //convert huge file's databloc address when in address[7]

	struct indirect_Data_Block_Layout indirectBlock0,*ptr_indirecrBlock,indirectBlock1,*ptr_indirecrBlock1;
	ptr_indirecrBlock=&indirectBlock0;
	ptr_indirecrBlock1=&indirectBlock1;
	indirectBlock0=indirectBlock_empty;
	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;

	int effectiveSize=filesize-256*512*7;
	int blocknum=effectiveSize/512;
	int Level1Num=blocknum/256;
	int Level2Num=blocknum%256;

	lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
	read(fd,ptr_inode,32);

	int blockid;

	if(inode0.address[7]==0){
		blockid=get_block(fd);
		inode0.address[7]=blockid;
		lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);


	}


	lseek(fd,512*inode0.address[7],SEEK_SET);
	read(fd,ptr_indirecrBlock,512);	//read in struct of 1st level indirect block

	if(indirectBlock0.address[Level1Num]==0){
		blockid=get_block(fd);
		indirectBlock0.address[Level1Num]=blockid;
		lseek(fd,inode0.address[7]*512,SEEK_SET);	//
		write(fd,ptr_indirecrBlock,512);


	}

	lseek(fd,512*indirectBlock0.address[Level1Num],SEEK_SET);
	read(fd,ptr_indirecrBlock1,512);
	if(indirectBlock1.address[Level2Num]==0){
		blockid=get_block(fd);
		indirectBlock1.address[Level2Num]=blockid;
		lseek(fd,512*indirectBlock0.address[Level1Num],SEEK_SET);
		write(fd,ptr_indirecrBlock1,512);

	}
	return indirectBlock1.address[Level2Num];

}





int create_File(int fd, char filename[]){

	  //get inode for file
	  int fileInode=get_inode(fd);

	  //initial inode field
	  initFileInode(fd,fileInode);

	  //add the file into directory layout DB block
	  int entryAddress=get_Directory_Entry_Offset(g_currentDirectoryInode,fd);
	  if(entryAddress==-1)
	  	{
	  		return -1;
	  	}

	  	struct directory_Entry_Single dirEntry0,*ptr_dirEntry;
	  	ptr_dirEntry=&dirEntry0;
	  	dirEntry0=dirEntry_empt;
	  	dirEntry0.inode0=fileInode;
	  	char test[14];
	  	strncpy(test,filename,14);					//only get the first 14 characters as the file name to avoid conflict
		strncpy(dirEntry0.filename0,test,14);

	  	lseek(fd,entryAddress,SEEK_SET);
	  	write(fd,ptr_dirEntry,16);
	  	return fileInode;

}



int sh_cpin(char **args)
{

	  if(g_fd==-1){
		  printf("please init the dist at first ! \n");
		  return 1;
	  }

	  char destinationFileName[14];
	  strcpy(destinationFileName,args[2]);

	  char buffer[512];

	  //open source file
	  int fdSource=open( args[1],O_RDWR | O_CREAT,0666);

	  //select the right dest file
	  int fdDest=g_fd;

	  //create the file
	  int assignedInode=create_File(fdDest,destinationFileName);

	  //start to copy
	  int numberReaded=0;
	  printf("Copying: \n");
	  do{
	  numberReaded=read (fdSource,buffer,512);
	  printf("*");
	  copy_DataBlock(fdDest,assignedInode,buffer,numberReaded); 	//size of copying equals to the byte read from source file
	  }while(numberReaded>=512);
	  printf("\n");

	  return 1;
}



int sh_cpout(char **args)
{

	  if(g_fd==-1){
		  printf("please init the dist at first ! \n");
		  return 1;
	  }
		  char sourceFileName[14];
		  strcpy(sourceFileName,args[1]);

		  int readout;
		  char buffer[512];


		  struct inode inode0,*ptr_inode;
		  ptr_inode=&inode0;

		  struct directory_Entries_Combination dirEntry0,*ptr_dirEntry;
		  ptr_dirEntry=&dirEntry0;
		  int inodeNum;
		  struct indirect_Data_Block_Layout indirectBlock0,*ptr_indirecrBlock,indirectBlock1,*ptr_indirecrBlock1;
		  ptr_indirecrBlock=&indirectBlock0;
		  ptr_indirecrBlock1=&indirectBlock1;
		  indirectBlock0=indirectBlock_empty;
	      struct file_Size filesize0,*ptr_filesize;
		  ptr_filesize=&filesize0;
		  unsigned int *ptr_fileSizeValue;
		  ptr_fileSizeValue=(unsigned int *)ptr_filesize;


		  //select the right dest file
		  int fdDest=open(args[2],O_RDWR | O_CREAT,0666);
		  //check whether the file is already there


		  //get inode, assume to be under the root dir.
		  lseek(g_fd,512*2+(g_currentDirectoryInode-1)*32,SEEK_SET);
		  read (g_fd,ptr_inode,32);
		  lseek(g_fd,512*inode0.address[0],SEEK_SET);
		  read(g_fd,ptr_dirEntry,512);
		  int i;

		  for (i=0;i<32;i=i+1){
			  if(strcmp(dirEntry0.superDir[i].filename0,sourceFileName)==0){
			  inodeNum=dirEntry0.superDir[i].inode0;
			  }
		  }

		  //check sourcefile's size;if it's extra largefile
			lseek(g_fd, 512*2+(inodeNum-1)*32,SEEK_SET);
		 	read(g_fd,ptr_inode,32);

			filesize0.mostsignificant=inode0.inodeFlagStruct.unalloc;
			filesize0.size0=inode0.size0;
			filesize0.size1=inode0.size1;
		    int remainfileSize=*ptr_fileSizeValue;
		    printf("copying start: \n");

	if(*ptr_fileSizeValue>512*256*7){		//copy for byte from 0-512*256*7

				int j;
				for (i=0;i<7;i=i+1){

					lseek(g_fd,512*inode0.address[i],SEEK_SET);
					read(g_fd,ptr_indirecrBlock,512);
					for(j=0;j<256;j=j+1)
						{
							lseek(g_fd,512*indirectBlock0.address[j],SEEK_SET);
							readout=read(g_fd,buffer,512);
							write(fdDest,buffer,readout);
							printf("*");
						}
				}

				remainfileSize=*ptr_fileSizeValue-512*256*7;	//copy 	for byte larger than 512*256*7
				//for byte greater thean 512*256*7
				lseek(g_fd,512*inode0.address[7],SEEK_SET);
				read(g_fd,ptr_indirecrBlock,512);

				for (i=0;i<256 && remainfileSize>0;i=i+1){
					lseek(g_fd,512*indirectBlock0.address[i],SEEK_SET);
					read(g_fd,ptr_indirecrBlock1,512);

					for (j=0;j<256 && remainfileSize>0;j=j+1){

						lseek(g_fd,indirectBlock1.address[j]*512,SEEK_SET);

						if(remainfileSize<512)
							readout=read(g_fd,buffer,remainfileSize);
						else
							readout=read(g_fd,buffer,512);
						write(fdDest,buffer,readout);
						remainfileSize=remainfileSize-readout;
						printf("*");
					}
				}
				printf("\n");


	}


	if(*ptr_fileSizeValue<=512*256*7 && *ptr_fileSizeValue > 512*8)	//large file copy
	{
	int j;
		for (i=0;i<7 && remainfileSize>0;i=i+1){

			lseek(g_fd,512*inode0.address[i],SEEK_SET);
			read(g_fd,ptr_indirecrBlock,512);

			for(j=0;j<256 && remainfileSize>0;j=j+1)
			{
				lseek(g_fd,512*indirectBlock0.address[j],SEEK_SET);

				if(remainfileSize<512)
					readout=read(g_fd,buffer,remainfileSize);
				else
					readout=read(g_fd,buffer,512);

				write(fdDest,buffer,readout);
				remainfileSize=remainfileSize-readout;
				printf("*");

			}

		}
		printf("\n");


	}


if(*ptr_fileSizeValue<=512*8)					//small file copy
	{	//small
		for (i=0;i<8 && remainfileSize>0 ;i=i+1){
			lseek(g_fd,inode0.address[i]*512,SEEK_SET);

			if(remainfileSize<512)
				readout=read(g_fd,buffer,remainfileSize);
			else
				readout=read(g_fd,buffer,512);

			write(fdDest,buffer,readout);
			remainfileSize=remainfileSize-readout;
			printf("*");
		}
	}
	printf("\n");
	return 1;
}



int get_Directory_Entry_Offset(int inode,int fd){
	//get an available entry in directory layout datablock
		struct directory_Entries_Combination dirEntry0,*ptr_dirEntry;
		ptr_dirEntry=&dirEntry0;
		struct inode inode0,*ptr_inode;
		ptr_inode=&inode0;
		lseek(fd,512*2+(inode-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);
		int i,j;
		for (j=0;j<8 && inode0.address[j]!=0;j=j+1){					//check all the address in inode
			lseek(fd,512*inode0.address[j],SEEK_SET);
			read (fd,ptr_dirEntry,512);
			for(i=0;i<32;i=i+1){
				 if(dirEntry0.superDir[i].inode0==0){
					return 512*inode0.address[j]+i*16;
				 }

			}

		}

		return -1;
}


int sh_cd( char **args)
{
	  if(g_fd==-1){
		  printf("please init the dist at first ! \n");
		  return 1;
	  }

	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	struct directory_Entries_Combination superDir0,*ptr_superDir;
	ptr_superDir=&superDir0;

	lseek(g_fd,512*2+(g_currentDirectoryInode-1)*32,SEEK_SET);
	read (g_fd,ptr_inode,32);

	//loop
	int i,j;
	for (i=0;i<8;i=i+1){

	lseek(g_fd,512*inode0.address[i],SEEK_SET);
	read(g_fd,ptr_superDir,512);

		for(j=0;j<32;j=j+1){
			if (strcmp(superDir0.superDir[j].filename0,args[1])==0)
			{
				//check whether it's directory
				lseek(g_fd,512*2+(superDir0.superDir[j].inode0-1)*32,SEEK_SET);
				read(g_fd,ptr_inode,32);
				if(inode0.inodeFlagStruct.fileType==2){
					g_currentDirectoryInode=superDir0.superDir[j].inode0;
					printf("Change directory to %s   \n",args[1]);
					return 1;
				}

			}

			}
		printf("cannot find the directory name %s\n",args[1]);
		return -1;
	}
	 return 1;
}


int sh_mkdir(char **args)
{
	if(g_fd==-1){
	  printf("please init the dist at first ! \n");
	  return 1;
	  }

	int entryAddress=get_Directory_Entry_Offset(g_currentDirectoryInode,g_fd);
	if(entryAddress==-1)
	{
		printf("reach the maximum files number under the folder \n");
		return -1;
	}

	//get inode

	int dirInode=get_inode(g_fd);


	lseek(g_fd,entryAddress,SEEK_SET);

	struct directory_Entry_Single dirEntry0,*ptr_dirEntry;
	ptr_dirEntry=&dirEntry0;

	dirEntry0.inode0=dirInode;
	strncpy(dirEntry0.filename0, args[1],14);			//just string copy 14 byte to directory name
	write(g_fd,ptr_dirEntry,16);

	init_directory(g_fd,dirInode,g_currentDirectoryInode);
	//initial the datablock of this directory

	 return 1;

}


//list the directory layout of current path
int sh_ls(char **args)
{

	  //open the file as partition
	  if(g_fd==-1){
		  printf("please init the dist at first ! \n");
		  return 1;
	  }

	//go to the nfree of the current path inode
	lseek(g_fd,512*2+(g_currentDirectoryInode-1)*32,SEEK_SET);//
	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	read (g_fd,ptr_inode,32);

	//check the addresses of the inode

	int i;
	for (i=0;i<8;i=i+1)
	{

		if(inode0.address[i]!=0)
		{
		printDirLayout(g_fd,inode0.address[i]);

		}
	}

return 1;

}

void printDirLayout(int fd, int datablockofDir)
{

	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;

	lseek(fd,512*datablockofDir,SEEK_SET);//

	struct directory_Entries_Combination directory0,*ptr_directory;
	ptr_directory=&directory0;

	struct file_Size filesize0,*ptr_filesize;
	ptr_filesize=&filesize0;

	unsigned int *ptr_fileSizeValue;
	ptr_fileSizeValue=(unsigned int *)ptr_filesize;
	lseek(fd,512*datablockofDir,SEEK_SET);//
	read (fd,ptr_directory,512);

	int i;
	printf("Directory Layout:\n");
	for (i=0;i<32;i=i+1){

		if(directory0.superDir[i].inode0 != 0)
		{

			lseek(fd,512*2+(directory0.superDir[i].inode0-1)*32,SEEK_SET);//
			read (fd,ptr_inode,32);
			filesize0.mostsignificant=inode0.inodeFlagStruct.unalloc;
			filesize0.size0=inode0.size0;
			filesize0.size1=inode0.size1;
			printf("inode: %d, filename: %s filetype: %c filesize:%d Byte\n",directory0.superDir[i].inode0,directory0.superDir[i].filename0,filletypeMappingTable[inode0.inodeFlagStruct.fileType],*ptr_fileSizeValue);
		}


	}
}



void add_block(int fd,int blockToFree){

	 struct super_Block superblock0,*ptr_superblock;
	 ptr_superblock=&superblock0;
	 lseek(fd,512*1,SEEK_SET);//
	//read the nfree and free[] array fields
	 read(fd,ptr_superblock,512);

	if ( (superblock0.nfree < 100) || (superblock0.nfree == 0)){ //if only superblock has list, free[0] =0
	//add the free blocks into free block list in super block

		superblock0.free[superblock0.nfree]=blockToFree;
		superblock0.nfree=superblock0.nfree+1;
		lseek(fd,512*1,SEEK_SET);
		write(fd,ptr_superblock,512);
	}
	else if (superblock0.nfree ==100){
    //copy nfree and free array into block to free
	lseek(fd,512*blockToFree,SEEK_SET);


	write(fd,&superblock0.nfree,2); //why ptr_superblock->nfree does not work!!!
	write(fd,ptr_superblock->free,200);

	superblock0.nfree=0;
	superblock0.free[superblock0.nfree]=blockToFree;
	superblock0.nfree=superblock0.nfree+1;

	lseek(fd,512*1,SEEK_SET);
	write(fd,ptr_superblock,512);

	}


}


int get_block(int fd){

	struct super_Block superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;

	lseek(fd,512*1,SEEK_SET);//
	read (fd,ptr_superblock,512);


	superblock0.nfree=superblock0.nfree-1;


	if(superblock0.free[superblock0.nfree]==0){// no free data blocks in SB and DB
		printf("no block availabe \n");
		return 0;

	}

	else if (superblock0.nfree==0) {

		int temp=superblock0.free[0];
		lseek(fd,512*superblock0.free[0],SEEK_SET);
		read(fd,&superblock0.nfree,2);	//
		read(fd,ptr_superblock->free,200);

		lseek(fd,512*1,SEEK_SET);
		write(fd,ptr_superblock,512);

		clear_Block(fd,temp);
		return temp;
	}
	else {

		lseek(fd,512*1,SEEK_SET);
		write(fd,ptr_superblock,512);
		clear_Block(fd,superblock0.free[superblock0.nfree]);
		return  (int)superblock0.free[superblock0.nfree];

	}



}


void clear_Block(int fd,int blocknum){

	lseek(fd,blocknum*512,SEEK_SET);
	unsigned char empty[512]={0};
	write(fd,empty,512);

}

//add mutex for inode in the future


void free_inode(int fd, int inodeNumber){
//set the inode valib bit =1;

	  //read the the nfree & free inode array into buffer
	  struct super_Block superblock0,*ptr_superblock;
	  ptr_superblock=&superblock0;
	  struct inode inode0,*ptr_inode;
	  ptr_inode=&inode0;
	  lseek(fd,1*512,SEEK_SET);
	  read (fd,ptr_superblock,512);

	  if(superblock0.ninode>=0 && superblock0.ninode<100)
	  {
		  //go the inode offset position

	  lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
	  read(fd,ptr_inode,32);
	  inode0.inodeFlagStruct.alloc=0;
	  lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
	  write(fd,ptr_inode,32);

	  superblock0.inode[superblock0.ninode]=inodeNumber;

	  superblock0.ninode=superblock0.ninode+1;	//update the Ninode
	  lseek(fd,1*512,SEEK_SET);
	  write(fd,ptr_superblock,512);
	  }

}


int scan_inodeList(int fd){

	struct super_Block superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;


	struct inode_List ilist0,*ptr_ilist;
	ptr_ilist=&ilist0;



	lseek(fd,1*512,SEEK_SET);
	read(fd,ptr_superblock,512);

	//get the isize to determin how many inodelist blocks to read


	int i,j;
	int allocatedInode =0;	//number of allocated inode during the scan


	for (i=0;i<superblock0.isize && allocatedInode<=100;i=i+1){
		lseek(fd,2*512+i*512,SEEK_SET);
		read(fd,ptr_ilist,512);
		for(j=0;j<16 && allocatedInode<=100;j=j+1){

			if(ilist0.inodeblock[j].inodeFlagStruct.alloc==0){ 	//if alloc is 0 and allocated inode is not greater than 100, continue
				free_inode(fd,i*16+j+1);
				allocatedInode=allocatedInode+1;
			}
		}

	}
	return allocatedInode;


}

int get_inode(int fd){

	struct super_Block superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;

	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;

	superblock0=SB_empty;
	lseek(fd,1*512,SEEK_SET);
	read(fd,ptr_superblock,512);
	//check the inodelist
	if (superblock0.ninode==0){	//if ninode is 0, read the i-list and place the number of all free inodes(up to 100) into the inode array
		if(scan_inodeList(fd) >0 ){

			get_inode(fd);
		}
		else {

			return 0;
		}
	}

	else if(superblock0.ninode>0){	// if the ninode is greater than 0, decrement it and return inode[ninode]

		superblock0.ninode=superblock0.ninode-1;
		//update the ninode;
		lseek(fd,1*512,SEEK_SET);
		write(fd,ptr_superblock,512);

		//update the allocated bit
		lseek(fd,512*2+(superblock0.inode[superblock0.ninode]-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);
		inode0.inodeFlagStruct.alloc=1;
		lseek(fd,512*2+(superblock0.inode[superblock0.ninode]-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);
		return superblock0.inode[superblock0.ninode];

	}
	return -1;

}


void initFileInode(int fd, int inodeFileNum){

	struct inode inode0, *ptr_inode;
	ptr_inode=&inode0;
	inode0=inode_empty;
	inode0.inodeFlagStruct.alloc=1;
	inode0.inodeFlagStruct.fileType=0;

	//add later
	lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
	write(fd,ptr_inode,32);


}




int init_inode(int fd, int inode){
	//

	struct inode inode0, *ptr_inode;
	ptr_inode=&inode0;
	inode0=inode_empty;
	lseek(fd,512*2+(inode-1)*32,SEEK_SET);
	int numberWrited=write(fd,ptr_inode,32);
	if (numberWrited==32){
		return 1;
	}
	else{
		return -1;
	}
}

//void init_iList(int fd,int numOfInode){
//	int i;
//	for (i=1;i<=numOfInode;i=i+1){
//		//initial inode from 1 to numOfInode
//		init_inode(fd,i);
//	}
//
//}

void clear_block(int fd, int blockNumber){

lseek(fd,512*blockNumber,SEEK_SET);
char empty_array[512]={0};
write(fd,empty_array,512);


}

void init_directory(int fd,int selfInode,int parentInode){


	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	inode0=inode_empty;

	int blockNumber=get_block(fd);
	inode0.address[0]=blockNumber;

	//get inode' address

	inode0.size1=512;
	inode0.modtime[0]=(unsigned short) time(NULL);
	inode0.gid=1;
	inode0.uid=1;
	inode0.inodeFlagStruct.alloc=1;
	inode0.inodeFlagStruct.exeOwner=1;
	inode0.inodeFlagStruct.fileType=2;
	inode0.inodeFlagStruct.readOwner=1;
	inode0.inodeFlagStruct.setGrp=1;
	inode0.inodeFlagStruct.setUid=1;
	inode0.inodeFlagStruct.wregrp=1;
	inode0.inodeFlagStruct.writeOwner=1;
	lseek(fd,2*512+(selfInode-1)*32,SEEK_SET);
	write(fd,ptr_inode,32);



	//manipulate the data blocks




	struct directory_Entries_Combination directory0,*ptr_dir;
	ptr_dir=&directory0;
	directory0=superDire_empty;

	strcpy(directory0.superDir[0].filename0,".");
	directory0.superDir[0].inode0=selfInode;	//itself

	strcpy(directory0.superDir[1].filename0,"..");
	directory0.superDir[1].inode0=parentInode;	//parent

	lseek(fd,512*blockNumber,SEEK_SET);
	write(fd,ptr_dir,512);


}

int sh_initfs(char **args)
{

  struct super_Block superblock0,*ptr_superblock;
  ptr_superblock=&superblock0;
  superblock0=SB_empty;

  g_fd=open(args[1],O_RDWR | O_CREAT,0666);


//check whether create a new disk or open previous one
  if(args[2]==NULL && args[3]==NULL){
	  //retrieve the necessary information number from disk file

	   lseek(g_fd,512,SEEK_SET);
	   read(g_fd,ptr_superblock,512);
	   g_currentDirectoryInode=1;
	   printf("Existing disk initialized! \n");
	   return 1;
  }

  int n1 = atoi(args[2]);
  int n2 = atoi(args[3]);

//init super block

lseek(g_fd,1*512,SEEK_SET);
write(g_fd,ptr_superblock,512);


	   //used variables
	 superblock0.isize = (unsigned short) n2/16; //number of blocks devoted to the inode-list
	 superblock0.fsize = (unsigned short) n1;//first block not used
	 superblock0.nfree = (unsigned short) 0; // initial to 0

//flock, ilock
	 superblock0.flock=0; //tbd
	 superblock0.ilock=0; //tbd
	 superblock0.fmod=0;
	 time_t tloc;
	 tloc=time(NULL);
	 superblock0.time[0]=(unsigned short) tloc;

	 lseek(g_fd,1*512,SEEK_SET);
	 write(g_fd,ptr_superblock,512);





//add blocks
	 int i;
  for (i=2+n2/16+1;i<n1-1;i=i+1){

	  add_block(g_fd,i);

 	   }

//remove blocks

//for (i=0;i<freeblocks-1;i=i+1){
//	get_block(fd);

//  }


  //  init_inode(fd, inodeDirInput);

  for (i=1;i<=n2;i=i+1){
	 // printf("now is %d \n",i);
	  //initialize the inode
	  init_inode(g_fd,n2);


  }

  //add inode into inodelist

  for (i=1;i<=n2;i=i+1){

	  free_inode(g_fd,i);

  }

// printf(" start to get inode \n");
//  for (i=1;i<=5;i=i+1){

//	  int nInode=get_inode(fd);

//  }




  //initial inode for root directory
    init_directory(g_fd,1,1);
    g_currentDirectoryInode=1;

   printf("New disk initialized! \n");

 return 1;
}





/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */



int sh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {   //if the execvp fails, return -1, otherwise no error
      perror("sh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("sh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int sh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < sh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return sh_launch(args);
}



/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *sh_read_line(void)
{
  int position = 0;
  int c;
  char *buffer = (char*)malloc(sizeof(char) * 1024);

  while (1) {
    // Read a character
    c = getchar();
    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;
  }
}



/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
#define SH_TOK_DELIM " \t\r\n\a"
char **sh_split_line(char *line)
{
  int bufsize = 64 , position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  token = strtok(line, SH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;
    token = strtok(NULL, SH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}



/**
   @brief Loop getting input and executing it.
 */
void sh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("Customized Unix V6 file system > ");
    line = sh_read_line();
    args = sh_split_line(line);
    status = sh_execute(args);

    free(line);
    free(args);
  } while (status);
}


/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  sh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}





