#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define N_LETTORI 8

typedef struct {
  char nome[32];
  char numero[16];
} Contatto;

Contatto rubrica[] = {
    {"Alice", "333-1111"},
    {"Bob", "333-2222"},
    {"Carlo", "333-3333"},
};
int n_contatti = 3;

pthread_rwlock_t rubrica_lock = PTHREAD_RWLOCK_INITIALIZER;

void *lettore(void *arg) {
  int id = *(int *)arg;
  for (int i = 0; i < 5; i++) {
    // Acquisisce il lock in lettura
    pthread_rwlock_rdlock(&rubrica_lock);

    int idx = i % n_contatti;
    printf("Lettore %d: %s -> %s\n", id, rubrica[idx].nome,
           rubrica[idx].numero);
    usleep(100000);

    // Rilascia il lock
    pthread_rwlock_unlock(&rubrica_lock);
  }
  return NULL;
}

void *scrittore(void *arg) {
  (void)arg;
  sleep(1); /* lascia partire i lettori */
  pthread_rwlock_wrlock(&rubrica_lock);

  printf("Scrittore: aggiorno il numero di Bob...\n");
  strcpy(rubrica[1].numero, "333-9999");
  sleep(2); /* simula aggiornamento lento */
  printf("Scrittore: aggiornamento completato.\n");

  pthread_rwlock_unlock(&rubrica_lock);

  return NULL;
}

int main(void) {
  pthread_rwlock_init(&rubrica_lock, NULL);

  pthread_t lettori[N_LETTORI], scr;
  int ids[N_LETTORI];

  // Thread scrittore
  pthread_create(&scr, NULL, scrittore, NULL);
  for (int i = 0; i < N_LETTORI; i++) {
    ids[i] = i;
    // Thread lettori
    pthread_create(&lettori[i], NULL, lettore, &ids[i]);
  }
  // Aspetto lo scrittore
  pthread_join(scr, NULL);
  for (int i = 0; i < N_LETTORI; i++)
    // Aspetto i lettori
    pthread_join(lettori[i], NULL);

  /* Rileggo ... */
  for (int i = 0; i < N_LETTORI; i++) {
    ids[i] = i;
    // Thread lettori
    pthread_create(&lettori[i], NULL, lettore, &ids[i]);
  }
  for (int i = 0; i < N_LETTORI; i++)
    // Aspetto i lettori
    pthread_join(lettori[i], NULL);

  pthread_rwlock_destroy(&rubrica_lock);
  return 0;
}
