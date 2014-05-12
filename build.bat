@del *.obj
del mbtPdfAsm.exe
if not c%1 == cfull goto notfull
del pcre.lib
:notfull
if exist pcre.lib goto makefile
call make -fmakefilePcre
:makefile
call make
@del *.obj
@del *.#*
@del *.csm
@del *.il*
@del *.map
@del *.tds
