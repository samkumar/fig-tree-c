CFLAGS=-O0 -ggdb3 -Wall -Wextra -Werror -Wpedantic -pedantic-errors -std=gnu11

all: figtree_test

figtree_test: main.o figtree.o interval.o utils.o
	gcc $(CFLAGS) main.o figtree.o interval.o utils.o -o figtree_test

main.o:
	gcc $(CFLAGS) -c main.c -o main.o

figtree.o:
	gcc $(CFLAGS) -c figtree.c -o figtree.o

interval.o:
	gcc $(CFLAGS) -c interval.c -o interval.o

utils.o:
	gcc $(CFLAGS) -c utils.c -o utils.o

clean:
	rm -f figtree_test *.o *~
