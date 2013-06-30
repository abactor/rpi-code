#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

int keep_alive;

void ctrlc(int sig)
{
    keep_alive = 0;
}

int main(int argc, char **argv){
	int x, z;
	//char *host_ip = "127.0.0.1";
	//short host_port = 1235;
	char *remote_ip = "127.0.0.1";
	short remote_port = 1234;
	int i;
//	int host_sock;
	int remote_sock;
	
	char datagram[512];
	int remote_len;
	
	//struct sockaddr_in host_add;
	struct sockaddr_in remote_add;

	keep_alive=1;
	//signal(SIGINT, ctrlc);
	
	//host_sock = socket(AF_INET, SOCK_DGRAM, 0);

	//host_add.sin_family = AF_INET;
	//host_add.sin_port = htons(host_port);
	//host_add.sin_addr.s_addr = inet_addr(host_ip);

	//z = bind(host_sock, (struct sockaddr *)&host_add, sizeof(host_add));
	remote_sock = socket(AF_INET, SOCK_DGRAM, 0);
	remote_add.sin_family = AF_INET;
	remote_add.sin_port = htons(remote_port);
	remote_add.sin_addr.s_addr = inet_addr(remote_ip);
	
	
	while(keep_alive){
		printf("Enter new command: ");
		bzero(datagram, 512);
		fgets(datagram, 512, stdin);



		/* Here we are sending the udp message as datagram to remote machine */
		x = sendto(remote_sock, datagram, strlen(datagram), 0, (struct sockaddr *)&remote_add, sizeof(remote_add));
		
		
		bzero(datagram, 512);


	}
	close(remote_sock);
	//close(host_sock);
	return 0;
}
