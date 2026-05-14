#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Dimensione array e numero di Thread */
#define SIZE 800000
#define T 4

static pthread_barrier_t barrier;

/* Struttura dati del worker*/
typedef struct {
  int *array;
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
  printf("... ]\n");
}

/* Inizializza l'array*/
int *init_array(int size) {
  int *a = (int *)malloc(sizeof(long long) * size);
  assert(a != NULL);
  for (int i = 0; i < size; i++) {
    a[i] = i % 1000;
  }
  return a;
}

long long sequential_sum(int *a) {
  long long sum = 0;
  for (int i = 0; i < SIZE; i++) {
    sum += a[i];
  }
  return sum;
}

void map(int *a, int from, int to) {
  for (int i = from; i < to; i++) {
    a[i] = a[i] * a[i];
  }
}

static long long reduce(int *a) {
  long long rc = 0;
  for (int i = 0; i < SIZE; i++) {
    rc += a[i];
  }
  return rc;
}

static void job(void *args) {
  worker_args *w = (worker_args *)args;
  pid_t tid = gettid();

  printf("[Thread %d] Iniziato il map...\n", tid);
  map(w->array, w->from, w->to);

  int rc = pthread_barrier_wait(&barrier);
  if (rc == PTHREAD_BARRIER_SERIAL_THREAD) {
    printf("[Thread %d Seriale %d] Iniziato il reduce...\n", tid, rc);
    w->result = reduce(w->array);
  }

  pthread_exit(NULL);
}

int main(void) {
  long long exp = 266266800000;
  int *a = init_array(SIZE);
  int *b = init_array(SIZE);

  map(b, 0, SIZE);
  printf("Somma dei quadrati (sequenziale): %lld\n", sequential_sum(b));
  assert(exp == sequential_sum(b));

  int step = SIZE / T;
  int from = 0;
  int to = step;

  pthread_barrier_init(&barrier, NULL, T);

  worker_args w[T] = {0}; // memset a zero
  pthread_t tid[T] = {0};
  for (int i = 0; i < T; i++) {
    w[i].array = a;
    w[i].from = from;
    w[i].to = to;
    w[i].result = -1;
    from = to;
    to += step;
    pthread_create(&tid[i], NULL, (void *)&job, &w[i]);
  }

  for (int i = 0; i < T; i++) {
    if (pthread_join(tid[i], NULL) == 0) {
      if (w[i].result != -1) {
        printf("Somma dei quadrati (parallela): %lld\n", w[i].result);

        assert(w[i].result == exp);
        printf("[OK] i risultati coincidono!\n");
      }
    }
  }

  pthread_barrier_destroy(&barrier);

  return 0;
}
