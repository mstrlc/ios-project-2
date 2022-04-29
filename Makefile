CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -pedantic
LDFLAGS = -pthread

.phony: clean run zip

proj2: proj2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

zip:
	zip proj2.zip *.c *.h Makefile

clean:
	$(RM) *.o *.zip *.out proj2

run:
	make
	./proj2 3 5 100 100
	sleep 0.3
	cat proj2.out