#ifndef __DIVERS_PDF_HPP__
#define __DIVERS_PDF_HPP__

#include <stdio.h>

#include "strMatcher.hpp"
extern strMatcher X_X_R;
extern strMatcher f_X_X_R;

typedef struct {
   const char *nom;
   int        code;
   int        optional;
   bool       inherited;
   int        type;
} T_keyWord;

#define TYPE_OPTIONAL 1
#define TYPE_REQUIRED 2

#define TYPE_ARRAY      3
#define TYPE_DICTIONARY 4
#define TYPE_BOOLEAN    5
#define TYPE_NUMERIC    6
#define TYPE_STRING     7
#define TYPE_STREAM     8
#define TYPE_NULL       9
#define TYPE_INDIRECT   10
#define TYPE_NAME       11
#define TYPE_DATE       12
#define TYPE_RECTANGLE  13
#define TYPE_INTEGER    14
#define TYPE_NUMBER     15

extern T_keyWord pageKeyWords[];
extern T_keyWord resourcesKeyWords[];
extern T_keyWord noKeyWord[];
extern T_keyWord xrefObjKeyWords[];
extern T_keyWord filterKeyWords[];
extern T_keyWord compressedKeyWords[];
extern T_keyWord fontKeyWords[];
extern T_keyWord actionKeyWords[];
extern T_keyWord typeKeyWordsValues[];

#define _notAType       0x0000

#define _Type           0x0001
#define _Parent         0x0002
#define _LastModified   0x0003
#define _Contents       0x0004
#define _Resources      0x0005
#define _MediaBox       0x0006
#define _CropBox        0x0007
#define _BleedBox       0x0008
#define _TrimBox        0x0009
#define _ArtBox         0x000A
#define _BoxColorInfo   0x000B
#define _Rotate         0x000C
#define _Group          0x000D
#define _Thumb          0x000E
#define __B             0x000F
#define _Dur            0x0010
#define _Trans          0x0011
#define _Annots         0x0012
#define _AA             0x0013
#define _Metadata       0x0014
#define _PieceInfo      0x0015
#define _ID             0x0016
#define _PZ             0x0017
#define _SeparationInfo 0x0018
#define _ColorSpace     0x0019
#define _StructParents  0x001A
#define _BCLPrivAnnots  0x001B
#define _Content        0x001C

//constante pour les resources
#define _cstResources 0x001C
#define _ProcSet       (_cstResources + 1)
#define _Font          (_cstResources + 2)
#define _ExtGState     (_cstResources + 3)
//#define _ColorSpace    (_cstResources + 4)
#define _Pattern       (_cstResources + 5)
#define _Shading       (_cstResources + 6)
#define _XObject       (_cstResources + 7)
#define _Properties    (_cstResources + 8)

//constantes pour l'objets Xref
#define _cstObjXref (_cstResources + 8)
#define __Size    (_cstObjXref + 1)
#define _Root    (_cstObjXref + 2)
#define __Index   (_cstObjXref + 3)
#define __W      (_cstObjXref + 4)
#define __Filter (_cstObjXref + 5)
#define _Prev    (_cstObjXref + 6)
#define _Length  (_cstObjXref + 7)
#define _Info    (_cstObjXref + 8)
//#define _ID      (_cstObjXref + 9)
#define _ModDate (_cstObjXref + 10)
#define _Encrypt (_cstObjXref + 11)

//constantes pour les filtres
#define _cstDecode (_cstObjXref + 12)
#define _ASCIIHexDecode  (_cstDecode + 1)
#define _ASCII85Decode   (_cstDecode + 2)
#define _LZWDecode       (_cstDecode + 3)
#define _FlateDecode     (_cstDecode + 4)
#define _RunLengthDecode (_cstDecode + 5)
#define _CCITTFaxDecode  (_cstDecode + 6)
#define _JBIG2Decode     (_cstDecode + 7)
#define _DCTDecode       (_cstDecode + 8)
#define _JPXDecode       (_cstDecode + 9)
#define _Crypt           (_cstDecode + 10)   

//constante pour les objets compressés
#define _cstCompressed (_cstDecode + 11 )
#define __N_   (_cstCompressed + 1)
#define _First (_cstCompressed + 2)

//constante pour les fonts
#define _cstFonts (_cstCompressed + 3)
//#define _Type           (_cstFonts + 1)
#define _Subtype        (_cstFonts + 2)
#define __Name          (_cstFonts + 3)
#define _BaseFont       (_cstFonts + 4)
#define _FirstChar      (_cstFonts + 5)
#define _LastChar       (_cstFonts + 6)
#define _Widths         (_cstFonts + 7)
#define _FontDescriptor (_cstFonts + 8)
#define _Encoding       (_cstFonts + 9)
#define _ToUnicode      (_cstFonts + 10)

//constante pour les fonts
#define _cstFontsValues (_cstFonts + 11)

//constante pour les actions
#define _cstActions (_cstFonts + 12)
//#define _Type           (_cstActions + 1)
#define __S (_cstActions + 2)
#define _D (_cstActions + 3)
#define _JS (_cstActions + 4)

extern int isWaKey(char *, T_keyWord *);
extern int tailleObj(char *);
extern int keyLen(int idKey, T_keyWord *pKW);
extern int tailleAttrib(const char *pc);
extern const char *nomAttrib(int idKey, T_keyWord *pKW);
extern int rewindRef(char *pc);

extern char *loadObjFromFileOffset(char *destBuf, int *lgDestBuf, FILE *pf, int offset, int *lgObj);

extern const char *fontNames[];
#define FONT_BOLD         1
#define FONT_OBLIQUE      2
#define TIMES_FONT                  0
#define TIMES_FONT_BOLD             (TIMES_FONT + FONT_BOLD)
#define TIMES_FONT_OBIQUE           (TIMES_FONT + FONT_OBLIQUE)
#define TIMES_FONT_BOLD_OBLIQUE     (TIMES_FONT + FONT_BOLD + FONT_OBLIQUE)
#define HELVETICA_FONT              4
#define HELVETICA_FONT_BOLD         (HELVETICA_FONT + FONT_BOLD)
#define HELVETICA_FONT_OBIQUE       (HELVETICA_FONT + FONT_OBLIQUE)
#define HELVETICA_FONT_BOLD_OBLIQUE (HELVETICA_FONT + FONT_BOLD + FONT_OBLIQUE)
#define COURIER_FONT                8
#define COURIER_FONT_BOLD           (COURIER_FONT + FONT_BOLD)
#define COURIER_FONT_OBIQUE         (COURIER_FONT + FONT_OBLIQUE)
#define COURIER_FONT_BOLD_OBLIQUE   (COURIER_FONT + FONT_BOLD + FONT_OBLIQUE)
#define SYMBOL_FONT                 12
#define ZAPFDINGBATS_FONT           13

#endif
