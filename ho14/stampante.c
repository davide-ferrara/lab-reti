#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define N_UTENTI 6

pthread_mutex_t stampante = PTHREAD_MUTEX_INITIALIZER;

void *utente(void *arg) {
  int id = *(int *)arg;

  /* Ora di sistema attuale (CLOCK_REALTIME) + 3 secondi */
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 3;

  /* Provo ad acquisire il mutex per 3 secondi */
  int rc = pthread_mutex_timedlock(&stampante, &ts);
  if (rc == 0) {
    // Lock acquisito
    printf("[Utente %d] Sta stampando... \n", id);
    sleep(2); // Simula la stampa
    printf("[Utente %d] Ha terminato la stampa :D \n", id);

    // Rilascio il mutex
    pthread_mutex_unlock(&stampante);
  } else if (rc == ETIMEDOUT) {
    printf("[Utente %d] Rinuncia alla stampa :(\n", id);
  } else {
    printf("[Utente %d] Errore nell'acquisone del mutex.\n", id);
  }

  return NULL;
}

int main(void) {
  pthread_t threads[N_UTENTI];
  int ids[N_UTENTI];
  for (int i = 0; i < N_UTENTI; i++) {
    ids[i] = i;
    pthread_create(&threads[i], NULL, utente, &ids[i]);
    usleep(200000); /* un utente ogni 0.2 s */
  }
  for (int i = 0; i < N_UTENTI; i++)
    pthread_join(threads[i], NULL);
  pthread_mutex_destroy(&stampante);
  return 0;
}
