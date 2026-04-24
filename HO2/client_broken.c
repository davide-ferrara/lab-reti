#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main(void) {
    int sockfd;
    struct sockaddr_in addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    char *messages[] = {
        "Primo messaggio di testo",
        "Secondo messaggio, piu lungo del primo",
        "Terzo"
    };

    for (int i = 0; i < 3; i++) {
        /* BUG: send non garantisce di inviare tutti i byte, il
               buffer di invio del kernel potrebbe essere pieno */
        send(sockfd, messages[i], strlen(messages[i]), 0);
        usleep(1000);
    }

    close(sockfd);
    return 0;
}
