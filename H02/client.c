#include "fibo.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_ADDR "0.0.0.0"
#define PORT 8080

char int_to_char(int n) { return '0' + n; }

int char_to_int(char c) { return c - '0'; }

void print_usage(void) { printf("Usage: client_tcp [Fibonacci Number n]\n"); }

int main(int argc, char **argv) {

  if (argc < 2) {
    print_usage();
    return 0;
  }
  if (argc > 3) {
    printf("Too many arguments...\n");
    print_usage();
    return 0;
  }

  int n = atoi(argv[1]);
  int reps = (argc == 3) ? atoi(argv[2]) : 1;

  // Genera la sequenza di Fibonacci fino a F(n) e la converte in stringa
  // Es. n=5 -> "0 1 1 2 3 5\n"
  fibo_sequence_t fs;
  fibo_init(&fs);
  fibo_sequence(&fs, n);
  char msg[1024];
  int cur = 0;
  for (int i = 0; i < fs.idx; i++) {
    if (i > 0) msg[cur++] = ' ';
    msg[cur++] = int_to_char(fs.array[i]);
  }
  msg[cur++] = '\n';
  msg[cur] = '\0';

  printf("[CLIENT.c] Sequenza di fibonacci: %s", msg);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr);

  // SOCK_STREAM crea un socket TCP orientato alla connessione
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    perror("Errore nella creazione del socket");
    exit(1);
  }

  // connect() stabilisce la connessione TCP con il server (three-way handshake)
  if (connect(client_fd, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) == -1) {
    perror("Errore nella connessione");
    exit(1);
  }

  // Invia il messaggio reps volte sulla stessa connessione e attende ACK/NACK
  for (int i = 0; i < reps; i++) {
    if (write(client_fd, msg, strlen(msg)) == -1) {
      perror("Errore nel socket");
      exit(1);
    }

    char buf[1024];
    int r = read(client_fd, buf, sizeof(buf));
    buf[r] = '\0';
    printf("[CLIENT.c] %s:%d: %s", SERVER_ADDR, PORT, buf);

    sleep(1);
  }

  // Chiude la connessione dopo aver inviato tutti i messaggi
  close(client_fd);

  return 0;
}
