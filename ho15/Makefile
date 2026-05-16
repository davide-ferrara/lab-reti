CC      = gcc
CFLAGS  = -Wall -Wextra
PTHREAD = -pthread

TARGETS = step1 step2 step3 step4

all: $(TARGETS)

step1: step1.c
	$(CC) $(CFLAGS) -o $@ $<

step2: step2.c
	$(CC) $(CFLAGS) $(PTHREAD) -o $@ $<

step3: step3.c
	$(CC) $(CFLAGS) $(PTHREAD) -o $@ $<

step4: step4.c
	$(CC) $(CFLAGS) $(PTHREAD) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
