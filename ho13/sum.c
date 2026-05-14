#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SIZE 1000000
#define T 4

typedef struct {
  const int *array;
  int from;
  int to;
  ssize_t result;
} worker_args;

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

long long sequential_sum(const int *a, int len) {
  ssize_t sum = 0;
  for (int i = 0; i < len; i++) {
    sum += a[i];
  }
  return sum;
}

long long partial_sum(const int *a, int from, int to) {
  long long sum = 0;
  for (int i = from; i < to; i++) {
    sum += a[i];
  }
  return sum;
}

static void job(void *args) {
  worker_args *w = (worker_args *)args;
  pid_t tid = gettid();

  w->result = partial_sum(w->array, w->from, w->to);
  printf("[SUM.c][Thread %d] Somma parziale: %lu\n", tid, w->result);

  pthread_exit(NULL);
}

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

  int step = SIZE / T;
  int from = 0, to = step;
  long long psum = 0;
  for (int i = 0; i < T; i++) {
    pthread_t tid;

    w[i].array = a;
    w[i].from = from;
    w[i].to = to;

    // printf("\n[SUM.c] from: %d, to: %d\n", from, to);
    from = to;
    to += step;

    pthread_create(&tid, NULL, (void *)&job, (void *)&w[i]);
    if (pthread_join(tid, NULL) == 0) {
      psum += w[i].result;
    }
  }

  printf("[SUM.c] Somma parallela (T=%d): %lld\n", T, psum);
  assert(psum == 49500000);

  return 0;
}
