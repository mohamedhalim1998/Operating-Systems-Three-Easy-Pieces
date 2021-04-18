#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

typedef struct _char_counter {
   char c;
   int count;
} char_counter;
typedef struct _pzip_args {
   char *txt;
   size_t start;
   size_t end;
   char *res_char;
   int *res_count;
   int thread_id;
} pzip_args;

void *zip(void *arg) {
   pzip_args *args = (pzip_args *)arg;
   char *res_char = args->res_char;
   int *res_count = args->res_count;
   int i = args->start;
   char curr = args->txt[i];
   int count = 1;
   i++;
   int index = 0;

   for (; i < args->end; i++) {
      if (curr != args->txt[i]) {
         res_char[index] = curr;
         res_count[index] = count;
         index++;
         count = 1;
         curr = args->txt[i];
      } else {
         count++;
      }
   }

   res_char[index] = curr;
   res_count[index] = count;
   index++;
   res_char[index] = curr;
   res_count[index] = -1;
   return NULL;
}

int main(int argc, char const *argv[]) {
   if (argc < 2) {
      printf("%s", "wzip: file1 [file2 ...]\n");
      exit(1);
   }
   int NO_OF_PROC = get_nprocs_conf();
   pthread_t p[NO_OF_PROC];
   int old_char = '\357';
   int old_count = 0;
   for (int i = 1; i < argc; i++) {
      int fd = open(argv[i], O_RDONLY);
      struct stat statbuf;
      int err = fstat(fd, &statbuf);
      if (err < 0) {
         printf("wzip: cannot open file\n");
         exit(1);
      }
      char *ptr = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
      close(fd);
      //  printf("%s", ptr);
      size_t len = statbuf.st_size;
      size_t part = len / NO_OF_PROC;
      pzip_args args[NO_OF_PROC];
      int *res_count[NO_OF_PROC];
      char *res_char[NO_OF_PROC];
      for (int j = 0; j < NO_OF_PROC; j++) {
         res_char[j] = malloc(1000000 * sizeof(char));
         res_count[j] = malloc(1000000 * sizeof(int));
         args[j].start = (part * j);
         args[j].end = j == NO_OF_PROC - 1 ? len : (part * j + part);
         args[j].txt = ptr;
         args[j].thread_id = j;
         args[j].res_char = res_char[j];
         args[j].res_count = res_count[j];
      }
      for (int j = 0; j < NO_OF_PROC; j++) {
         int rc = pthread_create(&p[j], NULL, zip, (void *)&args[j]);
         if (rc != 0) {
            printf("ERR: thread %d can not be created", j);
            exit(1);
         }
      }
      for (int j = 0; j < NO_OF_PROC; j++) {
         Pthread_join(p[j], NULL);
      }
      for (int j = 0; j < NO_OF_PROC; j++) {
         int index = 0;
         while (res_count[j][index] != -1) {
            if (old_char != res_char[j][index]) {
               if (old_count != 0) {
                  fwrite(&old_count, sizeof(int), 1, stdout);
                  fwrite(&old_char, sizeof(char), 1, stdout);
               }
               old_char = res_char[j][index];
               old_count = res_count[j][index];
            } else {
               old_count += res_count[j][index];
            }
            index++;
         }
      }
   }
   if (old_count > 0) {
      fwrite(&old_count, sizeof(int), 1, stdout);
      fwrite(&old_char, sizeof(char), 1, stdout);
   }
   return 0;
}
