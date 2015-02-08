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
		//OPEN_CONN_REQUEST.protocol[0]=0xe3;
		strcpy(OPEN_CONN_REQUEST.protocol,"\xe3myftp");
		OPEN_CONN_REQUEST.type=0xA1;
		OPEN_CONN_REQUEST.status=0;
		OPEN_CONN_REQUEST.length=htons(12);
		
		// read(OPEN_CONN_REQUEST, buffer, sizeof(struct message_s));
		memcpy(buffer, &OPEN_CONN_REQUEST,sizeof(OPEN_CONN_REQUEST));
		int length = sizeof(OPEN_CONN_REQUEST);
		buffer[length] = '\0';
		
		printf("OPEN_CONN_REQUEST.protocol: %02x\n",OPEN_CONN_REQUEST.protocol[0]);
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
		
		if((len=recv(sd,buffer,sizeof(OPEN_CONN_REPLY),0))<0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}

		memcpy(&OPEN_CONN_REPLY, buffer, len);
		printf("received reply of size %d,\nReceived data:", len);
		for (i = 0; i < len; i++) {
			printf("%02X ",(int)buffer[i]);
		}
		printf("\n");
		
		printf("length: %d\n",ntohs(OPEN_CONN_REPLY.length));
		printf("protocol: %s\n",OPEN_CONN_REPLY.protocol);
		printf("type: %02X\n", OPEN_CONN_REPLY.type);
		printf("status: %d\n", ntohs(OPEN_CONN_REPLY.status));
		
		if(OPEN_CONN_REPLY.status == 1) {
			isconn = 1;
		}
		
		return 0;
	}
	free(inputString);
}

//flow2: send auth message to the server
bool auth(int sd){

	struct message_s AUTH_REQUEST;
	struct message_s AUTH_REPLY;


	char *inputString = malloc(sizeof(char)*256);
	FILE *fp;
	char buffer[256]="";
	printf("You are ready to login...\n");
	fgets(inputString, 256, stdin);
	char authset[256]="";
	int len, i;
	
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
	strcat(authset,token[1]);
	strcat(authset," ");
	strcat(authset,token[2]);

	strcpy(AUTH_REQUEST.protocol,"\xe3myftp");
	AUTH_REQUEST.type=0xA3;
	AUTH_REQUEST.status=0;
	AUTH_REQUEST.length=htons(12+strlen(authset)+1);
	printf("AUTH_REQUEST.length:%d\n",ntohs(AUTH_REQUEST.length));
	memcpy(buffer,&AUTH_REQUEST,sizeof(AUTH_REQUEST));
	
	for(i=0;i<strlen(authset);i++)
		buffer[12+i]=authset[i];
	
	if((len=send(sd,buffer,12+strlen(authset)+1,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
	}
	
	printf("len have sent: %d\n", len);
	printf("They are:");
	for(i=0;i<len;i++)
		printf("%02x ", buffer[i]);
	printf("\n");

	if((len=recv(sd,&AUTH_REPLY,sizeof(AUTH_REPLY),0))<=0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	if (AUTH_REPLY.status==1) { isauth = 1;}
	else {
		isconn=0;
		close(sd);
		sd=0;
		}
		
	free(inputString); 
}

int main(){
	int sd = 0;
	while(1){
		if(!isconn) {
			if(sd==0)
				sd = socket(AF_INET, SOCK_STREAM, 0);
			connOpen(sd);
		} else if(isconn && !isauth) {
			auth(sd);
		} else break;
	}
	close(sd);
	return 0;
}
