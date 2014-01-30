all: jpegsplit

jpegsplit: jpegsplit.o Makefile
	$(CC) $(LDFLAGS) $< -lmagic -o $@

%.o: %.c Makefile
	$(CC) -Wall -std=c99 -D_BSD_SOURCE $(CFLAGS) -c $< -o $@

clean:
	rm -f jpegsplit jpegsplit.o
