/*
CSmbtPdfAsm(int argc, char *argv[]);
------------------------------------
This constructor takes parameter like a C/C++ runtime presents a command line to the main entry of a program.
argc is the number of string in argv. The strings of argv must be formated as presented at
http://thierry.schmit.free.fr/dev/mbtPdfAsm/enMbtPdfAsm2.html
Use the -S option to prevent any display.
exemple:
argv[] = {"-S", "-M*.pdf", "-dres.pdf"};
argc = 3;


inline int getErrorFlags() {return this->errorFlags;}
-----------------------------------------------------
see mbtPdfAsmError.h
note that errorFlags may report more than one error


inline char *getResBuf() {return this->resBuf;}
inline int getResBufSize() {return this->resBufSize;}
------------------------------------------------------
return a null terminated string comprising the result of the GET_MODE


*/
#ifndef __MBTPDFASMDLL_H__
#define __MBTPDFASMDLL_H__

#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#include "mbtPdfAsmError.h"

class DLL_API CSmbtPdfAsm {
private:
    int mbtPdfAsm(int argc, char *argv[]);
    int errorFlags;
    char *resBuf;
    int resBufSize;
public:
   CSmbtPdfAsm(int argc, char *argv[]);
   ~CSmbtPdfAsm();

   inline int getErrorFlags() {return this->errorFlags;}
   inline char *getResBuf() {return this->resBuf;}
   inline int getResBufSize() {return this->resBufSize;}
};

#endif