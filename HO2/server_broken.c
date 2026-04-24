#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT    8080
#define BUFLEN  2048

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buf[BUFLEN];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Server in ascolto su porta %d\n", PORT);
    client_fd = accept(server_fd, NULL, NULL);

    while (1) {
        /* BUG: recv potrebbe restituire meno byte del previsto, il
               buffer potrebbe contenere meno byte di quello richiesti
               dalla recv() */
        int n = recv(client_fd, buf, BUFLEN, 0);
        if (n <= 0) break;

        buf[n] = '\0';
        printf("Ricevuto: %s\n", buf);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
