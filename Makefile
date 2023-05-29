all: compiler

compiler: compiler.o hash.o
	g++ compiler.o hash.o -o compiler

compiler.o: compiler.c
	gcc --std=c99 -c compiler.c -o compiler.o

hash.o: hash.c hash.h
	gcc --std=c99 -c hash.c -o hash.o

test_add_recursive: test_add_recursive.c add_recursive.o
	gcc test_add_recursive.c add_recursive.o -o test_add_recursive

add_recursive.o: add_recursive.ll
	llc -filetype=obj add_recursive.ll

clean:
	rm -f test_add_recursive compiler *.o
