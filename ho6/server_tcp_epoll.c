#include <arpa/inet.h>
// #include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_EVENTS 64
#define BUFLEN 512

static void set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl F_GETFL");
    exit(1);
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl F_SETFL");
    exit(1);
  }
}

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

  /* Creo l'istanza epoll e registro il server_fd subito dopo listen() */
  int epfd = epoll_create1(0);
  struct epoll_event ev, events[MAX_EVENTS];
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

  printf("[SERVER_TCP.c] Server in ascolto su 0.0.0.0:%d...\n", PORT);

  while (1) {
    // n ritorna il numero di fd pronti
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      // Se l'evento avviene sul server_fd = nuovo client
      if (fd == server_fd) {
        struct sockaddr_in c_addr;
        socklen_t c_addr_len = sizeof(c_addr);

        // Accetto il nuovo client
        int cfd = accept(server_fd, (struct sockaddr *)&c_addr, &c_addr_len);
        if (cfd == -1) {
          perror("accept");
          continue;
        }

        // è buona pratica settare il non blicking nel file descripor
        set_nonblocking(cfd);

        // Struttura per l'ip del client
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &c_addr.sin_addr, ip, sizeof(ip));

        printf("[SERVER_TCP.c] Nuovo client connesso: %s\n", ip);

        // Registro il client
        ev.events = EPOLLIN;
        ev.data.fd = cfd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
          perror("epoll_ctl ADD client");
          close(cfd);
        }
      } else if (events[i].events &
                 EPOLLIN) { // Usando la bitmask vedo se ci sono eventi negli fd
                            // non uguali al server
        char buf[BUFLEN];
        ssize_t r = recv(fd, buf, sizeof(buf) - 1, 0);

        // r == 0 il client ha chiuso la connessione
        // r == -1 errore
        if (r <= 0) {
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
          close(fd);
          printf("[SERVER_TCP.c] Client disconnesso (fd=%d)\n", fd);

        } else {
          buf[r] = '\0';
          if (r > 0 && buf[r - 1] == '\n')
            buf[--r] = '\0';
          printf("[SERVER_TCP.c] [fd=%d] messaggio: \"%s\" (%zd byte)\n", fd,
                 buf, r);
        }

      } else if (events[i].events & (EPOLLERR | EPOLLHUP)) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        printf("[SERVER_TCP.c] Errore/Hangup su fd=%d, chiuso\n", fd);
      }
    }
  }

  // Chiudo i file descriptor
  close(epfd);
  close(server_fd);

  return 0;
}
