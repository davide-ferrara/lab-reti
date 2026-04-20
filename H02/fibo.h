#include "stdio.h"

typedef struct fibo_sequence {
  int array[64];
  int idx;
} fibo_sequence_t;

int fibo_sequence(fibo_sequence_t *fs, int n);

int fibo_init(fibo_sequence_t *fs);
int fibo_print(fibo_sequence_t *fs);
int fibo_push(fibo_sequence_t *fs, int n);
int fibo_compare(fibo_sequence_t *fs1, fibo_sequence_t *fs2);
