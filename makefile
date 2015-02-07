all: myftpserver  myftpclient

myftpserver: myftpserver.c
	gcc -g -Wall -o myftpserver myftpserver.c

myftpclient : myftpclient.c
	gcc -g -Wall -o myftpclient myftpclient.c