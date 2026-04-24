CC      = gcc
CFLAGS  = -Wall -Wextra -g

all: server_broken client_broken

server_broken: server_broken.c
	$(CC) $(CFLAGS) -o server_broken server_broken.c

client_broken: client_broken.c
	$(CC) $(CFLAGS) -o client_broken client_broken.c

clean:
	rm -f server_broken client_broken
