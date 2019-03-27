I=-I/local/courses/csse2310/include
L=-L/local/courses/csse2310/lib -la4
FLAG=-Wall -pedantic -std=gnu99 -g
THREAD=-pthread
rafiki:rafiki.o error.o tools.o zazu gopher
	gcc ${FLAG} ${I} ${L} ${THREAD} -o rafiki rafiki.o error.o tools.o
rafiki.o:rafiki.c rafiki.h
	gcc ${FLAG} ${I} ${L} -c rafiki.c
zazu:zazu.o error.o tools.o
	gcc ${FLAG} ${I} ${L} -o zazu zazu.o error.o tools.o
zazu.o:zazu.c zazu.h 
	gcc ${FLAG} ${I} ${L} -c zazu.c
gopher:gopher.o error.o tools.o
	gcc ${FLAG}  -o gopher gopher.o error.o tools.o

gopher.o:gopher.c error.o tools.o
	gcc ${FLAG} -c gopher.c
tools.o:tools.c
	gcc ${FLAG} -c tools.c
error.o:error.c
	gcc ${FLAG} -c error.c
clean:
	rm -rf *.o
	rm -rf rafiki
	rm -rf gopher
	rm -rf zazu
