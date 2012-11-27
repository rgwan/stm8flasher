/*
  stm8flash - Open Source ST STM8 flash program for *nix
  Copyright (C) 2010 Geoffrey McRae <geoff@spacevs.com>
  adapted for STM8 - by Georg Ottinger <g.ottinger@gmx.at>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef _STM8_H
#define _STM8_H

#include <stdint.h>
#include "serial.h"

typedef struct stm8		stm8_t;
typedef struct stm8_cmd	stm8_cmd_t;
typedef struct stm8_dev	stm8_dev_t;

struct stm8 {
	const serial_t		*serial;
	uint8_t			bl_version;
	uint8_t			version;
	uint8_t			option1, option2;
	uint16_t		pid;
	stm8_cmd_t		*cmd;
	const stm8_dev_t	*dev;
};

struct stm8_dev {
	uint16_t	id;
	char		*name;
	uint32_t	ram_start, ram_end;
	uint32_t	fl_start, fl_end;
	uint16_t	fl_pps; // pages per sector
	uint16_t	fl_ps;  // page size
	uint32_t	opt_start, opt_end;
	uint32_t	mem_start, mem_end;
};

stm8_t* stm8_init      (const serial_t *serial, const char init);
void stm8_close         (stm8_t *stm);
char stm8_read_memory   (const stm8_t *stm, uint32_t address, uint8_t data[], unsigned int len);
char stm8_write_memory  (const stm8_t *stm, uint32_t address, uint8_t data[], unsigned int len);
char stm8_erase_memory  (const stm8_t *stm, uint8_t pages);
char stm8_go            (const stm8_t *stm, uint32_t address);
char stm8_reset_device  (const stm8_t *stm);
uint8_t *stm8_get_e_w_routine(int *len, char bl_version);


#endif

