CC=gcc
FLAGS=-Wall -Wextra
LIBS=-pthread

all: sum mapreduce

sum: sum.c
	$(CC) $(FLAGS) $(LIBS) -o sum sum.c

mapreduce: mapreduce.c
	$(CC) $(FLAGS) -fsanitize=thread $(LIBS) -o mapreduce mapreduce.c

clean:
	rm -f sum mapreduce
