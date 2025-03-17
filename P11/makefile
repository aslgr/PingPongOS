CFLAGS = -Wall -g
LDFLAGS = -lm

CC = gcc

objects = pingpong-prodcons.o ppos_core.o queue.o

all: pingpong-prodcons

pingpong-prodcons: $(objects)
	$(CC) $^ -o $@ $(LDFLAGS)

pingpong-prodcons.o: pingpong-prodcons.c ppos.h
	$(CC) -c $< $(CFLAGS)

ppos_core.o: ppos_core.c ppos.h queue.h
	$(CC) -c $< $(CFLAGS)

queue.o: queue.c queue.h
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f $(objects) pingpong-prodcons