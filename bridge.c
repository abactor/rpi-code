#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "rs_big.h"
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
//these defines should be centralized
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
int data_in[NUM_BOARDS][RS_BYTES_SENT];
int data_vals[NUM_BOARDS][RS_NUM_ACTUAL_DATA_BYTES/2];//this is to store the current data

typedef struct board_data{
	#define MAX_NUM_SIGNALS 	16
	#define CURRENT_NUM_SIGNALS	4
	#define META_DATA_WIDTH 	3
	#define BUFFER_LENGTH 		48
	#define BUFFER_WORD_SIZE 	2
	#define DATA_OFFSET 		14
	#define CLOCK_OFFSET		3
	#define CLOCK_WORD_SIZE		4
	#define NEW_DAT_OFFSET		2			
	#define ID_OFFSET			1
	///just use pointer arithmetic, no need to memcpy data around---not true, need a local working copy, not the buffer that will get overwritten

	uint8_t 	framing_char;			//[index 0]
	uint8_t 	board_ID;				//[index 1]
	uint8_t 	ring_index;				//[index 2]
	uint32_t 	time_stamp;				//[index 3-6]
	
	uin32_t		last_time_stamp;		//[index 7-10]
	uint8_t		capture_since_transfer;	//[index 11]
	uint8_t		meta_data[2];			//[index 12-13]
	uint16_t 	data[BUFFER_LENGTH]		//[index 14-110]
	

} board_data;

board_data my_board_data[NUM_BOARDS];

uint8_t tx[NUM_BOARDS*RS_BYTES_SENT];
uint8_t rx[ARRAY_SIZE(tx)] = {0, };
fd_set readfds;
struct timeval tv,logtime;
const char * log_file_name="out_log.log";
int keep_alive;

void ctrlc(int sig)
{
    keep_alive = 0;
}


int decode_data(){
int ret,j,i;
	int er_pos[RS_NUM_PARITY_BYTES];
	int num_ers=0;
	int ret_val=0;
	struct timeval tick,tock;
	long tdiff;
 	
 	/*
 	puts("Before decode\n");
	for (ret = 0; ret < ARRAY_SIZE(rx); ret++) {
		if (!(ret % 165))
			puts("");
		//printf("%c", rx[ret]);
		printf("%.2X ", rx[ret]);

	}
 	*/
 	
	
	int temp=0;
 	for (i=0;i<(NUM_BOARDS*RS_BYTES_SENT);i+=RS_BYTES_SENT){
		
		memset(&rs_buff[0],0,ARRAY_SIZE(rs_buff));
		memcpy(&rs_buff[0],&rx[i],RS_NUM_ACTUAL_DATA_BYTES);
		memcpy(&rs_buff[KK],&rx[i+RS_NUM_ACTUAL_DATA_BYTES],RS_NUM_PARITY_BYTES);
	
		/*
		//num_ers=1;
		//er_pos[0]=1;
		puts("printing rs_buff\n");
		for (ret = 0; ret < ARRAY_SIZE(rs_buff); ret++) {
			if (!(ret % 200))
				puts("");
			//printf("%c", rx[ret]);
			printf("%.2X ", rs_buff[ret]);

		}
		*/		
		/*
		if (rs_buff[0]!='{'){//not properly framed, throw it out?
			er_pos[0]=0;
			num_ers=1;
		//	printf("\nFirst byte \'{\' erased from board %u\n",(i/RS_BYTES_SENT));
		}
		else{
			num_ers=0;
		}
		*/
		gettimeofday(&tick, NULL);
		
				
		
		j=eras_dec_rs(rs_buff,er_pos,num_ers);		//decode data
		
		//printf("returned %i from decode\n",j);
		gettimeofday(&tock, NULL);
		
		tdiff=1000000*((long)tock.tv_sec-(long)tick.tv_sec);	//seconds to microseconds
		tdiff+=((long)tock.tv_usec-(long)tick.tv_usec);		//add microseconds		
		/*
		printf("time to decode: %.3f ms\n",(double)(tdiff/1000.0));
		if (j==-1){
			puts("Too many errors to decode.");
		}
		else if(j>0){
			printf("%u errors found\n",j);
		}
		*/
		if(j>0){
			memcpy(&rx[i],&rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);	//if data is corrected, copy it over---return corrected
			//printf("%u Bytes corrected from board #%u\n",j,(i/RS_BYTES_SENT));
			if (ret_val)	
				ret_val+=j;
		}
		if(j==0){
			memcpy(&rx[i],&rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);	//if data is corrected, copy it over---return corrected
			//printf("All bytes correct from board #%u\n",(i/RS_BYTES_SENT));
		}
		if(j==-1){
			printf("Too many corrupted bytes from board #%u\n",(i/RS_BYTES_SENT));
			ret_val=-1;
		}
	}
	/*
	puts("After decode\n");
	for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		if (!(ret % 165))
			puts("");
		//printf("%c", rx[ret]);
		printf("%.2X ", rx[ret]);

	}
	*/
	return ret_val;
}


FILE *init_log_file(const char * log_filename){
	FILE *fid=fopen(log_filename, "a+");
	return fid;
}




int main(int argc, char **argv){
	int x, z,fd;
	char *host_ip = "127.0.0.1";
	short host_port = 1235;
	//char *remote_ip = "127.0.0.1";
	//short remote_port = 1234;
	
	int i;
	int recvd_flag=0;
	int host_sock, remote_sock;
	char datagram[512];
	int remote_len;
	init_rs();
	struct sockaddr_in host_add;
	//struct sockaddr_in remote_add;

	FILE *fid_log;
	fid_log=init_log_file(log_file_name);
	gettimeofday(&logtime,NULL);
	i=fprintf(fid_log,"Log file opened at %s",asctime(localtime(&logtime.tv_sec)));
	fflush(fid_log);
	
	keep_alive=1;
	//signal(SIGINT, ctrlc);
	
	host_sock = socket(AF_INET, SOCK_DGRAM, 0);

	host_add.sin_family = AF_INET;
	host_add.sin_port = htons(host_port);
	host_add.sin_addr.s_addr = inet_addr(host_ip);

	z = bind(host_sock, (struct sockaddr *)&host_add, sizeof(host_add));
	
	/*
	remote_sock = socket(AF_INET, SOCK_DGRAM, 0);
	remote_add.sin_family = AF_INET;
	remote_add.sin_port = htons(remote_port);
	remote_add.sin_addr.s_addr = inet_addr(remote_ip);
	*/
	
	while(keep_alive){
		int ret,i,ret_indx;
		int er_pos[RS_NUM_PARITY_BYTES];
		int num_ers=0;
		int ret_val=0;
		struct timeval tick,tock;
		long tdiff;
		FD_ZERO(&readfds);
		FD_SET(host_sock, &readfds);
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
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
				memset(&rx[0],0,sizeof(rx));
				
				
				z=recv(host_sock, &rx[0], sizeof(rx), 0);
				//datagram[z] = 0;//replace \n with NULL
				printf("got %i incoming socket data\n\n%s\n\n\n",z,rx);
				recvd_flag=1;
				
	    		}
		}
		
		//printf("doing something else\r");
	/*
		
		printf("Enter new command: ");
		bzero(datagram, 512);
		fgets(datagram, 512, stdin);
	


		// Here we are sending the udp message as datagram to remote machine 
		x = sendto(remote_sock, datagram, strlen(datagram), 0, (struct sockaddr *)&remote_add, sizeof(remote_add));
		
		
		bzero(datagram, 512);
	*/
		if(recvd_flag){
			ret=decode_data();
			recvd_flag=0;
			printf("decode returned %i\r",ret);
			int board_num;
			ret=fwrite(rx,2,RS_BYTES_SENT*NUM_BOARDS,fid_log);
			ret=fprintf(fid_log,"\n");
			for(board_num=0;board_num<NUM_BOARDS;board_num++){	
			//for better efficiency, this should go within decode_data function
				memcpy(&my_board_data[board_num],&rx[board_num*RS_BYTES_SENT], ACTUAL_DATA_BYTES);
				printf("Framing byte is: %c\n",my_board_data[board_num].framing_char);
				printf("board ID is: %u\n",my_board_data[board_num].board_ID);
				printf("board ring index is: %u\n",my_board_data[board_num].ring_index);
				printf("board time stamp is: %lu\n",my_board_data[board_num].time_stamp);
				printf("board data is: %u\n",my_board_data[board_num].data[my_board_data[board_num].board_ID]);
			}
			
			
			
			
			
		}
	}
	//close(remote_sock);
	close(host_sock);
	fclose(fid_log);
	return 0;
}
