# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <dirent.h>
# include <fcntl.h>
# include <pthread.h>
# include <libgen.h>
# include <limits.h>

//# define PORT 12345

//char *clientip[100];
unsigned long clientip[100]; 
unsigned short clientpt[100];

//structure
struct message_s {
unsigned char protocol[6]; /* protocol magic number (6 bytes) */
unsigned char type; /* type (1 byte) */
unsigned char status; /* status (1 byte) */
unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

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

//for the 12 byte message
int check(struct message_s messages, unsigned char type, unsigned char status, unsigned int len, int sfflag){
	int i=0;
	char fake_pro[]="\xe3myftp";
	printf("server.protocol: %s,%s\n", messages.protocol,fake_pro);
	printf("type?%d,%d\n", messages.type,type );
	printf("status?%d,%d\n", messages.status,status );
	printf("len?:%d\n",ntohl(messages.length));
	
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
	else if (sfflag==0){
		if (ntohl(messages.length) != len)
			return 0;}
	else if (sfflag==1){
		if (ntohl(messages.length) < len)
			return 0;}
	return 1;
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
	//printf("buffer content:\n");

	int i;
	//for (i = 0; i < len; i++) {
	//	printf("%02X ",(unsigned int)buff[i]);
	//}
	//printf("\n");

	/*
	printf("RECEIVED INFO: ");
	if(strlen(buff)!=0) {
		printf("length: %d\n",ntohl(OPEN_CONN_REQUEST.length));
		printf("protocol: %s\n",OPEN_CONN_REQUEST.protocol);
		printf("type: %X\n", OPEN_CONN_REQUEST.type);
		printf("status: %d\n", OPEN_CONN_REQUEST.status);
	}*/
	
	if(strcmp("exit",buff)==0){
		close(client_sd);
		return 0;
	}
	
	// Start making reply
	if(check(OPEN_CONN_REQUEST, 0xA1, OPEN_CONN_REQUEST.status, len, 0)){

		strcpy(OPEN_CONN_REPLY.protocol,"\xe3myftp");
		OPEN_CONN_REPLY.type=0xA2;
		OPEN_CONN_REPLY.status=1;
		OPEN_CONN_REPLY.length=htonl(12);
		
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
	//payload[strlen(payload)]=0x0d;

	if(check(AUTH_REQUEST, 0xA3, AUTH_REQUEST.status, len,0)){

	//for(i=0;i<strlen(payload);i++)
	//	printf("payload: %02x\n",payload[i]);
    //printf("inside the auth reply");
	//printf("payload: %s\n",payload);
	fp = fopen ("access.txt", "r"); //not work with rb

	//find the len of access.txt
	fseek(fp, 0L, SEEK_END); //why need L?
	len=ftell(fp);
	fseek(fp,0,SEEK_SET);
	fread(buffer, len ,1,fp);

	for(i=0;i<strlen(buffer);i++)
		printf("%02x\n", buffer[i]);
		//printf("buffer size %d\n", buffer[i]);

	payload[strlen(payload)]=0x00;
	printf("strlen(payload) = %d\n",strlen(payload));

	strcpy(AUTH_REPLY.protocol, "\xe3myftp");
	AUTH_REPLY.type = 0xA4;
	AUTH_REPLY.length = htonl(12);
	
	char *token[256];
	int name_len=tokenit(buffer, token);
	//for(i=0;i<strlen(token[0]);i++)
	//	printf("token: %02x\n", token[0][i]);

	for(i=0;i<name_len;i++){
		token[i][strlen(token[i])] = 0x00;
		printf("strlen of token: %d\n", strlen(token[i]));
		if (strcmp(token[i], payload) == 0){ //
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
	}
	fclose(fp); // dont know why not work to close fp!!!
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
	//LIST_REPLY.status=0;
	LIST_REPLY.length=htonl(len+12);
	
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
	char filebuff[4096] = {'\xff'};
	long int filelen=0;
	int len=0;
	int i, fd=0;
	//char realfilepath[PATH_MAX+1];
	//int isgoodfile;
	//FILE *fp;
	char openFrom[100]="";
	//reply the get request.status 0,1
	strcpy(GET_REPLY.protocol,"\xe3myftp");
	GET_REPLY.type=0xA8;
	GET_REPLY.length=htonl(12);

	//realpath(filename,realfilepath);
	//printf("realfilepath:%s\n", realfilepath);
	strcpy(openFrom, "./filedir/");
	strcat(openFrom, basename(filename));
	printf("openFrom:%s\n",openFrom);
	
	if((fd=open(openFrom,O_RDONLY))>-1){
		printf("This file exists!\n");
		GET_REPLY.status=1;
	} else{
		printf("This file does not exist!\n");
		GET_REPLY.status=0;
	}
	
	//printf("fopen of %s, status: %d\n",filename, GET_REPLY.status);
	
	if((len=send(client_sd, &GET_REPLY,12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	printf("after GET_reply\n");

	//ready to send FILE_DATA
	//filebuff > payload
	if (GET_REPLY.status==0)
		return 0;
	
	// go to end and find its size
	//fseek(fp, 0L, SEEK_END);
	filelen = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	//char *beforeGet = filebuff;
	//char *afterGet = fgets(filebuff,4096,fp);

	strcpy(FILE_DATA.protocol,"\xe3myftp");
	FILE_DATA.type=0xFF;
	//FILE_DATA.status=0;
	FILE_DATA.length=htonl(12+filelen);
	//printf("FILE_DATA.length:%d",ntohl(FILE_DATA.length));

	//send file header
	printf("send file header\n");
	if((len=send(client_sd,&FILE_DATA,12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	printf("sending file size: %ld\n", filelen);

	//send the payload
	
	if ( (len=sendfile(client_sd, fd, 0, filelen)) == -1) {
		printf("sending error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	
	printf("sendfile len:%d\n",len);
	close(fd);
}

int putFile(int client_sd, char request[100]) {
	struct message_s PUT_REPLY;
	struct message_s FILE_DATA;
	FILE *fp;
	int len = 0;
	char *filename = malloc(sizeof(char)*100);
	char buff[4096];
	
	strcpy(filename, &request[12]);
	printf("uploading: %s\n", filename);
	
	strcpy(PUT_REPLY.protocol,"\xe3myftp");
	PUT_REPLY.type=0xAA;
	//PUT_REPLY.status=0;
	PUT_REPLY.length=htonl(12);
	
	if((len=send(client_sd,&PUT_REPLY,12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	
	if((len=recv(client_sd,&FILE_DATA,12,0))<=0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	
	printf("received file header\n");
	printf("length: %d\n",ntohl(FILE_DATA.length));
	printf("protocol: %s\n",FILE_DATA.protocol);
	printf("type: %X\n", FILE_DATA.type);
	printf("status: %d\n", FILE_DATA.status);

	printf("recv len:%d\n",len);

	if (check(FILE_DATA, 0xFF, 0, len, 1)){

	int filesize = ntohl(FILE_DATA.length) - 12;
	
	char saveTo[100]="./filedir/";
	strcat(saveTo, basename(filename));
	
	printf("saveTo %s \n", saveTo);
	if((fp=fopen(saveTo, "wb"))==NULL){
		printf("open error: %s (Errno:%d)\n", strerror(errno),errno);
		return 0;
	}
	
	int totalsize = 0;
	int buffsize = sizeof(buff) < filesize? sizeof(buff) : filesize;
	
	while (filesize > 0) {
		
		printf("waiting for file\n");
		len = recv(client_sd, buff, buffsize, 0);
		printf("recved size: %d\n", len);
		if (len<=0) {
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			return 0;
		}
		fwrite(buff, 1, len, fp);
		totalsize += len;
		if (totalsize >= filesize) { 
			break;
		}
	}
	}

	fclose(fp);
	printf("uploaded!\n");
	return 1;
}

int quitConn(int client_sd){
	struct message_s QUIT_REPLY;
	int len = 0;
	strcpy(QUIT_REPLY.protocol,"\xe3myftp");
	QUIT_REPLY.type=0xAC;
	//QUIT_REPLY.status=0;
	QUIT_REPLY.length=htonl(12);

	if((len=send(client_sd,&QUIT_REPLY,12,0))<=0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	return 1;
}

void *threadFunc(void *arg) {
	int client_sd = (int)arg;
	int isconn=0;
	int isauth=0;
	int i=0;
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
			if (check(request, 0xA5,request.status,len,0)) {
				// this is a list request
				listFiles(client_sd);
				continue;
			} else if (check(request, 0xA7,request.status, len,0)) {
				// get request
				char *filename = malloc(sizeof(char)*100);
				//for(i=0;i<ntohl(request.length)-12;i++)
				strcpy(filename, &buff[12]);
				printf("get %s\n", filename);
				//for(i=0;i<len;i++)
				//	printf("%02x ", buff[i]);
				getFile(client_sd, filename);
				free(filename);
				continue;
			} else if (check(request, 0xA9,request.status,len,0)) {
				// put request
				putFile(client_sd, buff);
				continue;
			} else if (check(request, 0xAB,request.status,len,0)) {
				if(quitConn(client_sd)){
					printf("your client is offline.");
					printf("disconnection from IP:%s\n", inet_ntoa(clientip[client_sd]));
					printf("disconnection from PORT:%u\n", clientpt[client_sd]);
					close(client_sd);
					break;}
				else
					continue;
			}
			// printf("logouting...\n");
			// some more handler for other commands
			close(client_sd);
			break;
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char** argv){
    
    uint16_t serPORT=0;
	int sd=socket(AF_INET,SOCK_STREAM,0);
	int len;
	char buff[100];
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	
	serPORT=(uint16_t) strtol(argv[1], NULL, 10);
	printf("uint16_t PORT: %d\n",serPORT);
	server_addr.sin_port=htons(serPORT);

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
    
//////////////////////////////////////////////
	while(1){   
		// this loop is for the main thread of accept new client
		// Create a client for this new client
		// sub-threads shouldn't reach here.
		int client_sd=0;

		printf("BEFORE ACCEPT\n");
		if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
			printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}else{
           //clientip[client_sd]=client_addr.sin_addr;
            
	        	printf("receive connection from IP:%s\n", inet_ntoa(client_addr.sin_addr.s_addr));
         	    clientip[client_sd]=client_addr.sin_addr.s_addr;
	          	//printf("%d\n",clientip[client_sd]);

	           	printf("receive connection from PORT:%d\n", client_addr.sin_port);
	          	clientpt[client_sd]=client_addr.sin_port;
           		//printf("%d\n",clientpt[client_sd]);
   		}

  		printf("AFTER ACCEPT\n");
		pthread_t tid;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		pthread_create(&tid, &attr, threadFunc, (void*)client_sd);
	}
	close(sd);
	return 0;
}
