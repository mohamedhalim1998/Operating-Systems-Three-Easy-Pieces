#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 256

int main(int argc, char const *argv[])
{
   if (argc < 2)
   {
      exit(0);
   }
   for (int i = 1; i < argc; i++)
   {
      FILE *f = fopen(argv[i], "r");
      if (f == NULL)
      {
         printf("wcat: cannot open file\n");
         exit(1);
      }
      char buffer[BUFFER_SIZE];
      while (fgets(buffer, BUFFER_SIZE, f) != NULL)
      {
         printf("%s", buffer);
      }
      fclose(f);
   }

   return 0;
}
