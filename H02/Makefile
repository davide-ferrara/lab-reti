CC = gcc

.PHONY: all test clean

all: server_tcp_block server_tcp_nonblock client

server_tcp_block:
	$(CC) -Wall -Wextra -o server_tcp_block fibo.c server_tcp_block.c

server_tcp_nonblock:
	$(CC) -Wall -Wextra -o server_tcp_nonblock fibo.c server_tcp_nonblock.c

client:
	$(CC) -Wall -Wextra -o client fibo.c client.c

test:
	-while true; do echo "0 1 1 2 3" | nc 0.0.0.0 8080; sleep 1; done

clean:
	rm server_tcp_block server_tcp_nonblock client
