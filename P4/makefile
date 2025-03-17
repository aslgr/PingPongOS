CFLAGS = -Wall -g
LDFLAGS = -lm

CC = gcc

objects = teste.o ppos_core.o queue.o

all: teste

teste: $(objects)
	$(CC) $^ -o $@ $(LDFLAGS)

teste.o: teste.c ppos.h
	$(CC) -c $< $(CFLAGS)

ppos_core.o: ppos_core.c ppos.h queue.h
	$(CC) -c $< $(CFLAGS)

queue.o: queue.c queue.h
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f $(objects) teste