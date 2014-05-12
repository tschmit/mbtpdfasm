#include "pdfXrefTable.hpp"
#include "diversPdf.hpp"
#include "mpaMain.hpp"

#ifdef DEBUG_MEM_LEAK
#ifdef WIN32
   #include <crtdbg.h>
   #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
   #define new DEBUG_NEW
#endif
#endif

strMatcher X_X_R("^[0-9]* [0-9]* R", true, 0);
strMatcher f_X_X_R("[0-9]* [0-9]* R", true, 0);

T_keyWord pageKeyWords[] = {
   {"/Type",         _Type,            TYPE_REQUIRED, false, TYPE_NAME },
   {"/Parent",       _Parent,          TYPE_REQUIRED, false, TYPE_DICTIONARY },
   {"/LastModified", _LastModified,    TYPE_OPTIONAL, false, TYPE_DATE },
   {"/Contents",     _Contents,        TYPE_OPTIONAL, false, TYPE_ARRAY},
   {"/Resources",    _Resources,       TYPE_REQUIRED, true,  TYPE_DICTIONARY},
   {"/MediaBox",     _MediaBox,        TYPE_REQUIRED, true,  TYPE_RECTANGLE},
   {"/CropBox",      _CropBox,         TYPE_OPTIONAL, true,  TYPE_RECTANGLE},
   {"/BleedBox",     _BleedBox,        TYPE_OPTIONAL, false, TYPE_RECTANGLE},
   {"/TrimBox",      _TrimBox,         TYPE_OPTIONAL, false, TYPE_RECTANGLE},
   {"/ArtBox",       _ArtBox,          TYPE_OPTIONAL, false, TYPE_RECTANGLE},
   {"/BoxColorInfo", _BoxColorInfo,    TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/Rotate",       _Rotate,          TYPE_OPTIONAL, true,  TYPE_INTEGER},
   {"/Group",        _Group,           TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/Thumb",        _Thumb,           TYPE_OPTIONAL, false, TYPE_STREAM},
   {"/B",            __B,              TYPE_OPTIONAL, false, TYPE_ARRAY},
   {"/Dur",          _Dur,             TYPE_OPTIONAL, false, TYPE_NUMBER},
   {"/Trans",        _Trans,           TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/Annots",       _Annots,          TYPE_OPTIONAL, false, TYPE_ARRAY},
   {"/AA",           _AA,              TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/Metadata",     _Metadata,        TYPE_OPTIONAL, false, TYPE_STREAM},
   {"/PieceInfo",    _PieceInfo,       TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/StructParents", _StructParents,  TYPE_OPTIONAL, false, TYPE_INTEGER},
   {"/ID",           _ID,              TYPE_OPTIONAL, false, TYPE_STRING},
   {"/PZ",           _PZ,              TYPE_OPTIONAL, false, TYPE_NUMBER},
   {"/SeparationInfo",_SeparationInfo, TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/ColorSpace",    _ColorSpace,     TYPE_OPTIONAL, false, TYPE_ARRAY},
   {"/BCLPrivAnnots", _BCLPrivAnnots,  TYPE_OPTIONAL, false, TYPE_ARRAY},
   {(const char *)0, _notAType, 0, 0, _notAType}

};

T_keyWord resourcesKeyWords[] = {
   {"/ExtGState",    _ExtGState,  TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/ColorSpace",   _ColorSpace, TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/Pattern",      _Pattern,    TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/Shading",      _Shading,    TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/XObject",      _XObject,    TYPE_OPTIONAL, false, TYPE_STREAM},
   {"/ProcSet",      _ProcSet,    TYPE_OPTIONAL, false, TYPE_ARRAY},
   {"/Font",         _Font,       TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {"/Properties",   _Properties, TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {(const char *)0, _notAType, 0, 0}
};

T_keyWord xrefObjKeyWords[] = {
   {"/Type",    _Type,    TYPE_REQUIRED, false, TYPE_NAME },
   {"/Size",    __Size,   TYPE_REQUIRED, false, TYPE_INTEGER },
   {"/Root",    _Root,    TYPE_REQUIRED, false, TYPE_DICTIONARY },
   {"/Index",   __Index,  TYPE_REQUIRED, false, TYPE_ARRAY },
   {"/W",       __W,      TYPE_REQUIRED, false, TYPE_ARRAY },
   {"/Filter",  __Filter, TYPE_REQUIRED, false, TYPE_NAME },
   {"/Prev",    _Prev,    TYPE_REQUIRED, false, TYPE_INTEGER },
   {"/Length",  _Length,  TYPE_REQUIRED, false, TYPE_INTEGER },
   {"/Info",    _Info,    TYPE_REQUIRED, false, TYPE_DICTIONARY },
   {"/ID",      _ID,      TYPE_REQUIRED, false, TYPE_ARRAY },
   {"/ModDate", _ModDate, TYPE_REQUIRED, false, TYPE_DATE },
   {"/Encrypt", _Encrypt, TYPE_REQUIRED, false, TYPE_NAME },
   {(const char *)0, _notAType, 0, 0, _notAType}

};

T_keyWord filterKeyWords[] = {
   {"/ASCIIHexDecode",  _ASCIIHexDecode,  TYPE_OPTIONAL, false, TYPE_NAME },
   {"/ASCII85Decode",   _ASCII85Decode,   TYPE_OPTIONAL, false, TYPE_NAME },
   {"/LZWDecode",       _LZWDecode,       TYPE_OPTIONAL, false, TYPE_NAME },
   {"/FlateDecode",     _FlateDecode,     TYPE_OPTIONAL, false, TYPE_NAME },
   {"/RunLengthDecode", _RunLengthDecode, TYPE_OPTIONAL, false, TYPE_NAME },
   {"/CCITTFaxDecode",  _CCITTFaxDecode,  TYPE_OPTIONAL, false, TYPE_NAME },
   {"/JBIG2Decode",     _JBIG2Decode,     TYPE_OPTIONAL, false, TYPE_NAME },
   {"/DCTDecode",       _DCTDecode,       TYPE_OPTIONAL, false, TYPE_NAME },
   {"/JPIXDecode",      _JPXDecode,       TYPE_OPTIONAL, false, TYPE_NAME },
   {"/Crypt",           _Crypt,           TYPE_OPTIONAL, false, TYPE_NAME },
   {(const char *)0, _notAType, 0, 0, _notAType}

};

T_keyWord compressedKeyWords[] = {
   {"/Length", _Length,      TYPE_OPTIONAL, false, TYPE_NAME },
   {"/N",      __N_,         TYPE_OPTIONAL, false, TYPE_NAME },
   {"/First",  _First,       TYPE_OPTIONAL, false, TYPE_NAME },
   {"/Filter", __Filter,     TYPE_OPTIONAL, false, TYPE_NAME },
   {(const char *)0, _notAType, 0, 0, _notAType}

};

T_keyWord fontKeyWords[] = {
   {"/Type",           _Type,           TYPE_REQUIRED, false, TYPE_NAME },
   {"/Subtype",        _Subtype,        TYPE_REQUIRED, false, TYPE_NAME},
   {"/Name",           __Name,           TYPE_OPTIONAL, false, TYPE_NAME},
   {"/BaseFont",       _BaseFont,       TYPE_REQUIRED, false, TYPE_NAME},
   {"/FirstChar",      _FirstChar,      TYPE_REQUIRED, false, TYPE_INTEGER},
   {"/LastChar",       _LastChar,       TYPE_REQUIRED, false, TYPE_INTEGER},
   {"/Widths",         _Widths,         TYPE_REQUIRED, false, TYPE_ARRAY},
   {"/FontDescriptor", _FontDescriptor, TYPE_REQUIRED, false, TYPE_DICTIONARY},
   {"/Encoding",       _Encoding,       TYPE_OPTIONAL, false, TYPE_NAME},
   {"/ToUnicode",      _ToUnicode,      TYPE_OPTIONAL, false, TYPE_DICTIONARY},
   {(const char *)0, _notAType, 0, 0, _notAType}

};

T_keyWord actionKeyWords[] = {
   {"/Type",           _Type,           TYPE_OPTIONAL, false, TYPE_NAME },
   {"/D",              _D,              TYPE_REQUIRED, false, TYPE_NAME},
   {"/S",              __S,             TYPE_REQUIRED, false, TYPE_NAME},
   {"/JS",             _JS,             TYPE_REQUIRED, false, TYPE_NAME},
   {(const char *)0, _notAType, 0, 0, _notAType}

};

//tableau de valeurs pour les types d'objets
T_keyWord typeKeyWordsValues[] = {
   {"/Font", _Font, TYPE_OPTIONAL, false, TYPE_NAME},
   {(const char *)0, _notAType, 0, 0, _notAType}

};

T_keyWord noKeyWord[] = {
   {(const char *)0, _notAType, 0, 0}
};

bool cmpW2W(char *p1, char *p2) {
	while ( *p1 && *p2 && (*(p1) == *(p2)) ) {
		p1++; p2++;
	}
   if ( !(*p1 || *p2) )
      return true;
   return false;
}

/* *****************************************************
fonction permettant de déterminé si pc est un mot clé
défini dans pKW
*/

int isWaKey(char *pc, T_keyWord *pKW) {
unsigned char *w;
int i, res;

   res = _notAType;
   i = 1;
   w = (unsigned char *)pc + 1;
   //corrigé grace à Henning Nolte
   while ( (*w > ' ') && (*w != ' ') && (*w != '/') && (*w != '[') && (*w != '<') && (*w != '(')) {
      ++i;
      ++w;
   }
   w = (unsigned char *)new char[i + 1];
   w[i--] = 0;
   while ( i )
      w[i] = pc[i--];
   w[0] = pc[0];

   while ( pKW[0].nom != (const char *)0 ) {
      if ( cmpW2W((char *)pKW[0].nom, (char *)w) ) {
         res = pKW[0].code;
         break;
      }
      ++pKW;
   }

   delete w;

   return res;
}

/* *****************************************************
fonction permettant de déterminé si pc est un mot clé
défini dans pKW
*/

const char *nomAttrib(int id, T_keyWord *pKW) {
const char *res;

   res = (const char *)0;

   while ( pKW[0].code != 0 ) {
      if ( pKW[0].code == id ) {
         res = pKW[0].nom;
         break;
      }
      ++pKW;
   }

   return res;
}

/* ******************************************************
fonction renvoyant la taille d'un objet
c'est à dire le nombre d'octets entre << et >>
on assume que pc est situé derrière un << => j = 1
*/

int tailleObj(char *pc) {
int i, j;

   i = 0;
   j = 1;
   while ( j ) {
      if ( (pc[i] == '<') && (pc[i + 1] == '<') ) {
         ++j;
         ++i;
      }
      if ( (pc[i] == '>') && (pc[i + 1] == '>') ) {
         --j;
         ++i;
      }
      ++i;
   }
   
   return i - 1;
}


/* *********************************************************
renvoi la longeur d'une clé
*/
int keyLen(int idKey, T_keyWord *pKW) {
int i;

   while ( pKW[0].code && (pKW[0].code != idKey) )
      ++pKW;
   if ( pKW[0].code ) {
      i = 0;
      while (pKW[0].nom[i])
         ++i;

      return i;
   }

   return 0;
}

/* *********************************************************
renvoi la longeur de l'attribut à partir de pc

*/

int tailleAttrib(const char *pc) {
int res, j;

   res = 0;
   while ( pc[res] <= ' ' )
      ++res;
   switch ( pc[res] ) {
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
   case '+':
   case '-':
      ++res;
      while ( ((pc[res] >= '0') && (pc[res] <= '9')) || (pc[res] == '.') )
         ++res;
      if (pc[res] == ' ') { // c'est peut être une référence
         j = res;
         while ( pc[j] == ' ')
            ++j;
         if ( ((pc[j] < '0')) || (pc[j] > '9') )
            break;
         while ( (pc[j] >= '0') && (pc[j] <= '9') )
            ++j;
         if (pc[j] != ' ')
            break;
         while (pc[j] == ' ')
            ++j;
         if ( pc[j] == 'R' )
            res = j + 1;
      }
      break;
   case '[':
      ++res;
      j = 1;
      while ( j ) {
         if ( pc[res] == '(' )
            res += tailleAttrib(pc + res) - 1;
         if ( pc[res] == '[' )
            ++j;
         if ( pc[res] == ']' )
            --j;
         ++res;
      }
      break;
   case '<':
      if (pc[res + 1] == '<' ) {
         res += 2;
         j = 1;
         while ( j ) {
            if ( (pc[res] == '<') && (pc[res + 1] == '<') ) {
               ++j;
               ++res;
            }
            if ( (pc[res] == '>') && (pc[res + 1] == '>') ) {
               --j;
               ++res;
            }
            ++res;
         }
      }
      else {
         ++res;
         j = 1;
         while ( j ) {
            if ( pc[res] == '<' )
               ++j;
            if ( pc[res] == '>' )
               --j;
            ++res;
         }
      }
      break;
   case '/':
      ++res;
      while ( (pc[res] > ' ') && (pc[res] != '/' ) && (pc[res] != '[' ) && (pc[res] != '<' ) )
         ++res;
      break;
   case '(':
      ++res;
      j = 1;
      while ( j ) {
         if ( pc[res] == '(' )
            ++j;
         if ( pc[res] == ')' )
            --j;
         ++res;
      }
      break;
   default:
      while ( (pc[res] >= ' ') && !((pc[res] == '>') && (pc[res + 1] == '>')) )
         ++res;
   }

   return res;
}

/* ************************************************************
renvoi le nombre d'octet qu'il faut remonter à partir de " R".
pour se positionner sur le numéro de l'objet
*/

int rewindRef(char *pc) {
int j;

   j = 0;
   /* ' R'*/
   while ( *(pc + j) == ' ' )
      --j;
   /* 'X R' */
   if ( (*(pc + j) < '0') || (*(pc + j) > '9') ) {
       return 0;
   }
   while ( *(pc + j) != ' ' )
      --j;
   /* ' X...X R' */
   if ( (*(pc + j + 1) < '0') || (*(pc + j + 1) > '9') ) {
       return 0;
   }
   while ( *(pc + j) == ' ' )
      --j;
   /* 'Y X...X R'*/
   if ( (*(pc + j) < '0') || (*(pc + j) > '9') ) {
       return 0;
   }
   while ( (*(pc + j) >= '0') && (*(pc + j) <= '9') )
      --j;
   ++j;

   return -1 * j;
}

#define LOADOBJ_BUFFER_GROWTH 2048
char *loadObjFromFileOffset(char *destBuf, int *lgDestBuf, FILE *pf, int offset, int *lgObj) {
int i;

   if ( destBuf == 0 ) {
      if ( *lgDestBuf == 0 )
         *lgDestBuf = LOADOBJ_BUFFER_GROWTH;
      destBuf = new char[*lgDestBuf];
   }

   fseek(pf, offset, SEEK_SET);
   fread(destBuf, 1, *lgDestBuf, pf);
   while ( (i = findWinBuf("endobj", destBuf, *lgDestBuf)) == -1 ) {

      delete destBuf;
      *lgDestBuf += LOADOBJ_BUFFER_GROWTH;
      destBuf = new char[*lgDestBuf];
      fseek(pf, offset, SEEK_SET);
      fread(destBuf, 1, *lgDestBuf, pf);
   }

   *lgObj = i + 6;

   return destBuf;
}
#undef LOADOBJ_BUFFER_GROWTH

const char *fontNames[] = {
   "/Times-Roman",
   "/Times-Bold",
   "/Times-Italic",
   "/Times-BoldItalic",
   "/Helvetica",
   "/Helvetica-Bold",
   "/Helvetica-Oblique",
   "/Helvetica-BoldOblique",
   "/Courier",
   "/Courier-Oblique",
   "/Courier-BoldOblique",
   "/Courier-Bold",
   "/Symbol",
   "/ZapfDingbats"
};

