// client.cpp : Defines the entry point for the console application.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <WinSock2.h>
#include "stdafx.h"
#include <WS2tcpip.h>
#include "process.h"
#include "Stream4bytes.h"

#define SUCCESS_LOGIN "10"
#define SUCCESS_POST "20"
#define SUCCESS_LOGOUT "30"
#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
	//Input server_port and server_address
	char* SERVER_ADDR = argv[1];
	char* port = argv[2];
	if (SERVER_ADDR == NULL || port == NULL) {
		printf("Missing server_port or server_address");
		return 0;
	}
	int SERVER_PORT = atoi(port);

	//Init Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	//Set time-out for receiving
	int tv = 10000; //time-out: 10000ms
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	//Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	//Request to connect server
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		return 0;
	}
	printf("Connected server!\n");
	printf("USER + account: login\nPOST + message: post the message\nBYE: logout\n");

	//Communicate with server
	char buff[BUFF_SIZE];
	int ret, messLen;
	while (1) {
		//Send message
		printf("Send to server: ");
		gets_s(buff, BUFF_SIZE);
		messLen = strlen(buff);
		ret = send_Stream(client, buff);
		if (ret == SOCKET_ERROR)
			printf("Error %d: Cannot send data.", WSAGetLastError());

		//Receive message
		ret = recv_Stream(client, buff);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT)
				printf("Time-out!");
			else printf("Error %d: Cannot receive data", WSAGetLastError());
		}
		else if (strlen(buff) > 0) {
			buff[ret] = 0;
			showResult(buff);
		}
		if (strcmp(buff, "30") == 0) break;
	}
	closesocket(client);
	WSACleanup();
	return 0;
}

