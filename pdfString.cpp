#include "pdfString.hpp"
#include <stdio.h>

bool isHexa(char h) {
   if ( (h >= '0' ) && (h <= '9') )
      return true;
   if ( (h >= 'A' ) && (h <= 'F') )
      return true;
   if ( (h >= 'a' ) && (h <= 'f') )
      return true;
   return false;
}

int hexaValue(char h) {
   if ( (h >= '0' ) && (h <= '9') )
      return h - '0';
   if ( (h >= 'A' ) && (h <= 'F') )
      return h - 'A' + 10;
   if ( (h >= 'a' ) && (h <= 'f') )
      return h - 'a' + 10;
   return 0;
}

char getHexaCode(char h) {
   if ( h < 10 )
      return (char)(h + '0');
   return (char)(h - 10 + 'a');
}

char getSpecialCar(char c) {
   switch ( c ) {
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      return 0;
   case 'b':
      return '\b';
   case 'f':
      return '\f';
   case 'n':
      return '\n';
   case 'r':
      return '\r';
   case 't':
      return '\t';
   case '(':
      return '(';
   case ')':
      return ')';
   case '\\':
      return '\\';
   }

   return -1;
}

char pdfEscape(char c) {
   switch (c) {
   case '\n':
      return 'n';
   case '\r':
      return 'r';
   case '\t':
      return 't';
   case '\b':
      return 'b';
   case '\f':
      return 'f';
   case '(':
      return '(';
   case ')':
      return ')';
   case '\\':
      return '\\';
   }

   return 0;
}

/* ***************************************************************************************** */

pdfString::pdfString() {
   this->type = _unknownType;
   this->length = 0;
   this->maxLength = LG_PDF_STRING;
   this->value = new char[this->maxLength];
}

/* ***************************************************************************************** */

pdfString::pdfString(const char *src) {
   this->type = _unknownType;
   this->length = 0;
   this->maxLength = LG_PDF_STRING;
   this->value = new char[this->maxLength];
   this->copy(src);
}

/* ***************************************************************************************** */

pdfString::~pdfString() {
   if ( this->value != 0 ) {
      delete this->value;
   }
}

/* ***************************************************************************************** */

int pdfString::copy(const char *src) {
int i, j, lg;
int off;
char endCar, startCar;
char escCar = '\\';

   endCar = 0;
   off = 0;
   if ( !src )
      return 0;
   if ( src[off] == '(' ) {
      startCar = '(';
      endCar = ')';
      this->type = _carType;
   }
   if ( src[off] == '<' ) {
      startCar = '<';
      endCar = '>';
      this->type = _hexaType;
   }
   if ( endCar == 0 )
      return 0;

   lg = 0;
   ++off;
   j = 1;
   while ( lg < ABSOLUTE_MAX_PDF_STRING_LENGTH ) {
      if ( src[off + lg] == escCar ) {
          ++lg;
      }
      else {
         if ( src[off + lg] == endCar )
             --j;
         if ( src[off + lg] == startCar )
             ++j;
      }
      if ( j == 0 )
         break;
      ++lg;
   }
   if ( lg == ABSOLUTE_MAX_PDF_STRING_LENGTH )
      return 0;
   if ( lg > this->maxLength ) {
      delete this->value;
      this->maxLength = lg;
      this->value = new char[this->maxLength];
   }

   switch (this->type) {
   case _carType:
      i = 0;
      j = 1;
      lg = 0;
#pragma warning ( disable: 4127)
      while ( 1 ) {
#pragma warning  (default: 4127)
         if ( src[off + i] == endCar ) {
            --j;
            if ( j == 0 )
               break;
         }
         if ( src[off + i] == '(' ) {
            ++j;
         }
         if ( src[off + i] != '\\' ) {
            this->value[lg] = src[off + i];
            ++i;
         }
         else {
            this->value[lg] = getSpecialCar(src[off + i + 1]);
            if ( this->value[lg] == (char)-1 ) {
               --lg;
            }
            else {
               if ( this->value[lg] == 0 ) {
               int k, l, t, p;
                  k = off + i + 1;
                  l = 0;
                  while ( (src[k + l] >= '0') && (src[k + l] <= '9') && (l < 3) )
                     ++l;
                  --l;
                  i += l;
                  p = 1;
                  t = 0;
                  while ( l != -1 ) {
                     t += (src[k + l] - '0') * p;
                     p *= 8;
                     --l;
                  }
                  this->value[lg] = (char)t;
               }
            }
            i += 2;
         }
         ++lg;
      }
      break;
   case _hexaType:
      lg = 0;
      i = 0;
      while ( src[off + i] != endCar ) {
         while ( !isHexa(src[off + i]) && (src[off + i] != endCar) ) {
            ++i;
         }
         if (src[off + i] != endCar) {
            j = 16 * hexaValue(src[off + i]);
            ++i;
         }
         else {
            j = 0;
         }
         while ( !isHexa(src[off + i]) && (src[off + i] != endCar) ) {
            ++i;
         }
         if (src[off + i] != endCar) {
            j += hexaValue(src[off + i]);
            ++i;
         }
         this->value[lg++] = (char)j;
      }
      break;
   }

   this->length = lg;

   return this->length;
}

/* ***************************************************************************************** */

int pdfString::copy(const char *src, int lg, stringType stype) {
int i;

   this->type = stype;
   if ( lg > this->maxLength ) {
      delete value;
      this->maxLength = lg;
      value = new char[lg];
   }
   for (i = 0; i < lg; i++)
      this->value[i] = src[i];

   this->length = lg;
   return this->length;
}

/* ***************************************************************************************** */

int pdfString::copy(const pdfString *org) {
int i;

    if (org->maxLength > this->maxLength ) {
        if ( this->value ) {
            delete this->value;
        }
        this->maxLength = org->maxLength;
        this->value = new char[this->maxLength];
    }
    this->type = org->type;
    this->length = org->length;
    for (i = 0; i < this->length; i++)
      this->value[i] = org->value[i];

    return this->length;
}

/* ***************************************************************************************** */
// printable signifie telle que la string doit être écrite dans le fichier PDF
// renvoie la longueur de la string, ou la longueur attendue pour pouvoir effectuer l'opération

int pdfString::snprint(char *dest, int lg, bool printable) {
int i, j;

   if ( (dest == 0) || (lg < 1) )
      return 0;

   switch ( this->type ) {
   case _carType:
      if ( lg < ((this->length) * 2 + 1) ) {
         *dest = 0;
         return (this->length) * 2 + 1;
      }
      if ( printable ) {
         j = 0;
         for (i=0; i < this->length; i ++) {
            if ( pdfEscape(this->value[i]) != 0 ) {
               dest[j++] = '\\';
               dest[j++] = pdfEscape(this->value[i]);
            }
            else {
            /*unsigned char uc, reste;
               if ( this->value[i] <= 0 ) {
                  dest[j++] = '\\';
                  uc = (unsigned char)this->value[i];
                  dest[j++] = (char)(uc / 64 + '0');
                  reste = (unsigned char)(uc - (uc / 64) * 64);
                  dest[j++] = (char)(reste / 8 + '0');
                  reste = (unsigned char)(reste - (reste / 8) * 8);
                  dest[j++] = (char)(reste + '0');
               }
               else*/
                  dest[j++] = this->value[i];
            }
         }
         i = j;
      }
      else {
         for (i = 0; i < this->length; i++) {
            dest[i] = this->value[i];
         }
      }
      dest[i] = 0;
      break;
   case _hexaType:
      if ( printable ) {
         if ( lg < ((this->length * 2) + 1) ) {
            *dest = 0;
            return (this->length * 2) + 1;
         }
         for (i=0; i < this->length; i ++) {
            dest[i * 2] = getHexaCode((char)((this->value[i] >> 4) & 0x0F) );
            dest[(i * 2) + 1] = getHexaCode((char)(this->value[i] & 0x0F));
         }
         i *= 2;
         dest[i] = 0;
      }
      else {
         if ( lg < this->length ) {
            *dest = 0;
            return this->length;
         }
         for (i=0; i < this->length; i ++) {
            dest[i] = this->value[i];
         }
      }
      break;
   default:
      *dest = 0;
      return 0;
   }

   return i;
}

/* ***************************************************************************************** */

bool pdfString::echo(FILE *dest) {
char * buf;
    buf = new char[this->length * 2 + 2];
	this->snprint(buf, this->length * 2 + 2, true);
	buf[this->length] = 0;
	fprintf(dest, "%s", buf);
	delete buf;

	return true;
}

bool pdfString::compare(char *c, int lg) {
   if ( this->length < lg )
      return false;
   --lg;
   while ( lg != -1 ) {
      if ( this->value[lg] != c[lg] )
         return false;
      --lg;
   }

   return true;
}

int pdfString::compare(pdfString *orgStr) {
int i = 0;

   if ( this->length == 0 ) {
      if ( orgStr->length == 0 )
         return 0;
      return -1;
   }
   if ( orgStr->length == 0 ) {
      return 1;
   }

   while ( (i < this->length) && (i < orgStr->length) && (this->value[i] == orgStr->value[i]) )
      ++i;
   
   if ( (i == this->length) && (i ==orgStr->length) ) {
      return 0;
   }
   if ( (i == this->length) && (i != orgStr->length) ) {
      return -1;
   }
   if ( (i != this->length) && (i == orgStr->length) ) {
      return 1;
   }
   if ( this->value[i] < orgStr->value[i] ) {
      return -1;
   }
   
   return 1;
}
