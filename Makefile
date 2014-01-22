all: jpegsplit

jpegsplit: jpegsplit.o Makefile
	$(CC) $(LDFLAGS) $< -o $@

%.o: %.c Makefile
	$(CC) -Wall $(CFLAGS) -c $< -o $@

clean:
	rm -f jpegsplit jpegsplit.o
