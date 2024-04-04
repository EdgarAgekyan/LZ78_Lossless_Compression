SHELL := /bin/sh
CC = clang
CFLAGS = -Wall -g -Wpedantic -Werror -Wextra -Wstrict-prototypes $(shell pkg-config --cflags gmp) -gdwarf-4
LFLAGS = $(shell pkg-config --libs gmp)

all: encode decode

encode: encode.o trie.o word.o io.o
	$(CC) -o $@ $^ $(LFLAGS)

decode: decode.o trie.o word.o io.o
	$(CC) -o $@ $^ $(LFLAGS)

encode.o: encode.c trie.o word.o io.o
	$(CC) $(CFLAGS) -c $<

decode.o: decode.c trie.o word.o io.o
	$(CC) $(CFLAGS) -c $<

scan-build: clean
	scan-build --use-cc=$(CC) make

clean:
	rm -f encode decode *.o

format:
	clang-format -i -style=file *.[ch]

