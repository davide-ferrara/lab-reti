#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLINE 1024
#define MAXARGS 64

static int parse(char *line, char *argv[]) {
  int argc = 0;
  char *tok = strtok(line, " \t\n");
  while (tok && argc < MAXARGS - 1) {
    argv[argc++] = tok;
    tok = strtok(NULL, " \t\n");
  }
  argv[argc] = NULL;
  return argc;
}

/* Handler per SIGCHLD: raccoglie tutti i figli terminati in background.
 * Il loop è necessario perché più figli possono terminare mentre il segnale
 * è mascherato: i segnali dello stesso tipo non si accodano, quindi un
 * singolo SIGCHLD può rappresentare N terminazioni. WNOHANG evita di
 * bloccarsi se non ci sono altri figli pronti. */
static void sigchld_handler(int sig) {
  (void)sig;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

int main(void) {
  char line[MAXLINE];
  char *argv[MAXARGS];

  /* Installa l'handler prima del loop: ogni figlio in background
   * che termina manderà SIGCHLD al padre, che lo raccoglierà
   * immediatamente evitando i processi zombie. */
  signal(SIGCHLD, &sigchld_handler);

  while (1) {
    printf("minish> ");
    fflush(stdout);
    if (!fgets(line, MAXLINE, stdin))
      break;

    int argc = parse(line, argv);
    if (argv[0] == NULL)
      continue;

    if (strcmp(argv[0], "exit") == 0)
      break;

    /* Se l'ultimo token è '&' il processo va in background:
     * rimuoviamo '&' da argv prima di passarlo a execvp. */
    int bg = argc > 0 && !strcmp(argv[argc - 1], "&");
    if (bg)
      argv[--argc] = NULL;

    /* Built-in cd: chdir() modifica la directory corrente del processo
     * che la chiama. Se lo facessimo in un figlio, solo la directory
     * del figlio cambierebbe; quando il figlio termina la shell
     * si trova ancora nella directory di partenza.
     * cd senza argomenti va in $HOME. */
    if (!strcmp(argv[0], "cd")) {
      if (argc == 1) {
        const char *home = getenv("HOME");
        if (!home) { fprintf(stderr, "cd: HOME non impostata\n"); continue; }
        if (chdir(home) < 0) perror("cd");
      } else {
        if (chdir(argv[1]) < 0) perror("cd");
      }
      continue;
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); continue; }

    if (pid == 0) {
      execvp(argv[0], argv);
      perror(argv[0]);
      exit(127);
    } else {
      /* Se background: stampa [1] PID e torna al loop senza aspettare.
       * Il figlio verrà raccolto da sigchld_handler quando termina. */
      if (bg) {
        printf("[1] %d\n", pid);
        continue;
      }
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
