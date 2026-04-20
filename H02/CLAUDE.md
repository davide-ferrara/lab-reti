# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Ruolo principale

Scrivere le tesine in LaTeX per il laboratorio LabReSiD25. Il codice C è già scritto dallo studente; il compito è documentarlo nel report.

## Build

```bash
make                        # server_tcp_block, server_tcp_nonblock, client
make clean
gcc -Wall -Wextra -o <target> fibo.c <file>.c   # compilazione diretta
```

Report:
```bash
cd report && pdflatex HO2.tex
```

## Run

```bash
./server_tcp_block
./server_tcp_nonblock
./client <n> [reps]         # invia F(0)..F(n), reps volte sulla stessa connessione
```

## Architettura del codice

`fibo.h/fibo.c` — libreria condivisa. `fibo_sequence_t`: array[64] + idx. `fibo_compare` ritorna 0 se uguali. Tutti i binari linkano `fibo.c`.

Protocollo: client invia `"0 1 1 2 3 5\n"` via TCP. Server fa parsing con `parse_msg()` (`char - '0'`), genera sequenza attesa, confronta, risponde `ACK\n` o `NACK\n`. Connessione persistente: più messaggi per connessione.

`server_tcp_block` vs `server_tcp_nonblock`: stessa struttura a doppio loop. Il non-bloccante aggiunge `set_nonblocking(fd)` via `fcntl(O_NONBLOCK)` e gestisce `errno == EAGAIN` con `continue`. Comportamento esterno identico (un client alla volta); differenza: attesa attiva vs sleep.

## Regole per scrivere le tesine LaTeX

### Stile testo
- Italiano semplice, frasi brevi — come scriverebbe uno studente, non un manuale
- Niente frasi da AI: "vale la pena notare", "è importante sottolineare", "come vedremo", riferimenti a lezioni future
- Niente sezioni fuori traccia: no Compilazione, no fibo.h/fibo.c, no argomenti non richiesti

### Struttura sezioni (seguire HO2 attuale)
```
\section*{Traccia}
\section{Client}           % prima il client
  \subsection{Obbiettivo}
  \subsection{client.c}
    \subsubsection{...}
  \subsection{Output di esempio}
\section{Server TCP Bloccante}
  \subsection{Obbiettivo}
  \subsection{server_tcp_block.c}
    \subsubsection{Include e costanti}
    \subsubsection{funzione/blocco logico}
    ...
  \subsection{Limitazione}
  \subsection{Output di esempio}
\section{Server TCP Non Bloccante}
  ...
```

### Listing di codice
- Estratti rilevanti, non il file intero
- I commenti nei listing **devono essere identici** ai commenti nei file .c
- Accenti dentro `lstlisting`: usare ASCII (`finche'`, `puo'`, `e'`, `ritornera'`) — non UTF-8
- Ogni listing seguito da `\FloatBarrier`

### Output di esempio
- Output reale copiato da terminale
- Se lungo, si può riassumere tenendo le righe significative (connessione, risposta, disconnessione)
- Usare `[style=output]` nei listing di output
