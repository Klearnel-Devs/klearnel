# All variables used by the module make process

CC:=gcc

INC_DIR:=include
LIB_DIR:=lib

CFLAGS:=-Wall -I$(INC_DIR) -L$(LIB_DIR)
CFLAGS-DEBUG:=$(CFLAGS) -Wextra
CFLAGS-DEBUG-STRICT:=$(CFLAGS-DEBUG) -Werror

BUILD_DIR:=build

SRC:=$(wildcard *.c)
