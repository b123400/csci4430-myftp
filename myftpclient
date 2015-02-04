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

#define addr 127.0.0.1
#define PORT 12345

int isconn=0;
int isauth=0;
char *token[100];


int tokenit(char tmp[256]){
    int i=0;
    char *line = strtok(tmp, " \n");
    
    while (line != NULL){
          i++;
          token[i-1] = line;
	  //printf("%s\n",token[i-1]);
          line = strtok(NULL, " \n");
    }

    return i;
}

struct message_s {
unsigned char protocol[6]; /* protocol magic number (6 bytes) */
unsigned char type; /* type (1 byte) */
unsigned char status; /* status (1 byte) */
unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

int main(){

	//char addr[100];
	char *inputString = malloc(sizeof(char)*256);
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	int cmdlenght;

	struct sockaddr_in server_addr;
	
	while(!isconn){
	//printf("wait for open the connection...\n");
	fgets(inputString, 256, stdin);
	
	if (strcmp(inputString,"\n")==0) {
            continue;
        } else {
            cmdlenght=tokenit(inputString);
        }
	
	if (cmdlenght!=3){
		printf("pusage: open [IP address] [port number]\n");
		continue;
	}
	printf("received the OPEN cmd...\n");
	memset(&server_addr,0,sizeof(server_addr));
	
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(token[1]);
	server_addr.sin_port=htons(*token[2]);

	if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
		printf("Connection error: %s (Errno:%d)\n",strerror(errno),errno);
		printf("Server address: %s\n", token[1]);
		printf("Port: %s\n", token[2]);
		exit(0);}
	else {
		printf("Server connection accepted.");
		isconn = 1;
		}
	}
	
	close(sd);
	return 0;
}
