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
uint8_t tx[NUM_BOARDS*RS_BYTES_SENT];
uint8_t rx[ARRAY_SIZE(tx)] = {0, };
fd_set readfds;
struct timeval tv;

int keep_alive;

void ctrlc(int sig)
{
    keep_alive = 0;
}

int main(int argc, char **argv){
	int x, z,fd;
	char *host_ip = "127.0.0.1";
	short host_port = 1235;
	//char *remote_ip = "127.0.0.1";
	//short remote_port = 1234;
	int i;
	int host_sock, remote_sock;
	char datagram[512];
	int remote_len;
	
	struct sockaddr_in host_add;
	//struct sockaddr_in remote_add;

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
				printf("got incoming socket data\n\n%s\n\n\n",rx);
			
					
	    	}
		}
	/*
		
		printf("Enter new command: ");
		bzero(datagram, 512);
		fgets(datagram, 512, stdin);
	


		// Here we are sending the udp message as datagram to remote machine 
		x = sendto(remote_sock, datagram, strlen(datagram), 0, (struct sockaddr *)&remote_add, sizeof(remote_add));
		
		
		bzero(datagram, 512);
	*/

	}
	//close(remote_sock);
	close(host_sock);
	return 0;
}
