.PHONY: all
all:
	cl main.cpp reg.cpp /nologo /EHsc /std:c++latest /Zi /W4  /Feoutput/reg.exe /link advapi32.lib
