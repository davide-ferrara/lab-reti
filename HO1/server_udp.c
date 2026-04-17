#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUF_SIZE 1024

int main(void) {
  /*
   * Alloco nello stack e inizializzo a 0 la struttura sockaddr_in
   * specifica per IPv4 (a differenza della generica sockaddr), essa
   * conterrà varie informazioni sul socket come l'indirizzo e porta.
   *
   * INADDR_ANY = 0.0.0.0
   * Il server ascolta su TUTTE le interfacce di rete.
   * */
  struct sockaddr_in addr_in;
  memset(&addr_in, 0, sizeof(addr_in));
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(PORT);
  addr_in.sin_addr.s_addr = INADDR_ANY;

  /*
   * Creo il nuovo socket specificando il dominio di comunicazione AF_INET
   * ovvero la famiglia di indirizzi IPv4, il tipo di socket nel caso di un
   * server UDP uno di tipo Datagram e il protocollo di default con 0
   *
   * UDP è connectionless: niente connect()/listen()/accept()
   * ma rcvfrom() e sendto()
   * */
  int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd == -1) {
    perror("Creazione socket fallita!");
    exit(1);
  }

  // Associa il socket ad indirizzo e porta
  if (bind(server_fd, (struct sockaddr *)&addr_in, sizeof(addr_in)) == -1) {
    perror("Errore nel bind");
    close(server_fd);
    exit(1);
  }

  printf("Server UDP in ascolto sulla porta %d...\n", PORT);

  /*
   * Alloco nello stack il buffer per ricevere i dati e la struttura
   * sockaddr_in per memorizzare le informazioni sul client
   * */
  char buf[BUF_SIZE];
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  while (1) {
    // Chiamata bloccante per ascolto che riceve datagrammi UDP
    ssize_t n = recvfrom(server_fd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);
    if (n > 0) {
      buf[n] = '\0';

      printf("Ricevuto da %s:%d: %s\n", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port), buf);

      // Rispondo al client
      char *msg = "pong\n";
      sendto(server_fd, msg, strlen(msg), 0, (struct sockaddr *)&client_addr,
             client_len);
    }
  }

  // Chiudo file descriptor e termino programma
  close(server_fd);
  return 0;
}
