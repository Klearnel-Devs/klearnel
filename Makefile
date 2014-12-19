# Klearnel build makefile

CC = gcc

all:

	$(CC) klearnel.c -o klearnel.o

clean:

	@rm -r *.o
	@echo "The klearnel dir is now clean!"
