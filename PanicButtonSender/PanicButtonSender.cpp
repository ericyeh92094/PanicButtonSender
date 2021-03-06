// PanicButtonSender.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <winsock2.h>
#include <mstcpip.h>  

#pragma comment(lib,"ws2_32.lib")

#define SERVER "testnode1231231.azurewebsites.net"  //ip address of udp server
#define BUFLEN 512  //Max length of buffer
#define PORT 80   //The port on which to listen for incoming data

using namespace std;

int socket_set_keepalive(int s)
{
	struct tcp_keepalive outvars[1] = { 0 }, kavars[1] = {
		1,
		10 * 1000,        /* 10 seconds */
		5 * 1000          /* 5 seconds */
	};

	DWORD ret = 0;
	char alive;

	/* Set: use keepalive on s */
	alive = 1;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char *)&alive,
		sizeof(alive)) != 0)
	{
		printf("Set keep alive error: %s.\n", strerror(errno));
		return -1;
	}

	if (WSAIoctl(s, SIO_KEEPALIVE_VALS, kavars, sizeof(struct tcp_keepalive), outvars, sizeof(struct tcp_keepalive), &ret, NULL,
		NULL) != 0)
	{
		printf("Set keep alive error: %s.\n", strerror(WSAGetLastError()));
		return -1;
	}

	return 0;
}

bool key_pressed(int key) {

	return (GetAsyncKeyState(key) & 0x8000 != 0);
}

int main(int argc, char* argv[])
{
	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other), stcp;
	char buf[BUFLEN];
	WSADATA wsa;

	if (argc < 3)
		exit(EXIT_FAILURE);

	char server_url[_MAX_PATH];
	int button_id = 0;
	strcpy(server_url, argv[1]);
	button_id = atoi(argv[2]);

	char *format_string = "PUT /error/%d HTTP/1.1\r\nHost: %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
	char put_string[_MAX_PATH];

	sprintf(put_string, format_string, button_id, server_url);

	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");


	// Connect TCP for the heartbeat
	if ((stcp = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == SOCKET_ERROR)
	{
		printf("TCP socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	if (socket_set_keepalive(stcp) != 0)
	{
		printf("set_keepalive() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	//setup address structure
	struct hostent *host;
	host = gethostbyname(server_url);

	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	si_other.sin_addr.s_addr = *((unsigned long*)host->h_addr); //inet_addr(SERVER);

	//create TCP socket
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == SOCKET_ERROR)
	{
		printf("TCP socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	struct timeval tv; 
	tv.tv_sec = 5000; // time out 10 seconds
	tv.tv_usec = 0;

	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof tv))
	{
		printf("setsockopt() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}


	if (connect(s, (SOCKADDR*)(&si_other), sizeof(si_other)) != 0) {
		cout << "Could not connect";
		system("pause");
		return 1;
	}

	//start communication
	printf("Waiting for button trigger : ");

	bool waiting = TRUE;

	while (waiting)
	{
		if (key_pressed(VK_F4))
		{
			printf("Firing data sending\n");


			//send the stream
			if (send(s, put_string, strlen(put_string), 0) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			char buffer[10000];
			int nDataLength = 0;
			while ((nDataLength = recv(s, buffer, 10000, 0)) > 0) {
				int i = 0;
				while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r') {
					cout << buffer[i];
					i += 1;
				}
			}

			printf("Waiting for button trigger : ");

		}

	}

	closesocket(s);
	closesocket(stcp);
	WSACleanup();

	return 0;
}