CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -pthread

.phony: clean run zip

proj2: proj2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

zip:
	zip proj2.zip *.c *.h Makefile

clean:
	$(RM) *.o *.zip *.out proj2