@echo on

set PARAMS=%~1

cls
nmake -nologo -f Makefile.msc %PARAMS%
