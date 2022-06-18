// server.cpp : Defines the entry point for the console application.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include<stdio.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<math.h>
#include<ctime>
#include<string>
#include<process.h>
#include<windows.h>
#include<conio.h>
#include "Stream4bytes.h"
#pragma comment(lib,"Ws2_32.lib")

#define SUCCESS_LOGIN "10"
#define SUCCESS_POST "20"
#define SUCCESS_LOGOUT "30"
#define MAX_CLIENT 64
#define SERVER_ADDR "127.0.0.1"
#pragma comment(lib, "Ws2_32.lib")

st_u User[MAX_CLIENT];
SOCKET		listenSock, connSock;
sockaddr_in clientAddr;
int			clientAddrLen = sizeof clientAddr, ret, i, clientPort, numAccount;
char		clientIP[INET_ADDRSTRLEN], buff[BUFF_SIZE], sBuff[BUFF_SIZE], rBuff[BUFF_SIZE];

int acceptNewClient(SOCKET socks[], WSAEVENT events[], DWORD &nEvents, st_u User[]) {
	if ((connSock = accept(socks[0], (sockaddr *)&clientAddr, &clientAddrLen)) == SOCKET_ERROR) {
		printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
		return 1;
	}
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof clientIP);
	clientPort = ntohs(clientAddr.sin_port);
	//Add new socket into socks array
	printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
	socks[nEvents] = connSock;
	events[nEvents] = WSACreateEvent();

	User[nEvents].usersock = socks[nEvents];
	//strcpy(sess[nEvents].clientIP, clientIP);
	//sess[nEvents].clientPort = clientPort;
	User[nEvents].status = 0;

	WSAEventSelect(socks[nEvents], events[nEvents], FD_READ | FD_CLOSE);
	nEvents++;
	//reset event
	WSAResetEvent(events[0]);
	return 0;
}
/**
* @brief check request message
* @param[in] buff: message from client
* @return request message type. return 1 if login request, return 2 if post request, return 3 if logout request, return 0 other.
*/
void process(st_u User[], int index, char buff[], char sBuff[]) {
	int checkcase = check_case(buff);
	int Rely;
	if (checkcase == 1) Rely = login(User, index, buff);
	else if (checkcase == 2) Rely = post(User, index, buff);
	else if (checkcase == 3) Rely = logout(User, index);
	else Rely = REQUEST_UNKNOWN;
	showResult(Rely);
	sprintf(sBuff, "%d", Rely);
	send_Stream(User[index].usersock, sBuff);
	buff[0] = 0;
}
int main(int argc, char* argv[])
{
	//numAccount = readFile(acc);
	char buff[BUFF_SIZE], rbuff[BUFF_SIZE];
	DWORD		nEvents = 0;
	DWORD		index;
	SOCKET		socks[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT	events[WSA_MAXIMUM_WAIT_EVENTS];
	WSANETWORKEVENTS sockEvent;
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct LISTEN socket
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	socks[0] = listenSock;
	events[0] = WSACreateEvent(); //create new events
	nEvents++;

	// Associate event types FD_ACCEPT and FD_CLOSE
	// with the listening socket and newEvent
	WSAEventSelect(socks[0], events[0], FD_ACCEPT | FD_CLOSE);


	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error %d: Cannot place server socket in state LISTEN.", WSAGetLastError());
		return 0;
	}

	printf("Server started!\n");

	SOCKET connSock;
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);
	int ret, i;
	for (i = 1; i < MAX_CLIENT; i++) {
		User[i].usersock = 0;
		User[i].status = 0;
	}
	for (i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; i++) {
		socks[i] = 0;
	}
	while (1) {
		//wait for network events on all socket
		index = WSAWaitForMultipleEvents(nEvents, events, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
			break;
		}

		index = index - WSA_WAIT_EVENT_0;
		WSAEnumNetworkEvents(socks[index], events[index], &sockEvent);

		if (index == 0) {
			if (sockEvent.lNetworkEvents & FD_ACCEPT) {
				if (sockEvent.iErrorCode[FD_ACCEPT_BIT] != 0) {
					printf("FD_ACCEPT failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
					break;
				}

				if (acceptNewClient(socks, events, nEvents, User) == 1) break;
				//Add new socket into socks array
				int i;
				if (nEvents == WSA_MAXIMUM_WAIT_EVENTS) {
					printf("\nToo many clients.");
					closesocket(connSock);
				}
				//reset event
				WSAResetEvent(events[index]);
			}
		}
		else {
			if (sockEvent.lNetworkEvents & FD_READ) {
				//Receive message from client
				if (sockEvent.iErrorCode[FD_READ_BIT] != 0) {
					printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
					return 0;
				}

				memset(buff, NULL, sizeof buff);
				int ret = recv_Stream(User[index].usersock, buff);
				if (ret <= 0) {
					printf("Client disconnect\n");
					closesocket(socks[index]);
					User[index].nameuser[0] = 0;
					User[index].status = 0;
					User[index].usersock = 0;
					socks[index] = 0;
					WSACloseEvent(events[index]);
					nEvents--;
				}
				else {
					buff[ret] = 0;
					strcat_s(rBuff, buff);
					process(User, index, rBuff, sBuff);
					WSAResetEvent(events[index]);
				}
			}

			if (sockEvent.lNetworkEvents & FD_CLOSE) {
				//Release socket and event
				printf("Client disconnect\n");
				closesocket(socks[index]);
				User[index].nameuser[0] = 0;
				User[index].status = 0;
				User[index].usersock = 0;
				socks[index] = 0;
				WSACloseEvent(events[index]);
				nEvents--;
			}
		}
	}
	return 0;
}