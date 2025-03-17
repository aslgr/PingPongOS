CFLAGS = -Wall -g -lrt
LDFLAGS = -lm

CC = gcc

objects = teste.o ppos_core.o queue.o disk.o ppos_disk.o

all: teste

teste: $(objects)
	$(CC) $^ -o $@ $(LDFLAGS)

teste.o: teste.c ppos.h ppos_disk.h
	$(CC) -c $< $(CFLAGS)

ppos_core.o: ppos_core.c ppos.h queue.h
	$(CC) -c $< $(CFLAGS)

queue.o: queue.c queue.h
	$(CC) -c $< $(CFLAGS)

disk.o: disk.c disk.h
	$(CC) -c $< $(CFLAGS)

ppos_disk.o: ppos_disk.c ppos.h ppos_data.h
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f $(objects) teste