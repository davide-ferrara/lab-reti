CC=gcc
FLAGS=-Wall -Wextra

all: sender receiver chat

sender: sender.c
	$(CC) sender.c -o sender $(FLAGS)

receiver: receiver.c
	$(CC) receiver.c -o receiver $(FLAGS)

chat: chat.c
	$(CC) chat.c -o chat $(FLAGS)

clean:
	rm -rf sender receiver chat

.PHONY: all clean
