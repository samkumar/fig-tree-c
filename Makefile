# This is based on Makefiles from CS 162 homework assignments

SRCS=main.c figtree.c figtreenode.c interval.c utils.c
EXECUTABLES=figtree_test

CC=gcc
CFLAGS=-O0 -ggdb3 -Wall -Wextra -Werror -Wpedantic -pedantic-errors -std=gnu11
LDFLAGS=

OBJS=$(SRCS:.c=.o)

all: $(EXECUTABLES)

$(EXECUTABLES): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXECUTABLES) $(OBJS) *~
