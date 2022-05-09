.PHONY: run

run: cminus.exe file
	./cminus < file
cminus.y.cpp cminus.y.h: cminus.y
	bison --defines='cminus.y.h' --report=all --report-file=cminus.report -o 'cminus.y.cpp' 'cminus.y'
cminus.l.cpp: cminus.l cminus.y.h
	lex -t 'cminus.l' > 'cminus.l.cpp'
cminus.exe: cminus.y.cpp cminus.l.cpp
	g++ 'cminus.y.cpp' 'cminus.l.cpp' '-LD:\cygwin64\usr\local\lib' '-lstdc++' '-ljsoncpp' -o 'cminus'

clean:
	rm -f cminus.y.h *.c *.exe cminus.report