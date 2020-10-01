PORT=53691
CFLAGS = -DPORT=$(PORT) -g -Wall -std=gnu99

all : hcq_server

hcq_server: hcq_server.o hcq.o
	gcc $(CFLAGS) -o hcq_server hcq_server.o hcq.o

hcq_server.o: hcq_server.c hcq.h
	gcc $(CFLAGS) -c hcq_server.c hcq.h

hcq.o: hcq.c hcq.h
	gcc $(CFLAGS) -c hcq.c

clean: 
	rm hcq_server hcq_client *.o
