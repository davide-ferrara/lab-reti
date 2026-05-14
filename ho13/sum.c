#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Dimensione array e numero di Thread */
#define SIZE 1000000
#define T 4

/* Struttura dati del worker*/
typedef struct {
  const int *array;
  int from;
  int to;
  long long result;
} worker_args;

/* Funzione per stampare l'array */
void print_array(const int *a, int limit) {
  printf("[ ");
  for (int i = 0; i < limit; i++) {
    printf("%d ", a[i]);
    if (i % 40 == 0 && i != 0) {
      printf("\n");
    }
  }
  printf("]\n");
}

/* Somma l'array in modo sequenziale classico*/
long long sequential_sum(const int *a, int len) {
  ssize_t sum = 0;
  for (int i = 0; i < len; i++) {
    sum += a[i];
  }
  return sum;
}

/* Somme parte di un array partendo da from fino a to*/
long long partial_sum(const int *a, int from, int to) {
  long long sum = 0;
  for (int i = from; i < to; i++) {
    sum += a[i];
  }
  return sum;
}

/* Job del thread, fa il casting dell'args da void* in worker_args*
 * in questo modo il programma sa come leggere la struttura in memoria.
 * Uso la funziona gettid() per ottenere il thread id.
 * Il thread chiama partial_sum() e salva il risultato nella struttura,
 * il thread ritorna NULL.
 * */
static void job(void *args) {
  worker_args *w = (worker_args *)args;
  pid_t tid = gettid();

  w->result = partial_sum(w->array, w->from, w->to);
  printf("[SUM.c][Thread %d] Somma parziale: %lld\n", tid, w->result);

  pthread_exit(NULL);
}

/* Inizializza l'array*/
int *init_array(const int size) {
  int *a = (int *)malloc(sizeof(long long) * size);
  assert(a != NULL);
  for (int i = 0; i < size; i++) {
    a[i] = i % 100;
  }
  return a;
}

int main(void) {
  const int *a = init_array(SIZE);

  // print_array(a, 500);

  printf("[SUM.c] Somma sequenziale: %lld\n", sequential_sum(a, SIZE));
  assert(sequential_sum(a, SIZE) == 49500000);

  worker_args w[T] = {0};
  pthread_t tid[T];

  /* Stabilisco lo step per dividere l'array in base a T */
  int step = SIZE / T;
  int from = 0, to = step;

  /* Lancio tutti i thread in parallelo: segmenti disgiunti,
   * nessuna sincronizzazione necessaria. */
  for (int i = 0; i < T; i++) {
    w[i].array = a;
    w[i].from = from;
    w[i].to = to;
    from = to;
    to += step;
    pthread_create(&tid[i], NULL, (void *)&job, (void *)&w[i]);
  }

  long long psum = 0;
  for (int i = 0; i < T; i++) {
    if (pthread_join(tid[i], NULL) == 0)
      psum += w[i].result;
  }

  printf("[SUM.c] Somma parallela (T=%d): %lld\n", T, psum);
  assert(psum == 49500000);
  printf("[OK] i risultati coincidono!\n");

  return 0;
}
