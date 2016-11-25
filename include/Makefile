# All variables used by the module make process
CC:=gcc

INC_DIR:=include
LIB_DIR:=lib

CFLAGS:=-Wall -I$(INC_DIR) -Wextra

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    CFLAGS += -D OSX -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib  
endif

BUILD_DIR:=build

SRC:=$(wildcard *.c)