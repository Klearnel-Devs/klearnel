# Klearnel build makefile

CC = gcc

default:
	INC_DIR = include
	CFLAGS = -Wall -Werror -I$(INC_DIR)

	$(CC) $(CFLAGS) klearnel.c -o klearnel.o

module:

	obj-m += klearnel.o
clean:

	@rm -r *.o
	@echo "The klearnel dir is now clean!"
