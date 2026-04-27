#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define BUFLEN 2048

ssize_t recv_all(int sockfd, void *buf, size_t n);

/*
 * len | payalod
 * 2   | 0x1 0x1
 * */
typedef struct message {
  uint32_t len; /* host byte order */
  char *payload;
  uint8_t checksum;
} message;

// XOR di tutti i byte
uint8_t checksum(char *payload) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < strlen(payload); i++) {
    checksum ^= (uint8_t)payload[i];
  }
  // printf("\nChecksum: %d", checksum);
  return checksum;
}

/*
 * Riceve un messaggio con header di lunghezza.
 * Alloca dinamicamente il buffer e lo restituisce in * out_buf
 * Scrive la lunghezza in * out_len
 * Ritorna 0 in caso di successo , -1 in caso di errore o connessione chiusa
 Il chiamante e ’ responsabile di fare free (* out_buf )
*/
int recv_message(int sockfd, message *msg) {

  if (recv_all(sockfd, &msg->len, sizeof(uint32_t)) <= 0)
    return -1;

  // Converto da len da network a host long
  msg->len = ntohl(msg->len);

  // Alloco la memoria per il buffer
  msg->payload = (char *)malloc(sizeof(char) * msg->len + 1);

  // Leggo il payload
  if (recv_all(sockfd, msg->payload, msg->len) <= 0)
    return -1;

  // Termino la stringa
  msg->payload[msg->len] = '\0';
  printf("[DEBUG.c] len: %d\n", msg->len);

  // Ricevo il checksum dal client
  if (recv_all(sockfd, &msg->checksum, sizeof(uint8_t)) <= 0)
    return -1;

  uint8_t computed = checksum(msg->payload);
  printf("[SERVER.c] Checksum ricevuto: %d | Checksum calcolato: %d\n",
         msg->checksum, computed);

  // Verifico il checksum
  if (!(checksum(msg->payload) && msg->checksum)) {
    printf("[DEBUG] Checksum fallito!\n");
    return -1;
  }
  printf("[DEBUG] Checksum corretto, il messaggio è integro!\n");

  return 0;
}

ssize_t recv_all(int sockfd, void *buf, size_t n) {
  ssize_t r = 0;

  while ((size_t)r < n) {
    // Casto a char per fare artitmetica di puntatori
    // r scorre il buffer man mano che ricevo bytes
    // n - r è la nuova lunghezza del buffer
    ssize_t received = recv(sockfd, (char *)buf + r, n - r, 0);
    if (received == -1) {
      perror("[ERROR] recv errore: ");
      return -1;
    }
    if (received == 0) {
      printf("[DEBUG] connessione chiusa!\n");
      return 0;
    }
    r += received;
    printf("[DEBUG] r=%ld\n", r);
  }

  return n;
}

int main(void) {
  int server_fd, client_fd;
  struct sockaddr_in addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
  listen(server_fd, 5);

  printf("[SERVER.c] Server in ascolto su porta %d\n", PORT);
  client_fd = accept(server_fd, NULL, NULL);

  while (1) {
    message *msg = (message *)malloc(sizeof(message));
    int n = recv_message(client_fd, msg);
    if (n < 0)
      exit(0);

    // Taglio il payload per stamparlo tanto non mi server avere tutto il
    // messaggio, il checksum mi dirà se è integro
    if (strlen(msg->payload) > 64) {
      msg->payload[61] = '.';
      msg->payload[62] = '.';
      msg->payload[63] = '.';
      msg->payload[64] = '\0';
    }
    printf("[SERVER.c] Messaggio: %s\n", msg->payload);
  }

  close(client_fd);
  close(server_fd);
  return 0;
}
