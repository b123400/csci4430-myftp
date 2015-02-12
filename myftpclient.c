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
#include <fcntl.h>
#include <libgen.h>
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

//for the 12 byte message
int check(struct message_s messages, unsigned char type, unsigned char status, unsigned int len){
	int i=0;
	char fake_pro[]="\xe3myftp";
	printf("server.protocol: %s,%s\n", messages.protocol,fake_pro);
	printf("type?%d,%d\n", messages.type,type );
	printf("status?%d,%d\n", messages.status,status );
	printf("len?:%d\n",ntohs(messages.length));
	
	if(messages.protocol[0]!=0xe3)
		return 0;
	for(i=1;i<6;i++){
		//printf("%02x, %02x", (int)messages.protocol[i],fake_pro[i]);
		if(messages.protocol[i]!=fake_pro[i])
			return 0;
	}

	if (messages.type!=type)
		return 0;
	else if (messages.status!=status)
		return 0;
	else if ( ntohs(messages.length) != len)
		return 0;
	else return 1;
}

//global vairables
char *token[100];
char addr[100];
uint16_t PORT;
int cmdlenght;

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
	struct sockaddr_in server_addr;
	fgets(inputString, 256, stdin);
	char buffer[1024];
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
		//printf("Server address: %s\n", token[1]);
		//printf("Port: %s\n", token[2]);
		return 0;
	} else {
		printf("Server connection accepted.\n");
		strcpy(OPEN_CONN_REQUEST.protocol,"\xe3myftp");
		OPEN_CONN_REQUEST.type=0xA1;
		OPEN_CONN_REQUEST.status=0;
		OPEN_CONN_REQUEST.length=htons(12);
		
		memcpy(buffer, &OPEN_CONN_REQUEST,sizeof(OPEN_CONN_REQUEST));
		int length = sizeof(OPEN_CONN_REQUEST);
		buffer[length] = '\0';
		
		//printf("OPEN_CONN_REQUEST.protocol: %02x\n",OPEN_CONN_REQUEST.protocol[0]);
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
		
		if(check(OPEN_CONN_REPLY, 0xA2, 1, len)) {
			isconn = 1;
		}
		
		return 0;
	}
	free(inputString);
}

//flow2: send auth message to the server
// return 1 = login success
// return 0 = login canceled(entering wrong command)
// return -1 = login failed(wrong password)
bool auth(int sd){

	struct message_s AUTH_REQUEST;
	struct message_s AUTH_REPLY;


	char *inputString = malloc(sizeof(char)*256);
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
	memcpy(buffer, &AUTH_REQUEST, sizeof(AUTH_REQUEST));
	
	for(i=0;i<strlen(authset);i++)
		buffer[12+i]=authset[i];
	
	buffer[12+i] = 0x00; // the last char is 0, null terminated
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
	if (check(AUTH_REPLY, 0xA4, 1, len)) {
		isauth = 1;
		printf("Logged in\n");
	} else {
		isconn=0;
		isauth=0;
		close(sd);
		printf("Login failed.\n");
	}
		
	free(inputString);
	if (check(AUTH_REPLY, 0xA4, 1, len)) {
		return 1;
	} else {
		return -1;
	}
}

//flow3: ls cmd
bool listFiles(int sd){
	struct message_s LIST_REQUEST;
	struct message_s LIST_REPLY;
	char buffer[4096];
	
	strcpy(LIST_REQUEST.protocol,"\xe3myftp");
	LIST_REQUEST.type=0xA5;
	LIST_REQUEST.status=0;
	LIST_REQUEST.length=htons(12);
	
	memcpy(buffer, &LIST_REQUEST,sizeof(LIST_REQUEST));
	int length = sizeof(LIST_REQUEST);
	buffer[length] = '\0';
	
	printf("sending list request:\n");
	int i;
	for (i = 0; i < length; i++) {
		printf("%02X ",(int)buffer[i]);
	}
	printf("\n");
	
	int len;
	if((len=send(sd,(void *)&LIST_REQUEST,sizeof(LIST_REQUEST),0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	
	if((len=recv(sd,buffer,sizeof(buffer),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}

	printf("received reply of size %d,\nReceived data:", len);
	// for (i = 0; i < len; i++) {
	// 	printf("%02X ",(int)buffer[i]);
	// }
	// printf("\n");
	
	memcpy(&LIST_REPLY, buffer, 12);
	printf("length: %d\n",ntohs(LIST_REPLY.length));
	printf("protocol: %s\n",LIST_REPLY.protocol);
	printf("type: %02X\n", LIST_REPLY.type);
	printf("status: %d\n", ntohs(LIST_REPLY.status));

	if(check(LIST_REPLY,0xA6,0,len)){
		printf("Files are:\n");
		printf("%s\n", &buffer[12]);
	}
}

// flow4: get cmd
int getFile(int sd){
	struct message_s GET_REQUEST;
	struct message_s GET_REPLY;
	struct message_s FILE_DATA;
	char buffer[4096]="";
	char filebuffer[4096];
	int i=0;
	FILE *fp;
	
	strcpy(GET_REQUEST.protocol,"\xe3myftp");
	GET_REQUEST.type=0xA7;
	GET_REQUEST.status=0;
	GET_REQUEST.length=htons(12+strlen(token[1])+1);
	int length = sizeof(GET_REQUEST);
	buffer[length] = '\0';

	memcpy(buffer, &GET_REQUEST,sizeof(GET_REQUEST));
	
	for(i=0;i<strlen(token[1]);i++)
		buffer[13+i]=token[1][i];

	printf("sending get request:\n");

	for (i = 0; i < (12+strlen(token[1])+1); i++) {
		printf("%02X ",(int)buffer[i]);
	}
	printf("\n");

	int len=0;
	if((len=send(sd,buffer,12+strlen(token[1])+1,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	if((len=recv(sd,&GET_REPLY,12,0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}

	
	printf("length: %d\n",ntohs(GET_REPLY.length));
	printf("protocol: %s\n",GET_REPLY.protocol);
	printf("type: %02X\n", GET_REPLY.type);
	printf("status: %d\n", GET_REPLY.status);

	if (check(GET_REPLY,0xA8,1,len)){ 
		
		if((len=recv(sd,&FILE_DATA,sizeof(FILE_DATA),0))<0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		//memcpy(&FILE_DATA, buffer, 12);

		printf("FD length: %d\n",ntohs(FILE_DATA.length));
		printf("FD protocol: %s\n",FILE_DATA.protocol);
		printf("FD type: %02X\n", FILE_DATA.type);
		printf("FD status: %d\n", FILE_DATA.status);
		
		char *outputFilename = basename(token[1]);

		if (check(FILE_DATA,0xFF,0,len)){
			fp = fopen (outputFilename, "w");
			int filesize = ntohs(FILE_DATA.length)-12;
			int totalsize = 0;
			int buffsize = sizeof(filebuffer) < filesize? sizeof(filebuffer) : filesize;

			while (filesize > 0) {
				printf("waiting for file\n");
				len=recv(sd,filebuffer,buffsize,0);
				printf("recved size: %d\n", len);
				if (len<=0) {
					printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
					return 0;
				}

				fwrite(filebuffer, 1, len, fp);
				totalsize += len;
				if (totalsize >= filesize) {
					printf("File downloaded.\n");
					break;
				}
			
			}
			//for(i=0;i<len-12;i++){
			//	filebuffer[i]=buffer[12+i];
			printf("%s",filebuffer);
			//}
			
			fclose(fp);
			return 1;
		}
	} else 
		printf("Download Fail!\n");
}

// flow5: put cmd
int putFile(int sd, char *filename) {
	struct message_s PUT_REQUEST;
	struct message_s PUT_REPLY;
	struct message_s FILE_DATA;
	char buffer[256];
	int len = 0;
	
	int fd = 0;
	if((fd = open(filename, O_RDONLY)) <= -1) {
		printf("cannot read file");
		return 0;
	}
	
	int filenameLength = strlen(filename);
	
	strcpy(PUT_REQUEST.protocol,"\xe3myftp");
	PUT_REQUEST.type=0xA9;
	PUT_REQUEST.status=0;
	PUT_REQUEST.length=htons(12+filenameLength+1);
	
	memcpy(buffer, &PUT_REQUEST, sizeof(PUT_REQUEST));
	strcpy(&buffer[12], filename);
	buffer[12+filenameLength] = 0x00;
	
	int i;
	for (i = 0; i < (12+filenameLength+1); i++) {
		printf("%02X ",(int)buffer[i]);
	}
	
	if((len=send(sd,buffer,12+filenameLength+1,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	printf("after put_request\n");
	
	if((len=recv(sd,buffer,sizeof(PUT_REPLY),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	
	printf("received put_reply\n");
	
	if(check(PUT_REPLY,0xAA,0,len)){

	int filesize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	strcpy(FILE_DATA.protocol,"\xe3myftp");
	FILE_DATA.type=0xFF;
	FILE_DATA.status=0;
	FILE_DATA.length=htons(12+filesize);
	
	printf("send file header\n");
	if((len=send(sd,&FILE_DATA,12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	
	memcpy(buffer, &PUT_REQUEST, sizeof(PUT_REQUEST));
	for (i = 0; i < 12; i++) {
		printf("%02X ",(int)buffer[i]);
	}
	printf("\n");
	
	printf("uploading file size: %d\n", filesize);
	if (sendfile(sd, fd, 0, filesize) == -1) {
		printf("sending error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}}
	close(fd);
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
			if (isauth==-1) {
				// failed, reset sd
				sd = 0;
			}
		} else {
			// connected and authed
			char *inputString = malloc(sizeof(char)*256);
			int cmdlen=0;
			fgets(inputString, 256, stdin);
			cmdlen=tokenit(inputString);

			if (strcmp(token[0], "ls")==0 && cmdlen==1) {
				listFiles(sd);
			} 
			else if (strcmp(token[0], "get")==0 && cmdlen==2){
				getFile(sd);
				//printf("downloading...\n");
			}
			else if (strcmp(token[0], "put")==0 && cmdlen==2){
				printf("uploading...\n");
				putFile(sd, token[1]);
			}
			else if (strcmp(token[0], "quit")==0 && cmdlen==1){
				printf("quiting...\n");
			}
			else {
				printf("pusage:\n");
				printf("ls\n");
				printf("get [filename]\n");
				printf("put [filename]\n");
				printf("quit\n");
			}
			free(inputString);
		}
	}
	close(sd);
	return 0;
}
