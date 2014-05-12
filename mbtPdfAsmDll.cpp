#include "stdafx.h"

#define DLL_EXPORTS
#include "mbtPdfAsmDll.h"

DLL_API CSmbtPdfAsm::CSmbtPdfAsm(int argc, char *argv[]) {
   this->errorFlags = 0;
   this->resBuf = 0;
   this->resBufSize = 0;

   this->mbtPdfAsm(argc, argv);
}

DLL_API CSmbtPdfAsm::~CSmbtPdfAsm() {
   if ( this->resBuf )
      delete this->resBuf;
}


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
