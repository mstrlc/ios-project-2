CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -lpthread

.phony: clean

proj2: proj2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

pack:
	zip proj2.zip *.c *.h Makefile

clean:
	$(RM) *.o *.zip proj2