#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
char cmd[1024];
char asmRep[1024];
FILE *org;

   org = fopen("pdfAsmRep.txt", "r");
   if ( !org ) {
      fprintf(stdout, "saisisser le nom du répertoire dans lequel se trouve mbtPdfAsm.exe:\r\n");
      fscanf(stdin, "%s", asmRep);
      org = fopen("pdfAsmRep.txt", "w");
      fprintf(org, "%s", asmRep);
      fclose(org);
   }
   else {
      fgets(asmRep, 1023, org);
      fclose(org);
   }

   sprintf(cmd, "java -jar mbtPdfAsm.jar %s", asmRep);
   fprintf(stdout, "%s\r\n", cmd);
   system(cmd);

   return 1;
}