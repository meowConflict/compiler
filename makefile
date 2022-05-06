.PHONY: run

run: cminus.exe file
	./cminus < file
cminus.y.c cminus.y.h: cminus.y
	bison --defines='cminus.y.h' --report=all --report-file=cminus.report -o 'cminus.y.c' 'cminus.y'
cminus.l.c: cminus.l cminus.y.h
	lex -t 'cminus.l' > 'cminus.l.c'
cminus.exe: cminus.y.c cminus.l.c
	gcc 'cminus.y.c' 'cminus.l.c' -o 'cminus'

clean:
	rm -f *.h *.c *.exe cminus.report