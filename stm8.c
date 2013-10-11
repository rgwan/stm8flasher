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

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "stm8.h"
#include "utils.h"
#include "e_w_routines.h"

#define STM8_ACK	0x79
#define STM8_NACK	0x1F
#define STM8_BUSY	0xAA
#define STM8_CMD_INIT	0x7F
#define STM8_CMD_GET	0x00	/* get the version and command supported */


struct stm8_cmd {
	uint8_t get;
	uint8_t rm;
	uint8_t go;
	uint8_t wm;
	uint8_t er; /* this may be extended erase */
};

/* device table */
const stm8_dev_t devices[] = {
	{0x010, "Medium density STM8S 32kB", 0x000000, 0x0007FF, 0x008000, 0x00FFFF, 1, 512, 0x004800, 0x00487F, 0x004000, 0x0043FF},
	{0x012, "Medium density STM8S 32kB", 0x000000, 0x0007FF, 0x008000, 0x00FFFF, 1, 512, 0x004800, 0x00487F, 0x004000, 0x0043FF},
	{0x013, "Medium density STM8S 32kB", 0x000000, 0x0007FF, 0x008000, 0x00FFFF, 1, 512, 0x004800, 0x00487F, 0x004000, 0x0043FF},
	{0x020, "High density STM8S 128kB", 0x000000, 0x0007FF, 0x008000, 0x027FFF, 1, 512, 0x004800, 0x00487F, 0x004000, 0x0047FF},
	{0x021, "High density STM8S 128kB", 0x000000, 0x0007FF, 0x008000, 0x027FFF, 1, 512, 0x004800, 0x00487F, 0x004000, 0x0047FF},
	{0x022, "High density STM8S 128kB", 0x000000, 0x0007FF, 0x008000, 0x027FFF, 1, 512, 0x004800, 0x00487F, 0x004000, 0x0047FF},
	{0x0}
};


extern FILE *fp_stderr;

/* internal functions */
uint8_t stm8_gen_cs(const uint32_t v);
void    stm8_send_byte(const stm8_t *stm, uint8_t byte);
uint8_t stm8_read_byte(const stm8_t *stm);
char    stm8_send_command(const stm8_t *stm, const uint8_t cmd);

/* stm8 programs */
extern unsigned int	stmreset_length;
extern unsigned char	stmreset_binary[];

uint8_t stm8_gen_cs(const uint32_t v) {
	return  ((v & 0xFF000000) >> 24) ^
		((v & 0x00FF0000) >> 16) ^
		((v & 0x0000FF00) >>  8) ^
		((v & 0x000000FF) >>  0);
}

void stm8_send_byte(const stm8_t *stm, uint8_t byte) {	
	serial_err_t err;
	err = serial_write(stm->serial, &byte, 1);
	if (err != SERIAL_ERR_OK) {
		perror("send_byte");
		assert(0);
	}
}

uint8_t stm8_read_byte(const stm8_t *stm) {
	uint8_t byte;
	serial_err_t err;
	err = serial_read(stm->serial, &byte, 1);
	if (err != SERIAL_ERR_OK) {
		perror("read_byte");
		assert(0);
	}
	//REPLY-MODE
	stm8_send_byte(stm, byte);
	return byte;
}

char stm8_send_command(const stm8_t *stm, const uint8_t cmd) {
	stm8_send_byte(stm, cmd);
	stm8_send_byte(stm, cmd ^ 0xFF);
	if (stm8_read_byte(stm) != STM8_ACK) {
		fprintf(fp_stderr, "Error sending command 0x%02x to device\n", cmd);
		return 0;
	}
	return 1;
}


struct stm8_dev *stm8_get_device(char bl_version)
{
	int i=0;

	do {
		if(devices[i].id == bl_version)
		{
			
			return (struct stm8_dev*)&(devices[i]);
		}
	} while(devices[++i].id);

	return NULL;
}

uint8_t *stm8_get_e_w_routine(int *len, char bl_version)
{
	int i;

	for(i = 0; i < E_W_ROUTINES_NUM ;i++)
	{
		if(e_w_routines[i].bl_version == bl_version)
		{
			*len = *(e_w_routines[i].bytes);
			return e_w_routines[i].data;
		}
	}

	return NULL;	
}
	

stm8_t* stm8_init(const serial_t *serial, const char init) {
	uint8_t len;
	stm8_t *stm;
	int routine_len;
	int routine_offset;
	uint8_t *routine_data;

	stm      = calloc(sizeof(stm8_t), 1);
	stm->cmd = calloc(sizeof(stm8_cmd_t), 1);
	stm->serial = serial;

	if (init) {
		stm8_send_byte(stm, STM8_CMD_INIT);
		if (stm8_read_byte(stm) != STM8_ACK) {
			stm8_close(stm);
			fprintf(fp_stderr, "Failed to get init ACK from device\n");
			return NULL;
		}
	}

	/* get the bootloader information */
	if (!stm8_send_command(stm, STM8_CMD_GET)) return 0;
	len              = stm8_read_byte(stm) + 1;
	stm->bl_version  = stm8_read_byte(stm); --len;
	stm->cmd->get    = stm8_read_byte(stm); --len;
	stm->cmd->rm     = stm8_read_byte(stm); --len;
	stm->cmd->go     = stm8_read_byte(stm); --len;
	stm->cmd->wm     = stm8_read_byte(stm); --len;
	stm->cmd->er     = stm8_read_byte(stm); --len;
	if (len > 0) {
		fprintf(fp_stderr, "Seems this bootloader returns more then we understand in the GET command, we will skip the unknown bytes\n");
		while(len-- > 0) stm8_read_byte(stm);
	}

	if (stm8_read_byte(stm) != STM8_ACK) {
		stm8_close(stm);
		return NULL;
	}


	//FIX ME: Points to first device in List
	stm->dev = stm8_get_device(stm->bl_version); 
	if(!stm->dev)
	{
		fprintf(fp_stderr, "Device Information not found - check device table in stm8.c\n");
		return NULL;	
	}


	routine_data = stm8_get_e_w_routine(&routine_len,stm->bl_version);

	if(!routine_data)
	{
		fprintf(fp_stderr, "Erase and Write Routines for Bootloader-Version not found!\n");
		return NULL;	
	}

	routine_offset = 0x0;

	while(routine_len)
	{
		if(routine_len > 128)
		{
			if(!stm8_write_memory(stm, 0xA0 + routine_offset,&routine_data[routine_offset],128))
				return 0;
			routine_len-=128;
			routine_offset+=128;
		} else {
			if(!stm8_write_memory(stm, 0xA0 + routine_offset,&routine_data[routine_offset],routine_len))
				return 0;
			routine_len=0;
		}
	}

	return stm;
}

void stm8_close(stm8_t *stm) {
	if (stm) free(stm->cmd);
	free(stm);
}

char stm8_read_memory(const stm8_t *stm, uint32_t address, uint8_t data[], unsigned int len) {
	uint8_t cs;
	unsigned int i;
	assert(len > 0 && len < 257);

	/* must be 32bit aligned */
	//assert(address % 4 == 0);

	address = be_u32      (address);
	cs      = stm8_gen_cs(address);

	if (!stm8_send_command(stm, stm->cmd->rm)) return 0;
	assert(serial_write(stm->serial, &address, 4) == SERIAL_ERR_OK);
	stm8_send_byte(stm, cs);
	if (stm8_read_byte(stm) != STM8_ACK) return 0;

	i = len - 1;
	stm8_send_byte(stm, i);
	stm8_send_byte(stm, i ^ 0xFF);
	if (stm8_read_byte(stm) != STM8_ACK) return 0;

	i=0;
	while(len--) data[i++] = stm8_read_byte(stm);

//	assert(serial_read(stm->serial, data, len) == SERIAL_ERR_OK);
	return 1;
}

char stm8_write_memory(const stm8_t *stm, uint32_t address, uint8_t data[], unsigned int len) {
	uint8_t cs;
	unsigned int i;
//	int c;
//	int extra;
	char ack;
	assert(len > 0 && len < 129);

//	/* must be 32bit aligned */
//	assert(address % 4 == 0);

	address = be_u32      (address);
	cs      = stm8_gen_cs(address);

	/* send the address and checksum */
	if (!stm8_send_command(stm, stm->cmd->wm)) return 0;
	assert(serial_write(stm->serial, &address, 4) == SERIAL_ERR_OK);
	stm8_send_byte(stm, cs);

	do {
		ack = stm8_read_byte(stm);
	} while (ack == STM8_BUSY);	
	if (ack != STM8_ACK) return 0;

	/* setup the cs and send the length */
//	extra = len % 4;
//	cs = len - 1 + extra;
	cs = len - 1;
	stm8_send_byte(stm, cs);

	/* write the data and build the checksum */
	for(i = 0; i < len; ++i)
		cs ^= data[i];

	assert(serial_write(stm->serial, data, len) == SERIAL_ERR_OK);

	/* write the alignment padding */
/*	for(c = 0; c < extra; ++c) {
		stm8_send_byte(stm, 0xFF);
		cs ^= 0xFF;
	}
*/

	/* send the checksum */
	stm8_send_byte(stm, cs);

	do {
		ack = stm8_read_byte(stm);
	} while (ack == STM8_BUSY);	
	return ack == STM8_ACK;

}

char stm8_erase_memory(const stm8_t *stm, uint8_t pages) {
	char ack;

	if (!stm8_send_command(stm, stm->cmd->er)) return 0;
	if (pages == 0xFF) {
		return stm8_send_command(stm, 0xFF);
	} else {
		uint8_t cs = 0;
		uint8_t pg_num;
		stm8_send_byte(stm, pages);
		cs ^= pages;
		for (pg_num = 0; pg_num <= pages; pg_num++) {
			stm8_send_byte(stm, pg_num);
			cs ^= pg_num;
		}
		stm8_send_byte(stm, cs);

		do {
			ack = stm8_read_byte(stm);
		} while (ack == STM8_BUSY);	
		return ack == STM8_ACK;
	}
}

char stm8_go(const stm8_t *stm, uint32_t address) {
	uint8_t cs;

	address = be_u32      (address);
	cs      = stm8_gen_cs(address);

	if (!stm8_send_command(stm, stm->cmd->go)) return 0;
	serial_write(stm->serial, &address, 4);
	serial_write(stm->serial, &cs     , 1);

	return stm8_read_byte(stm) == STM8_ACK;
}

char stm8_reset_device(const stm8_t *stm) {
	/*
		since the bootloader does not have a reset command, we
		upload the stmreset program into ram and run it, which
		resets the device for us
	*/

//For now we stick with DTR Reset

/* 
	uint32_t length		= stmreset_length;
	unsigned char* pos	= stmreset_binary;
	uint32_t address	= stm->dev->ram_start;
	while(length > 0) {
		uint32_t w = length > 256 ? 256 : length;
		if (!stm8_write_memory(stm, address, pos, w))
			return 0;

		address	+= w;
		pos	+= w;
		length	-= w;
	}

	return stm8_go(stm, stm->dev->ram_start);
*/
	return 1;

}

