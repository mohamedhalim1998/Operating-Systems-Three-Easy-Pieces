#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
   size_t BUFFER_SIZE = 256;
   if (argc < 2)
   {
      printf("%s", "wzip: file1 [file2 ...]\n");
      exit(1);
   }
   char curr;
   int count = 0;
   for (int i = 1; i < argc; i++)
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
         int len = strlen(buffer);
         int i = 0;
         if (count == 0)
         {
            curr = buffer[i];
            count = 1;
            i = 1;
         }
         for (; i < len; i++)
         {
            if (curr != buffer[i])
            {
               fwrite(&count, sizeof(int), 1, stdout);
               fwrite(&curr, sizeof(char), 1, stdout);
               curr = buffer[i];
               count = 1;
            }
            else
            {
               count++;
            }
         }

         line = getline(&buffer, &BUFFER_SIZE, f);
      }
      fclose(f);
   }
   fwrite(&count, sizeof(int), 1, stdout);
   fwrite(&curr, sizeof(char), 1, stdout);

   return 0;
}
