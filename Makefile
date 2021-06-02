bmp.o:
	gcc -g -Wall -c bmp.c -lm

transformations.o:
	gcc -g -Wall -c transformations.c -lm

all: clean bmp.o transformations.o
	gcc -g -Wall -c main.c -lm
	gcc -g -Wall main.o bmp.o transformations.o -o bmp -lm

test: clean bmp.o transformations.o
	gcc -Wall -c test_main.c -lm
	gcc -Wall test_main.o bmp.o transformations.o -o test_bmp -lm

clean:
	rm -f *o all
