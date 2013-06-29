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
#include "rs_big.h"
#include <assert.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>                // for gettimeofday()

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

int keep_alive;

char *host_ip = "127.0.0.1"; 	//Here we have used localhost address for host machine 
short host_port = 1234; 
int host_sock;
char datagram[512];
fd_set readfds;
struct timeval tv;
struct timespec ntv;
struct sockaddr_in host_add;

static void pabort(const char *s)
{
	perror(s);
	abort();
}

struct timeval time_holder;
struct timeval time_start;
long	time_stamp;

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 5000;
static uint16_t delay;

#define NUM_BOARDS 6
#define RS_BYTES_SENT 165 
static uint16_t numbytes=RS_BYTES_SENT*NUM_BOARDS;

//static uint16_t numbytes=25*7;


//MM and KK are defined in rs.h
//MM is 8
//KK is 245
//NN is 255

#define RS_NUM_PARITY_BYTES (NN-KK)					//55
//#define RS_NUM_ZERO_PAD NN-RS_BYTES_SENT				//255-25=230
//alternate define:
#define RS_NUM_ACTUAL_DATA_BYTES (RS_BYTES_SENT-RS_NUM_PARITY_BYTES)	//110
#define RS_NUM_ZERO_PAD (KK-RS_NUM_ACTUAL_DATA_BYTES)			//90

dtype rs_buff[NN];
//will have to change this properly
uint8_t tx[NUM_BOARDS*RS_BYTES_SENT];
/*={
		'{','!','A','6',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','5',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','4',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','3',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','2',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'{','!','A','1',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};		//initialize with enumerated SPI address values
*/
uint8_t rx[ARRAY_SIZE(tx)] = {0, };


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
	memset(rs_buff,0,NN);
	/*
	for (i=0;i<NN;i++){
		rs_buff[i]=0;
		//printf("This is the %ith element of rs_buff: %u\n",i,rs_buff[i]);
	}
	*/

}

static int transfer(int fd)
{
	int ret,i,ret_indx;
	int er_pos[RS_NUM_PARITY_BYTES];
	int num_ers=0;
	int ret_val=0;
	struct timeval tick,tock;
	long tdiff;
//	uint8_t tx[] = "{6This is the raspi val!}{5This is the raspi val!}{4This is the raspi val!}{3This is the raspi val!}{2This is the raspi val!}{1This is the raspi val!}{0This is the null val!}";
	
	
	for (i=0;i<NUM_BOARDS;i++){
		memcpy(&rs_buff[0],&tx[i*RS_BYTES_SENT],RS_NUM_ACTUAL_DATA_BYTES);
		//no need to encode data if all data is zero
		encode_rs(&rs_buff[0],&rs_buff[KK]);
		memcpy(&tx[i*RS_BYTES_SENT],&rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);
		memcpy(&tx[(i*RS_BYTES_SENT)+RS_NUM_ACTUAL_DATA_BYTES],&rs_buff[KK],RS_NUM_PARITY_BYTES);
		//printf("Sending %.*s %li\n", RS_BYTES_SENT, &tx[i*RS_BYTES_SENT],(i*RS_BYTES_SENT));
		
	}
		
	
		//uint8_t tx[]="{6This is the raspi val!}{5This is the raspi val!}{4This is the raspi val!}{3This is the raspi val!}{2This is the raspi val!}{1This is the raspi val!}";
	//uint8_t tx[] = {"We should be sending 32 bytes+2!!            "};
	
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
	
	gettimeofday(&tick, NULL);
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	gettimeofday(&tock, NULL);
		
	tdiff=1000000*((long)tock.tv_sec-(long)tick.tv_sec);	//seconds to microseconds
	tdiff+=((long)tock.tv_usec-(long)tick.tv_usec);		//add microseconds		
		
	printf("time to transfer: %.3f ms\n",(double)(tdiff/1000.0));
	
	
	
	gettimeofday(&time_holder, NULL);
	//time_stamp=1000000*((long)time_holder.tv_sec);	//seconds to microseconds
	//time_stamp+=((long)time_holder.tv_usec);		//add microseconds		
	
	
	time_stamp=1000000*((long)time_holder.tv_sec-(long)time_start.tv_sec);	//seconds to microseconds
	time_stamp+=((long)time_holder.tv_usec-(long)time_start.tv_usec);		//add microseconds		
	
	//time_stamp=1000000.0*(time_holder.tv_sec-time_start.tv_sec);	//seconds to microseconds
	//time_stamp+=(time_holder.tv_usec-time_start.tv_usec);		//add microseconds		
	
	if (ret < 1)
		pabort("can't send spi message");

    for (ret_indx=0;ret_indx<(NUM_BOARDS*RS_BYTES_SENT);ret_indx+=RS_BYTES_SENT){
		memcpy(&rs_buff[0],&rx[ret_indx],RS_NUM_ACTUAL_DATA_BYTES);
		memcpy(&rs_buff[KK],&rx[ret_indx+RS_NUM_ACTUAL_DATA_BYTES],RS_NUM_PARITY_BYTES);

		if (rs_buff[0]!='{'){
			er_pos[0]=0;
			num_ers=1;
//			printf("\nFirst byte \'{\' erased from board %u\n",(ret_indx/RS_BYTES_SENT));
		}
		else{
			num_ers=0;
		}
		gettimeofday(&tick, NULL);
		i=eras_dec_rs(rs_buff,er_pos,num_ers);		//decode data
		gettimeofday(&tock, NULL);
		
		tdiff=1000000*((long)tock.tv_sec-(long)tick.tv_sec);	//seconds to microseconds
		tdiff+=((long)tock.tv_usec-(long)tick.tv_usec);		//add microseconds		
		
		printf("time to decode: %.3f ms\n",(double)(tdiff/1000.0));
		if (i==-1){
			puts("Too many errors to decode.");
		}
		else if(i>0){
			printf("%u errors found\n",i);
		}
		
		if(i>0){
			memcpy(&rx[ret_indx],&rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);	//if data is corrected, copy it over
	//		printf("%u Bytes corrected from board #%u\n",i,(ret_indx/RS_BYTES_SENT));
			ret_val+=i;
		}
		if(i==0){
	//		printf("All bytes correct from board #%u\n",(ret_indx/RS_BYTES_SENT));
		}
		if(i==-1){
			//printf("Too many corrupted bytes from board #%u\n",(ret_indx/RS_BYTES_SENT));
			ret_val=-1;
		}
	}
	
	//puts("");
	//puts("Now for returned");
	
	for (ret = 0; ret < ARRAY_SIZE(tx); ret+=RS_BYTES_SENT) {
		//if (!(ret % 25))
		//	puts("");
		
		if(rx[ret]=='{'){
			puts("");
			printf("From board %i:\t",ret/RS_BYTES_SENT);
			printf("%s", &rx[ret]);
			puts("");
			int board_time;
			memcpy(&board_time,&rx[3],4);
			printf("Board metadata: Board Num: %u\t Ring buffer index: %u\t Board time: %u",rx[1],rx[2],board_time);
			puts("");
			printf("Data acquired at local time: %.3f ms",(double)(time_stamp/1000.0));
			puts("");
			int temp_incr;
		/*
			for (temp_incr=0;temp_incr<RS_NUM_ACTUAL_DATA_BYTES;temp_incr++){
				if (!(temp_incr % 25))
					puts("");
				//printf("%.2X ", rx[temp_incr+ret]);
				printf("%.3d ", rx[temp_incr+ret]);
			}
			puts("");
		*/	
		}
		
	}
/*
	puts("\n");
	for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		if (!(ret % 25))
			puts("");
		//printf("%c", rx[ret]);
		printf("%.2X ", rx[ret]);

	}
	*/
	puts("");
	
	return ret_val;
    
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

int init_sockets(){
int ret;
	host_sock = socket(AF_INET, SOCK_DGRAM, 0);
	printf("Host sock is %i\n\n",host_sock);
	host_add.sin_family = AF_INET;
	host_add.sin_port = htons(host_port);
	host_add.sin_addr.s_addr = inet_addr(host_ip);
	ret = bind(host_sock, (struct sockaddr *)&host_add, sizeof(host_add));

	return ret;

}


void ctrlc(int sig)
{
    keep_alive = 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	int new_char=0;
	int kbd_buff_len=0;
	int i;
	gettimeofday(&time_start, NULL);
	memset(tx,0,numbytes);
	for (i=0;i<NUM_BOARDS;i++){
		int offset=i*RS_BYTES_SENT;
		tx[offset]='{';
		tx[offset+1]='!';
		tx[offset+2]='A';
		tx[offset+3]=i+0x31;//add '1' for ASCII offset
	}
	uint8_t input_buffer[RS_NUM_ACTUAL_DATA_BYTES+2]={'{',};
	keep_alive=1;
	signal(SIGINT, ctrlc);
	init_rs();
	init_rs_buff();
	if(init_sockets()!=0){
		puts("Problem binding socket!");
		keep_alive=0;
		goto done;
	}
	
	
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

	ret=transfer(fd);
	
	//clear buffer to default.
	//memset(input_buffer,0,RS_NUM_ACTUAL_DATA_BYTES+2);
	//input_buffer[0]='{';
//	memcpy(&input_buffer[0],&default_buff[0],RS_NUM_ACTUAL_DATA_BYTES);	
	puts("Fungible$ ");
	//printf("Fungible$ ");
	ret=0;
	int z,sink;
	int err_num=0;
	memset(datagram,0,RS_NUM_ACTUAL_DATA_BYTES+2);
	datagram[0]='{';
	

	while(keep_alive==1){
		
		ret=0;
		
		FD_ZERO(&readfds);
		FD_SET(host_sock, &readfds);
		tv.tv_sec = 0;
		tv.tv_usec = 3000;
		ret = select(host_sock+1, &readfds, NULL, NULL, &tv);
		
		if (ret == -1) {
		    puts("select error"); // error occurred in select()
		} 
		else if (ret == 0) {
		    //printf("\nTimeout occurred!  Error# %u\n",err_num++);
		} 
		
		else {
		    // one or both of the descriptors have data
			//printf("\nsocket is ready\n");
			if (FD_ISSET(host_sock, &readfds)) {
				memset(&datagram[1],0,RS_NUM_ACTUAL_DATA_BYTES+1);
				z=recv(host_sock, &datagram[1], 14, 0);
				//datagram[z] = 0;//replace \n with NULL
				datagram[z] = 0;//replace \n with NULL
			
				for(ret=0;ret<NUM_BOARDS;ret++){
					memcpy(&tx[RS_BYTES_SENT*ret],&datagram[0],RS_NUM_ACTUAL_DATA_BYTES);
					printf("tx buff:\nSTART%sEND\n",&tx[RS_BYTES_SENT*ret]);
				}
			
				ret=transfer(fd);
				printf("New command is:%s\n", datagram);
				//bzero(datagram, 15);
				
				for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
					if (!(ret % 25))
						puts("");
//					printf("%s", tx[ret]);
					printf("%.2X ", tx[ret]);

				}
				puts("");
				puts("Now for returned");
/*				
*/
				for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
					if (!(ret % 25))
						puts("");
					printf("%c", rx[ret]);
					//printf("%.2X ", rx[ret]);

				}

				puts("");



				puts("\n");
				for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
					if (!(ret % 25))
						puts("");
					//printf("%c", rx[ret]);
					printf("%.2X ", rx[ret]);

				}
				puts("");
				
				puts("are we getting to here?");

				//memset(datagram,0,RS_NUM_ACTUAL_DATA_BYTES+2);
				//datagram[0]='{';
				//memset(tx,0,NUM_BOARDS*RS_BYTES_SENT);
				//memset(rx,0,NUM_BOARDS*RS_BYTES_SENT);
				//memset(rs_buff,0,NN);
			}
			
			memset(tx,0,NUM_BOARDS*RS_BYTES_SENT);
			puts("or here?");		
	    	}

		
		ntv.tv_sec=0;
		ntv.tv_nsec=100;
	//	nanosleep(&ntv,NULL);
		sink=transfer(fd);
		

		//memset(tx,0,NUM_BOARDS*RS_BYTES_SENT);
		
	//puts("Fungible$ ");
	/*
	//this is old keyboard code
		if(new_char==0x1B){ //Escape key sent
			keep_alive=0;
		}
		
		else if (new_char=='\r'){
			puts("CR caught");
		}
		
		else if(new_char=='\n'){ //command ended
			//copy buffer contents and send
			//then clear buffer

			//printf("%.*s \n",kbd_buff_len+2, input_buffer);		
			printf("Sending Buffer: %s\n", input_buffer);		
			printf("\r\ntransferring again\n\n");
			for(ret=0;ret<NUM_BOARDS;ret++){
				memcpy(&tx[RS_BYTES_SENT*ret],&input_buffer[0],RS_NUM_ACTUAL_DATA_BYTES);
			}
			transfer(fd);
			puts("Fungible$ ");
			//clear buffer to default.
			memset(input_buffer,0,RS_NUM_ACTUAL_DATA_BYTES+2);
			input_buffer[0]='{';
		//	memcpy(&input_buffer[0],&default_buff[0],RS_NUM_ACTUAL_DATA_BYTES);
			kbd_buff_len=0;

			//printf("\r\nFungible$ ");
		//	puts("Fungible$ ");
		}
		else if((new_char>=' ') && (kbd_buff_len<RS_NUM_ACTUAL_DATA_BYTES-1)){
			input_buffer[++kbd_buff_len]=new_char;
			input_buffer[kbd_buff_len+1]=0;
		}
	*/	
	
	
	
	}
	
	done:
		//do cleanup here
		close(host_sock);
		printf("\r\n\nClosing!\n\n");
	
		close(fd);
	
	return ret;
}