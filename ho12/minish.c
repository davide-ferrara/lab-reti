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

static void sigchld_handler(int sig) {
  (void)sig;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

int main(void) {
  char line[MAXLINE];
  char *argv[MAXARGS];

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

    int bg = argc > 0 && !strcmp(argv[argc - 1], "&");
    if (bg)
      argv[--argc] = NULL;

    if (!strcmp(argv[0], "cd")) {
      if (argc == 1) {
        const char *home = getenv("HOME");
        if (!home) {
          fprintf(stderr, "cd: HOME non impostata\n");
          continue;
        }
        if (chdir(home) < 0)
          perror("cd");
      } else {
        if (chdir(argv[1]) < 0)
          perror("cd");
      }
      continue;
    }

    /* cerca | in argv */
    char **argv_right = NULL;
    int i = 0;
    while (i < argc) {
      if (!strcmp(argv[i], "|")) {
        argv[i] = NULL;
        argv_right = &argv[i + 1];
        break;
      }
      i++;
    }

    /* Se nel comando è presente la PIPE */
    if (argv_right != NULL) {
      char **argv_left = argv;

      /* Creo la pipe, tutto ció che scrivo in fd[1] puó essere letto da fd[0]*/
      int fd[2];
      if (pipe(fd) < 0) {
        perror("pipe");
        continue;
      }

      /* Primo fork che eseguirà il comando di sinistra.
       * Il fork copia l'intero programma, compreso il
       * program counter, child1 entrerà nell'if ed eseguira
       * con execvp il comando.
       * */
      pid_t child1 = fork();
      if (child1 < 0) {
        perror("fork");
        continue;
      }
      /*
       * Con dup2 duplichiamo il file descriptor, l'output del comando argv_left
       * verrà copiato in fd[1] della nostra PIPE.
       * La PIPE quindi non è altro che un array di fd.
       * Chiudiamo i fd cosi che nel secondo fork() saranno ready quindi
       * leggibili.
       * */
      if (child1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(argv_left[0], argv_left);

        // Ragggiungibile solo in caso di errore
        perror(argv_left[0]);
        exit(127);
      }

      pid_t child2 = fork();
      if (child2 < 0) {
        perror("fork");
        continue;
      }

      /* Medesima logica ma in questo caso leggiamo dal fd[0],
       * ció che entra in fd[1] esce da fd[0].
       * processo1 → write(fd[1], dati) → [kernel buffer ~64KB] → read(fd[0],
       * dati) → processo2
       * */
      if (child2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);

        // es: Grep legge da STDIN quindi trova i dati scritti da child1
        execvp(argv_right[0], argv_right);

        // Ragggiungibile solo in caso di errore
        perror(argv_right[0]);
        exit(127);
      }

      /* Chiudi i fd, aspetto child1 e child2 */
      close(fd[0]);
      close(fd[1]);
      waitpid(child1, NULL, 0);
      waitpid(child2, NULL, 0);
      // Torno in cima al loop
      continue;
    }

    /* Normale ciclo di esecuzione */
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      continue;
    }

    if (pid == 0) {
      execvp(argv[0], argv);
      perror(argv[0]);
      exit(127);
    } else {
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
