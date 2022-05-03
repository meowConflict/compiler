lex: yacc.y lex.l
	bison -vdty  yacc.y
	flex lex.l
	gcc -o lex y.tab.c lex.yy.c -ly -lfl	
clean:
	rm lex lex.o y.tab.c lex.yy.c \
	y.output y.tab.h
