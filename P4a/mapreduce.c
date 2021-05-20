#include "mapreduce.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"
#define SIZE 1000000

pthread_mutex_t val_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sort_lock = PTHREAD_MUTEX_INITIALIZER;

struct data_item {
   char *data[100];
   char *key;
};
struct node {
   char *key;
   int index;
   struct node *next;
};

struct reduce_args {
   Reducer reduce;
   Getter get_next;
   int partition_number;
};
struct node *sorted_data;
struct data_item data_values[SIZE];
int partions;
void reduce_thread(void *arg) {
   struct reduce_args *args = arg;
   int partion = args->partition_number;
   struct node *curr = sorted_data[partion].next;
   while (curr != NULL) {
      args->reduce(curr->key, args->get_next, partion);
      curr = curr->next;
   }
}
unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
   unsigned long hash = 5381;
   int c;
   while ((c = *key++) != '\0') hash = hash * 33 + c;
   return hash % num_partitions;
}

char *get_next(char *key, int partition_number) {
   int hash = MR_DefaultHashPartition(key, SIZE);
   int i = 0;
   while (data_values[hash + i].key != NULL &&
          strcmp(key, data_values[hash + i].key) != 0) {
      i++;
      printf("inc: %d", i);
   }
   if (data_values[hash + i].key == NULL) {
      data_values[hash + i].key = malloc(strlen(key) * sizeof(char));
      strcpy(data_values[hash + i].key, key);
   }
   int j = 0;

   while (data_values[hash + i].data[j] != NULL) {
      j++;
   }
   if (j == 0) return NULL;
   j--;
   char *res = data_values[hash + i].data[j];
   data_values[hash + i].data[j] = NULL;
   return res;
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers, Reducer reduce,
            int num_reducers, Partitioner partition) {
   sorted_data = malloc(num_reducers * sizeof(struct node));

   partions = num_reducers;
   for (int i = 1; i < argc;) {
      pthread_t p[num_mappers];
      int j = 0;
      for (; j < num_mappers && i < argc; j++, i++) {
         Pthread_create(&p[j], NULL, (void *)map, argv[i]);
      }
      j--;
      for (; j >= 0; j--) {
         Pthread_join(p[j], NULL);
      }
   }
   pthread_t p[num_reducers];
   struct reduce_args args[num_reducers];
   for (int i = 0; i < num_reducers; i++) {
      struct reduce_args arg;
      arg.get_next = get_next;
      arg.partition_number = i;
      arg.reduce = reduce;
      args[i] = arg;
   }
   for (int i = 0; i < num_reducers; i++) {
      Pthread_create(&p[i], NULL, (void *)reduce_thread, (void *)&args[i]);
   }

   for (int i = 0; i < num_reducers; i++) {
      Pthread_join(p[i], NULL);
   }
}

void MR_Emit(char *key, char *value) {
   int partion = MR_DefaultHashPartition(key, partions);
   int hash = MR_DefaultHashPartition(key, SIZE);
   Pthread_mutex_lock(&val_lock);
   int i = 0;
   while (data_values[hash + i].key != NULL &&
          strcmp(key, data_values[hash + i].key) != 0) {
      i++;
   }
   if (data_values[hash + i].key == NULL) {
      data_values[hash + i].key = malloc(strlen(key) * sizeof(char));
      strcpy(data_values[hash + i].key, key);
   }
   int j = 0;
   while (data_values[hash + i].data[j] != NULL) {
      j++;
   }
   data_values[hash + i].data[j] = malloc(strlen(value) * sizeof(char));
   strcpy(data_values[hash + i].data[j], value);
   Pthread_mutex_unlock(&val_lock);

   Pthread_mutex_lock(&sort_lock);
   struct node *prev = &sorted_data[partion];
   struct node *curr = sorted_data[partion].next;
   while (curr != NULL && strcmp(key, curr->key) > 0) {
      prev = curr;
      curr = curr->next;
   }
   if (curr != NULL && strcmp(key, curr->key) == 0) {
      Pthread_mutex_unlock(&sort_lock);
      return;
   }
   if (curr == NULL) {
      struct node *n = malloc(sizeof(struct node));
      n->key = malloc(strlen(key) * sizeof(char));
      strcpy(n->key, key);
      prev->next = n;
      n->index = hash + i;
      n->next = NULL;
   } else {
      struct node *n = malloc(sizeof(struct node));
      n->key = malloc(strlen(key) * sizeof(char));
      strcpy(n->key, key);
      n->index = hash + i;
      n->next = prev->next;
      prev->next = n;
   }
   Pthread_mutex_unlock(&sort_lock);
}
