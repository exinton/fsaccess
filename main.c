#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Function Declarations for builtin shell commands:
 */

int sh_test(char **);
int sh_exit(char **);
int sh_initfs(char **);

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


int sh_initfs(char **args)
{
  printf("fsaccess\n");
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
  char *buffer = malloc(sizeof(char) * 1024);

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

