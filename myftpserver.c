# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
#include <dirent.h>

# define PORT 12345

//structure
struct message_s {
unsigned char protocol[6]; /* protocol magic number (6 bytes) */
unsigned char type; /* type (1 byte) */
unsigned char status; /* status (1 byte) */
unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

/*
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
}*/

int tokenit(char tmp[256], char **output){
    int i=0;
    char *line = strtok(tmp, "\n");
    
    while (line != NULL){
          i++;
          output[i-1] = line;
	  //printf("%s\n",token[i-1]);
          line = strtok(NULL, "\n");
    }
    return i;
}

int check(struct message_s messages, unsigned char type, unsigned char status,unsigned int len){

	printf("server.protocol: %s,%s\n", messages.protocol,"\xe3myftp");
	printf("type?%d,%d\n", messages.type,type );
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

//step1: connhandle
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
	
	if(strcmp("exit",buff)==0){
		close(client_sd);
		return 0;
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
		return 1;
	}
	return 0;
}

//step2: authandle
int authandle(int client_sd){
	FILE *fp;
	struct message_s AUTH_REQUEST;
	struct message_s AUTH_REPLY;
	char buff[256];
	int i,len;
	char payload[256]="";
	char buffer[256]="";
	int isauth = 0; // this is local, every client has one
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
	payload[strlen(payload)]=0x0d;
	printf("strlen(payload) = %d\n",strlen(payload));
	
	strcpy(AUTH_REPLY.protocol, "\xe3myftp");
	AUTH_REPLY.type = 0xA4;
	AUTH_REPLY.length = htons(12);
	
	char *token[256];
	int name_len=tokenit(buffer, token);
	//for(i=0;i<strlen(token[0]);i++)
	//	printf("token: %02x\n", token[0][i]);

	for(i=0;i<name_len;i++){
	
		printf("strlen of token: %d\n", strlen(token[i]));
		if (strcmp(token[i], payload) == 0){
			AUTH_REPLY.status = 1;
			printf("Your input is right!\n");
			isauth=1;
			break;
		} else {
			printf("difference: %d\n", strcmp(token[i], payload));
			AUTH_REPLY.status = 0;
		}
	}
	
	if((len=send(client_sd,(void *)&AUTH_REPLY,12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	
	//close(fp); // dont know why not work to close fp!!!
	return isauth;
}

// step3 list files
int listFiles(int client_sd) {
	struct message_s LIST_REPLY;
	char buff[256];
	
	int len = 0;
	
	DIR *dirp;
	struct dirent *dp;

	if ((dirp = opendir("./filedir")) == NULL) {
		perror("couldn't open '.'");
		return;
	}

	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			strcpy(&buff[len+12], dp->d_name);
			//printf("found %s\n", dp->d_name);
			len += strlen(dp->d_name)+1;
			strcpy(&buff[len+12-1], "\n");
			//printf("len is now%d\n", len);
		}
	} while (dp != NULL);
	closedir(dirp);
	
	strcpy(LIST_REPLY.protocol,"\xe3myftp");
	LIST_REPLY.type=0xA6;
	LIST_REPLY.status=0;
	LIST_REPLY.length=htons(len+12);
	
	printf("Reply with data:\n");
	memcpy(buff, &LIST_REPLY,12);
	// int i;
	// for (i = 0; i < len+12; i++) {
	// 	printf("%02X ",(unsigned int)buff[i]);
	// }
	// printf("\n");
	
	if((len=send(client_sd,buff,len+12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	printf("sent length %d\n",len);
	return 0;
}

//real path
//step4
int getFile(int client_sd, char filename[100]){
	struct message_s GET_REPLY;
	struct message_s FILE_DATA;
	char buff[4096]="";
	char filebuff[4096]="";
	int filelen;
	int len = 0;
	int i;
	FILE *fp;
	
	//reply the get request.status 0,1
	strcpy(GET_REPLY.protocol,"\xe3myftp");
	GET_REPLY.type=0xA8;
	GET_REPLY.length=htons(12);
	

	if((fp=fopen(filename, "r"))!=NULL){
		printf("This file exists!\n");
		GET_REPLY.status=1;}
	else{
		printf("This file does not exist!\n");
		GET_REPLY.status=0;}
	
	printf("fopen of %s, status: %d\n",filename, GET_REPLY.status);

	if((len=send(client_sd,&GET_REPLY,12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	printf("after GET_reply\n");

	//ready to send FILE_DATA
	//filebuff > payload
	if (GET_REPLY.status==0)
		return 0;
	fseek(fp, 0, SEEK_SET);
	if ( fgets(filebuff,4096,fp)!=NULL){

		puts(filebuff);
		filelen=strlen(filebuff);

		strcpy(FILE_DATA.protocol,"\xe3myftp");
		FILE_DATA.type=0xFF;
		FILE_DATA.status=0;
		FILE_DATA.length=htons(filelen+12);
		
		memcpy(buff,&FILE_DATA,sizeof(FILE_DATA));
		printf("FILE_DATA: filelen: %d\n",filelen);

		for(i=0;i<filelen;i++)
			buff[12+i]=filebuff[i];
		
		if((len=send(client_sd,buff,filelen+12,0))<=0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}

		printf("after FILE_DATA\n");
	}

fclose(fp);
}

int main(int argc, char** argv){

	int sd=socket(AF_INET,SOCK_STREAM,0);
	int len;
	char buff[100];
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



//////////////////////////////////////////////
	while(1){   
		// this loop is for the main thread of accept new client
		// Create a client for this new client
		// sub-threads shouldn't reach here.
		int client_sd=0;
		int isconn=0;
		int isauth=0;
		printf("BEFORE ACCEPT\n");
		if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
			printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}else{
      		printf("receive connection from %d\n",inet_ntoa(client_addr.sin_addr.s_addr));
   		}
  		printf("AFTER ACCEPT\n");
		
		// probably something like this, for multiple threading
		// if (run in new thread) {
		while(1) {
			// this loop is for each client
			if(isconn!=1){
				isconn = connhandle(client_sd);
			} else if (isauth!=1){
				isauth = authandle(client_sd);
				if (!isauth) {
					// user entered the wrong password
					close(client_sd);
					// by breaking, this thread ends.
					break;
				}
			} else {
				printf("You can send/upload/ls file...\n");
				
				char buff[100]="";
				struct message_s request;
				int len;
				
			    printf("BEFORE RECV\n");
				if((len=recv(client_sd,buff,sizeof(buff),0))<=0){
					printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
					break;
				}
				memcpy(&request, buff, 12);
				printf("received request\n");
				int i;
				for (i = 0; i < len; i++) {
					printf("%02X ",(unsigned int)buff[i]);
				}
				printf("\n");
				if (request.type == 0xA5) {
					// this is a list request
					listFiles(client_sd);
					continue;
				} else if (request.type == 0xA7) {
					char filename[100]="";
					for(i=0;i<ntohs(request.length)-13;i++)
						filename[i]=buff[13+i];
					printf("get %s\n", filename);
					getFile(client_sd, filename);
					continue;
				}
				// printf("logouting...\n");
				// some more handler for other commands
				close(client_sd);
				break;
			}
		}
		// break, because this thread is for client,
		// it shouldn't do anything more than handling this client
		// if we don't break here, this thread will accept other client.
		//	break;
		//} else {
		//	do nothing, this is the main thread
		//}
	}
	close(sd);
	return 0;
}
