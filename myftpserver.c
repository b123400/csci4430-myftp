#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> //in_addr_t

#define addr 192.168.1.110
#define PORT 44380

int main(){

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	int client_sd;

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(PORT);

	if(bind(sd,(struct sockaddr *) &server_addr,sizeof(server_addr))<0){
		printf("bind error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	if(listen(sd,3)<0){
		printf("listen error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	int addr_len=sizeof(client_addr);

	if((client_sd = accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
		printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

close(sd);
return 0;
}
