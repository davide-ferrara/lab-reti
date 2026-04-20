#include "fibo.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

/*
 * Parsa il messaggio ricevuto dal client e popola la struttura fibo_sequence_t.
 * I numeri sono separati da spazi. Caratteri \n e \r vengono ignorati.
 * Ogni cifra viene convertita da char a int sottraendo '0' (valore ASCII base).
 */
void parse_msg(char *msg, fibo_sequence_t *fs) {
  fibo_init(fs);

  while (*msg != '\0') {
    if (*msg == ' ' || *msg == '\n' || *msg == '\r') {
      msg++;
      continue;
    }

    int n = *msg - '0';
    fibo_push(fs, n);

    msg++;
  }
}

int main() {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  // SOCK_STREAM crea un socket TCP orientato alla connessione
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("Errore nella creazione del socket");
    exit(1);
  }

  // Evita "Address already in use" al riavvio: TCP tiene il socket
  // in TIME_WAIT per ~2 minuti dopo la chiusura
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Associa il socket all'indirizzo e porta configurati
  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("Errore nel bind del socket");
    exit(1);
  }

  // Il backlog di 5 indica quante connessioni il kernel può accodare
  // mentre il server è occupato a servire un client
  if (listen(server_fd, 5) == -1) {
    perror("Error during listening");
  }
  printf("[TPC_SERVER_BLOCKING.c] Server is listening...\n");

  struct sockaddr_in client_addr;
  socklen_t addrlen = sizeof(client_addr);
  char buf[1024];

  // Loop esterno: il server non termina mai, accetta un client alla volta
  while (1) {
    // accept() blocca finché un client non si connette.
    // Ritorna un nuovo fd dedicato a quella connessione.
    // Finché siamo qui dentro, nessun altro client viene accettato.
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd == -1) {
      perror("Could not accept connection");
      continue;
    }

    // Loop interno: legge più messaggi dalla stessa connessione
    // finché il client non si disconnette
    while (1) {
      // read() blocca finché non arrivano dati.
      // Ritorna 0 quando il client chiude la connessione (EOF).
      ssize_t n = read(client_fd, buf, sizeof(buf));
      if (n == 0) {
        printf("Il Client si è disconnesso!\n");
        close(client_fd);
        break;
      }
      if (n == -1) {
        printf("Errore del client!\n");
        break;
      }

      char ip_str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
      uint16_t port = ntohs(client_addr.sin_port);

      buf[n] = '\0'; // aggiunge terminatore per usare buf come stringa C

      printf("[SERVER_TPC_BLOCK.c] %s:%d: %s", ip_str, port, buf);

      fibo_sequence_t fs, client_fs;
      memset(&fs, 0, sizeof(fibo_sequence_t));
      fibo_init(&fs);

      // Parsa la stringa ricevuta e genera la sequenza attesa
      parse_msg(buf, &client_fs);
      fibo_sequence(&fs, client_fs.idx - 1);

      if (fibo_compare(&fs, &client_fs) == 0) {
        char *resp = "ACK\n";
        write(client_fd, resp, strlen(resp));
        printf("[SERVER_TPC_BLOCK.c] Sent response to %s:%d: %s", ip_str, port,
               resp);
      } else {
        char *resp = "NACK\n";
        write(client_fd, resp, strlen(resp));
        printf("[SERVER_TPC_BLOCK.c] Sent response to %s:%d: %s", ip_str, port,
               resp);
      }
      // non chiudiamo: il client può inviare altri messaggi
    }
  }

  return 0;
}
