.PHONY: all touch

CC=gcc
CFLAGS=-Wall -pedantic -std=gnu99 -pthread -lm

DEPS=grain.h
OBJ=grain.o solvelp.o
EXECUTABLE=solvelp

all: $(EXECUTABLE)

debug: CFLAGS+= -DDEBUG -g
debug: all

solvelp: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

touch:
	touch *

clean:
	rm $(OBJ)
	rm $(EXECUTABLE)
