# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>

# define PORT 12345

int isconn=0;
int isauth=0;
char payload[256];

//structure
struct message_s {
unsigned char protocol[6]; /* protocol magic number (6 bytes) */
unsigned char type; /* type (1 byte) */
unsigned char status; /* status (1 byte) */
unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

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

int check(struct message_s messages, unsigned char type, unsigned char status,unsigned int len){

	printf("server.protocol: %s,%s\n", messages.protocol,"\xe3myftp");
	printf("tpye?%d,%d\n", messages.type,type );
	printf("status?%d,%d\n", messages.status,status );
	printf("len?:%d,%d\n",ntohs(messages.length), len);

	//if (strcmp(messages.protocol,"\xe3myftp")!=0)
		//return 0;
	if (messages.type!=type)
		return 0;
	else if (messages.status!=status)
		return 0;
	else if ( ntohs(messages.length) != len)
		return 0;
	else return 1;
}

int connhandle(int client_sd){

	struct message_s OPEN_CONN_REPLY;
	struct message_s OPEN_CONN_REQUEST;

	char buff[100]="";
	int len;

        printf("BEFORE RECV\n");
		if((len=recv(client_sd,buff,sizeof(buff),0))<=0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		memcpy(&OPEN_CONN_REQUEST, buff, 12);

		printf("len: %d\n",len);
        printf("AFTER RECV\n");
		buff[len]='\0';
		printf("buffer content:\n");

		int i;
		for (i = 0; i < len; i++) {
			printf("%02X ",(unsigned int)buff[i]);
		}
		printf("\n");
		
		printf("RECEIVED INFO: ");
		if(strlen(buff)!=0) {
			printf("length: %d\n",ntohs(OPEN_CONN_REQUEST.length));
			printf("protocol: %s\n",OPEN_CONN_REQUEST.protocol);
			printf("type: %X\n", OPEN_CONN_REQUEST.type);
			printf("status: %d\n", OPEN_CONN_REQUEST.status);
		}
		
		// Start making reply
		if(check(OPEN_CONN_REQUEST, 0xA1,0,12)){

		strcpy(OPEN_CONN_REPLY.protocol,"\xe3myftp");
		OPEN_CONN_REPLY.type=0xA2;
		OPEN_CONN_REPLY.status=1;
		OPEN_CONN_REPLY.length=htons(12);
		
		len = sizeof(OPEN_CONN_REPLY);
		printf("Reply with data:\n");
		memcpy(buff, &OPEN_CONN_REPLY,len);

		for (i = 0; i < len; i++) {
			printf("%02X ",(unsigned int)buff[i]);
		}
		printf("\n");
		
		if((len=send(client_sd,buff,len,0))<=0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		
		printf("just finish sending with size %d.\n", len);
		isconn=1;
		if(strcmp("exit",buff)==0){
			close(client_sd);
			return 0;
		
    	}
}
return 0;
}

int authandle(int client_sd){
	FILE *fp;
	struct message_s AUTH_REQUEST;
	struct message_s AUTH_REPLY;
	char buff[256];
	int i,len;
	char payload[256]="";
	char buffer[256]="";
	printf("BEFORE login\n");

	if((len=recv(client_sd,buff,sizeof(buff),0))<=0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}else {
		printf("loging... please wait!\n");
	}

	memcpy(&AUTH_REQUEST, buff, 12);
	
	for(i=0;i<len-12;i++)
		payload[i]=buff[12+i];

	printf("payload: %s\n",payload);
	fp = fopen ("access.txt", "r");
	fread(buffer, 256,1,fp);
	printf("%s\n", buffer);
	printf("sizeof(payload) = %d\n",strlen(payload));

	strcpy(AUTH_REPLY.protocol, "\xe3myftp");
	AUTH_REPLY.type = 0xA4;
	AUTH_REPLY.length = htons(12);
	if (acpw(buffer, payload)==1){
		AUTH_REPLY.status = 1;
		printf("Your input is right!\n");
		isauth=1;
	} else {AUTH_REPLY.status = 0;}
		
	if((len=send(client_sd,(void *)&AUTH_REPLY,12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	
//close(fp); // dont know why not work to close fp!!!
return 0;
}

int main(int argc, char** argv){
	int sd=socket(AF_INET,SOCK_STREAM,0);
	int client_sd;
	int len;
	int buff[100];
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(12345);

	if(bind(sd,(struct sockaddr *) &server_addr,sizeof(server_addr))<0){
		printf("bind error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	if(listen(sd,3)<0){
		printf("listen error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	struct sockaddr_in client_addr;
	int addr_len=sizeof(client_addr);
	/*if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
		printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}*/
    printf("BEFORE ACCEPT\n");
	if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
		printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}else{
        printf("receive connection from %d\n",inet_ntoa(client_addr.sin_addr.s_addr));
    }
    printf("AFTER ACCEPT\n");

//////////////////////////////////////////////
	while(1){
		if(isconn!=1){ connhandle(client_sd); }
		else if (isauth!=1){ authandle(client_sd); }
		else {  printf("You can send/upload/ls file...\n");
			printf("logouting...\n");
			close(client_sd);
			break; }
}

	close(sd);
	return 0;
}
