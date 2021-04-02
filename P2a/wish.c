#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void show_error();
void run_exit(char *program[]);
void run_cd(char *program[]);
void run_path(char *program[]);
void run_built_in_command(char *program[]);
void parse_command(char *buffer, char *program[]);
pid_t run_program(char *program[]);
void run_interactive_mode();
void run_batch_mode(const char *file_name);
pid_t handle_command(char *buffer);
void check_redirect(char *buffer);
char *strtrim(char *str);
void handle_parallel_command(char *buffer);


char *path[10];
int redirect = -1;
char *redirect_file;


int main(int argc, char const *argv[]) {
   path[0] = "/bin";
   path[1] = NULL;

   if (argc == 1) {
      run_interactive_mode();
   } else if (argc == 2) {
      run_batch_mode(argv[1]);
   } else {
      show_error();
      exit(1);
   }
   return 0;
}

void run_interactive_mode() {
   char promet[6] = "wish> ";
   write(STDOUT_FILENO, promet, strlen(promet));
   size_t BUFFER_SIZE = 256;
   char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
   size_t line = getline(&buffer, &BUFFER_SIZE, stdin);
   while (line != -1) {
      handle_parallel_command(buffer);
      write(STDOUT_FILENO, promet, strlen(promet));
      line = getline(&buffer, &BUFFER_SIZE, stdin);
   }
}

void run_batch_mode(const char *file_name) {
   FILE *f = fopen(file_name, "r");
   if (f == NULL) {
      show_error();
      exit(1);
   }
   size_t BUFFER_SIZE = 256;
   char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
   size_t line = getline(&buffer, &BUFFER_SIZE, f);
   while (line != -1) {
      handle_parallel_command(buffer);
      line = getline(&buffer, &BUFFER_SIZE, f);
   }
   fclose(f);
}
void handle_parallel_command(char *buffer) {
   char *commands[10];
   int i = 0;
   char *token = strtok(buffer, "&");
   commands[i] = token;
   while (token != NULL) {
      token = strtok(NULL, "&");
      commands[++i] = token;
   }
   pid_t pids[i];
   i = 0;
   while (commands[i] != NULL) {
      char command[50];
      strcpy(command, commands[i]);
      strcat(command, "\n");
      pids[i] = handle_command(command);
      i++;
   }
   i = 0;
   while (commands[i] != NULL) {
      if (pids[i] != -1) {
         waitpid(pids[i], NULL, 0);
      }
      i++;
   }
}

pid_t handle_command(char *buffer) {
   char *program[10];
   check_redirect(buffer);
   parse_command(buffer, program);
   if (redirect != -1) {
      if (redirect_file == NULL || strstr(redirect_file, " ") != NULL) {
         show_error();
         return -1;
      }
   }
   if (program[0] != NULL) {
      if (strcmp(program[0], "exit") == 0 || strcmp(program[0], "cd") == 0 ||
          strcmp(program[0], "path") == 0) {
         run_built_in_command(program);
         return -1;
      } else {
         return run_program(program);
      }
   }
   return -1;
}

void parse_command(char *buffer, char *program[]) {
   buffer = strtok(buffer, "\n");
   int i = 0;
   char *token = strtok(buffer, " ");
   program[i] = token;
   while (token != NULL) {
      token = strtok(NULL, " ");
      program[++i] = token;
   }
}

void check_redirect(char *buffer) {
   char *token = strtok(buffer, ">");
   token = strtok(NULL, "\0");
   if (token == NULL) {
      redirect = -1;
      return;
   }
   token = strtok(token, "\n");
   if (token == NULL) {
      redirect = 1;
      redirect_file = NULL;
      return;
   }
   redirect_file = strtrim(token);
   redirect = 1;
}

void run_built_in_command(char *program[]) {
   if (strcmp(program[0], "exit") == 0) {
      run_exit(program);
   } else if (strcmp(program[0], "cd") == 0) {
      run_cd(program);
   } else if (strcmp(program[0], "path") == 0) {
      run_path(program);
   }
}

void run_exit(char *program[]) {
   if (program[1] != NULL) {
      show_error();
      return;
   }
   exit(0);
}

void run_cd(char *program[]) {
   if (program[1] == NULL || program[2] != NULL) {
      show_error();
      return;
   }
   int res = chdir(program[1]);
   if (res == -1) {
      show_error();
   }
}

void run_path(char *program[]) {
   int i = 1;
   path[0] = NULL;
   while (program[i] != NULL) {
      int len = strlen(program[i]);
      path[i - 1] = malloc(len * sizeof(char));
      for (int j = 0; j < len; j++) {
         path[i - 1][j] = program[i][j];
      }
      path[i] = NULL;
      i++;
   }
}

pid_t run_program(char *program[]) {
   int pid = fork();
   if (pid < 0) {
      show_error();
      return -1;
   } else if (pid == 0) {
      char program_path[50];
      int i = 0;
      do {
         if (path[i] == NULL) {
            show_error();
            exit(0);
         }
         strcpy(program_path, path[i]);
         strcat(program_path, "/");
         strcat(program_path, program[0]);
         i++;
      } while (access(program_path, X_OK) != 0);
      if (redirect != -1) {
         freopen(redirect_file, "w", stdout);
         freopen(redirect_file, "w", stderr);
      } else {
         freopen("/dev/stdout", "a", stdout);
         freopen("/dev/stderr", "a", stderr);
      }
      int status = execvp(program_path, program);
      if (status == -1) {
         show_error();
         exit(0);
      }
      return -1;
   } else {
      return pid;
   }
}

void show_error() {
   char error_message[30] = "An error has occurred\n";
   write(STDERR_FILENO, error_message, strlen(error_message));
}

char *strtrim(char *str) {
   if (str == NULL) return str;
   char *end;

   while (isspace((unsigned char)*str)) str++;

   if (*str == 0) return str;

   end = str + strlen(str) - 1;
   while (end > str && isspace((unsigned char)*end)) end--;

   end[1] = '\0';

   return str;
}