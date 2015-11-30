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

int static currentPathInode=1;
int static initblock[4]={0,0,0,0};
char static *selectedDisk;

struct superblock {
	unsigned short isize[1];
	unsigned short fsize[1];
	unsigned short nfree[1];
	unsigned short free[100][1];
	unsigned short ninode[1];
	unsigned short inode[100][1];
	char flock[1];
	char ilock[1];
	char fmod[1];
	unsigned short time[2];
	};


struct inodeFlag{
	unsigned alloc:1;
	unsigned fileType:2;
	unsigned largeFile:1;
	unsigned setUid:1;
	unsigned setGrp:1;
	unsigned unalloc:1;
	unsigned readOwner:1;
	unsigned writeOwner:1;
	unsigned exeOwner:1;
	unsigned wregrp:3;
	unsigned wreothers:3;
}park;

struct inode{  //inode's length is 32 in total
	struct inodeFlag inodeFlagStruct;
	char nlinks[1];		//number of links for files
	char uid[1];
	char gid[1];
	char size0[1];
	unsigned short size1;
	unsigned short address[8];	//address of data blocks pointing to
	unsigned short acttime[2];	//time of last access
	unsigned short modtime[2];	//time of last modification
}inode_empty={{0,0,0,0,0,0,0,0,0,0,0},0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0},{0,0}};

typedef struct Inode MyInode;



struct dirEntryLayout{

	unsigned short inode0;
	char	filename0[14];

};


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
  "v6mkdir"
};


int (*builtin_func[]) (char **) = {  //pointer to  function
  &sh_exit,
  &sh_test,
  &sh_initfs,
  &sh_ls,
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
  return 0;
}

int sh_test(char **args)
{
  printf(">>>>>>>>>>>>>>>>>>>>>> \n");
  return 1;

}


int sh_cpin(char **args)
{

	 //trouble shooting print out
	  printf("fsaccess\n");
	  printf( "You entered: %s %s %s %s \n ", args[0],args[1], args[2]);
	  char *sourceFilePath = args[1];
	  char *destinationFilePath = args[2];

	  //open source file
	  //cp souce file's datablock content to destination and descend the count(source file size), as well as increase the destination file's size.
	  //when the source file's remaingint count is less than 512, handle the last data block properly
	  //


	  return 1;






}

int sh_cpout(char **args)
{

	//get to the address and find datablock
	struct inode MyInode,*ptr_inode;
	ptr_inode=&MyInode;
	int numberReaded =read (fd,ptr_inode,202);

	//check whether the inode is directory
	if(MyInode.inodeFlagStruct.fileType!=2){
		printf("target is not directory");
		return -1;

	}

	 return 1;

}

int fetch_directoryLayoutentry(int inode,int fd){

	int position=lseek(fd,512*2+(inode-1)*32,SEEK_SET);

		//get to the address and find datablock
		struct inode MyInode,*ptr_inode;
		ptr_inode=&MyInode;
		int numberReaded =read (fd,ptr_inode,32);

		printf("datablock address %d\n",MyInode.address[0]);

		position=lseek(fd,512*MyInode.address[0],SEEK_SET);
		int i;
		struct dirEntryLayout dirEntry0,*ptr_dirEntry;
			ptr_dirEntry=&dirEntry0;

		for (i=0;i<512;i=i+16){
			int numberReaded =read (fd,ptr_dirEntry,16);
			printf("file inode %d, file name %s \n",dirEntry0.inode0,dirEntry0.filename0);
			if(dirEntry0.inode0==0){
					return 512*MyInode.address[0]+i;
			}

		}

		return -1;
}





int sh_mkdir(char **args)
{
	selectedDisk=&("disktest");
	printf("hanging here");
	int fd=open(selectedDisk,O_RDWR | O_CREAT,0666);
	 //trouble shooting print out
	printf("fsaccess\n");
	printf( "You entered: %s %s %s %s \n ", args[0],args[1], args[2]);
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

	int dirInode=get_block(fd);

	int position=lseek(fd,entryAddress,SEEK_SET);
	struct dirEntryLayout dirEntry0,*ptr_dirEntry;
	ptr_dirEntry=&dirEntry0;
	dirEntry0.inode0=dirInode;
	strcpy(dirEntry0.filename0, args[1]);
	int numberWrited=write(fd,ptr_dirEntry,16);


	//get to the address and find datablock



	//if datablock has no empty entry, then search next availabe datablock

	//get an empty entry's address in the databloc?lseek, if no ,return 0;

	//get a free datablock


	//if all datablock is filled, return error;
	//initial the new directory

	 return 1;

}


//list the directory layout of current path
int sh_ls(char **args)
{

	  //open the file as partition
	selectedDisk=&("disktest");
	printf("hanging here");
	int fd=open(selectedDisk,O_RDWR | O_CREAT,0666);
	printf("fd is %d,selectedDisk is %c \n",fd,*selectedDisk);
	//go to the nfree of the current path inode
	int position=lseek(fd,512*2,SEEK_SET);//
	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	int numberReaded =read (fd,ptr_inode,32);

	//check the addresses of the inode

	int i;
	for (i=0;i<8;i=i+1)
	{
		printf("Dir DB addresses are %d \n",inode0.address[i]);
	if(inode0.address[i]!=0)
		{

		printDirLayout(fd,inode0.address[i]);


		}
	}

	close(fd);
return 1;

}

void printDirLayout(int fd, int datablockofDir)
{

struct dirEntryLayout directory0,*ptr_directory;
ptr_directory=&directory0;
int position=lseek(fd,512*datablockofDir,SEEK_SET);//

int i;
for (i=0;i<32;i=i+1){
	int numberReaded =read (fd,ptr_directory,16);
	if(directory0.inode0!=0)
	{
		printf("inode number: %d, file name %s\n",directory0.inode0,directory0.filename0);
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



void add_block(int fd,int blockToCheck, int offset, int blockToFree){  //add the blocknumber into free block array, and return the free blocks in the array
//where does the blocknumber comes from when initializing
//they come from the total free blocks

	//buffer to store the content in nfree and free[]
	unsigned short buffer[101];
	//go to the nfree of the super block
	int position=lseek(fd,512*blockToCheck+offset,SEEK_SET);//
	//read the nfree and free[] array fields
	int numberReaded =read (fd,buffer,202);


	if ( (buffer[0] < 100) && (buffer[1] == 0)){ //if only superblock has list, free[0] =0
	//add the free blocks into free block list in super block
		buffer[0]=buffer[0]+1;
		buffer[buffer[0]+1]=blockToFree;
		lseek(fd,512*blockToCheck+offset,SEEK_SET);

		int numberWrited=write(fd,buffer,202);

	}

	else if ((buffer[0] =100) && (buffer[1] == 0)){
    //fill the free[0] with the blockToFree's number as the head of next chain
		buffer[1]=blockToFree;
		lseek(fd,512*blockToCheck+offset,SEEK_SET);
		int numberWrited=write(fd,buffer,202);
		lseek(fd,512*blockToFree,SEEK_SET);
		numberWrited=write(fd,initblock,4);
		printf("case 2 buffer[0], buffer[1] is %d %d !\n",buffer[0],buffer[1]);
		sleep(1);
	}

	else if (buffer[1] != 0){

		add_block(fd,buffer[1],0,blockToFree);

	}

}


int get_block(int fd){
	int blockToCheck=1;
	int offset =4;

	unsigned short buffer[101];
		//go to the nfree of the super block

	int position=lseek(fd,512*blockToCheck+offset,SEEK_SET);//
		//read the nfree and free[] array fields
	int numberReaded =read (fd,buffer,202);

	if (buffer[0]==0 && buffer[1]==0){ // no free data blocks in SB and DB

		return 0;
	}
	else if (buffer[0]>0) {
		printf("case 2:before get buffer[0], buffer[1] is %d %d !\n",buffer[0],buffer[1]);
		buffer[0]=buffer[0]-1;
		printf("case 2: after get buffer[0], buffer[1] is %d %d !\n",buffer[0],buffer[1]);
		printf("case 2: return value  %d !\n",buffer[buffer[0]-1]);
		int position=lseek(fd,512*blockToCheck+offset,SEEK_SET);//
		int numberWrited=write(fd,buffer,202);


		return buffer[buffer[0]-1];
	}
	else if (buffer[0]==0 && buffer[1]!=0){
		printf(" case 3: before get buffer[0], buffer[1] is %d %d !\n",buffer[0],buffer[1]);
		lseek(fd,512*buffer[1],SEEK_SET);
		int temp=buffer[1];
		int numberReaded =read (fd,buffer,202);
		lseek(fd,512*blockToCheck+offset,SEEK_SET);
		int numberWrited=write(fd,buffer,202);
		printf("case 3: after get buffer[0], buffer[1] is %d %d !\n",buffer[0],buffer[1]);
		return temp;
	}


}


//add mutex for inode in the future


void free_inode(int fd, int inodeNumber){
//set the inode valib bit =1;

	  if (inodeNumber <2 ){

		  printf("can not free inode less than 2! or larger than defined\n");
		  return ;

		  }

//get the inode offset position

	  lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
	  struct inodeFlag inodeFlag0,*ptr_inodeFlag;
	  int numberReaded =read (fd,ptr_inodeFlag,2);
	  printf("before get inode flag alloc %d !\n",inodeFlag0.alloc);
	  inodeFlag0.alloc=0;


	  lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
	  int numberWrited=write(fd,ptr_inodeFlag,2);
	  lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
	  numberReaded =read (fd,ptr_inodeFlag,2);
	  printf("after get inode alloc flag 1 %d !\n",inodeFlag0.alloc);

	  //get the ninode
	  lseek(fd,1*512+206,SEEK_SET);
	  unsigned short buffer[1];
	  numberReaded =read (fd,buffer,2);


		//update the ninode and inode array

	  lseek(fd,1*512+208+buffer[0],SEEK_SET);
	  numberWrited=write(fd,&inodeNumber,2);

		 lseek(fd,1*512+206,SEEK_SET);
		 buffer[0]=buffer[0]+1;
		 numberWrited=write(fd,buffer,2);
}


int scan_inodeList(int fd){

	unsigned short bufferInSB[101];

	//check the inodelist
	lseek(fd,1*512+206,SEEK_SET);
	int numberReaded =read (fd,bufferInSB,2);

   //get isize
	unsigned short buffer[1];
	lseek(fd,1*512,SEEK_SET);
	numberReaded =read (fd,buffer,2);
	int nisize = buffer[0];
	//scan the blocks of inode list
	lseek(fd,2*512+32,SEEK_SET);
	int inodeToScan=nisize*512-32;
	int validInodeScanned;
	int inodeNumber=2;  //root inode is excluded
	while (inodeToScan>0 && bufferInSB <=100){


		 struct inodeFlag inodeFlag0,*ptr_inodeFlag;
		 int numberReaded =read (fd,ptr_inodeFlag,2);

		 //check validness
		 if (inodeFlag0.alloc==0){
			 free_inode(fd,inodeNumber);
			 validInodeScanned=validInodeScanned+1;
		 }


		 inodeNumber=inodeNumber+1;
	}
	return validInodeScanned;


}

int get_inode(int fd){

	unsigned short buffer[101];

	//check the inodelist
	lseek(fd,1*512+206,SEEK_SET);
	int numberReaded =read (fd,buffer,2);
	if (buffer[0]==0){


		if(scan_inodeList(fd) >0 ){

			get_inode(fd);
		}
		else {

			return 0;
		}
	}

	else if(buffer[0]>0){

		//update the ninode;
		lseek(fd,1*512+206,SEEK_SET);
		int temp[1];
		numberReaded =read (fd,temp,2);
		int inodeNumber=temp[0]; //inodenumber mush larget than 1 to exclude root dirctory
		temp[0]=temp[0]-1;
		int numberWrited=write(fd,temp,2);


		//get the inode offset position
		lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
		struct inodeFlag inodeFlag0,*ptr_inodeFlag;
		numberReaded =read (fd,ptr_inodeFlag,2);
		inodeFlag0.alloc=1;
		lseek(fd,2*512+(inodeNumber-1)*32,SEEK_SET);
		numberWrited=write(fd,ptr_inodeFlag,2);



		return inodeNumber;
	}



	//return inode number
	return 1;
}



void init_inode(int fd, int *inodeInputArray){
	//
	struct inodeFlag inodeFlag0,*ptr_inodeFlag;
	struct inode inode0, *ptr_inode;
	ptr_inode=&inode0;
	inode0.inodeFlagStruct.alloc=1;



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
	int count =512;
	int value[1];
	value[0]=0;
	while(count>0){

		int numberWrited=write(fd,value[0],1);
		count=count-1;

	}


}

void init_directory(int inode,int fd){


	struct inode inode0,*ptr_inode;
	ptr_inode=&inode0;
	inode0=inode_empty;

	//init_inode(fd,directory);
	int blockNumber=get_block(fd);
	printf("assigned block number is %d",blockNumber);
	inode0.address[0]=blockNumber;

	//get inode' address
	lseek(fd,2*512+(inode-1)*32,SEEK_SET);
	printf("address id %d",inode0.address[0]);
	int numberWrited=write(fd,ptr_inode,32);



	//manipulate the data blocks

	clear_block(fd,blockNumber);
	printf(" hanging inside init_root! really \n");
	lseek(fd,512*blockNumber,SEEK_SET);

	struct dirEntryLayout directory0,*ptr_dir;
	ptr_dir=&directory0;

	strcpy(directory0.filename0,".");
	directory0.inode0=1;

	numberWrited=write(fd,ptr_dir,16);

	strcpy(directory0.filename0,"..");
	directory0.inode0=1;


	numberWrited=write(fd,ptr_dir,16);


}





void init_directoryDataBlock(int df,int *dirDBArray){

}



void init_bootBlock(){
//do nothing
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
struct superblock sb,*ptr_sb;
	  ptr_sb=&sb;
	 freeblocks = n1-2-nisize(n2);		//total number of free datablocks in the volume
	   //used variables
	  sb.isize[0] = (unsigned short) nisize(n2); //number of blocks devoted to the inode-list
	   sb.fsize[0] = (unsigned short) n1;//first block not used
	   sb.nfree[0] = (unsigned short) 0; // initial to 0


	   int i;
	   for (i=0;i<100;i=i+1){
	   	//printf("add block %d \n",i);
	   	 sb.free[i][0]=0;
	   }

	   if (n2 > 100){
		   sb.ninode[0] =(unsigned short) 100 ;
	   }
	   else
	   {
	   sb.ninode[0] =(unsigned short) n2 ;
	   }

	   int j;

	       for (j=0;j<100;j=j+1)
	       {
	        sb.inode[j][0]= (unsigned short)(j+1);
	       }

	   //flock, ilock
	       sb.flock[0]=0; //tbd
	       sb.ilock[0]=0; //tbd
	       sb.fmod[0]=0;
	       time_t tloc;
	       tloc=time(NULL);
	       sb.time[0]=(unsigned short) tloc;

	       lseek(fd,1*512,SEEK_SET);

	       int numberWrited=write(fd,ptr_sb,415);





  printf(" test hanging in init! \n");




//add blocks

  for (i=0;i<freeblocks;i=i+1){
 	   	//printf("add block %d \n",i);
	  add_block(fd, 1, 4, 2+nisize(n2)+i );

 	   }






//initial inode for root directory
  init_directory(1,fd);



  //  init_inode(fd, inodeDirInput);


  printf(" hanging here! \n");



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




