#makefile
BORLAND=c:\borland\bcc55
OUT=out
BIN=bin

STARTUPOBJ=c0x32.obj
LIBS = pcre.lib import32.lib cw32.lib
INCLUDEPATH=$(BORLAND)\include;.
LIBPATH=$(BORLAND)\lib
CC=$(BORLAND)\bin\bcc32
LL=$(BORLAND)\bin\ilink32
CFLAGS=-c -w -I$(INCLUDEPATH) -L$(LIBPATH) -tWC -O2 -DWIN32 -n$(OUT) -w-8071
#CFLAGS=-c -w -H=test.csm -I$(INCLUDEPATH) -L$(LIBPATH) -tWC -O2 -DWIN32 -n$(OUT)
LFLAGS=-ap -Tpe -L$(LIBPATH) -x -j$(OUT) -I$(OUT) -Gn
RFLAGS= -32

OBJ=main.obj listeFichiers.obj pdfXrefTable.obj pdfFile.obj diversPdf.obj pdfString.obj md5.obj rc4.obj strMatcher.obj pdfEncrypt.obj pdfFileOutline.obj pdfNames.obj pdfObject.obj calc.obj pdfUtils.obj
OBJfr=string.obj
OBJen=stringEn.obj
OBJzlib = adler32.obj compress.obj crc32.obj deflate.obj gzio.obj infback.obj inffast.obj inflate.obj inftrees.obj trees.obj uncompr.obj zutil.obj

AllFiles: $(BIN)\mbtPdfAsm.exe $(BIN)\mbtPdfAsmEn.exe

string.obj: string.cpp
    $(CC) $(CFLAGS) -DLANG_FR string.cpp

stringEn.obj: string.cpp
    $(CC) $(CFLAGS) -DLANG_EN -o$(OUT)\stringEn string.cpp
    
.c.obj:
    $(CC) $(CFLAGS) $<

.cpp.obj:
    $(CC) $(CFLAGS) $<
    
adler32.obj: adler32.c zlib.h zconf.h

compress.obj: compress.c zlib.h zconf.h

crc32.obj: crc32.c zlib.h zconf.h crc32.h

deflate.obj: deflate.c deflate.h zutil.h zlib.h zconf.h

gzio.obj: gzio.c zutil.h zlib.h zconf.h

infback.obj: infback.c zutil.h zlib.h zconf.h inftrees.h inflate.h inffast.h inffixed.h

inffast.obj: inffast.c zutil.h zlib.h zconf.h inftrees.h inflate.h inffast.h

inflate.obj: inflate.c zutil.h zlib.h zconf.h inftrees.h inflate.h inffast.h inffixed.h

inftrees.obj: inftrees.c zutil.h zlib.h zconf.h inftrees.h

trees.obj: trees.c zutil.h zlib.h zconf.h deflate.h trees.h

uncompr.obj: uncompr.c zlib.h zconf.h

zutil.obj: zutil.c zutil.h zlib.h zconf.h

$(BIN)\mbtPdfAsm.exe: $(OBJ) $(OBJfr) $(OBJzlib)
    $(LL) $(LFLAGS) $(STARTUPOBJ) $(OBJ) $(OBJfr) $(OBJzlib), $(BIN)\mbtPdfAsm.exe, , $(LIBS), ,

$(BIN)\mbtPdfAsmEn.exe: $(OBJ) $(OBJen) $(OBJzlib)
    $(LL) $(LFLAGS) $(STARTUPOBJ) $(OBJ) $(OBJen) $(OBJzlib), $(BIN)\mbtPdfAsmEn.exe, , $(LIBS), ,
