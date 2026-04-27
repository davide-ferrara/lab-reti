#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <arpa/inet.h>  /* inet_ntoa()                           */
#include <netinet/in.h> /* IPPROTO_ICMP, struct sockaddr_in      */
#include <netinet/ip.h> /* struct iphdr                          */
#include <sys/socket.h> /* socket(), recvfrom()                  */

#define BUFSIZE 65536

const char *proto_name(uint8_t protocol) {
  switch (protocol) {
  case 1:
    return "ICMP";
  case 6:
    return "TCP";
  case 17:
    return "UDP";
  default:
    return "sconosciuto";
  }
}

unsigned char *ip_payload(const unsigned char *buf, ssize_t n,
                          size_t *payload_len) {
  struct iphdr *ip = (struct iphdr *)buf;
  if (ip->ihl < 5)
    return NULL; /* malformato */
  int hlen = ip->ihl * 4;
  if (n < hlen)  /* Se n < hlen e accedo a buf + hlen vado in seg fault */
    return NULL; /* troncato, lo scarto perche non c'è in IP un meccanismo di
                    ritrasmissione */
  *payload_len = n - hlen;            /* lunghezza payload */
  return (unsigned char *)buf + hlen; /* puntatore al payload */
}

int main(void) {
  /* AF_INET + SOCK_RAW + IPPROTO_ICMP:
     il kernel consegna tutti i pacchetti ICMP in arrivo,
     gia' decapsulati dal frame Ethernet.
     Il buffer inizia direttamente dall'header IP. */
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock < 0) {
    perror("socket");
    return 1;
  }

  printf("In ascolto (solo ICMP)... premi Ctrl+C per uscire\n\n");

  unsigned char buf[BUFSIZE];
  int pkt = 0;

  while (1) {
    ssize_t n = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
    if (n < 0) {
      perror("recvfrom");
      break;
    }

    /* I primi byte del buffer sono l'header IP */
    struct iphdr *ip = (struct iphdr *)buf;

    /* tot_len e' in network byte order: serve ntohs() */
    printf("Pacchetto #%d  (%d byte)\n", ++pkt, ntohs(ip->tot_len));

    /* inet_ntoa() usa un buffer statico interno:
       chiamarla due volte nella stessa printf sovrascrive
       il primo risultato. Per ora usiamo due printf separate. */
    struct in_addr s, d;
    s.s_addr = ip->saddr;
    d.s_addr = ip->daddr;

    /* se ihl < 5 il pacchetto è malformato e va scartato */
    if (ip->ihl < 5) {
      printf("[SNIFFER.c] Pacchetto malformato\n");
      continue;
    }

    uint16_t tot_len = ntohs(ip->tot_len);
    int len = ip->ihl * 4;
    uint16_t fo = ntohs(ip->frag_off);
    int flag_df = (fo >> 14) & 1; /* Don’t Fragment */
    int flag_mf = (fo >> 13) & 1; /* More Fragments */
    int offset = fo & 0x1FFF;     /* Fragment Offset (in unita’ da 8 byte) */

    printf("  VERSION: %d\n", ip->version);
    printf("  IHL: %d byte\n", len);
    printf("  LUNGHEZZA: %d byte\n", tot_len);
    printf("  TTL: %d\n", ip->ttl);
    printf("  PROTO: %s (%d)\n", proto_name(ip->protocol), ip->protocol);
    printf("  SRC: %s\n", inet_ntoa(s));
    printf("  DST: %s\n", inet_ntoa(d));
    printf("  FLAGS: %s%s\n", flag_df ? "DF " : "", flag_mf ? "MF" : "");
    printf("  FRAG OFF: %d\n\n", fo);

    size_t payload_len;
    unsigned char *payload = ip_payload(buf, n, &payload_len);
    if (payload == NULL)
      continue;

    printf("  PAYLOAD:\n");
    if (ip->protocol == 1) {
      struct icmphdr *icmp = (struct icmphdr *)payload;
      printf("  TYPE:     %d\n", icmp->type);
      printf("  CODE:     %d\n", icmp->code);
      printf("  CHECKSUM: %d\n", ntohs(icmp->checksum));
      printf("  ID:       %d\n", ntohs(icmp->un.echo.id));
      printf("  SEQ:      %d\n\n", ntohs(icmp->un.echo.sequence));
    }
  }

  close(sock);
  return 0;
}
