all:
	bison --defines='cminus.y.h' --report=all --report-file=cminus.report -o 'cminus.y.c' 'cminus.y'
	lex -t 'cminus.l' >'cminus.l.c'
	gcc 'cminus.y.c' 'cminus.l.c' -o 'cminus'
	./cminus < file

clean:
	rm -f *.h *.c cminus cminus.report
