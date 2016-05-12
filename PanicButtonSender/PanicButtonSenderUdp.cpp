// PanicButtonSender.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <winsock2.h>
#include <mstcpip.h>  

#pragma comment(lib,"ws2_32.lib")

#define SERVER "testnode1231231.azurewebsites.net"  //ip address of udp server
#define BUFLEN 512  //Max length of buffer
#define PORT 30301   //The port on which to listen for incoming data

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

int main(void)
{
	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other), stcp;
	char buf[BUFLEN];
	WSADATA wsa;

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
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);

	//create UDP socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("UDP socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	
	//start communication
	printf("Waiting for button trigger : ");

	bool waiting = TRUE;

	while (waiting)
	{
		if (key_pressed(VK_F4))
		{
			printf("Firing data sending for 5 seconds\n");

			HANDLE hTimer = NULL;
			LARGE_INTEGER liDueTime;

			liDueTime.QuadPart = -50000000;

			// Create a waitable timer.
			hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
			if (NULL == hTimer)
			{
				printf("CreateWaitableTimer failed (%d)\n", GetLastError());
				exit(EXIT_FAILURE);;
			}


			// Set a timer to wait for 10 seconds.
			if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
			{
				printf("SetWaitableTimer failed (%d)\n", GetLastError());
				exit(EXIT_FAILURE);;
			}

			memset(buf, 0xaa, BUFLEN);

			while (WaitForSingleObject(hTimer, 500) != WAIT_OBJECT_0)
			{

				//send the stream
				if (sendto(s, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
				{
					printf("sendto() failed with error code : %d", WSAGetLastError());
					exit(EXIT_FAILURE);
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