CC = gcc
CFLAGS = -Wall -g -pthread
TARGETS = server client

all: $(TARGETS)

server: server.o logger.o monitor.o
	$(CC) $(CFLAGS) -o server server.o logger.o monitor.o

client: client.o
	$(CC) $(CFLAGS) -o client client.o

# Build object files
server.o: server.c logger.h monitor.h
	$(CC) $(CFLAGS) -c server.c

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) -c logger.c

monitor.o: monitor.c monitor.h
	$(CC) $(CFLAGS) -c monitor.c

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f $(TARGETS) *.o chat.log

# Rebuild target for convenience
rebuild: clean all

.PHONY: all clean rebuild
