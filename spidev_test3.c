/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "rs.h"
#include <assert.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 5000;
static uint16_t delay;

#define NUM_BOARDS 6
#define RS_BYTES_SENT 25 
static uint16_t numbytes=RS_BYTES_SENT*NUM_BOARDS;
//static uint16_t numbytes=25*7;


//MM and KK are defined in rs.h
//MM is 8
//KK is 245
//NN is 255

#define RS_NUM_PARITY_BYTES (NN-KK)					//255-245=10
//#define RS_NUM_ZERO_PAD NN-RS_BYTES_SENT				//255-25=230
//alternate define:
#define RS_NUM_ACTUAL_DATA_BYTES (RS_BYTES_SENT-RS_NUM_PARITY_BYTES)	//25-10=15
#define RS_NUM_ZERO_PAD (KK-RS_NUM_ACTUAL_DATA_BYTES)			//245-15=230

uint8_t default_buff[15]={'{',0,0,0,0,0,0,0,0,0,0,0,0,0,0};

dtype rs_buff[NN];
uint8_t tx[NUM_BOARDS*RS_BYTES_SENT]={
		'{','!','A','6',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','5',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','4',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','3',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','2',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','1',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};		//initialize with enumerated SPI address values


struct termios orig_termios;
struct termios new_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
	//tcsetattr(0, TCSADRAIN, &orig_termios);
}

void putback_terminal_mode()
{
    //tcsetattr(0, TCSANOW, &orig_termios);
	tcsetattr(0, TCSANOW, &new_termios);
	//tcsetattr(0, TCSADRAIN, &new_termios);
	//tcsetattr(0, TCSAFLUSH, &new_termios);
}

void set_conio_terminal_mode()
{
    

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    //tcsetattr(0, TCSANOW, &new_termios);
	tcsetattr(0, TCSADRAIN, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}


static void init_rs_buff(){
int i;
	for (i=0;i<NN;i++){
		rs_buff[i]=0;
		//printf("This is the %ith element of rs_buff: %u\n",i,rs_buff[i]);
	}

}

static void transfer(int fd)
{
	int ret,i,ret_indx;
	int er_pos[RS_NUM_PARITY_BYTES];
	int num_ers=0;
//	uint8_t tx[] = "{6This is the raspi val!}{5This is the raspi val!}{4This is the raspi val!}{3This is the raspi val!}{2This is the raspi val!}{1This is the raspi val!}{0This is the null val!}";
	
	
	for (i=0;i<NUM_BOARDS;i++){
		memcpy(&rs_buff[0],&tx[i*RS_BYTES_SENT],RS_NUM_ACTUAL_DATA_BYTES);
		encode_rs(&rs_buff[0],&rs_buff[KK]);
		memcpy(&tx[i*RS_BYTES_SENT],&rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);
		memcpy(&tx[(i*RS_BYTES_SENT)+RS_NUM_ACTUAL_DATA_BYTES],&rs_buff[KK],RS_NUM_PARITY_BYTES);
		printf("Sending %.*s \n", RS_BYTES_SENT, &tx[i*RS_BYTES_SENT]);
	}
		
	
		//uint8_t tx[]="{6This is the raspi val!}{5This is the raspi val!}{4This is the raspi val!}{3This is the raspi val!}{2This is the raspi val!}{1This is the raspi val!}";
	//uint8_t tx[] = {"We should be sending 32 bytes+2!!            "};
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };
	
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		//.len = ARRAY_SIZE(tx),
		.len =numbytes,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	assert (numbytes<=ARRAY_SIZE(tx));
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

    for (ret_indx=0;ret_indx<(NUM_BOARDS*RS_BYTES_SENT);ret_indx+=RS_BYTES_SENT){
		memcpy(&rs_buff[0],&rx[ret_indx],RS_NUM_ACTUAL_DATA_BYTES);
		memcpy(&rs_buff[KK],&rx[ret_indx+RS_NUM_ACTUAL_DATA_BYTES],RS_NUM_PARITY_BYTES);

		if (rs_buff[0]!='{'){
			er_pos[0]=0;
			num_ers=1;
			printf("\nFirst byte \'{\' erased from board %u\n",(ret_indx/RS_BYTES_SENT));
		}
		else{
			num_ers=0;
		}

		i=eras_dec_rs(rs_buff,er_pos,num_ers);		//decode data
		
		if(i>0){
			memcpy(&rx[ret_indx],&rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);	//if data is corrected, copy it over
			printf("%u Bytes corrected from board #%u\n",i,(ret_indx/RS_BYTES_SENT));
		}
		if(i==0){
			printf("All bytes correct from board #%u\n",(ret_indx/RS_BYTES_SENT));
		}
		if(i==-1){
			printf("Too many corrupted bytes from board #%u\n",(ret_indx/RS_BYTES_SENT));
		}
	}
	for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		if (!(ret % 25))
			puts("");
		printf("%c", rx[ret]);
		//printf("%.2X ", rx[ret]);

	}

	puts("\n");
	for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		if (!(ret % 25))
			puts("");
		//printf("%c", rx[ret]);
		printf("%.2X ", rx[ret]);

	}
	puts("");
	
    
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev1.1)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -d --delay    delay (usec)\n"
	     "  -b --bpw      bits per word \n"
	     "  -l --loop     loopback\n"
	     "  -H --cpha     clock phase\n"
	     "  -O --cpol     clock polarity\n"
	     "  -L --lsb      least significant bit first\n"
	     "  -C --cs-high  chip select active high\n"
	     "  -3 --3wire    SI/SO signals shared\n");
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "bpw",     1, 0, 'b' },
			{ "loop",    0, 0, 'l' },
			{ "cpha",    0, 0, 'H' },
			{ "cpol",    0, 0, 'O' },
			{ "lsb",     0, 0, 'L' },
			{ "cs-high", 0, 0, 'C' },
			{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'R' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NR", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'l':
			mode |= SPI_LOOP;
			break;
		case 'H':
			mode |= SPI_CPHA;
			break;
		case 'O':
			mode |= SPI_CPOL;
			break;
		case 'L':
			mode |= SPI_LSB_FIRST;
			break;
		case 'C':
			mode |= SPI_CS_HIGH;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'N':
			mode |= SPI_NO_CS;
			break;
		case 'R':
			mode |= SPI_READY;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	int new_char=0;
	int kbd_buff_len=0;
	uint8_t input_buffer[RS_NUM_ACTUAL_DATA_BYTES+2]={'{',};
	int keep_alive=1;
	
	init_rs();
	init_rs_buff();
	
	parse_opts(argc, argv);

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	transfer(fd);
	memcpy(&input_buffer[0],&default_buff[0],RS_NUM_ACTUAL_DATA_BYTES);	//clear buffer to default.
	puts("Fungible$ ");
	//printf("Fungible$ ");
	ret=0;
	set_conio_terminal_mode();
	reset_terminal_mode();	//reset the terminal to normal mode	
	putback_terminal_mode();

	
	while(keep_alive==1){
		
		ret=0;
		new_char=getch(); /* get the character */
		reset_terminal_mode();	//reset the terminal to normal mode	
		
		
		
		
	//puts("Fungible$ ");
		
		
		
		if(new_char==0x1B){ //Escape key sent
			keep_alive=0;
			continue;
		}
		
		else if (new_char=='\r'){
			puts("CR caught");
		}
		
		else if(new_char=='\n'){ //command ended
			//copy buffer contents and send
			//then clear buffer
		//reset_terminal_mode();	//reset the terminal to normal mode

			//printf("%.*s \n",kbd_buff_len+2, input_buffer);		
			printf("Sending Buffer: %s\n", input_buffer);		
			printf("\r\ntransferring again\n\n");
			for(ret=0;ret<NUM_BOARDS;ret++){
				memcpy(&tx[RS_BYTES_SENT*ret],&input_buffer[0],RS_NUM_ACTUAL_DATA_BYTES);
			}
			transfer(fd);
			puts("Fungible$ ");
			memcpy(&input_buffer[0],&default_buff[0],RS_NUM_ACTUAL_DATA_BYTES);
			kbd_buff_len=0;
			continue;
		//putback_terminal_mode();
		//set_conio_terminal_mode();
			//printf("\r\nFungible$ ");
		//	puts("Fungible$ ");
		}
		else if((new_char>=' ') && (kbd_buff_len<RS_NUM_ACTUAL_DATA_BYTES-1)){
			input_buffer[++kbd_buff_len]=new_char;
			input_buffer[kbd_buff_len+1]=0;
		//reset_terminal_mode();	//reset the terminal to normal mode
				//printf("%.*s",kbd_buff_len+1, input_buffer);
				//printf("%.*s",kbd_buff_len+1, input_buffer);
				//printf("THis is a test\r");
			
			continue;
		//putback_terminal_mode();
		//set_conio_terminal_mode();
		}
				
		//reset_terminal_mode();	//reset the terminal to normal mode
		//printf("\r\n next char: %c \r\n",new_char);
		//set_conio_terminal_mode();
		
		putback_terminal_mode();
		
		while (!kbhit()) {
			/* do some work */
			//this is where the 
			transfer(fd);
		//	printf("Nothing doing \t\t%u\r",ret);
		
		}
		
		
		
	}
	
	reset_terminal_mode();	//reset the terminal to normal mode
	//reset_terminal_mode();	//reset the terminal to normal mode
	printf("\r\n\nClosing!\n\n");
	
	close(fd);
	
	return ret;
}
