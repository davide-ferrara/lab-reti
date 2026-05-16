/*
 * Hands-On #15 — Fase 3
 * Mutex aggiunto: corruzione scomparsa, ma deadlock garantito.
 *
 * Compile: gcc -Wall -Wextra -pthread -o fase3 fase3.c
 * Run:     ./fase3
 *
 * Il programma si blocca quando il buffer si riempie o si svuota.
 * Interrompere con Ctrl+C.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define N 4 /* buffer piccolo: deadlock piu' rapido */
#define NITEM 32

typedef struct {
  int buf[N];
  int head, tail, count;
} Buffer;

void buf_put(Buffer *b, int v) {
  b->buf[b->tail] = v;
  b->tail = (b->tail + 1) % N;
  b->count++;
}

int buf_get(Buffer *b) {
  int v = b->buf[b->head];
  b->head = (b->head + 1) % N;
  b->count--;
  return v;
}

typedef struct {
  Buffer b;
  pthread_mutex_t mtx;
  pthread_cond_t not_full;
  pthread_cond_t not_empty;
} Shared;

/*
 * Produttore con mutex.
 *
 * Il mutex protegge count, head, tail: la corruzione scompare.
 * Ma quando il buffer e' pieno, il produttore entra nel while
 * TENENDO il mutex. Il consumatore non puo' acquisirlo per
 * prelevare un elemento. Nessuno dei due avanza: deadlock.
 */
static void *produttore(void *arg) {
  Shared *s = (Shared *)arg;
  for (int i = 0; i < NITEM; i++) {
    pthread_mutex_lock(&s->mtx);

    /*
     * PROBLEMA: il mutex e' tenuto qui dentro.
     * Il consumatore non puo' acquisirlo per svuotare
     * il buffer, quindi count non scendera' mai.
     * Questo loop non terminera' mai.
     */
    while (s->b.count == N) {
      // Se il buffer è pieno aspetto
      pthread_cond_wait(&s->not_full, &s->mtx);
    }

    buf_put(&s->b, i);
    printf("[produttore] inserito %d (count=%d)\n", i, s->b.count);

    // Avendo prodotto un elemento sveglio un thread (non tutti)
    pthread_cond_signal(&s->not_empty);
    pthread_mutex_unlock(&s->mtx);
  }
  return NULL;
}

/*
 * Consumatore con mutex.
 *
 * Stesso problema speculare: quando il buffer e' vuoto,
 * il consumatore aspetta tenendo il mutex e il produttore
 * non puo' inserire.
 */
static void *consumatore(void *arg) {
  Shared *s = (Shared *)arg;
  for (int i = 0; i < NITEM; i++) {
    pthread_mutex_lock(&s->mtx);

    while (s->b.count == 0) {
      pthread_cond_wait(&s->not_empty, &s->mtx);
    }

    int v = buf_get(&s->b);
    printf("[consumatore] prelevato %d (count=%d)\n", v, s->b.count);

    // Segnalo che non è piu pieno e puó essere prodotto un altro elemento
    pthread_cond_signal(&s->not_full);
    pthread_mutex_unlock(&s->mtx);
  }
  return NULL;
}

int main(void) {
  Shared s = {0};
  pthread_mutex_init(&s.mtx, NULL);
  pthread_cond_init(&s.not_full, NULL);
  pthread_cond_init(&s.not_empty, NULL);

  pthread_t tp, tc;
  pthread_create(&tp, NULL, produttore, &s);
  pthread_create(&tc, NULL, consumatore, &s);

  pthread_join(tp, NULL);
  pthread_join(tc, NULL);

  pthread_mutex_destroy(&s.mtx);
  pthread_cond_destroy(&s.not_full);
  pthread_cond_destroy(&s.not_empty);
}
