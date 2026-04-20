#include "fibo.h"

/*
 * Genera la successione di Fibonacci fino all'n-esimo elemento
 * e la salva nella struttura fibo_sequence_t.
 *
 * La successione segue la formula: F_n = F_(n-1) + F_(n-2)
 * con valori iniziali F_0 = 0, F_1 = 1
 * Esempio: 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, ...
 *
 * Teniamo solo gli ultimi due valori (a e b) invece dell'intera storia,
 * perché ogni nuovo termine dipende solo dai due precedenti.
 */
int fibo_sequence(fibo_sequence_t *fs, int n) {
  int a = 0, b = 1;

  if (fs == NULL) {
    return -1;
  }

  fibo_push(fs, a);

  if (n == 0) return 0;

  fibo_push(fs, b);

  for (int i = 2; i <= n; i++) {
    int temp = a + b;
    a = b;
    b = temp;
    fibo_push(fs, b);
  }

  return 0;
}

/* Inizializza la struttura azzerando l'indice dell'array */
int fibo_init(fibo_sequence_t *fs) {
  if (fs == NULL) {
    return -1;
  }
  fs->idx = 0;
  return 0;
}

/* Aggiunge un elemento in coda all'array e incrementa l'indice */
int fibo_push(fibo_sequence_t *fs, int n) {
  if (fs == NULL) {
    return -1;
  }
  fs->array[fs->idx] = n;
  fs->idx++;
  return 0;
}

/* Stampa tutti gli elementi della sequenza separati da spazio */
int fibo_print(fibo_sequence_t *fs) {
  if (fs == NULL) {
    return -1;
  }
  for (int i = 0; i < fs->idx; i++) {
    printf("%d ", fs->array[i]);
  }
  printf("\n");
  return 0;
}

/*
 * Confronta due sequenze di Fibonacci elemento per elemento.
 * Ritorna 0 se sono uguali, -1 se diverse o se i puntatori sono NULL.
 */
int fibo_compare(fibo_sequence_t *fs1, fibo_sequence_t *fs2) {
  if (fs1 == NULL || fs2 == NULL) {
    return -1;
  }
  if (fs1->idx != fs2->idx) {
    return -1;
  }
  int len = (fs1->idx) - 1;
  for (int i = 0; i <= len; i++) {
    if (fs1->array[i] != fs2->array[i])
      return -1;
  }
  return 0;
}
