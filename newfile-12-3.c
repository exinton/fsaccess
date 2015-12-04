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

int static currentPathInode=1;
int static initblock[4]={0,0,0,0};
char static *selectedDisk;
char static fileType[4]="FCDB";


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



int static fd;//file descripter for the disk


/*
  List of builtin commands, followed by their corresponding functions.
 */

char *builtin_str[] = {  //pointer to command strings
  "q",
  "test",
  "initfs",
  "v6ls",
  "cpin",
  "cpout",
  "v6mkdir",
  "v6cd"
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
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int sh_exit(char **args) //quit the shell
{
  return 0;
}

int sh_test(char **args)
{


  selectedDisk=&("disktest");
  int fd=open(selectedDisk,O_RDWR | O_CREAT,0666);


  int i;
  for (i=0;i<900;i=i+1){
	  printf("get block is %d\n",get_block(fd));
  }

  return 1;




}



int checkinodeaddress(){  //return a datablock for inode

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


	unsigned int *ptr_fileSizeValue;
	ptr_fileSizeValue=(unsigned int *)ptr_filesize;

	filesize0.mostsignificant=inode0.inodeFlagStruct.unalloc;
	filesize0.size0=inode0.size0;
	filesize0.size1=inode0.size1;


	printf("%d,%d,%d\n",inode0.inodeFlagStruct.unalloc,inode0.size0,inode0.size1);
	printf("%d,%d,%d\n",filesize0.mostsignificant,filesize0.size0,filesize0.size1);
	printf("filesize %d \n",*ptr_fileSizeValue);



	//check the dirlayout
		lseek(fd,512*2,SEEK_SET);
		read(fd,ptr_inode,32);
		printf("address o is %d \n",inode0.address[0]);

	//if the destfile is still small file
	if ((*ptr_fileSizeValue/512)<8 )
	{  //file size belongs to small file

		//assign the datablock to inode
		int newFileDB=get_block(fd);

		//calculate the address field in Inode to fill
		int inodeAddrNum=inode0.size1/512;
		inode0.address[inodeAddrNum]=newFileDB;


		//copy the size of byte into dest datablock
		int position=lseek(fd,512*newFileDB,SEEK_SET);
		int numberWrited=write(fd,buffer,size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+size;
		printf("size value is %d\n",*ptr_fileSizeValue);

		//updated the fileszie

		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		inode0.inodeFlagStruct.fileType=0;

		position=lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		numberWrited=write(fd,ptr_inode,32);


		//verify
		printf("*verify the size* \n");
		position=lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		int numberReaded =read (fd,ptr_inode,32);
		printf("size0 is %d \n",(unsigned)inode0.size0);
		printf("size1 is %d \n",inode0.size1);
		printf("most significant bit is %d \n",inode0.inodeFlagStruct.unalloc);

		return *ptr_fileSizeValue;

	}

	else if((*ptr_fileSizeValue/512)>=8)
	{

		//get the existing data blocks in address into buffer
		printf("********************************this is 8****************************** \n");
		//get 8 additional as address in inode as indirect block
		unsigned short temp[8];
		int i;
		for (i=0;i<8;i=i+1){
			temp[i]=inode0.address[i];
			printf("inode addres %d is %d \n",i,inode0.address[i]);
				}

		//update inode address with 8 new indirect block



		inode0.address[0]=(unsigned short)get_block(fd);
		printf("inode addres is %d \n",inode0.address[i]);
		lseek(fd,512*inode0.address[0],SEEK_SET);
		write(fd,ptr_indirecrBlock,512);  //initial indirect datablocks



		printf("inode is %d\n",inodeFileNum);
		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);		//update into inode



		//update the address in 1st indirect block

		for (i=0;i<8;i=i+1){
		indirectBlock0.address[i]=temp[i];
		printf("indirectBlock0.address[%d] is %d \n",i,indirectBlock0.address[i]);
		}



		indirectBlock0.address[8]=get_block(fd);
		lseek(fd,512*inode0.address[0],SEEK_SET);
		write(fd,ptr_indirecrBlock,512);




		printf("indirectBlock0.address[8] is %d \n",indirectBlock0.address[8]);

		//copy content
		lseek(fd,512*indirectBlock0.address[8],SEEK_SET);
		write(fd,buffer,size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+size;
		printf("size value is %d\n",*ptr_fileSizeValue);

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
		printf("new block is %d \n",newBlock);
		lseek(fd,512*newBlock,SEEK_SET);
		write(fd,buffer,size);
		*ptr_fileSizeValue=*ptr_fileSizeValue+size;
		printf("size value is %d\n",*ptr_fileSizeValue);


		//update size
		inode0.size1=filesize0.size1;
		inode0.size0=filesize0.size0;
		inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
		inode0.inodeFlagStruct.fileType=0;

		lseek(fd,512*2+(inodeFileNum-1)*32,SEEK_SET);
		write(fd,ptr_inode,32);

		return *ptr_fileSizeValue;


		}

	else if(*ptr_fileSizeValue>=7*256*512)//extra large file
		{

		//copy 512 byte to the data block

				int blocktowrite=sizetoblockhuge(fd,*ptr_fileSizeValue,inode0.address[7]);
				printf("blk to write %d \n",blocktowrite);
				lseek(fd,blocktowrite*512,SEEK_SET);
				write(fd,buffer,size);
				*ptr_fileSizeValue=*ptr_fileSizeValue+size;
					printf("size value is %d\n",*ptr_fileSizeValue);

				//update size
						inode0.size1=filesize0.size1;
						inode0.size0=filesize0.size0;
						inode0.inodeFlagStruct.unalloc=filesize0.mostsignificant;
						inode0.inodeFlagStruct.fileType=0;

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
	read (fd,ptr_inode,32);

	int blocknum=filesize/512;
	int Level1Num=blocknum/256;
	int Level2Num=blocknum%256;

	lseek(fd,512*2+(inodeNum-1)*32,SEEK_SET);
	read(fd,ptr_inode,32);

	if(inode0.address[Level1Num]==0){
		inode0.address[Level1Num]=get_block(fd);
	}

	lseek(fd,512*inode0.address[blocknum/256],SEEK_SET);
	read(fd,ptr_indirecrBlock,512);	//read in struct of 1st level indirect block

	if(indirectBlock0.address[Level2Num]==0){
		indirectBlock0.address[Level2Num]=get_block(fd);
	}

	return indirectBlock0.address[Level2Num];


}




int sizetoblockhuge(int fd, int filesize,int Level1blk){ //convert huge file's databloc address when in address[7]

	struct indirectBlock indirectBlock0,*ptr_indirecrBlock,indirectBlock1,*ptr_indirecrBlock1;
	ptr_indirecrBlock=&indirectBlock0;
	ptr_indirecrBlock1=&indirectBlock1;
	indirectBlock0=indirectBlock_empty;

	int effectiveSize=filesize-256*512;
	int blocknum=effectiveSize/512;
	int Level1Num=blocknum/256;
	int Level2Num=blocknum%256;

	lseek(fd,512*Level1blk,SEEK_SET);
	read(fd,ptr_indirecrBlock,512);	//read in struct of 1st level indirect block
	if(indirectBlock0.address[Level1Num]==0){
		indirectBlock0.address[Level1Num]=get_block(fd);
	}
	lseek(fd,512*indirectBlock0.address[Level1Num],SEEK_SET);
	read(fd,ptr_indirecrBlock1,512);
	if(indirectBlock1.address[Level2Num]==0){
		indirectBlock1.address[Level2Num]=get_block(fd);
	}
	lseek(fd,512*indirectBlock0.address[Level1Num]+indirectBlock1.address[Level2Num],SEEK_SET);
	unsigned short addressBuffer[1];
	read(fd,addressBuffer,1);
	if(addressBuffer[0]!=0){		//if there is datablock, return it
		return addressBuffer[0];
	}
	int blockseq=get_block(fd);		//if there is not address, get data block
	write(fd,&blocknum,2);
	return blockseq;


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

	 //trouble shooting print out
	  printf("fsaccess\n");
	  printf( "You entered: %s %s %s \n ", args[0],args[1], args[2]);



	  printf("size of args[2] %d\n",sizeof(args[2]));
	  printf("size of args[0] %d\n",sizeof(args[1]));
	  printf("size of args[0] %d\n",sizeof(args[0]));
	  char *sourceFilePath;
	  sourceFilePath=args[1];

	  char destinationFileName[14];
	  strcpy(destinationFileName,args[2]);

	  printf("destinationFileName  is %s, size is %d \n",destinationFileName,sizeof(destinationFileName));

	  char buffer[512];

	  //open source file
	  //cp souce file's datablock content to destination and descend the count(source file size), as well as increase the destination file's size.
	  //when the source file's remaingint count is less than 512, handle the last data block properly
	  //

	  //open source file
	  int fdSource=open( args[1],O_RDWR | O_CREAT,0666);



	  //select the right dest file
	  selectedDisk=&("disktest");
	  int fdDest=open(selectedDisk,O_RDWR | O_CREAT,0666);

	  //check whether the file is already there



	  //create the file in the directory's layout file.

	  int assignedInode=createFile(fdDest,destinationFileName);
	  printf("assinged inode of new file is %d \n",assignedInode);



	  //start to copy
	  int i=0;
	  while(i<1792){
	  int numberReaded=read (fdSource,buffer,512);
	  printf("copy the %d th block\n",i);
	  printf("assinged inode of new file is %d \n",assignedInode);
	  int cpystatus=bufferCpy(fdDest,assignedInode,buffer,numberReaded); //size of copying equals to the byte read from source file
	  i=i+1;
	  }
	//  printf("%d byte copied",cpystatus);

	  return 1;



}

int sh_cpout(char **args)
{

	//get to the address and find datablock
	struct inode MyInode,*ptr_inode;
	ptr_inode=&MyInode;
	read (fd,ptr_inode,202);

	//check whether the inode is directory
	if(MyInode.inodeFlagStruct.fileType!=2){
		printf("target is not directory");
		return -1;

	}

	 return 1;

}

int fetch_directoryLayoutentry(int inode,int fd){
	//get an available entry in directory layout datablock
		struct superDir dirEntry0,*ptr_dirEntry;
		ptr_dirEntry=&dirEntry0;
		struct inode MyInode,*ptr_inode;
				ptr_inode=&MyInode;

		lseek(fd,512*2+(inode-1)*32,SEEK_SET);
		read (fd,ptr_inode,32);

		printf("datablock address %d \n",MyInode.address[0]);
		printf("datablock address %d \n",MyInode.address[1]);
		printf("datablock address %d \n",MyInode.address[2]);
		printf("datablock address %d \n",MyInode.address[3]);
		printf("datablock address %d \n",MyInode.address[4]);
		printf("datablock address %d \n",MyInode.address[5]);
		printf("datablock address %d \n",MyInode.address[6]);
		printf("datablock address %d \n",MyInode.address[7]);
		printf("get here\n");
		int i,j;
		for (j=0;j<8 && MyInode.address[j]!=0;j=j+1){					//check all the address in inode
			lseek(fd,512*MyInode.address[j],SEEK_SET);
			printf("get here, j is %d , myinode is %d\n",j,MyInode.address[j]);
			read (fd,ptr_dirEntry,512);

			for(i=0;i<32;i=i+1){
				 printf("%d\n",dirEntry0.superDir[i].inode0);
				 printf("get here, i is %d\n",i);
				 if(dirEntry0.superDir[i].inode0==0){
					printf("get one offset %d \n", 512*MyInode.address[j]+i*16);
					return 512*MyInode.address[j]+i*16;
				 }

			}

		}

		//not enough

		return -1;
}


int sh_cd( char **args)
{
	selectedDisk=&("disktest");
	int fd=open(selectedDisk,O_RDWR | O_CREAT,0666);

	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	struct superDir superDir0,*ptr_superDir;
	ptr_superDir=&superDir0;

	lseek(fd,512*2+(currentPathInode-1)*32,SEEK_SET);
	read (fd,ptr_inode,32);

	//loop
	int i,j;
	for (i=0;i<8;i=i+1){

	lseek(fd,512*inode0.address[i],SEEK_SET);
	read(fd,ptr_superDir,512);

		for(j=0;j<32;j=j+1){
			if (strcmp(superDir0.superDir[j].filename0,args[1])==0)
			{
				//check whether it's directory
				lseek(fd,512*2+(superDir0.superDir[j].inode0-1)*32,SEEK_SET);
				read(fd,ptr_inode,32);
				if(inode0.inodeFlagStruct.fileType==2){
					currentPathInode=superDir0.superDir[j].inode0;
					printf("Change directory to %s \n",args[1]);
					return 1;
				}

			}

			}

printf("cannot find the directory name %s\n",args[1]);
return -1;//

	}
}



int sh_mkdir(char **args)
{
	selectedDisk=&("disktest");

	int fd=open(selectedDisk,O_RDWR | O_CREAT,0666);
	 //trouble shooting print out
	printf("fsaccess\n");
	printf( "You entered: %s %s \n ", args[0],args[1]);
	//char *newDirectory = args[1];
	//get the file descriptor



	//CHECK THE PERMIT

	//check same directory name

	//check the length of the directory no more than 15 chars.

	printf("currentpathinode %d fd %d \n",currentPathInode,fd);
	int entryAddress=fetch_directoryLayoutentry(currentPathInode,fd);
	if(entryAddress==-1)
	{
		printf("reach the maximum files number under the folder \n");
		return -1;
	}

	//get inode

	int dirInode=get_inode(fd);

	printf("dir inode we get is %d\n",dirInode);

	lseek(fd,entryAddress,SEEK_SET);

	struct dirEntryLayout dirEntry0,*ptr_dirEntry;
	ptr_dirEntry=&dirEntry0;
	dirEntry0.inode0=dirInode;
	strncpy(dirEntry0.filename0, args[1],14);			//just string copy 14 byte to directory name
	int numberWrited=write(fd,ptr_dirEntry,16);

	init_directory(fd,dirInode,currentPathInode);
	//initial the datablock of this directory

	 return 1;

}


//list the directory layout of current path
int sh_ls(char **args)
{

	  //open the file as partition
	selectedDisk=&("disktest");

	int fd=open(selectedDisk,O_RDWR | O_CREAT,0666);
	printf("fd is %d,currentpathinode is %d\n",fd,currentPathInode);

	//go to the nfree of the current path inode
	int position=lseek(fd,512*2+(currentPathInode-1)*32,SEEK_SET);//
	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	read (fd,ptr_inode,32);

	//check the addresses of the inode

	int i;
	for (i=0;i<8;i=i+1)
	{
		printf("inode address %d is %d \n",i,inode0.address[i]);
		//printf("Dir DB %d addresses are %d \n",i,inode0.address[i]);
		if(inode0.address[i]!=0)
		{
		printf("xxx inside ls loop \n");
		printDirLayout(fd,inode0.address[i]);


		}
	}

	close(fd);
return 1;

}

void printDirLayout(int fd, int datablockofDir)
{

struct inode inode0,*ptr_inode;
ptr_inode=&inode0;

int position=lseek(fd,512*datablockofDir,SEEK_SET);//

struct superDir directory0,*ptr_directory;
ptr_directory=&directory0;

struct filesize filesize0,*ptr_filesize;
ptr_filesize=&filesize0;

unsigned int *ptr_fileSizeValue;
ptr_fileSizeValue=(unsigned int *)ptr_filesize;



position=lseek(fd,512*datablockofDir,SEEK_SET);//
int numberReaded =read (fd,ptr_directory,512);




int i;
for (i=0;i<32;i=i+1){

	if(directory0.superDir[i].inode0 != 0)
	{

		lseek(fd,512*2+(directory0.superDir[i].inode0-1)*32,SEEK_SET);//
		read (fd,ptr_inode,32);
		filesize0.mostsignificant=inode0.inodeFlagStruct.unalloc;
		filesize0.size0=inode0.size0;
		filesize0.size1=inode0.size1;

		printf("check point here in dir!\n");
		printf("inode: %d, filename: %s filetype: %c filesize:%d Byte\n",directory0.superDir[i].inode0,directory0.superDir[i].filename0,fileType[inode0.inodeFlagStruct.fileType],*ptr_fileSizeValue);
	}


}
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

	//printf("superblock nfree %d \n",superblock0.nfree);
	//printf("superblock free[nfree] %d \n",superblock0.free[superblock0.nfree]);

	superblock0.nfree=superblock0.nfree-1;

	//printf("superblock nfree %d \n",superblock0.nfree);
//	printf("superblock free[nfree] %d \n",superblock0.free[superblock0.nfree]);
	//printf("stuck here3 \n");

	if(superblock0.free[superblock0.nfree]==0){// no free data blocks in SB and DB
		printf("no block availabe \n");
		sleep(10);
		return 0;

	}

	else if (superblock0.nfree==0) {
//		printf("stuck here2 \n");
		int temp=superblock0.free[0];
//		printf("temp is %d \n",temp);
		lseek(fd,512*superblock0.free[0],SEEK_SET);

		int wordsread=read(fd,&superblock0.nfree,2);	//

//		printf("nfree %d words readed\n",wordsread);
//		printf("ptr_superblock-> %d \n",ptr_superblock->nfree);
		wordsread=read(fd,ptr_superblock->free,200);
//		printf("free array %d words readed\n",wordsread);

		lseek(fd,512*1,SEEK_SET);
		write(fd,ptr_superblock,512);
		printf("block %d removed \n",temp);
		empty_block(temp);
		return temp;
	}
	else {

		lseek(fd,512*1,SEEK_SET);
		write(fd,ptr_superblock,512);
		printf("block %d removed \n",superblock0.free[superblock0.nfree]);
		empty_block(fd,superblock0.free[superblock0.nfree]);
		return  superblock0.free[superblock0.nfree];

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

	  if (inodeNumber <2 ){

		  printf("can not free inode less than 2! or larger than defined\n");
		  return -1;

		  }



	  //read the the nfree & free inode array into buffer
	  struct superblock superblock0,*ptr_superblock;
	  ptr_superblock=&superblock0;
	  lseek(fd,1*512,SEEK_SET);
	  read (fd,ptr_superblock,512);

	  if(superblock0.ninode>=100){

		  return 100;
	  }
	  else if(superblock0.ninode<0){
		  return -1;
	  }
	  else if(superblock0.ninode>=0 && superblock0.ninode<100)
	  {
		  //go the inode offset position

	  lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
	  struct inode inode0,*ptr_inode;
	  ptr_inode=&inode0;
	  read(fd,ptr_inode,32);
	  inode0.inodeFlagStruct.alloc=0;
	  lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
	  write(fd,ptr_inode,32);

	  superblock0.inode[superblock0.ninode]=inodeNumber;

	  superblock0.ninode=superblock0.ninode+1;	//update the Ninode
	  lseek(fd,1*512,SEEK_SET);
	  write(fd,ptr_superblock,512);

	  return superblock0.ninode;

	  }

}


int scan_inodeList(int fd){


	struct superblock superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;
	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;

	lseek(fd,1*512,SEEK_SET);
	read(fd,ptr_superblock,512);

	//get the isize to determin how many inodelist blocks to read

	lseek(fd,2*512,SEEK_SET);
	int i=0;
	int allocatedInode =0;	//number of allocated inode during the scan

	for (i=1;i<=superblock0.isize*16;i=i+1){
		read(fd,ptr_inode,32);
		if(inode0.inodeFlagStruct.alloc==0 && allocatedInode<=100){ 	//if alloc is 0 and allocated inode is not greater than 100, continue

			allocatedInode=allocatedInode+1;
			superblock0.inode[allocatedInode]=i;

		}

		//write back the free inode array and ninode to superblock
		lseek(fd,1*512,SEEK_SET);

		superblock0.ninode=allocatedInode;
		write(fd,ptr_superblock,512);

	}
	return allocatedInode;


}

int get_inode(int fd){

	struct superblock superblock0,*ptr_superblock;
	ptr_superblock=&superblock0;
	superblock0=SB_empty;
	lseek(fd,1*512,SEEK_SET);
	read(fd,ptr_superblock,512);
	//check the inodelist
	if (superblock0.ninode==0){	//if ninode is 0, read the i-list and place the number of all free inodes(up to 100) into the inode array

		if(scan_inodeList(fd) >0 ){

			get_inode(fd);
		}
		else {
			printf("no inode number\n ");

			return 0;
		}
	}

	else if(superblock0.ninode>0){	// if the ninode is greater than 0, decrement it and return inode[ninode]

		superblock0.ninode=superblock0.ninode-1;
		//update the ninode;
		lseek(fd,1*512,SEEK_SET);
		write(fd,ptr_superblock,512);

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



	int blockNumber=get_block(fd);
	printf("assigned block number is %d",blockNumber);
	inode0.address[0]=blockNumber;

	//get inode' address
	lseek(fd,2*512+(selfInode-1)*32,SEEK_SET);
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

	int numberWrited=write(fd,ptr_inode,32);



	//manipulate the data blocks


	lseek(fd,512*blockNumber,SEEK_SET);

	struct superDir directory0,*ptr_dir;
	ptr_dir=&directory0;
	directory0=superDire_empty;

	strcpy(directory0.superDir[0].filename0,".");
	directory0.superDir[0].inode0=selfInode;	//itself

	strcpy(directory0.superDir[1].filename0,"..");
	directory0.superDir[1].inode0=parentInode;	//parent


	numberWrited=write(fd,ptr_dir,512);


}





void init_directoryDataBlock(int df,int *dirDBArray){

}



void init_superBlock(int fd,int n1,int n2){



}



int sh_initfs(char **args)
{
	//check wheather


   //trouble shooting print out
  printf("fsaccess\n");
  printf( "You entered: %s %s %s %s \n ", args[0],args[1], args[2],args[3]);
  int n1 = atoi(args[2]);
  int n2 = atoi(args[3]);
  int freeblocks = n1-2-nisize(n2);		//total number of free datablocks in the volume
  //used variables


//check whether the following file path existed, if yes, then ask if overwrited,other wise creat a new disk
  //open the file as partition
int fd=open(args[1],O_RDWR | O_CREAT,0666);


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
  for (i=0;i<freeblocks;i=i+1){
 	   	//printf("add block %d \n",i);
	  add_block(fd,i+2+nisize(n2));
	//  printf("add block %d\n",i+2+nisize(n2));


 	   }

//remove blocks

//for (i=0;i<freeblocks-1;i=i+1){
   	//printf("add block %d \n",i);
//	get_block(fd);
//  printf("add block %d\n",i+2+nisize(n2));


  // }




  printf(" inode part begin here! \n");

  //  init_inode(fd, inodeDirInput);

  for (i=1;i<=n2;i=i+1){

	  //initialize the inode
	  init_inode(fd,n2);


  }
  //add inode into inodelist

  for (i=2;i<=120;i=i+1){

	  int nInode=free_inode(fd,i);



  }



  //initial inode for root directory
    init_directory(fd,1,1);
    currentPathInode=1;


  close(fd);

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




