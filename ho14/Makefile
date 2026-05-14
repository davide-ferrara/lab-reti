CC=gcc
FLAGS=-Wall -Wextra
LIBS=-pthread

all: banca rubrica stampante

banca: banca.c
	$(CC) $(FLAGS) $(LIBS) -o banca banca.c

rubrica: rubrica.c
	$(CC) $(FLAGS) -fsanitize=thread $(LIBS) -o rubrica rubrica.c

stampante: stampante.c
	$(CC) $(FLAGS) -fsanitize=thread $(LIBS) -o stampante stampante.c

clean:
	rm -f banca rubrica stampante
