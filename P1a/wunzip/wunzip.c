#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
   if (argc < 2)
   {
      printf("%s", "wunzip: file1 [file2 ...]\n");
      exit(1);
   }
   for (int j = 1; j < argc; j++)
   {
      FILE *f = fopen(argv[j], "r");
      if (f == NULL)
      {
         printf("wgrep: cannot open file\n");
         exit(1);
      }
      int n;
      int num = fread(&n, sizeof(int), 1, f);
      char c;
      int ch = fread(&c, sizeof(char), 1, f);
      while (num >= 1 && ch >= 1)
      {

         for (int i = 0; i < n; i++)
         {
            fwrite(&c, sizeof(char), 1, stdout);
         }
         num = fread(&n, sizeof(int), 1, f);
         ch = fread(&c, sizeof(char), 1, f);
      }
      fclose(f);
   }
   return 0;
}
