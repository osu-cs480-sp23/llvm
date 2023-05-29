all: compiler

compiler: compiler.o hash.o
	g++ compiler.o hash.o -o compiler

compiler.o: compiler.c
	gcc --std=c99 -c compiler.c -o compiler.o

hash.o: hash.c hash.h
	gcc --std=c99 -c hash.c -o hash.o

clean:
	rm -f compiler *.o
