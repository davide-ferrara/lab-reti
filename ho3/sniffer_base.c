#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <errno.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <signal.h>
#include <sys/socket.h>

#define BUFSIZE 65536

volatile int running = 1;
unsigned long cnt_ipv4 = 0;
unsigned long cnt_arp = 0;
unsigned long cnt_ipv6 = 0;
unsigned long cnt_other = 0;

void print_stats(void) {
  printf("\nSNIFFING STATS: PROTOCOL | COUNT\n");
  printf("                  IPv4     | %ld\n", cnt_ipv4);
  printf("                  ARP      | %ld\n", cnt_arp);
  printf("                  IPv6     | %ld\n", cnt_ipv6);
  printf("                  Altri    | %ld\n", cnt_other);
  return;
}

void handle_sigint(int sig) {
  (void)sig;
  running = 0;
}

const char *ethertype_name(uint16_t proto) {
  switch (proto) {
  case ETHERTYPE_IP:
    cnt_ipv4++;
    return "IPv4";
  case ETHERTYPE_ARP:
    cnt_arp++;
    return "ARP";
  case ETHERTYPE_IPV6:
    cnt_ipv6++;
    return "IPv6";
  default:
    cnt_other++;
    return "sconociuto";
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT, handle_sigint);

  if (argc < 2) {
    fprintf(stderr, "USAGE: sniffer <INTERFACE> [ipv4|arp|ipv6]\n");
    exit(EXIT_FAILURE);
  }
  const char *iface = argv[1];

  printf("[SNIFFER.c] Sniffing su %s...\n", iface);

  /* Apre il raw socket: cattura tutti i protocolli */
  int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock == -1) {
    if (errno == EPERM)
      fprintf(stderr, "Errore: esegui con sudo o cap_net_raw\n");
    else
      perror("socket");
    exit(EXIT_FAILURE);
  }

  /* Recupera l'indice numerico dell'interfaccia */
  unsigned int ifindex = if_nametoindex(iface);
  if (ifindex == 0) {
    fprintf(stderr, "Errore: interfaccia '%s' non trovata\n", iface);
    close(sock);
    exit(EXIT_FAILURE);
  }

  // Si puó vedere col comando "ip a"
  printf("[SNIFFER.c] Indice interfaccia %d\n", ifindex);

  /* Associa il socket all'interfaccia specificata */
  struct sockaddr_ll sll;
  memset(&sll, 0, sizeof(sll));
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ALL);
  sll.sll_ifindex = ifindex;

  /* Controllo bind */
  if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
    perror("bind");
    close(sock);
    exit(EXIT_FAILURE);
  }

  uint16_t filter = 0;
  if (argc == 3) {
    if (strcmp(argv[2], "ipv4") == 0)
      filter = ETHERTYPE_IP;
    else if (strcmp(argv[2], "arp") == 0)
      filter = ETHERTYPE_ARP;
    else if (strcmp(argv[2], "ipv6") == 0)
      filter = ETHERTYPE_IPV6;
    else {
      fprintf(stderr, "Protocollo non supportato: %s\n", argv[2]);
      close(sock);
      exit(EXIT_FAILURE);
    }
  }

  unsigned char buf[BUFSIZE];
  int frame = 0;

  while (running) {
    ssize_t n = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
    if (n < (ssize_t)sizeof(struct ethhdr))
      continue;

    struct ethhdr *eth = (struct ethhdr *)buf;

    uint16_t proto = ntohs(eth->h_proto);
    if (filter != 0 && proto != filter)
      continue;

    const char *name = ethertype_name(proto);

    printf("Frame #%d  (%zd byte)\n", ++frame, n);
    printf("  DST: %02X:%02X:%02X:%02X:%02X:%02X\n", eth->h_dest[0],
           eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4],
           eth->h_dest[5]);
    printf("  SRC: %02X:%02X:%02X:%02X:%02X:%02X\n", eth->h_source[0],
           eth->h_source[1], eth->h_source[2], eth->h_source[3],
           eth->h_source[4], eth->h_source[5]);
    printf("  EtherType: %d %s\n", proto, name);

    if (running == 0)
      print_stats();
  }

  close(sock);
  return 0;
}
