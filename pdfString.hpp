#ifndef __PDFSTRING_HPP__
#define __PDFSTRING_HPP__

#include <stdio.h>

enum stringType {_unknownType, _hexaType, _carType};

#define LG_PDF_STRING 255
#define ABSOLUTE_MAX_PDF_STRING_LENGTH 10000

class pdfString {
private:
   stringType type;
   int length;
   int maxLength;
   char *value;
public:
   pdfString();
   pdfString(const char *src);
   ~pdfString();

   /* copie une chaine dans value en utilisant le premier caractère [ ou ( pour déterminer si il s'agit
   d'un chaine hexa ou car*/
   int copy(const char *);
   /* copie une chaine dans value en utilisant int comme longuer à copier*/
   int copy(const char *, int, stringType);
   /* copie la chaine org dans la chaine this */
   int copy(const pdfString *org);
   int snprint(char *dest, int lg, bool printable); // printable = true si on veut une version affichable de la chaine cela n'et utile que dans le cas d'une chaine hexa
   bool echo(FILE *dest);
   bool compare(char *c, int lg);

   //retourne 0 si les chaînes sont identiques, -1 si this < org, et 1 si this > org
   int compare(pdfString *orgStr);

   inline int getLength() { return this->length; }
};

#endif
