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


int sh_exit(char **);
int sh_initfs(char **);
int sh_cpin(char **);
int sh_cpout(char **);
int sh_mkdir(char **);


static int  currentPathInode=1;
char static fileType[4]="FCDB";
int static fd=-1;//file descripter for the disk


struct superblock {
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


struct inodeFlag{
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

struct inode{  //inode's length is 32 in total
	struct inodeFlag inodeFlagStruct;
	char nlinks;		//number of links for files
	char uid;
	char gid;
	unsigned char size0;
	unsigned short size1;
	unsigned short address[8];	//address of data blocks pointing to
	unsigned short acttime[2];	//time of last access
	unsigned short modtime[2];	//time of last modification
}inode_empty={{0},0,0,0,0,0,{0},{0},{0}};

struct ilist{

	struct inode inodeblock[16];

};


struct filesize{
	unsigned short size1;
	unsigned char size0;
	unsigned char mostsignificant;

}filesize_empty={0,0,0};



struct dirEntryLayout{

	unsigned short inode0;
	char	filename0[14];

}dirEntry_empt={0,{0}};

struct superDir{

	struct dirEntryLayout superDir[32];

}superDire_empty={0};

struct indirectBlock{

	unsigned short address[256];


}indirectBlock_empty={0};




/*
  List of builtin commands, followed by their corresponding functions.
 */

char *builtin_str[] = {  //pointer to command strings
  "q",
  "initfs",
  "cpin",
  "cpout",
  "v6mkdir"
};


int (*builtin_func[]) (char **) = {  //pointer to  function
  &sh_exit,
  &sh_initfs,
  &sh_cpin,
  &sh_cpout,
  &sh_mkdir
};

int sh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}


/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int sh_exit(char **args) //quit the shell
{
	close(fd);
  return 0;
}




int bufferCpy(int fd, int inodeFileNum,char buffer[],int size  ){

	lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	read (fd,ptr_inode,32);

	struct filesize filesize0,*ptr_filesize;
	ptr_filesize=&filesize0;

	struct indirectBlock indirectBlock0,*ptr_indirecrBlock,indirectBlock1,*ptr_indirecrBlock1;
	ptr_indirecrBlock=&indirectBlock0;
	ptr_indirecrBlock1=&indirectBlock1;
	indirectBlock0=indirectBlock_empty;

	struct superblock superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;

	lseek(fd,512,SEEK_SET);
	read(fd,ptr_superblock,512);




	unsigned int *ptr_fileSizeValue;
	ptr_fileSizeValue=(unsigned int *)ptr_filesize;
	filesize0.mostsignificant=inode0.inodeFlagStruct.unalloc;
	filesize0.size0=inode0.size0;
	filesize0.size1=inode0.size1;
	printf("filesize is %d \n",*ptr_fileSizeValue);


	//if the destfile is still small file
if ((*ptr_fileSizeValue/512)<8 )

		{  //file size belongs to small file

		//assign the datablock to inode
		int newFileDB=get_block(fd);

		//calculate the address field in Inode to fill
		int inodeAddrNum=*ptr_fileSizeValue/512;
		inode0.address[inodeAddrNum]=newFileDB;


		//copy the size of byte into dest datablock
		int position=lseek(fd,512*newFileDB,SEEK_SET);
		int numberWrited=write(fd,buffer,size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+size;
	//	printf("size value is %d\n",*ptr_fileSizeValue);

		//updated the fileszie

		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		inode0.inodeFlagStruct.fileType=0;

		position=lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		numberWrited=write(fd,ptr_inode,32);


		return *ptr_fileSizeValue;

	}

	else if((*ptr_fileSizeValue/512)==8)
	{

		unsigned short temp[8];
		int i;
		for (i=0;i<8;i=i+1){
			temp[i]=inode0.address[i];
			printf("inode addres %d is %d \n",i,inode0.address[i]);
				}

		//update inode address with 8 new indirect block





		for (i=0;i<8;i=i+1){
			printf("do i run here \n");
			inode0.address[i]=0;
			}




		//update the address in 1st indirect block

		for (i=0;i<8;i=i+1){
		indirectBlock0.address[i]=temp[i];
		}


		inode0.address[0]=get_block(fd);;
		indirectBlock0.address[8]=get_block(fd);
		lseek(fd,512*inode0.address[0],SEEK_SET);
		write(fd,ptr_indirecrBlock,512);



	//	printf("indirectBlock0.address[8] is %d \n",indirectBlock0.address[8]);

		//copy content
		lseek(fd,512*indirectBlock0.address[8],SEEK_SET);
		write(fd,buffer,size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+size;
	//	printf("size value is %d\n",*ptr_fileSizeValue);

		//update size
		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		inode0.inodeFlagStruct.largeFile=1;
		inode0.inodeFlagStruct.fileType=0;

		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);

		return *ptr_fileSizeValue;

	}


	else if((*ptr_fileSizeValue/512)>8 && (*ptr_fileSizeValue/(512*256))<7 ){


		int newBlock = sizetoblocklarge(fd, *ptr_fileSizeValue,inodeFileNum);
//		printf("new block is %d \n",newBlock);
		lseek(fd,512*newBlock,SEEK_SET);
		write(fd,buffer,size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+size;
	//	printf("size value is %d\n",*ptr_fileSizeValue);


		lseek(fd, 512*2+(inodeFileNum-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);

		//update size
		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		inode0.inodeFlagStruct.fileType=0;

		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);
		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);
		printf("inode address2 is %d \n",inode0.address[*ptr_fileSizeValue/(256*512)]);


		return *ptr_fileSizeValue;
		}


	else if(*ptr_fileSizeValue >= 7*256*512)//extra large file
		{

		//copy 512 byte to the data block

				int blocktowrite=sizetoblockhuge(fd,*ptr_fileSizeValue,inodeFileNum);
			//	printf("blk to write %d \n",blocktowrite);
				lseek(fd,blocktowrite*512,SEEK_SET);
				write(fd,buffer,size);
				*ptr_fileSizeValue=*ptr_fileSizeValue+size;
			//		printf("size value is %d\n",*ptr_fileSizeValue);

				//update size
						lseek(fd, 512*2+(inodeFileNum-1)*32,SEEK_SET);
						read(fd,ptr_inode,32);

						inode0.size1=filesize0.size1;
						inode0.size0=filesize0.size0;
						inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;

						lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
						write(fd,ptr_inode,32);
						return *ptr_fileSizeValue;



		}


}

int sizetoblocklarge(int fd, int filesize,int inodeNum){
	struct indirectBlock indirectBlock0,*ptr_indirecrBlock,indirectBlock1,*ptr_indirecrBlock1;
	ptr_indirecrBlock=&indirectBlock0;
	ptr_indirecrBlock1=&indirectBlock1;

	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;

	lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
	read(fd,ptr_inode,32);
	printf("before inode0 address is %d \n",inode0.address[0]);
	printf("before inode0 address is %d \n",inode0.address[1]);
	printf("before inode0 address is %d \n",inode0.address[2]);

	int blocknum=filesize/512;
	int Level1Num=blocknum/256;
	int Level2Num=blocknum%256;
	printf("leve1, level2 are %d %d \n",Level1Num,Level2Num);
	printf("leve1number %d address is %d \n",Level1Num,inode0.address[Level1Num]);


	lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
	read(fd,ptr_inode,32);

	if(inode0.address[Level1Num]==0){	//if inode0.address[Level1Num] no assigned with data blocks
		int blockid=get_block(fd);
		printf("large bloc id is %d ,level1num is % d\n",blockid,Level1Num);		//??
		inode0.address[Level1Num]=blockid;		//?
		printf("after assignment : leve1number %d address is %d \n",Level1Num,inode0.address[Level1Num]);
		lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);
		lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
		read(fd,ptr_inode,32);
		printf("inode address is %d \n",inode0.address[Level1Num]);
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
	printf("inode1 address is %d \n",inode0.address[Level1Num]);

	return indirectBlock0.address[Level2Num];


}




int sizetoblockhuge(int fd, int filesize,int inodeNum){ //convert huge file's databloc address when in address[7]

	struct indirectBlock indirectBlock0,*ptr_indirecrBlock,indirectBlock1,*ptr_indirecrBlock1;
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





int createFile(int fd, char filename[]){

	  //get inode for file
	  int fileInode=get_inode(fd);
	  printf("fd is %d, inode get in createfile is %d",fd,fileInode);

	  //initial inode field
	  initFileInode(fd,fileInode);

	  printf("the  currentPathInode is %d, inode number is %d\n",currentPathInode,fileInode);

	  //add the file into directory layout DB block
	  int entryAddress=fetch_directoryLayoutentry(currentPathInode,fd);
	  printf("the entryAddress is %d; currentPathInode is %d \n",entryAddress,currentPathInode);

	  if(entryAddress==-1)
	  	{
	  		printf("reach the maximum files number under the folder \n");
	  		return -1;
	  	}



	  	struct dirEntryLayout dirEntry0,*ptr_dirEntry;
	  	ptr_dirEntry=&dirEntry0;
	  	dirEntry0=dirEntry_empt;
	  	dirEntry0.inode0=fileInode;
	  	char test[14];
	  	strncpy(test,filename,14);


		printf("filename string is %s \n",filename);
		printf("filename pointer is %p \n",filename);
		printf("size of filename %d \n",sizeof(filename));
		printf("size of dir struct filename %d \n",sizeof(dirEntry0.filename0));

		strncpy(dirEntry0.filename0,test,14);

		//strcpy(dirEntry0.filename0,filename);

		//strcpy(dirtest,filename); ///why dirEntry0.filename0 doesn't work!!!!

	  	printf("dirEntry0 is %s\n",dirEntry0.filename0);
	  	lseek(fd,entryAddress,SEEK_SET);
	  	write(fd,ptr_dirEntry,16);
		printf("before return \n");
	  	return fileInode;

}



int sh_cpin(char **args)
{

	  if(fd==-1){
		  printf("please init the dist at first ! \n");
		  return 1;
	  }

		  char *sourceFilePath;
	  sourceFilePath=args[1];

	  char destinationFileName[14];
	  strcpy(destinationFileName,args[2]);


	  char buffer[512];

	  //open source file
	  //cp souce file's datablock content to destination and descend the count(source file size), as well as increase the destination file's size.
	  //when the source file's remaingint count is less than 512, handle the last data block properly
	  //

	  //open source file
	  int fdSource=open( args[1],O_RDWR | O_CREAT,0666);



	  //select the right dest file
	  int fdDest=fd;
	  //check whether the file is already there



	  //create the file in the directory's layout file.

	  int assignedInode=createFile(fdDest,destinationFileName);
	  printf("assinged inode of new file is %d \n",assignedInode);



	  //start to copy
	  int i=0;
	  int numberReaded=0;
	  do{
	  numberReaded=read (fdSource,buffer,512);
	  printf("copy the %d th block by reading %d byte\n",i,numberReaded);
	  printf("assinged inode of new file is %d \n",assignedInode);
	  int cpystatus=bufferCpy(fdDest,assignedInode,buffer,numberReaded); //size of copying equals to the byte read from source file
	  i=i+1;
	  }while(numberReaded>=512);
	//  printf("%d byte copied",cpystatus);

	  return 1;



}



int sh_cpout(char **args)
{

	  if(fd==-1){
		  printf("please init the dist at first ! \n");
		  return 1;
	  }
		  char sourceFileName[14];
		  strcpy(sourceFileName,args[1]);

		  int readout;
		  int fileSize;
		  char buffer[512];

		  //open source file
		  //cp souce file's datablock content to destination and descend the count(source file size), as well as increase the destination file's size.
		  //when the source file's remaingint count is less than 512, handle the last data block properly
		  //
		  struct inode inode0,*ptr_inode;
		  ptr_inode=&inode0;

		  struct superDir dirEntry0,*ptr_dirEntry;
		  ptr_dirEntry=&dirEntry0;
		  int inodeNum;
		  struct indirectBlock indirectBlock0,*ptr_indirecrBlock,indirectBlock1,*ptr_indirecrBlock1;
		  ptr_indirecrBlock=&indirectBlock0;
		  ptr_indirecrBlock1=&indirectBlock1;
		  indirectBlock0=indirectBlock_empty;
	      struct filesize filesize0,*ptr_filesize;
		  ptr_filesize=&filesize0;
		  unsigned int *ptr_fileSizeValue;
		  ptr_fileSizeValue=(unsigned int *)ptr_filesize;


		  //select the right dest file
		  int fdDest=open(args[2],O_RDWR | O_CREAT,0666);
		  //check whether the file is already there


		  //get inode, assume to be under the root dir.
		  lseek(fd,512*2+(1-1)*32,SEEK_SET);
		  printf("we are here\n");
		  read (fd,ptr_inode,32);
		  printf("we are here\n");
		  lseek(fd,512*inode0.address[0],SEEK_SET);
		  read(fd,ptr_dirEntry,512);
		  int i;

		  for (i=0;i<32;i=i+1){

			  printf("we have the file inode %d and name %s\n",dirEntry0.superDir[i].inode0,dirEntry0.superDir[i].filename0);
			  if(strcmp(dirEntry0.superDir[i].filename0,sourceFileName)==0){
			  inodeNum=dirEntry0.superDir[i].inode0;
			  printf("we get the file inode %d and name %s\n",dirEntry0.superDir[i].inode0,dirEntry0.superDir[i].filename0);

			  }
		  }

		  //check sourcefile's size;if it's extra largefile
			lseek(fd, 512*2+(inodeNum-1)*32,SEEK_SET);
		 	read(fd,ptr_inode,32);

			filesize0.mostsignificant=inode0.inodeFlagStruct.unalloc;
			filesize0.size0=inode0.size0;
			filesize0.size1=inode0.size1;
		    int remainfileSize=*ptr_fileSizeValue;

		  	printf("remainfile size is %d \n",remainfileSize);


	if(*ptr_fileSizeValue>512*256*7){

		//huge

		//copy the large part

		//for byte from 0-512*256*7

		int j;
		for (i=0;i<7;i=i+1){

			lseek(fd,512*inode0.address[i],SEEK_SET);
			read(fd,ptr_indirecrBlock,512);
			for(j=0;j<256;j=j+1)
				{
					lseek(fd,512*indirectBlock0.address[j],SEEK_SET);
					readout=read(fd,buffer,512);
					write(fdDest,buffer,readout);

				}

		}

		remainfileSize=*ptr_fileSizeValue-512*256*7;
	  	printf("remainfile size is %d \n",remainfileSize);

		//for byte greater thean 512*256*7
		lseek(fd,512*inode0.address[7],SEEK_SET);
		read(fd,ptr_indirecrBlock,512);

		for (i=0;i<256 && remainfileSize>0;i=i+1){

			lseek(fd,512*indirectBlock0.address[i],SEEK_SET);
			read(fd,ptr_indirecrBlock1,512);

			for (j=0;j<256 && remainfileSize>0;j=j+1){

				lseek(fd,indirectBlock1.address[j]*512,SEEK_SET);

				if(remainfileSize<512)
					readout=read(fd,buffer,remainfileSize);
				else
					readout=read(fd,buffer,512);
				printf("readout of  file is %d\n",readout);
				write(fdDest,buffer,readout);
				remainfileSize=remainfileSize-readout;

			}
		  	printf("remainfile size is %d \n",remainfileSize);

		}


}


	if(*ptr_fileSizeValue<=512*256*7 && *ptr_fileSizeValue > 512*8)	//large
	{
	int j;
		for (i=0;i<7 && remainfileSize>0;i=i+1){

			lseek(fd,512*inode0.address[i],SEEK_SET);

			printf("inode address %d \n",inode0.address[i]);

			read(fd,ptr_indirecrBlock,512);

			for(j=0;j<256 && remainfileSize>0;j=j+1)
			{
				lseek(fd,512*indirectBlock0.address[j],SEEK_SET);

				if(remainfileSize<512)
					readout=read(fd,buffer,remainfileSize);
				else
					readout=read(fd,buffer,512);

				printf("readout of  file is %d\n",readout);
				write(fdDest,buffer,readout);
				remainfileSize=remainfileSize-readout;

			}

		}


	}


if(*ptr_fileSizeValue<=512*8)
	{	//small
	int j;
		for (i=0;i<8 && remainfileSize>0 ;i=i+1){
			printf("remaining file is %d\n",remainfileSize);
			lseek(fd,inode0.address[i]*512,SEEK_SET);

			if(remainfileSize<512)
				readout=read(fd,buffer,remainfileSize);
			else
				readout=read(fd,buffer,512);

			printf("readout of  file is %d\n",readout);
			//printf("handle block %d \n",indirectBlock1.address[j]);
			write(fdDest,buffer,readout);

			remainfileSize=remainfileSize-readout;

		}


	}



}

int fetch_directoryLayoutentry(int inode,int fd){
	//get an available entry in directory layout datablock
		struct superDir dirEntry0,*ptr_dirEntry;
		ptr_dirEntry=&dirEntry0;
		struct inode inode0,*ptr_inode;
		ptr_inode=&inode0;

		printf("the current dir's inode is %d \n",inode);
		int seeked=lseek(fd,512*2+(inode-1)*32,SEEK_SET);
		printf("the seek value  %d \n",seeked);
		read(fd,ptr_inode,32);

		printf("datablock address %d \n",inode0.address[0]);
		printf("datablock address %d \n",inode0.address[1]);
		printf("datablock address %d \n",inode0.address[2]);
		printf("datablock address %d \n",inode0.address[3]);
		printf("datablock address %d \n",inode0.address[4]);
		printf("datablock address %d \n",inode0.address[5]);
		printf("datablock address %d \n",inode0.address[6]);
		printf("datablock address %d \n",inode0.address[7]);
		printf("get here\n");

		int i,j;
		for (j=0;j<8 && inode0.address[j]!=0;j=j+1){					//check all the address in inode

			printf("get here, j is %d , myinode is %d\n",j,inode0.address[j]);

			lseek(fd,512*inode0.address[j],SEEK_SET);
			read (fd,ptr_dirEntry,512);

			for(i=0;i<32;i=i+1){
				 printf("%d\n",dirEntry0.superDir[i].inode0);
				 printf("get here, i is %d\n",i);
				 if(dirEntry0.superDir[i].inode0==0){
					printf("get one offset %d \n", 512*inode0.address[j]+i*16);
					return 512*inode0.address[j]+i*16;
				 }

			}

		}

		//not enough

		return -1;
}





int sh_mkdir(char **args)
{
	if(fd==-1){
	  printf("please init the dist at first ! \n");
	  return 1;
	  }

	printf("currentpathinode %d fd %d \n",currentPathInode,fd);
	int entryAddress=fetch_directoryLayoutentry(currentPathInode,fd);
	if(entryAddress==-1)
	{
		printf("reach the maximum files number under the folder \n");
		return -1;
	}

	//get inode

	int dirInode=get_inode(fd);


	lseek(fd,entryAddress,SEEK_SET);

	struct dirEntryLayout dirEntry0,*ptr_dirEntry;
	ptr_dirEntry=&dirEntry0;

	dirEntry0.inode0=dirInode;
	strncpy(dirEntry0.filename0, args[1],14);			//just string copy 14 byte to directory name
	write(fd,ptr_dirEntry,16);

	init_directory(fd,dirInode,currentPathInode);
	//initial the datablock of this directory

	 return 1;

}





//caculate the isize

int nisize(int n){

	if (n%16 == 0){
		return n/16;
	}
	else{
		return n/16+1;
	}
}

int nfreeblk(int n1, int n2){

 int z ;
 z = nisize(n2);

 if ( n1-2-z >100 )
 {
	 return 100;
 }
 else
 {
	 return n1-2-z;
 }

 }


void add_block(int fd,int blockToFree){  //add the blocknumber into free block array, and return the free blocks in the array
//where does the blocknumber comes from when initializing
//they come from the total free blocks
	 struct superblock superblock0,*ptr_superblock;
	 ptr_superblock=&superblock0;


	//buffer to store the content in nfree and free[]
	//go to the nfree of the super block
	lseek(fd,512*1,SEEK_SET);//
	//read the nfree and free[] array fields
	read(fd,ptr_superblock,512);


	if ( (superblock0.nfree < 100) || (superblock0.nfree == 0)){ //if only superblock has list, free[0] =0
	//add the free blocks into free block list in super block

		superblock0.free[superblock0.nfree]=blockToFree;
		superblock0.nfree=superblock0.nfree+1;
		lseek(fd,512*1,SEEK_SET);
		write(fd,ptr_superblock,512);
		printf("block %d added \n",blockToFree);
	}
	else if (superblock0.nfree =100){
    //copy nfree and free array into block to free
	lseek(fd,512*blockToFree,SEEK_SET);


	int wordscopied=write(fd,&superblock0.nfree,2); //why ptr_superblock->nfree does not work!!!

	printf("1st: words copied is %d \n",wordscopied);
	printf("attention, nfree is %d\n",ptr_superblock->nfree);
	printf("attention, free block array head is %d\n",blockToFree);
	wordscopied=write(fd,ptr_superblock->free,200);
	printf("2nd: words copied is %d \n",wordscopied);

	superblock0.nfree=0;
	superblock0.free[superblock0.nfree]=blockToFree;
	superblock0.nfree=superblock0.nfree+1;

	lseek(fd,512*1,SEEK_SET);
	write(fd,ptr_superblock,512);

	printf("block %d added \n",blockToFree);

	}


}


int get_block(int fd){

	struct superblock superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;

	lseek(fd,512*1,SEEK_SET);//
	read (fd,ptr_superblock,512);

	printf("superblock nfree %d \n",superblock0.nfree);

	superblock0.nfree=superblock0.nfree-1;

	//printf("superblock nfree %d \n",superblock0.nfree);
	//printf("superblock free[nfree] %d \n",superblock0.free[superblock0.nfree]);
	//printf("stuck here3 \n");

	if(superblock0.free[superblock0.nfree]==0){// no free data blocks in SB and DB
		printf("no block availabe \n");

		return 0;

	}

	else if (superblock0.nfree==0) {

		int temp=superblock0.free[0];
		printf("temp is %d \n",temp);

		lseek(fd,512*superblock0.free[0],SEEK_SET);



		int wordsread=read(fd,&superblock0.nfree,2);	//
		printf("nfree read from file is %d\n",superblock0.nfree);

		printf("nfree %d words readed\n",wordsread);
		printf("ptr_superblock-> %d \n",ptr_superblock->nfree);
		wordsread=read(fd,ptr_superblock->free,200);
		printf("free array %d words readed\n",wordsread);


		lseek(fd,512*1,SEEK_SET);
		write(fd,ptr_superblock,512);
		printf("block %d removed \n",temp);
		printf("superblock nfree %d \n",superblock0.nfree);
		empty_block(fd,temp);
		return temp;
	}
	else {

		lseek(fd,512*1,SEEK_SET);
		write(fd,ptr_superblock,512);
		printf("block %d removed \n",superblock0.free[superblock0.nfree]);
		printf("superblock nfree %d \n",superblock0.nfree);
		empty_block(fd,superblock0.free[superblock0.nfree]);
		return  (int)superblock0.free[superblock0.nfree];

	}



}


void empty_block(int fd,int blocknum){

	lseek(fd,blocknum*512,SEEK_SET);
	unsigned char empty[512]={0};
	write(fd,empty,512);

}

//add mutex for inode in the future


int free_inode(int fd, int inodeNumber){
//set the inode valib bit =1;

	  //read the the nfree & free inode array into buffer
	  struct superblock superblock0,*ptr_superblock;
	  ptr_superblock=&superblock0;
	  struct inode inode0,*ptr_inode;
	  ptr_inode=&inode0;
	  lseek(fd,1*512,SEEK_SET);
	  read (fd,ptr_superblock,512);

	  if(superblock0.ninode>=100){
		  printf("now free inode %d \n",100);
		  return 100;
	  }
	  else if(superblock0.ninode<0){
		  return -1;
	  }
	  else if(superblock0.ninode>=0 && superblock0.ninode<100)
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

	  printf("now free inode %d \n",inodeNumber);

	  return superblock0.ninode;

	  }

}


int scan_inodeList(int fd){
	printf("scanning inodes \n");

	struct superblock superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;

	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;

	struct ilist ilist0,*ptr_ilist;
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
				printf("find free inode %d \n",i*16+j+1);
				free_inode(fd,i*16+j+1);
				allocatedInode=allocatedInode+1;
			}
		}

	}
	printf("scanned total %d inodes \n",allocatedInode);
	return allocatedInode;


}

int get_inode(int fd){

	struct superblock superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;

	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;

	superblock0=SB_empty;
	lseek(fd,1*512,SEEK_SET);
	read(fd,ptr_superblock,512);
	//check the inodelist
	if (superblock0.ninode==0){	//if ninode is 0, read the i-list and place the number of all free inodes(up to 100) into the inode array
		printf("ninode is 0! \n");

		if(scan_inodeList(fd) >0 ){

			get_inode(fd);
		}
		else {
			printf("******************no inode number\n ");

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

		printf("now allocated inode %d \n",superblock0.inode[superblock0.ninode]);
		return superblock0.inode[superblock0.ninode];

	}

}


void initFileInode(int fd, int inodeFileNum){


	//
	struct inodeFlag inodeFlag0,*ptr_inodeFlag;
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
int numberWrited=write(fd,empty_array,512);


}

void init_directory(int fd,int selfInode,int parentInode){


	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	inode0=inode_empty;


	printf("stop \n");
	int blockNumber=get_block(fd);
	printf("assigned block number is %d",blockNumber);
	inode0.address[0]=blockNumber;

	//get inode' address

	printf("address id %d",inode0.address[0]);
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




	struct superDir directory0,*ptr_dir;
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

  int n1 = atoi(args[2]);
  int n2 = atoi(args[3]);
  int freeblocks = n1-2-nisize(n2);		//total number of free datablocks in the volume
  //used variables


//check whether the following file path existed, if yes, then ask if overwrited,other wise creat a new disk
  //open the file as partition
  fd=open(args[1],O_RDWR | O_CREAT,0666);


//init super block
struct superblock superblock0,*ptr_superblock;
ptr_superblock=&superblock0;
superblock0=SB_empty;
lseek(fd,1*512,SEEK_SET);
int numberWrited=write(fd,ptr_superblock,512);


	 freeblocks = n1-2-nisize(n2);		//total number of free datablocks in the volume
	   //used variables
	 superblock0.isize = (unsigned short) nisize(n2); //number of blocks devoted to the inode-list
	 superblock0.fsize = (unsigned short) n1;//first block not used
	 superblock0.nfree = (unsigned short) 0; // initial to 0

//flock, ilock
	 superblock0.flock=0; //tbd
	 superblock0.ilock=0; //tbd
	 superblock0.fmod=0;
	 time_t tloc;
	 tloc=time(NULL);
	 superblock0.time[0]=(unsigned short) tloc;

	 lseek(fd,1*512,SEEK_SET);
	 numberWrited=write(fd,ptr_superblock,512);





//add blocks
	 int i;
  for (i=2+nisize(n2)+1;i<n1-1;i=i+1){

	  add_block(fd,i);

 	   }

//remove blocks

//for (i=0;i<freeblocks-1;i=i+1){
//	get_block(fd);

//  }




  printf(" inode part begin here! \n");

  //  init_inode(fd, inodeDirInput);

  for (i=1;i<=n2;i=i+1){
	 // printf("now is %d \n",i);
	  //initialize the inode
	  init_inode(fd,n2);


  }

  //add inode into inodelist
  printf(" inode add part begin here! \n");
  for (i=1;i<=n2;i=i+1){

	  free_inode(fd,i);

  }

// printf(" start to get inode \n");
//  for (i=1;i<=5;i=i+1){

//	  int nInode=get_inode(fd);

//  }




  printf(" initial directory part begin here! \n");
  //initial inode for root directory
    init_directory(fd,1,1);
    currentPathInode=1;

   printf("fd is %d \n",fd);

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
  char *token, **tokens_backup;

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
    printf("welcome > ");
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




