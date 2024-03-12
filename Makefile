.PHONY: all
all:
	cl main.cpp reg.cpp /nologo /EHsc /std:c++latest /Zi /W4 /Fdoutput/ /Fooutput/ /Feoutput/main.exe /link advapi32.lib

.PHONY: clean
clean:
	del /q output\*
