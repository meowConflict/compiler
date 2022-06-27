.PHONY: run

run: cminus file.c
	./cminus < file.c
	llvm-as a.ll
	llc a.bc
	clang a.s
	./a.out
cminus.y.cpp cminus.y.h: cminus.y
	bison --defines='cminus.y.h' --report=all --report-file=cminus.report -o 'cminus.y.cpp' 'cminus.y'
cminus.l.cpp: cminus.l cminus.y.h
	lex -t 'cminus.l' > 'cminus.l.cpp'
# tmp.o: ast.cpp
# 	g++ $< -std=c++20 -c -o $@
cminus: cminus.y.cpp cminus.l.cpp main.cpp jsonGen.cpp ast.cpp ast.hpp
	g++ cminus.y.cpp cminus.l.cpp main.cpp jsonGen.cpp ast.cpp *.a -std=c++17 `llvm-config --cxxflags` -DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1 `llvm-config --ldflags --system-libs --libs all` '-lstdc++' -ljsoncpp -lfl -o $@

clean:
	rm -f cminus.y.h *.c *.exe cminus.report
