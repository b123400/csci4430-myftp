#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <limits.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> //in_addr_t
#define MAX 1000000
//structure
struct message_s {
unsigned char protocol[6]; /* protocol magic number (6 bytes) */
unsigned char type; /* type (1 byte) */
unsigned char status; /* status (1 byte) */
unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

//flag of status
int isconn=0;
int isauth=0;

//global vairables
char *token[100];
char addr[100];
uint16_t PORT;
int cmdlenght;
struct sockaddr_in server_addr;


//separate the cmd (return the number of arguments)
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

int acpw(char buffer[256], char authset[256]){
	int i=0,j=0;
	int same=0;
	while(i<strlen(buffer)){
		if (j==strlen(authset))
			break;
		else if (buffer[i]==authset[j]){
			j++;
			i++;
			printf("i: %d, j: %d\n",i,j);} 
		else if (j==0) {
			i++;
			printf("i: %d, j: %d\n",i,j);}
		else if (j>0) {
			i=i-j+1;
			j=0;
			printf("i: %d, j: %d\n",i,j);
		}
	}

	if(j==strlen(authset))
		return 1;
	else 
		return 0;
}

//flow1: connection
bool connOpen(int sd){
	char *inputString = malloc(sizeof(char)*256);
	struct message_s OPEN_CONN_REPLY;
	struct message_s OPEN_CONN_REQUEST;
	fgets(inputString, 256, stdin);
	unsigned char *buffer;
	buffer=(unsigned char *)malloc(sizeof(unsigned char) * MAX);
	int len;
	
	//empty && call tokenit
	if (strcmp(inputString,"\n")==0) {
            return 0;
        } else {
            cmdlenght=tokenit(inputString);
        }

	//input: exit
	if (strcmp(token[0],"exit")==0){
		printf("Thank you!\n");
		exit(1);
	}
	
	//Wrong arguments
	if (cmdlenght!=3){
		printf("pusage: open [IP address] [port number]\n");
		return 0;
	}

	//ready to connect
	printf("receiving the OPEN cmd...\n");
	memset(&server_addr,0,sizeof(server_addr));
	strcpy(addr,token[1]); //addr
	printf("char addr: %s\n",addr);
	printf("char PORT: %s\n",token[2]);

	PORT=(unsigned short) strtoul(token[2], NULL, 0); //PORT
	printf("uint16_t PORT: %d\n",PORT);
	
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	server_addr.sin_port=htons(12345);

	if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
		printf("Connection error: %s (Errno:%d)\n",strerror(errno),errno);
		printf("Server address: %s\n", token[1]);
		printf("Port: %s\n", token[2]);
		return 0;
	} else {
		printf("Server connection accepted.\n");
		// memcpy(OPEN_CONN_REQUEST.protocol,"0xe3myftp0xA10000000C",12);
		OPEN_CONN_REQUEST.protocol[0]= htons(0xe3);
		strcat(OPEN_CONN_REQUEST.protocol,"myftp");
		OPEN_CONN_REQUEST.type=htons(0xA1);
		OPEN_CONN_REQUEST.status=htons(0);
		OPEN_CONN_REQUEST.length=htons(12);
		
		// read(OPEN_CONN_REQUEST, buffer, sizeof(struct message_s));
		memcpy(buffer, &OPEN_CONN_REQUEST,sizeof(OPEN_CONN_REQUEST));
		int length = sizeof(OPEN_CONN_REQUEST);
		buffer[length] = '\0';
		
		int i;
		for (i = 0; i < length; i++) {
			printf("%02X ",(int)buffer[i]);
		}
		printf("\n");
		
		if((len=send(sd,(void *)&OPEN_CONN_REQUEST,sizeof(OPEN_CONN_REQUEST),0))<=0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		printf("len: %d\n",len);
		/*
		if((len=recv(sd,(struct message_s *)&OPEN_CONN_REPLY,sizeof(OPEN_CONN_REPLY),0))<=0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}*/
		if(OPEN_CONN_REPLY.status == 1) {
			isconn = 1;
		}
		
		return 1;
	}
	free(inputString);
}

//send auth message to the server
bool auth(int sd){
	char *inputString = malloc(sizeof(char)*256);
	FILE *fp;
	char buffer[256]="";
	printf("You are ready to login...\n");
	fgets(inputString, 256, stdin);
	char authset[256]="";
	int len;
	
	//empty && call tokenit
	if (strcmp(inputString,"\n")==0) {
        return 0;
    } else {
        cmdlenght=tokenit(inputString);
    }

	//input: exit
	if (strcmp(token[0],"exit")==0){
		printf("Thank you!\n");
		exit(1);
	}

	//Wrong arguments
	if (cmdlenght!=3){
		printf("pusage: auth [name] [password]\n");
		return 0;
	}


	if((len=send(sd,inputString,strlen(inputString),0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	
	//below should place on the server side
	/*
	strcat(authset,token[1]);
	strcat(authset," ");
	strcat(authset,token[2]);
	//strcat(authset,"\n");
	
	printf("authset: %s\n",authset);
	fp = fopen ("access.txt", "r");
	fread(buffer, 256,1,fp);
	printf("%s\n", buffer);
	printf("sizeof(authset) = %d\n",strlen(authset));

	if(acpw(buffer, authset)==1)
		printf("Your input is right!\n");
	
	close(fp);*/
	free(inputString); 
}

int main(){
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	while(1){
		if(!isconn) {
			connOpen(sd);
		} else if(isconn && !isauth) {
			auth(sd);
		}
	}
	close(sd);
	return 0;
}
