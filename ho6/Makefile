CC=gcc
FLAGS= -W -Wall -O2

all: demo server_tcp_epoll

demo:
	$(CC) server_epoll_demo.c -o server_epoll_demo

server_tcp_epoll:
	$(CC) $(FLAGS) server_tcp_epoll.c -o server_tcp_epoll

clean:
	rm -rf server_tcp_epoll server_epoll_demo

PHONY:
	server_epoll_demo server_tcp_epoll
