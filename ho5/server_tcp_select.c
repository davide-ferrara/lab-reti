#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 1024
#define BUFLEN 512

int main(void) {
  struct sockaddr_in server_addr = {.sin_family = AF_INET,
                                    .sin_addr.s_addr = INADDR_ANY,
                                    .sin_port = htons(PORT)};

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Errore server_fd:");
    exit(1);
  }

  /* Riutilizza la porta subito dopo la chiusura */
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  socklen_t server_fd_len = sizeof(server_addr);

  if (bind(server_fd, (struct sockaddr *)&server_addr, server_fd_len) < 0) {
    perror("Errore nel binding del socket");
    exit(1);
  }

  if (listen(server_fd, 5) < 0) {
    perror("Imposibile mettere il server in ascolto");
    exit(1);
  }

  printf("[SERVER_TCP.c] Server in ascolto su 0.0.0.0:%d...\n", PORT);

  /* MODIFICA 1: rubrica dei socket di connessione + tracciamento del fd massimo */
  int client_fds[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; i++)
    client_fds[i] = -1;   /* -1 = slot libero (fd reale mai negativo) */
  int max_fd = server_fd; /* unico fd presente all'avvio */

  while (1) {
    /* MODIFICA 2: ricostruzione set ad ogni ciclo + select() + accept() via FD_ISSET */
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(server_fd, &read_set);
    for (int i = 0; i < MAX_CLIENTS; i++)
      if (client_fds[i] != -1)
        FD_SET(client_fds[i], &read_set);

    int n = select(max_fd + 1, &read_set, NULL, NULL, NULL);

    if (n == -1) {
      if (errno == EINTR)
        continue; /* segnale: riprova */
      perror("select");
      break;
    }

    if (FD_ISSET(server_fd, &read_set)) {
      struct sockaddr_in client_addr;
      socklen_t client_addrlen = sizeof(client_addr);

      int client_fd =
          accept(server_fd, (struct sockaddr *)&client_addr, &client_addrlen);
      if (client_fd == -1) {
        perror("accept");
        continue;
      }

      printf("[SERVER_TCP.c] accettato %s:%d\n",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

      int saved = 0;
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] == -1) {
          client_fds[i] = client_fd;
          saved = 1;
          break;
        }
      }
      if (!saved) {
        printf("    [!] troppi client, rifiuto fd=%d\n", client_fd);
        close(client_fd);
        continue;
      }

      /* Aggiorna max_fd se necessario */
      if (client_fd > max_fd)
        max_fd = client_fd;
    }

    /* MODIFICA 3: recv() con FD_ISSET su tutti i client */
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (client_fds[i] == -1)
        continue; /* slot vuoto */
      if (!FD_ISSET(client_fds[i], &read_set))
        continue; /* non pronto in questo ciclo */

      char buf[BUFLEN];
      ssize_t r = recv(client_fds[i], buf, sizeof(buf) - 1, 0);

      if (r <= 0) {
        /* r==0: client ha chiuso la connessione
           r==-1: errore                          */
        printf("[-] client disconnesso (fd=%d)\n", client_fds[i]);
        close(client_fds[i]);
        client_fds[i] = -1; /* libera slot */

        /* Ricalcola max_fd */
        max_fd = server_fd;
        for (int j = 0; j < MAX_CLIENTS; j++)
          if (client_fds[j] > max_fd)
            max_fd = client_fds[j];

      } else {
        buf[r] = '\0';
        /* Rimuovi newline finale (netcat aggiunge \n) */
        if (r > 0 && buf[r - 1] == '\n')
          buf[--r] = '\0';
        printf("[fd=%d] messaggio: \"%s\" (%zd byte)\n", client_fds[i], buf, r);
      }
    }
  }

  close(server_fd);

  return 0;
}
