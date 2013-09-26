CC=$(CROSS_COMPILE)gcc
CFLAGS=-static -g -Wall -fPIC -mcpu=5208 -DLANTRONIX_CPM
INCLUDES=-I$(ROOTDIR)/include -I$(ROOTDIR)/user/lantronix/libcp -I./parsers -I.
LDFLAGS=-static -g -fPIC -lparsers -lcp -lm
LIBRARIES=-L$(ROOTDIR)/user/lantronix/libcp -L$(ROOTDIR)/lib -L./parsers
SOURCES=main.c utils.c stm8.c e_w_routines.c serial_common.c serial_platform.c  
OBJECTS=$(SOURCES:.c=.o)



all: lib_parsers $(SOURCES) stm8flash

stm8flash: $(OBJECTS) 
	$(CC) $(OBJECTS) $(LIBRARIES) $(LDFLAGS) -o $@

lib_parsers:
	$(MAKE) -C parsers

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -f *.o
	rm -f stm8flash
	rm -f stm8flash.gdb
	$(MAKE) -C parsers clean
	
romfs:
