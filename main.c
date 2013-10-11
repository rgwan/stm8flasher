/*
  stm32flash - Open Source ST STM32 flash program for *nix
  Copyright (C) 2010 Geoffrey McRae <geoff@spacevs.com>
..Copyright (C) 2011 Steve Markgraf <steve@steve-m.de>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "utils.h"
#include "serial.h"
#include "stm8.h"
#include "parser.h"

#ifdef LANTRONIX_CPM
#endif

#include "parsers/binary.h"
#include "parsers/hex.h"

FILE *fp_stdout;
FILE *fp_stderr;

/* device globals */
serial_t	*serial		= NULL;
stm8_t		*stm		= NULL;

void		*p_st		= NULL;
parser_t	*parser		= NULL;

/* settings */
char		*device		= NULL;
serial_baud_t	baudRate	= SERIAL_BAUD_115200;
int		rd	 	= 0;
int		wr		= 0;
int		wu		= 0;
int 		eb 		= 0;
int		dtr_reset	= 0;
int		cpm_reset_flag	= 0;
int 	        redirect_stderr_stdout = 0;

int		npages		= 0xFF;
char		verify		= 0;
int		retry		= 10;
char		exec_flag	= 0;
uint32_t	execute		= 0;
char		init_flag	= 1;
char		force_binary	= 0;
char		reset_flag	= 1;
char		*filename;

/* functions */
int  parse_options(int argc, char *argv[]);
void show_help(char *name);

#ifdef LANTRONIX_CPM
void cpm_reset()
{
	system("cpm -N STM8_RESET -V 1");
	system("cpm -N STM8_RESET -V 0");
	usleep(50000);
}
#endif

int isMemZero(uint8_t *data, int len)
{
	if(len <= 0) return 0;

	do {
		len--;
		if(data[len]) return 0;
	} while(len);
	
	return 1;
}
		

int main(int argc, char* argv[]) {
	int ret = 1;
	parser_err_t perr;

	

	fp_stdout = stdout; fp_stderr = stderr;


	fprintf(fp_stdout,"stm8flash based on stm32flash - http://stm32flash.googlecode.com/\n\n");
	if (parse_options(argc, argv) != 0)
		goto close;

	if(redirect_stderr_stdout)
	{
		fp_stdout=fopen("/tmp/stm8flasher.stdout","a");
		fp_stderr=fopen("/tmp/stm8flasher.stderr","a");	
	}

	if (wr) {
		/* first try hex */
		if (!force_binary) {
			parser = &PARSER_HEX;
			p_st = parser->init();
			if (!p_st) {
				fprintf(fp_stderr, "%s Parser failed to initialize\n", parser->name);
				goto close;
			}
		}

		if (force_binary || (perr = parser->open(p_st, filename, 0)) != PARSER_ERR_OK) {
			if (force_binary || perr == PARSER_ERR_INVALID_FILE) {
				if (!force_binary) {
					parser->close(p_st);
					p_st = NULL;
				}

				/* now try binary */
				parser = &PARSER_BINARY;
				p_st = parser->init();
				if (!p_st) {
					fprintf(fp_stderr, "%s Parser failed to initialize\n", parser->name);
					goto close;
				}
				perr = parser->open(p_st, filename, 0);
			}

			/* if still have an error, fail */
			if (perr != PARSER_ERR_OK) {
				fprintf(fp_stderr, "%s ERROR: %s\n", parser->name, parser_errstr(perr));
				if (perr == PARSER_ERR_SYSTEM) perror(filename);
				goto close;
			}
		}

		fprintf(fp_stdout, "Using Parser : %s\n", parser->name);
	} else {
		parser = &PARSER_BINARY;
		p_st = parser->init();
		if (!p_st) {
			fprintf(fp_stderr, "%s Parser failed to initialize\n", parser->name);
			goto close;
		}
	}

	serial = serial_open(device);
	if (!serial) {
		perror(device);
		goto close;
	}

	if (serial_setup(
		serial,
		baudRate,
		SERIAL_BITS_8,
//		SERIAL_PARITY_EVEN,
// REPLY-MODE
		SERIAL_PARITY_NONE,
		SERIAL_STOPBIT_1
	) != SERIAL_ERR_OK) {
		perror(device);
		goto close;
	}


	if(dtr_reset)
	{
		serial_dtr_reset(serial);
		usleep(10000); //FIXME
	}

#ifdef LANTRONIX_CPM
	if(cpm_reset_flag)
	{
		cpm_reset();
	}
#endif
	
	fprintf(fp_stdout,"Serial Config: %s\n", serial_get_setup_str(serial));
	if (!(stm = stm8_init(serial, init_flag))) goto close;

	fprintf(fp_stdout,"BL-Version   : 0x%02x\n", stm->bl_version);
/*	fprintf(fp_stdout,"Option 1     : 0x%02x\n", stm->option1);
	fprintf(fp_stdout,"Option 2     : 0x%02x\n", stm->option2);
	fprintf(fp_stdout,"Device ID    : 0x%04x (%s)\n", stm->pid, stm->dev->name);
	fprintf(fp_stdout,"RAM          : %dKiB  (%db reserved by bootloader)\n", (stm->dev->ram_end - 0x20000000) / 1024, stm->dev->ram_start - 0x20000000);
	fprintf(fp_stdout,"Flash        : %dKiB (sector size: %dx%d)\n", (stm->dev->fl_end - stm->dev->fl_start ) / 1024, stm->dev->fl_pps, stm->dev->fl_ps);
	fprintf(fp_stdout,"Option RAM   : %db\n", stm->dev->opt_end - stm->dev->opt_start);
	fprintf(fp_stdout,"System RAM   : %dKiB\n", (stm->dev->mem_end - stm->dev->mem_start) / 1024);
*/

	uint8_t		buffer[256];
	uint8_t		wbuffer[128];
	uint32_t	addr;
	unsigned int	len;
	int		failed = 0;

	if (rd) {
		fprintf(fp_stdout,"\n");

		if ((perr = parser->open(p_st, filename, 1)) != PARSER_ERR_OK) {
			fprintf(fp_stderr, "%s ERROR: %s\n", parser->name, parser_errstr(perr));
			if (perr == PARSER_ERR_SYSTEM) perror(filename);
			goto close;
		}

		addr = stm->dev->fl_start;
		fprintf(fp_stdout, "\x1B[s");
		fflush(fp_stdout);
		while(addr < stm->dev->fl_end) {
			uint32_t left	= stm->dev->fl_end - addr;
			len		= sizeof(buffer) > left ? left : sizeof(buffer);
			if (!stm8_read_memory(stm, addr, buffer, len)) {
				fprintf(fp_stderr, "Failed to read memory at address 0x%08x, target write-protected?\n", addr);
				goto close;
			}
			assert(parser->write(p_st, buffer, len) == PARSER_ERR_OK);
			addr += len;

			fprintf(fp_stdout,
				"\x1B[uRead address 0x%08x (%.2f%%) ",
				addr,
				(100.0f / (float)(stm->dev->fl_end - stm->dev->fl_start)) * (float)(addr - stm->dev->fl_start)
			);
			fflush(fp_stdout);
		}
		fprintf(fp_stdout,	"Done.\n");
		ret = 0;
		goto close;

	} else if (wu) {

//FIXME: Write-unprotecting on STM8 ????
//		fprintf(fp_stdout, "Write-unprotecting flash\n");
		/* the device automatically performs a reset after the sending the ACK */
//		reset_flag = 0;
//		stm8_wunprot_memory(stm);
//		fprintf(fp_stdout,	"Done.\n");
//FIXME: END
	} else if (eb) {

		addr = 0x487E; //OPTBL
		if (!stm8_write_memory(stm, addr, (uint8_t *)"\x55\xAA", 2)) {
			fprintf(fp_stderr, "Failed to OPTION Bytes memory at address 0x%08x\n", addr);
			goto close;
		}

	} else if (wr) {
		fprintf(fp_stdout,"\n");

		off_t 	offset = 0;
		ssize_t r;
		unsigned int size = parser->size(p_st);

		// Binaryfiles are put at beginning of Flash (0x8000) - Intel Hexfiles include the correct adress
		if(parser == &PARSER_HEX) size -= 0x8000;

		if (size > stm->dev->fl_end - stm->dev->fl_start) {
			fprintf(fp_stderr,"Size: %d Flash-Start: %x Flash-End: %x\n", size, stm->dev->fl_start, stm->dev->fl_end);
			fprintf(fp_stderr, "File provided larger then available flash space.\n");
			goto close;
		}

		stm8_erase_memory(stm, npages);

		addr = stm->dev->fl_start;
		fprintf(fp_stdout, "\x1B[s");
		fflush(fp_stdout);
		while(addr < stm->dev->fl_end && offset < size) {
			uint32_t left	= stm->dev->fl_end - addr;
			len		= sizeof(wbuffer) > left ? left : sizeof(wbuffer);
			len		= len > size - offset ? size - offset : len;

			if (parser->read(p_st, wbuffer, &len) != PARSER_ERR_OK)
				goto close;
	
			again:
			if(!isMemZero(wbuffer,len))
			{
				if (!stm8_write_memory(stm, addr, wbuffer, len)) {
					fprintf(fp_stderr, "Failed to write memory at address 0x%08x\n", addr);
					goto close;
				}
			}
			if (verify) {
				uint8_t compare[len];
				if (!stm8_read_memory(stm, addr, compare, len)) {
					fprintf(fp_stderr, "Failed to read memory at address 0x%08x\n", addr);
					goto close;
				}

				for(r = 0; r < len; ++r)
					if (wbuffer[r] != compare[r]) {
						if (failed == retry) {
							fprintf(fp_stderr, "Failed to verify at address 0x%08x, expected 0x%02x and found 0x%02x\n",
								(uint32_t)(addr + r),
								wbuffer [r],
								compare[r]
							);
							goto close;
						}
						++failed;
						goto again;
					}

				failed = 0;
			}

			addr	+= len;
			offset	+= len;

			fprintf(fp_stdout,
				"\x1B[uWrote %saddress 0x%08x (%.2f%%) ",
				verify ? "and verified " : "",
				addr,
				(100.0f / size) * offset
			);
			fflush(fp_stdout);

		}

		fprintf(fp_stdout,	"Done.\n");
		ret = 0;
		goto close;
	} else
		ret = 0;

close:
	if (stm && exec_flag && ret == 0) {
		if (execute == 0)
			execute = stm->dev->fl_start;

		fprintf(fp_stdout, "\nStarting execution at address 0x%08x... ", execute);
		fflush(fp_stdout);
		if (stm8_go(stm, execute)) {
			reset_flag = 0;
			fprintf(fp_stdout, "done.\n");
		} else
			fprintf(fp_stdout, "failed.\n");
	}

	if (stm && reset_flag) {
		fprintf(fp_stdout, "\nResetting device... ");
		fflush(fp_stdout);
		if(dtr_reset)
		{
			serial_dtr_reset(serial);
			usleep(10000); //FIXME	
#ifdef LANTRONIX_CPM
		} else if (cpm_reset_flag) {
			cpm_reset();
			usleep(30000);
#endif		
		} else {		
			if (stm8_reset_device(stm)) {fprintf(fp_stdout, "done.\n");}
			else { fprintf(fp_stdout, "failed.\n"); }
		}
	}

	if (p_st  ) parser->close(p_st);
	if (stm   ) stm8_close  (stm);
	if (serial) serial_close (serial);
//	if (redirect_stderr_stdout)
	{
		fclose(fp_stderr);
		fclose(fp_stdout);
	}
	fprintf(fp_stdout,"\n");
	return ret;
}

int parse_options(int argc, char *argv[]) {
	int c;
	while((c = getopt(argc, argv, "b:r:w:e:vn:g:fchudsql")) != -1) {
		switch(c) {
			case 'b':
				baudRate = serial_get_baud(strtoul(optarg, NULL, 0));
				if (baudRate == SERIAL_BAUD_INVALID) {
					fprintf(fp_stderr,	"Invalid baud rate, valid options are:\n");
					for(baudRate = SERIAL_BAUD_1200; baudRate != SERIAL_BAUD_INVALID; ++baudRate)
						fprintf(fp_stderr, " %d\n", serial_get_baud_int(baudRate));
					return 1;
				}
				break;
			case 'r':
			case 'w':
			case 'l':
				rd = rd || c == 'r';
				wr = wr || c == 'w';
				eb = eb || c == 'l';
				if ((rd && wr) || (rd && eb) || (wr && eb) ) {
					fprintf(fp_stderr, "ERROR: Invalid options, can't read & write or enable bootloader at the same time\n");
					return 1;
				}
				filename = optarg;
				break;
			case 'e':
				npages = strtoul(optarg, NULL, 0);
				if (npages > 0xFF || npages < 0) {
					fprintf(fp_stderr, "ERROR: You need to specify a page count between 0 and 255");
					return 1;
				}
				break;
			case 'u':
				wu = 1;
				if (rd || wr) {
					fprintf(fp_stderr, "ERROR: Invalid options, can't write unprotect and read/write at the same time\n");
					return 1;
				}
				break;
			case 'v':
				verify = 1;
				break;

			case 'n':
				retry = strtoul(optarg, NULL, 0);
				break;

			case 'g':
				exec_flag = 1;
				execute   = strtoul(optarg, NULL, 0);
				break;

			case 'f':
				force_binary = 1;
				break;

			case 'c':
				init_flag = 0;
				break;
			case 'd':
				dtr_reset = 1;
				break;
			case 's':
				cpm_reset_flag = 1;
				break;
			case 'q':
				redirect_stderr_stdout = 1;
				break;
			case 'h':
				show_help(argv[0]);
				return 1;
		}
	}

	for (c = optind; c < argc; ++c) {
		if (device) {
			fprintf(fp_stderr, "ERROR: Invalid parameter specified\n");
			show_help(argv[0]);
			return 1;
		}
		device = argv[c];
	}

	if (device == NULL) {
		fprintf(fp_stderr, "ERROR: Device not specified\n");
		show_help(argv[0]);
		return 1;
	}

	if (!wr && verify) {
		fprintf(fp_stderr, "ERROR: Invalid usage, -v is only valid when writing\n");
		show_help(argv[0]);
		return 1;
	}

	return 0;
}

void show_help(char *name) {
	fprintf(stderr,
		"Usage: %s [-bvngfhc] [-[rw] filename] /dev/ttyS0\n"
		"	-b rate		Baud rate (default 115200)\n"
		"	-r filename	Read flash to file\n"
		"	-w filename	Write flash to file\n"
		"	-l		Enable STM8 Bootloader OPTION-Bytes\n"
		"	-u		Disable the flash write-protection\n"
		"	-e n		Only erase n pages before writing the flash\n"
		"	-v		Verify writes\n"
		"	-n count	Retry failed writes up to count times (default 10)\n"
		"	-g address	Start execution at specified address (0 = flash start)\n"
		"	-f		Force binary parser\n"
		"	-h		Show this help\n"
		"	-d		Use DTR-Line for Reset (Arduino-Style ;) )\n"
#ifdef LANTRONIX_CPM
		"	-s		OggStreamer/Lantronix XportPRO CPM STM8 Reset\n"
#endif
		"	-q		redirect stdout and stderr to /tmp/stm8flasher.stdout(stderr)\n"
		"	-c		Resume the connection (don't send initial INIT)\n"
		"			*Baud rate must be kept the same as the first init*\n"
		"			This is useful if the reset fails\n"
		"\n"
		"Examples:\n"
		"	Get device information:\n"
		"		%s /dev/ttyS0\n"
		"\n"
		"	Write with verify and then start execution:\n"
		"		%s -w filename -v -g 0x0 /dev/ttyS0\n"
		"\n"
		"	Read flash to file:\n"
		"		%s -r filename /dev/ttyS0\n"
		"\n"
		"	Start execution:\n"
		"		%s -g 0x0 /dev/ttyS0\n",
		name,
		name,
		name,
		name,
		name
	);
}

