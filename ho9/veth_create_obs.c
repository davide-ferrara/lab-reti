/*
 * veth_create_obs.c — crea una veth pair da zero, tutto in C
 * ============================================================
 * COMPILAZIONE:  gcc -Wall -o veth_create veth_create.c
 * UTILIZZO:      sudo ./veth_create
 * VERIFICA:      ip link show veth0; ip link show veth1
 * ============================================================
 *
 * Netlink e' il protocollo con cui lo spazio utente parla al
 * kernel per creare/modificare interfacce di rete.
 * E' la stessa cosa che fa "ip link add veth0 type veth peer name veth1"
 * ma scritta in C.
 *
 * Un messaggio Netlink e' fatto cosi':
 *
 *   [ nlmsghdr | ifinfomsg | attributi... ]
 *    ^header     ^chi creare  ^cosa creare
 *
 * Gli attributi sono coppie (tipo, valore) e possono essere annidati.
 * Per la veth la struttura e':
 *
 *   IFLA_IFNAME   "veth0"       <- nome prima interfaccia
 *   IFLA_LINKINFO               <- tipo e dati
 *     IFLA_INFO_KIND  "veth"    <- e' una veth
 *     IFLA_INFO_DATA            <- dati specifici veth
 *       VETH_INFO_PEER          <- descrizione del peer
 *         ifinfomsg             <- header peer (vuoto)
 *         IFLA_IFNAME  "veth1"  <- nome seconda interfaccia
 */

#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Un messaggio Netlink e' un buffer con una struttura precisa.        */
/* Usiamo una struct che contiene tutto: header + spazio per gli attr. */
/* ------------------------------------------------------------------ */
struct {
  struct nlmsghdr hdr;  /* header: tipo messaggio, lunghezza, flags */
  struct ifinfomsg ifi; /* interfaccia da creare                    */
  char buf[512];        /* spazio per gli attributi              */
} msg;

/*
 * Aggiunge un attributo (tipo + valore) al messaggio.
 * Ogni attributo ha un header rtattr seguito dal valore.
 * Ritorna il puntatore all'attributo appena aggiunto
 * (serve per gli attributi annidati).
 */
static struct rtattr *aggiungi_attr(int tipo, const void *valore, int len) {
  /* L'attributo va messo subito dopo la fine del messaggio attuale */
  struct rtattr *a =
      (struct rtattr *)((char *)&msg + NLMSG_ALIGN(msg.hdr.nlmsg_len));

  a->rta_type = tipo;
  a->rta_len = RTA_LENGTH(len); /* header + valore      */

  if (valore)
    memcpy(RTA_DATA(a), valore, len); /* copia il valore      */

  /* Aggiorna la lunghezza totale del messaggio */
  msg.hdr.nlmsg_len = NLMSG_ALIGN(msg.hdr.nlmsg_len) + RTA_ALIGN(a->rta_len);

  return a; /* restituito per poter aggiornare rta_len dopo (nested) */
}

/*
 * Gli attributi annidati (es. IFLA_LINKINFO che contiene IFLA_INFO_KIND)
 * si aprono con aggiungi_attr(..., NULL, 0) e si chiudono aggiornando
 * la loro lunghezza quando tutti i figli sono stati aggiunti.
 */
static void chiudi_attr(struct rtattr *a) {
  a->rta_len = (char *)&msg + NLMSG_ALIGN(msg.hdr.nlmsg_len) - (char *)a;
}

static void dump_hex(void *buf, int len) {
  unsigned char *p = buf;
  printf("=== dump messaggio (%d byte) ===\n", len);
  for (int i = 0; i < len; i++) {
    printf("%02x ", p[i]);
    if ((i + 1) % 16 == 0)
      printf("\n");
  }
  printf("\n================================\n");
}

/* ================================================================== */
int main(void) {
  /* ── Passo 1: apri il socket Netlink ── */
  /*
   * AF_NETLINK e' una famiglia di socket per parlare con il kernel.
   * NETLINK_ROUTE gestisce interfacce, rotte, indirizzi.
   */
  int nl = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

  struct sockaddr_nl sa = {.nl_family = AF_NETLINK};
  bind(nl, (struct sockaddr *)&sa, sizeof(sa));

  /* ── Passo 2: costruisci il messaggio ── */
  memset(&msg, 0, sizeof(msg));

  /* Header: vogliamo creare (RTM_NEWLINK) una nuova interfaccia */
  msg.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(msg.ifi));
  msg.hdr.nlmsg_type = RTM_NEWLINK;
  msg.hdr.nlmsg_flags = NLM_F_REQUEST  /* e' una richiesta al kernel */
                        | NLM_F_CREATE /* crea se non esiste          */
                        | NLM_F_EXCL   /* errore se esiste gia'       */
                        | NLM_F_ACK;   /* voglio una conferma         */

  /* ifinfomsg: famiglia generica (non IPv4 ne' IPv6) */
  msg.ifi.ifi_family = AF_UNSPEC;

  /* ----- TODO: Qui va costruito il messaggio NETLINK -----*/

  /* Byte 32-33 = 0a 00 (rta_len=10: 4 header + 6 "veth0\0")
   * Byte 34-35 = 03 00 (rta_type=3 = IFLA_IFNAME)
   * Byte 36-41 = 76 65 74 68 30 00 ("veth0\0" in ASCII)
   * Byte 42-43 = 00 00 (padding allineamento a 4 byte) */
  aggiungi_attr(IFLA_IFNAME, "veth0", strlen("veth0") + 1);

  printf("Dimensione messaggio: %d byte\n", msg.hdr.nlmsg_len);

  /* Byte 44-45 = 34 00 (rta_len=52: chiuso da close_attr a posteriori)
   * Byte 46-47 = 12 00 (rta_type=18 = IFLA_LINKINFO) */
  struct rtattr *info = aggiungi_attr(IFLA_LINKINFO, NULL, 0);

  printf("Dimensione messaggio: %d byte\n", msg.hdr.nlmsg_len);

  /* Byte 48-49 = 09 00 (rta_len=9: 4 header + 5 "veth\0")
   * Byte 50-51 = 01 00 (rta_type=1 = IFLA_INFO_KIND)
   * Byte 52-56 = 76 65 74 68 00 ("veth\0" in ASCII)
   * Byte 57-59 = 00 00 00 (padding allineamento a 4 byte) */
  aggiungi_attr(IFLA_INFO_KIND, "veth", strlen("veth") + 1);

  printf("Dimensione messaggio: %d byte\n", msg.hdr.nlmsg_len);

  /* Byte 60-61 = 24 00 (rta_len=36: chiuso da close_attr a posteriori)
   * Byte 62-63 = 02 00 (rta_type=2 = IFLA_INFO_DATA) */
  struct rtattr *data = aggiungi_attr(IFLA_INFO_DATA, NULL, 0);

  printf("Dimensione messaggio: %d byte\n", msg.hdr.nlmsg_len);

  /* Byte 64-65 = 20 00 (rta_len=32: chiuso da close_attr a posteriori)
   * Byte 66-67 = 01 00 (rta_type=1 = VETH_INFO_PEER) */
  struct rtattr *peer = aggiungi_attr(VETH_INFO_PEER, NULL, 0);

  printf("Dimensione messaggio: %d byte\n", msg.hdr.nlmsg_len);

  /* Byte 68-83 = 00 * 16 (ifinfomsg del peer tutto zero: indice libero, DOWN)
   * caso speciale: non si usa aggiungi_attr(), si avanza nlmsg_len manualmente
   */
  msg.hdr.nlmsg_len += sizeof(struct ifinfomsg);

  printf("Dimensione messaggio: %d byte\n", msg.hdr.nlmsg_len);

  /* Byte 84-85 = 0a 00 (rta_len=10: 4 header + 6 "veth1\0")
   * Byte 86-87 = 03 00 (rta_type=3 = IFLA_IFNAME)
   * Byte 88-93 = 76 65 74 68 31 00 ("veth1\0" in ASCII)
   * Byte 94-95 = 00 00 (padding allineamento a 4 byte) */
  aggiungi_attr(IFLA_IFNAME, "veth1", strlen("veth1") + 1);

  printf("Dimensione messaggio: %d byte\n", msg.hdr.nlmsg_len);

  chiudi_attr(peer);
  chiudi_attr(data);
  chiudi_attr(info);

  dump_hex(&msg, msg.hdr.nlmsg_len);

  /* ── Passo 3: invia il messaggio al kernel ── */
  send(nl, &msg, msg.hdr.nlmsg_len, 0);

  /* ── Passo 4: leggi la risposta (ACK o errore) ── */
  char risposta[256];
  recv(nl, risposta, sizeof(risposta), 0);

  struct nlmsghdr *h = (struct nlmsghdr *)risposta;
  struct nlmsgerr *e = (struct nlmsgerr *)NLMSG_DATA(h);

  if (h->nlmsg_type == NLMSG_ERROR && e->error != 0) {
    fprintf(stderr, "Errore dal kernel: %d\n", -e->error);
    fprintf(stderr, "(se e' -17 = EEXIST: veth0 esiste gia')\n");
    close(nl);
    return 1;
  }

  printf("veth0 <-> veth1 create.\n");
  printf("Verifica: ip link show veth0\n");
  printf("          ip link show veth1\n");
  printf("Pulizia:  ip link del veth0\n");

  close(nl);
  return 0;
}
