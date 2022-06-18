// server.cpp : Defines the entry point for the console application.
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
#include "Resource.h"
#define SERVER_ADDR "127.0.0.1"

#define SUCCESS_LOGIN "10"
#define SUCCESS_POST "20"
#define SUCCESS_LOGOUT "30"
#define MAX_CLIENT 1024
#pragma comment(lib, "Ws2_32.lib")

/**
* @structure to save login status
* @param1[in] usersock: Socket identifier on the server
* @param2[in] nameuser: Account's name logged in at client
* @param3[in] status: login status
*/
typedef struct status_user {
	SOCKET usersock;
	char nameuser[100] = "";
	int status;
} st_u;

st_u User[MAX_CLIENT];

/**
* @brief check request message
* @param[in] buff: message from client
* @return request message type. return 1 if login request, return 2 if post request, return 3 if logout request, return 0 other.
*/
int check_case(char buff[BUFF_SIZE]) {
	char buff_[BUFF_SIZE] = "";
	strcat_s(buff_, BUFF_SIZE, buff);
	buff_[5] = '\0';
	if (strcmp(buff_, "USER ") == 0) return 1;
	else if (strcmp(buff_, "POST ") == 0) return 2;
	else {
		buff_[3] = '\0';
		if (strcmp(buff_, "BYE") == 0) return 3;
	}
	return 0;
}

/**
* @brief check account
* @param[in] buff: account from client
* @return request message type. return 1 if account active, return 0 if account locked, return 2 if account does not exist
*/
int check_user(char buff[BUFF_SIZE]) {
	char buff_[BUFF_SIZE] = "";
	strcat_s(buff_, BUFF_SIZE, buff + 5);
	for (int i = 0; i < 2050; i++)
		if (strcmp(buff_, user[i]) == 0) {
			if (status_acc[i] == 0) return 1;
			else return 0;
		}
	return 2;
}

/**
* @brief process the data and send the response to the client
* @param[in] x: request case
* @param[in] index: index of the User array handling the current socket
* @param[in] buff: the message from client
* @return request message type. return 1 if account active, return 2 if account locked
*/
void process(int x, int index, char *buff)
{
	int ret;
	switch (x)
	{
	case 2: //POST request
		if (User[index].status == 1) { //logged in
			strcpy_s(buff, BUFF_SIZE, SUCCESS_POST);
			strcat_s(buff, BUFF_SIZE, " You have successfully posted");
			printf("Mess: %s\n", buff);
			ret = send_Stream(User[index].usersock, buff);
			break;
		}
		else {
			strcpy_s(buff, BUFF_SIZE, "21"); //not login
			strcat_s(buff, BUFF_SIZE, " You are not logged in");
			printf("Mess: %s\n", buff);
			ret = send_Stream(User[index].usersock, buff);
			break;
		}
	case 1:
		if (User[index].status == 1) {   //logged in
			char rbuff[BUFF_SIZE] = "";
			strcpy_s(buff, BUFF_SIZE, "14");
			sprintf_s(rbuff, BUFF_SIZE, " You are logging in account: %s", User[index].nameuser);
			strcat_s(buff, BUFF_SIZE, rbuff);
			printf("Mess: %s\n", buff);
			ret = send_Stream(User[index].usersock, buff);
			break;
		}
		else {
			if (check_user(buff) == 0) {         //account locked
				strcpy_s(buff, BUFF_SIZE, "11");
				strcat_s(buff, BUFF_SIZE, " Account has been locked");
				printf("Mess: %s\n", buff);
				ret = send_Stream(User[index].usersock, buff);
				break;
			}
			else if(check_user(buff) == 1){ //successfully login
				User[index].status = 1;
				strcat_s(User[index].nameuser, sizeof(User[index].nameuser), buff + 5);
				strcpy_s(buff, BUFF_SIZE, SUCCESS_LOGIN);
				strcat_s(buff, BUFF_SIZE, " You have successfully logged in");
				printf("Mess: %s\n", buff);
				ret = send_Stream(User[index].usersock, buff);
				break;
			}
			else {
				strcpy_s(buff, BUFF_SIZE, "12");
				strcat_s(buff, BUFF_SIZE, " Account does not exist");
				printf("Mess: %s\n", buff);
				ret = send_Stream(User[index].usersock, buff);
				break;
			}
			break;
		}
	case 0: //other message
		strcpy_s(buff, BUFF_SIZE, "99");
		strcat_s(buff, BUFF_SIZE, " Message unidentified");
		printf("Mess: %s\n", buff);
		ret = send_Stream(User[index].usersock, buff);
		break;
	case 3:  //logout request
		if (User[index].status == 1) {
			User[index].nameuser[0] = '\0';     //delete name user
			User[index].status = 0;				//delete login status if user send BYE
			strcpy_s(buff, BUFF_SIZE, SUCCESS_LOGOUT);
			strcat_s(buff, BUFF_SIZE, " You have successfully logged out");
			printf("Mess: %s\n", buff);
			ret = send_Stream(User[index].usersock, buff);
			User[index].usersock = 0;       //delete socket
			printf("Client disconnected!");
			break;
		}
		else if (User[index].status == 0) {
			strcpy_s(buff, BUFF_SIZE, "21");
			strcat_s(buff, BUFF_SIZE, " You are not logged in");
			printf("Mess: %s\n", buff);
			ret = send_Stream(User[index].usersock, buff);
			break;
		}
	}
}

int main(int argc, char* argv[])
{
	//Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Construct socket
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error! Cannot bind this address.");
		return 0;
	}

	//Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error! Cannot listen.");
		return 0;
	}

	printf("Server started!\n");

	SOCKET connSock;
	fd_set readfds, initfds; //use initfds to initiate readfds at the begining of every loop step
	sockaddr_in clientAddr;
	int ret, nEvents, clientAddrLen;
	char buff[BUFF_SIZE];

	for (int i = 0; i < MAX_CLIENT; i++) {
		User[i].usersock = 0;	// 0 indicates available entry
		User[i].status = 0;
	}

	FD_ZERO(&initfds);
	FD_SET(listenSock, &initfds);

	//Communicate with clients
	while (1) {
		readfds = initfds;		/* structure assignment */
		nEvents = select(0, &readfds, 0, 0, 0);
		if (nEvents < 0) {
			printf("\nError! Cannot poll sockets: %d", WSAGetLastError());
			break;
		}

		//new client connection
		if (FD_ISSET(listenSock, &readfds)) {
			clientAddrLen = sizeof(clientAddr);
			if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
				printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {
				printf("You got a connection from %s\n", inet_ntoa(clientAddr.sin_addr)); /* prints client's IP */

				int i;
				for (i = 0; i < MAX_CLIENT; i++)
					if (User[i].usersock == 0) {
						User[i].usersock = connSock;
						FD_SET(User[i].usersock, &initfds);
						break;
					}

				if (i == MAX_CLIENT) {
					printf("\nToo many clients.");
					closesocket(connSock);
				}

				if (--nEvents == 0)
					continue; //no more event
			}
		}

		//receive data from clients
		for (int i = 0; i < MAX_CLIENT; i++) {
			if (User[i].usersock == 0)
				continue;

			if (FD_ISSET(User[i].usersock, &readfds)) {
				ret = recv_Stream(User[i].usersock, buff);
				if (ret <= 0) {
					FD_CLR(User[i].usersock, &initfds);
					closesocket(User[i].usersock);
					User[i].usersock = 0;
				}
				else if (ret > 0) {
					int x = check_case(buff);
					process(x, i, buff);
				}
			}

			if (--nEvents <= 0)
				continue; //no more event
		}

	}

	closesocket(listenSock);
	WSACleanup();
	return 0;
}
