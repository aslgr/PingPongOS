CFLAGS = -Wall -g
LDFLAGS = -lm

CC = gcc

objects = testafila.o queue.o

all: testafila

testafila: $(objects)
	$(CC) $^ -o $@ $(LDFLAGS)

testafila.o: testafila.c queue.h
	$(CC) -c $< $(CFLAGS)

queue.o: queue.c queue.h
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f $(objects) testafila