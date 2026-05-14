/*
 * nl_dummy.c — crea un'interfaccia "dummy" via Netlink
 * =====================================================
 * COMPILAZIONE:  gcc -Wall -o nl_dummy nl_dummy.c
 * UTILIZZO:      sudo ./nl_dummy
 * VERIFICA:      ip link show dummy0
 * PULIZIA:       sudo ip link del dummy0
 *
 * Un'interfaccia dummy è la più semplice che esiste:
 * scarta tutto il traffico ricevuto, come /dev/null.
 * Utile per test, per binding di servizi, per routing.
 *
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_link.h>

/*
 * Il messaggio Netlink e' un blocco contiguo di memoria composto da tre parti:
 *   1. hdr  — intestazione fissa (16 byte): tipo, lunghezza, flags
 *   2. ifi  — payload fisso (16 byte): famiglia, indice, flags interfaccia
 *   3. buf  — spazio per gli attributi rtattr variabili
 *
 * Deve essere una struct unica perche' send() manda byte contigui: tre variabili
 * separate in memoria non funzionerebbero.
 * Il qualificatore static inizializza tutto a zero automaticamente prima del main().
 */
static struct {
    struct nlmsghdr  hdr;
    struct ifinfomsg ifi;
    char             buf[256];
} msg;

/*
 * add_attr() — scrive un attributo rtattr in coda al messaggio.
 *
 * Ogni attributo ha un header (tipo + lunghezza) seguito dal valore.
 * La funzione:
 *   1. Calcola dove nel buffer finisce il messaggio corrente (posizione del nuovo attributo)
 *   2. Scrive l'header rtattr (tipo e lunghezza)
 *   3. Copia il valore subito dopo l'header
 *   4. Aggiorna nlmsg_len per "avanzare il cursore" in fondo al messaggio
 *
 * Ritorna il puntatore al nuovo attributo — usato solo per attributi annidati
 * (contenitori) che vanno poi chiusi con close_attr().
 * Per attributi foglia (IFLA_IFNAME, IFLA_INFO_KIND) il valore di ritorno si ignora.
 */
static struct rtattr *add_attr(int type, const void *data, int len)
{
    /* posiziona il nuovo attributo subito dopo la fine del messaggio corrente */
    struct rtattr *a = (struct rtattr *)
        ((char *)&msg + NLMSG_ALIGN(msg.hdr.nlmsg_len));
    a->rta_type = type;
    a->rta_len  = RTA_LENGTH(len); /* header rtattr (4 byte) + valore */
    /* se data e' NULL si sta aprendo un contenitore annidato: nessun valore da copiare */
    if (data) memcpy(RTA_DATA(a), data, len);
    /* aggiorna la lunghezza totale del messaggio, allineata a 4 byte */
    msg.hdr.nlmsg_len =
        NLMSG_ALIGN(msg.hdr.nlmsg_len) + RTA_ALIGN(a->rta_len);
    return a;
}

/*
 * close_attr() — corregge la lunghezza di un attributo contenitore.
 *
 * Quando si apre un contenitore (es. IFLA_LINKINFO) con add_attr(..., NULL, 0),
 * la sua rta_len vale solo 4 (solo l'header, nessun figlio ancora).
 * Dopo aver aggiunto tutti i figli, questa funzione ricalcola rta_len come:
 *   rta_len = (fine attuale del messaggio) - (inizio del contenitore)
 * cosi' rta_len copre header + tutti i figli annidati.
 */
static void close_attr(struct rtattr *a)
{
    a->rta_len = (char *)&msg + NLMSG_ALIGN(msg.hdr.nlmsg_len) - (char *)a;
}

int main(void)
{
    /*
     * Passo 1: Apro il socket NETLINK di tipo RAW in quanto non ho bisogno
     * di nessun protocollo di trasporto ma ho bisogno solo di leggere
     * messaggi binari.
     *
     * NETLINK_ROUTE e' il sottosistema kernel che gestisce interfacce, indirizzi
     * IP e rotte — lo stesso usato da "ip link", "ip addr", "ip route".
     */
    int nl = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    /*
     * bind() registra l'identita' del processo lato kernel.
     * Non ci si connette a nessuno (non e' TCP): si dice solo al kernel
     * "le risposte per questo socket vanno a questo processo".
     *
     * nl_pid = 0  → il kernel sostituisce automaticamente con il PID del processo
     * nl_groups = 0 → nessun gruppo multicast: vogliamo solo mandare richieste
     *                 e ricevere ACK, non notifiche asincrone
     */
    struct sockaddr_nl sa = { .nl_family = AF_NETLINK };
    bind(nl, (struct sockaddr *)&sa, sizeof(sa));

    /*
     * Passo 2: Costruisco il messagio di tipo NEWLINK, che serve per creare o
     * modificare un interfaccia di rete (un link). RTM invece sta per ROUTE
     * MESSAGE.
     *
     * NLM_F_REQUEST obbligatorio su ogni richiesta userspace→kernel. Senza, il
     * kernel ignora il messaggio.
     *
     * NLM_F_ACK voglio una risposta. Il kernel manda
     * sempre NLMSG_ERROR con error=0 (successo) o error=-errno (fallimento).
     *
     * NLM_F_CREATE Se non esiste l'interfaccia, creala.
     *
     * NLM_F_EXCL Se esiste ritorna errore invece di modificarla,
     * combinando i due flags il link viene creato solo se non esistente,
     * eventuali dummy0 gia presenti non vengono modificati.
     *
     * NLMSG_LENGTH(sizeof(ifi)) = 16 (header) + 16 (ifinfomsg) = 32 byte.
     * E' la lunghezza iniziale prima di aggiungere qualsiasi attributo.
     *
     * ifi_family = AF_UNSPEC: nessuna famiglia specifica, interfaccia generica
     * (non IPv4 ne' IPv6 in particolare). Tutti gli altri campi di ifi
     * restano zero: ifi_index = 0 dice al kernel di assegnare un indice libero,
     * ifi_flags = 0 significa interfaccia DOWN alla creazione.
     */
    memset(&msg, 0, sizeof(msg));
    msg.hdr.nlmsg_len   = NLMSG_LENGTH(sizeof(msg.ifi));
    msg.hdr.nlmsg_type  = RTM_NEWLINK;
    msg.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK
                        | NLM_F_CREATE  | NLM_F_EXCL;
    msg.ifi.ifi_family  = AF_UNSPEC;

    /*
     * IFLA_IFNAME: attributo che trasporta il nome dell'interfaccia.
     * IFLA = InterFace Link Attribute, prefisso di tutti gli attributi che
     * descrivono un'interfaccia di rete.
     * E' una costante intera (valore 3) che va nel campo rta_type dell'header
     * rtattr: dice al kernel come interpretare i byte che seguono.
     *
     * strlen("dummy0") + 1: la stringa viene inviata col terminatore '\0' incluso
     * perche' il kernel la legge come stringa C e ha bisogno del '\0' per sapere
     * dove finisce.
     */
    add_attr(IFLA_IFNAME, "dummy0", strlen("dummy0") + 1);

    /*
     * IFLA_LINKINFO: attributo contenitore che raggruppa le informazioni
     * specifiche del tipo di interfaccia. Non contiene un valore diretto
     * ma altri rtattr al suo interno (attributi annidati).
     *
     * Si apre con NULL e lunghezza 0 perche' non ha un valore proprio:
     * la sua dimensione finale dipende dai figli che si aggiungono dopo.
     * Il puntatore viene salvato per passarlo a close_attr() alla fine.
     *
     * IFLA_INFO_KIND: figlio di IFLA_LINKINFO, contiene il tipo di driver
     * come stringa. Il kernel legge "dummy", cerca il modulo dummy.ko e lo
     * chiama per creare l'interfaccia.
     *
     * close_attr() corregge rta_len di IFLA_LINKINFO: ora che tutti i figli
     * sono stati scritti, si puo' calcolare la dimensione totale del
     * contenitore come (fine messaggio) - (inizio IFLA_LINKINFO).
     */
    struct rtattr *info = add_attr(IFLA_LINKINFO, NULL, 0);
        add_attr(IFLA_INFO_KIND, "dummy", strlen("dummy") + 1);
    close_attr(info);

    /*
     * Passo 3: Invio il messaggio al kernel.
     * send() manda esattamente nlmsg_len byte a partire dall'indirizzo di msg.
     * Il kernel riceve i byte e li interpreta come messaggio Netlink.
     */
    printf("Messaggio: %d byte\n", msg.hdr.nlmsg_len);
    send(nl, &msg, msg.hdr.nlmsg_len, 0);

    /*
     * Passo 4: Leggo la risposta del kernel.
     * Il kernel risponde sempre con un messaggio NLMSG_ERROR.
     * La struttura nlmsgerr contiene il campo error:
     *   error = 0  → successo
     *   error < 0  → fallimento, il valore e' -errno (es. -1 = EPERM, -17 = EEXIST)
     *
     * NLMSG_DATA(h) e' una macro che restituisce il puntatore al payload
     * del messaggio (i byte subito dopo l'header nlmsghdr).
     */
    char risposta[256];
    recv(nl, risposta, sizeof(risposta), 0);
    struct nlmsghdr *h = (struct nlmsghdr *)risposta;
    struct nlmsgerr *e = (struct nlmsgerr *)NLMSG_DATA(h);

    if (h->nlmsg_type == NLMSG_ERROR && e->error != 0) {
        fprintf(stderr, "Errore: %d", -e->error);
        if (-e->error == 1)  fprintf(stderr, " (EPERM: serve sudo)");
        if (-e->error == 17) fprintf(stderr, " (EEXIST: dummy0 esiste gia')");
        fprintf(stderr, "\n");
        close(nl);
        return 1;
    }

    printf("dummy0 creata.\n");
    printf("Verifica: ip link show dummy0\n");
    printf("Pulizia:  sudo ip link del dummy0\n");

    close(nl);
    return 0;
}
