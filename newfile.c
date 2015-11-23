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
int static initblock[4]={0,0,0,0};

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
	unsigned int alloc:1;
	  unsigned int fileType:2;
	  unsigned int largeFile:1;
	  unsigned int setUid:1;
	  unsigned int setGrp:1;
	  unsigned int unalloc:1;
	  unsigned int readOwner:1;
	  unsigned int writeOwner:1;
	  unsigned int exeOwner:1;
	  unsigned int wregrp:3;
	  unsigned int wreothers:3;
}park;

struct inode{  //inode's length is 32 in total
	struct inodeFlag flag;
	char nlinks[1];		//number of links for files
	char uid[1];
	char gid[1];
	char size0[1];
	unsigned short size1[1];
	unsigned short address[8][1];	//address of data blocks pointing to
	unsigned short acttime[2][1];	//time of last access
	unsigned short modtime[2][1];	//time of last modification
};




/*
  List of builtin commands, followed by their corresponding functions.
 */

char *builtin_str[] = {  //pointer to command strings
  "q",
  "test",
  "initfs"
};


int (*builtin_func[]) (char **) = {  //pointer to  function
  &sh_exit,
  &sh_test,
  &sh_initfs
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

//calculate the nfree size



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
	inode0.flag.alloc=1;


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

void init_root_directory(int fd){

	//get inode
	lseek(fd,512*2+32*(0)+8,SEEK_SET);


	//init_inode(fd,directory);
	int blockNumber=get_block(fd);

	int address[2][1];
	address[0][0]=blockNumber;
	address[1][0]=1;

	int numberWrited=write(fd,address[0][0],2);

	//manipulate the data blocks

	clear_block(fd,blockNumber);
	printf(" hanging inside init_root! really \n");
	lseek(fd,512*blockNumber,SEEK_SET);
	char pathName[2][1];
	pathName[0][0]=".";
	pathName[1][0] ="..";
	numberWrited=write(fd,address[1][0],2);
	numberWrited=write(fd,pathName[0][0],14);

	numberWrited=write(fd,address[1][0],2);
	numberWrited=write(fd,pathName[1][0],14);


}





void init_directoryDataBlock(int df,int *dirDBArray){

}



void init_bootBlock(){
//do nothing
}



int sh_initfs(char **args)
{
   //trouble shooting print out
  printf("fsaccess\n");
  printf( "You entered: %s %s %s %s \n ", args[0],args[1], args[2],args[3]);
  int n1 = atoi(args[2]);
  int n2 = atoi(args[3]);
  int freeblocks = n1-2-nisize(n2);		//total number of free datablocks in the volume
  //used variables




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
  init_root_directory(fd);





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




