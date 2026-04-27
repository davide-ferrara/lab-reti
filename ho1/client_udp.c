#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ADDR "127.0.0.1"
#define PORT 8080
#define BUF_SIZE 1024

int main(void) {
  /*
   * Procedura analoga per il server, alloco il socker per il client
   * in questo caso ma la logica rimane identica
   * */
  int client_fd;
  client_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (client_fd == -1) {
    perror("Errore client socket");
    exit(1);
  }

  // Struttra dati per il server
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ADDR);
  server_addr.sin_port = htons(PORT);

  printf("Avvio del Client...\n");

  // Invio ping -> ricevo dal server -> aspetto 1 secondo prima di ripetere
  char buf[BUF_SIZE];
  while (1) {
    sendto(client_fd, "ping\n", 5, 0, (struct sockaddr *)&server_addr,
           sizeof(server_addr));

    printf("Inviato a %s:%d: ping\n", ADDR, PORT);

    // Chiamata bloccante
    ssize_t n = recvfrom(client_fd, buf, sizeof(buf) - 1, 0, NULL, NULL);
    if (n > 0) {
      buf[n] = '\0';

      printf("Ricevuto da %s:%d: %s\n", ADDR, PORT, buf);
    }

    sleep(1);
  }

  // Chiudura del file descriptor
  close(client_fd);
  return 0;
}
