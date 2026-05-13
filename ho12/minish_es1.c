#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLINE 1024
#define MAXARGS 64

/* Tokenizza 'line' in argv[], ritorna il numero di argomenti.
 * Modifica 'line' in place (strtok inserisce '\0' al posto degli spazi). */
static int parse(char *line, char *argv[]) {
  int argc = 0;
  char *tok = strtok(line, " \t\n");
  while (tok && argc < MAXARGS - 1) {
    argv[argc++] = tok;
    tok = strtok(NULL, " \t\n");
  }
  argv[argc] = NULL; /* termine obbligatorio per execvp */
  return argc;
}

int main(void) {
  char line[MAXLINE];
  char *argv[MAXARGS];

  while (1) {

    /* 1. Stampa il prompt e leggi una riga da stdin.
     * printf usa un buffer line-buffered: senza fflush il prompt non appare
     * sullo schermo perché "minish> " non termina con '\n'.
     * fgets() ritorna NULL su EOF (Ctrl-D): in quel caso esci. */
    printf("minish> ");
    fflush(stdout);
    if (!fgets(line, MAXLINE, stdin))
      break;

    /* 2. Parsing: riempie argv[] e ritorna il numero di token.
     *    Se la riga è vuota o solo spazi, argc == 0: riparte il loop. */
    int argc = parse(line, argv);
    (void)argc;
    if (argv[0] == NULL)
      continue;

    /* 3. Built-in: exit.
     * Il confronto va fatto PRIMA di fork(): se venisse fatto nel figlio,
     * sarebbe il figlio ad uscire e non la shell. */
    if (strcmp(argv[0], "exit") == 0)
      break;

    /* 4. Crea un processo figlio. */
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      continue;
    }

    if (pid == 0) {
      /* -- FIGLIO --
       * execvp cerca il programma nel PATH, quindi "ls" funziona
       * anche senza scrivere "/bin/ls".
       * Se execvp ha successo NON ritorna mai: sostituisce l'immagine
       * del processo. Il codice dopo è raggiungibile solo in caso di errore.
       * Uscire con exit(127) è la convenzione POSIX per "command not found". */
      if (execvp(argv[0], argv) == -1) {
        perror(argv[0]);
        exit(127);
      }

    } else {
      /* -- PADRE --
       * waitpid(pid, ...) aspetta il figlio specifico.
       * 'status' codifica il modo in cui il figlio è terminato:
       *   WIFEXITED(status)   → terminato normalmente con exit()
       *   WEXITSTATUS(status) → codice passato a exit()
       *   WIFSIGNALED(status) → ucciso da un segnale
       *   WTERMSIG(status)    → numero del segnale */
      int status;
      waitpid(pid, &status, 0);
      if (WIFEXITED(status) && WEXITSTATUS(status))
        printf("[Exit %d]\n", WEXITSTATUS(status));
      if (WIFSIGNALED(status))
        printf("[Signal %d]\n", WTERMSIG(status));
    }
  }

  return 0;
}
