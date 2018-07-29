#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include<list>
using namespace std;

#define MAXPACKETSIZE 4096

class TCPServer
{
	public:
	int sockfd, n, pid;
	//list<int> sockfdList(100);
	//std::atomic<bool> capture=false;
	//std::atomic<bool> sendImg=false;
	struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;
	pthread_t serverThread;
	char msg[ MAXPACKETSIZE ];
	static string Message;

	void setup(int port);
	string receive();
	string getMessage();
	void Send(string msg);
	void detach();
	void clean();
void createframegrabber();
	private:
	static void * Task(void * argv);
};

#endif
