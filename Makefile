CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
export CC
export AR

all:
	$(MAKE) -C parsers
	$(CC) -g -o stm8flash -I./ \
		main.c \
		utils.c \
		stm8.c \
		e_w_routines.c \
		serial_common.c \
		serial_platform.c \
		parsers/parsers.a \
		-Wall

clean:
	$(MAKE) -C parsers clean
	rm -f stm8flash
	rm -f stm8flash.gdb

romfs:
