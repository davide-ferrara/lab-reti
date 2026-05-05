CC     = gcc
CFLAGS = -Wall -O2

all: server_tcp_select server_select_demo

server_tcp_select: server_tcp_select.c
	$(CC) $(CFLAGS) -o server_tcp_select server_tcp_select.c

server_select_demo: server_select_demo.c
	$(CC) $(CFLAGS) -o server_select_demo server_select_demo.c

clean:
	rm -rf server_tcp_select server_select_demo

.PHONY: all clean
