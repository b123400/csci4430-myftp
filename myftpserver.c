# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>

# define PORT 12346

//structure
struct message_s {
unsigned char protocol[6]; /* protocol magic number (6 bytes) */
unsigned char type; /* type (1 byte) */
unsigned char status; /* status (1 byte) */
unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

int main(int argc, char** argv){
	int sd=socket(AF_INET,SOCK_STREAM,0);
	int client_sd;

	struct message_s OPEN_CONN_REPLY;
	struct message_s OPEN_CONN_REQUEST;

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
		char buff[100]="";
		int len;
        printf("BEFORE RECV\n");
		if((len=recv(client_sd,buff,sizeof(buff),0))<=0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		memcpy(&OPEN_CONN_REQUEST, buff, len);
		
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
		OPEN_CONN_REPLY.protocol[0]= htons(0xe3);
		strcat(OPEN_CONN_REPLY.protocol,"myftp");
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
		
		if(strcmp("exit",buff)==0){
			close(client_sd);
			break;
    	}
	}
	close(sd);
	return 0;
}
