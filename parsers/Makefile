CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
CFLAGS=-static -g -Wall -fPIC -mcpu=5208 

all:
	$(CC) $(CFLAGS) -c -I../ binary.c hex.c
	$(AR) r libparsers.a        binary.o hex.o

clean:
	rm -f *.o libparsers.a
