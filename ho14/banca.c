#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define N_SPORTELLI 4
#define N_OPERAZIONI 100000 // 100k

int saldo = 100000; /* saldo iniziale in euro */

/*
 * Expands to
{
  {
    0, 0, 0, 0, PTHREAD_MUTEX_TIMED_NP, 0, 0, { ((void *)0), ((void *)0) }
  }
}
 * */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Per N_OPERAZIONI si calcola un importo random che va a 0 a 100.
 * Se `i` è pari allora l'importo si somma altrimenti si sottrae
 * Il controllo if (saldo >= importo) non è sufficiente in quanto la funzione
 * sportello viene eseguita da piu thread e saldo è una variabile globale
 * condivisia, se ad essa due thread scrivono parallelamente uno dei due
 * potrebbe andare a sovrascrivere il risultato dell'altro creando
 * inconsistenza nei dati.
 * Dobbiamo aggiunge un MUTEX per fare in modo che solo un thread alla volta
 * possa scrivere nella variabile, quella è di fatto una sezione critica.
 * */
void *sportello(void *arg) {
  (void)arg;
  /* Setto il seed ad un numero fisso cosi da avere consistenza nei risultati.
   * non uso rand() in quanto non thread safe.
   */
  unsigned int seed = 518629;
  for (int i = 0; i < N_OPERAZIONI; i++) {
    int importo = (rand_r(&seed) % 100) + 1;

    // Sezione critica
    pthread_mutex_lock(&mutex);
    if (i % 2 == 0) {
      saldo += importo; /* deposito  */
    } else {
      if (saldo >= importo) /* prelievo  */
        saldo -= importo;
    }
    pthread_mutex_unlock(&mutex);
    // Fine sezione critica
  }
  return NULL;
}

int main(void) {

  pthread_t threads[N_SPORTELLI];
  int ids[N_SPORTELLI];
  for (int i = 0; i < N_SPORTELLI; i++) {
    ids[i] = i;
    pthread_create(&threads[i], NULL, sportello, &ids[i]);
  }
  for (int i = 0; i < N_SPORTELLI; i++)
    pthread_join(threads[i], NULL);
  printf("Saldo finale: %d euro\n", saldo);

  pthread_mutex_destroy(&mutex);

  return 0;
}
