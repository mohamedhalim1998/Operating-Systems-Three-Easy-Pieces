#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
   size_t BUFFER_SIZE = 256;
   if (argc < 2)
   {
      printf("wgrep: searchterm [file ...]\n");
      exit(1);
   }
   if (argc >= 3)
   {
      for (int i = 2; i < argc; i++)
      {
         FILE *f = fopen(argv[i], "r");
         if (f == NULL)
         {
            printf("wgrep: cannot open file\n");
            exit(1);
         }
         char *buffer;
         buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
         size_t line = getline(&buffer, &BUFFER_SIZE, f);
         while (line != -1)
         {
            if (strstr(buffer, argv[1]) != NULL)
               printf("%s", buffer);
            line = getline(&buffer, &BUFFER_SIZE, f);
         }
         fclose(f);
      }
   }
   else
   {
      char *buffer;
      size_t line;
      buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
      line = getline(&buffer, &BUFFER_SIZE, stdin);
      while (line != -1)
      {
         if (strstr(buffer, argv[1]) != NULL)
            printf("%s", buffer);
         line = getline(&buffer, &BUFFER_SIZE, stdin);
      }
   }

   return 0;
}
