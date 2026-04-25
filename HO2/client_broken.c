#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#define PORT 8080

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

ssize_t send_all(int sockfd, void *buf, size_t n);

message *create_message(const char *payload, uint32_t len) {
  message *msg = malloc(sizeof(message));
  if (!msg)
    return NULL;
  msg->payload = malloc(len + 1);
  if (!msg->payload) {
    free(msg);
    return NULL;
  }
  memcpy(msg->payload, payload, len);
  msg->payload[len] = '\0';
  msg->len = len;
  msg->checksum = checksum(msg->payload);
  return msg;
}

void free_message(message *msg) {
  if (!msg)
    return;
  free(msg->payload);
  free(msg);
}

int send_message(int sockfd, const message *msg) {
  printf("[CLIENT.c] Checksum inviato: %d\n", msg->checksum);
  uint32_t net_len = htonl(msg->len);
  if (send_all(sockfd, &net_len, sizeof(uint32_t)) < 0)
    return -1;
  if (send_all(sockfd, (void *)msg->payload, msg->len) < 0)
    return -1;
  if (send_all(sockfd, (void *)&msg->checksum, sizeof(uint8_t)) < 0)
    return -1;
  return 0;
}

ssize_t send_all(int sockfd, void *buf, size_t n) {
  ssize_t s = 0;

  while ((size_t)s < n) {
    // Casto a char per fare artitmetica di puntatori
    // s scorre il buffer man mano che invio bytes
    // n - s è la nuova lunghezza del buffer
    ssize_t sent = send(sockfd, (char *)buf + s, n - s, 0);
    if (sent == -1) {
      perror("[ERROR] send errore: ");
      return -1;
    }
    if (sent == 0) {
      printf("[DEBUG] connessione chiusa!\n");
      return 0;
    }
    s += sent;
    printf("[DEBUG] r=%ld\n", s);
  }

  return n;
}

int main(void) {

  FILE *divina_fd = fopen("divina_commedia.txt", "r");
  if (divina_fd == NULL) {
    perror("Impossibile aprire file: ");
    exit(1);
  }

  char divina[64 * 1024];
  size_t byte_letti = fread(divina, 1, sizeof(divina) - 1, divina_fd);
  divina[byte_letti] = '\0';

  printf("[DEBUG] Lunghezza divina_commedia.txt: %ld bytes\n", strlen(divina));

  int sockfd;
  struct sockaddr_in addr;
  char *server_addr = "192.168.8.230";

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  inet_pton(AF_INET, (char *)server_addr, &addr.sin_addr);

  connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));

  while (1) {
    // message *msg =
    //     create_message("Ciao col protocollo!", strlen("Ciao col
    //     protocollo!"));
    // printf("[CLIENT.c] Inviando: %s\n", msg->payload);
    // send_message(sockfd, msg);
    // free_message(msg);
    // sleep(2);

    message *msg_big = create_message(divina, byte_letti);
    printf("[CLIENT.c] Inviando divina_commedia.txt (%u bytes)\n",
           msg_big->len);
    send_message(sockfd, msg_big);
    free_message(msg_big);
    sleep(2);
  }

  close(sockfd);
  return 0;
}
