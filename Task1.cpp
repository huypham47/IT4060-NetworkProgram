// Task1.cpp : Defines the entry point for the console application.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "stdio.h"
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "WinSock2.h"
#include "stdafx.h"
#include "WS2tcpip.h"
#include "process.h"
#include "Stream4bytes.h"

#define SERVER_ADDR "127.0.0.1"

#define SUCCESS_LOGIN "10"
#define SUCCESS_POST "20"
#define SUCCESS_LOGOUT "30"
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

//database username + status
char username[20][20] = { "tungbt", "admin", "ductq" };
char statusUser[20][20] = { "0", "0", "1" }; //0: active 1: locked

st_u User[4]; //create global user
int index_user; //global index of User is being used

/**
* @brief check request message
* @param[in] buff: message from client
* @return request message type. return 1 if login request, return 2 if post request, return 3 if logout request, return 0 other.
*/
int check_case(char buff[BUFF_SIZE]) {
	char buff_[BUFF_SIZE] = "";
	strcat_s(buff_, BUFF_SIZE, buff);
	buff_[4] = '\0';
	if (strcmp(buff_, "USER") == 0) return 1;
	else if (strcmp(buff_, "POST") == 0) return 2;
	else {
		buff_[3] = '\0';
		if (strcmp(buff_, "BYE") == 0) return 3;
	}
	return 0;
}

/**
* @brief check account
* @param[in] buff: account from client
* @return request message type. return 1 if account active, return 2 if account locked
*/
int check_user(char buff[BUFF_SIZE]) {
	char buff_[BUFF_SIZE] = "";
	strcat_s(buff_, BUFF_SIZE, buff + 5);
	for (int i = 0; i < 3; i++)
		if (strcmp(buff_, username[i]) == 0)
			if (strcmp(statusUser[i], "0") == 0) return 1;
	return 0;
}

/* echoThread - Thread to receive the message from client and echo*/
unsigned __stdcall echoThread(void *param) {
	//find index User is NULL
	for (int i = 0; i < 3; i++) {
		if (User[i].usersock == NULL) {
			index_user = i;
			break;
		}
	}
	char buff[BUFF_SIZE];
	char rbuff[BUFF_SIZE] = "";
	int ret;
	SOCKET connectedSocket = (SOCKET)param;
	User[index_user].usersock = connectedSocket; //set socket
	while (1) {
		int index = index_user;
		//Receive message
		ret = recv_Stream(connectedSocket, buff);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot receive data.\n", WSAGetLastError());
			break;
		}
		else {
			buff[ret] = 0;
			printf("Receive from client %s\n", buff);
			int x = check_case(buff);
			switch (x)
			{
			case 2: //POST request
				if (User[index].status == 1) { //logged in
					strcpy_s(buff, BUFF_SIZE, SUCCESS_POST);
					strcat_s(buff, BUFF_SIZE, " You have successfully posted");
					printf("buff: %s\n", buff);
					ret = send_Stream(connectedSocket, buff);
					break;
				}
				else {
					strcpy_s(buff, BUFF_SIZE, "21"); //not login
					strcat_s(buff, BUFF_SIZE, " You are not logged in");
					printf("buff: %s\n", buff);
					ret = send_Stream(connectedSocket, buff);
					break;
				}
			case 1:
				if (check_user(buff) == 0) {         //account locked
					strcpy_s(buff, BUFF_SIZE, "11");
					strcat_s(buff, BUFF_SIZE, " Account has been locked");
					printf("buff: %s\n", buff);
					ret = send_Stream(connectedSocket, buff);
					break;
				}
				else {
					if (User[index].status == 1) {   //logged in
						strcpy_s(buff, BUFF_SIZE, "14");
						sprintf_s(rbuff, BUFF_SIZE, " You are logging in account: %s", User[index].nameuser);
						strcat_s(buff, BUFF_SIZE, rbuff);
						printf("buff: %s\n", buff);
						ret = send_Stream(connectedSocket, buff);
						break;
					}
					else {     //Account is being used in another client
						char tmp_user[100] = "";
						int flag = 0;
						strcat_s(tmp_user, sizeof(tmp_user), buff + 5);
						for (int i = 0; i < 3; i++) {
							if (strcmp(tmp_user, User[i].nameuser) == 0) {
								strcpy_s(buff, BUFF_SIZE, "13");
								sprintf_s(rbuff, BUFF_SIZE, " Account \"%s\" is being used in another client", User[i].nameuser);
								strcat_s(buff, BUFF_SIZE, rbuff);
								printf("buff: %s\n", buff);
								ret = send_Stream(connectedSocket, buff);
								flag = 1;
								break;
							}
						}
						if (flag == 0) {  //successfully login
							User[index].status = 1;
							strcat_s(User[index].nameuser, sizeof(User[index].nameuser), buff + 5);
							strcpy_s(buff, BUFF_SIZE, SUCCESS_LOGIN);
							strcat_s(buff, BUFF_SIZE, " You have successfully logged in");
							printf("buff: %s\n", buff);
							ret = send_Stream(connectedSocket, buff);
							break;
						}
						break;
					}
				}
			case 0: //other message
				strcpy_s(buff, BUFF_SIZE, "99");
				strcat_s(buff, BUFF_SIZE, " Message unidentified");
				printf("buff: %s\n", buff);
				ret = send_Stream(connectedSocket, buff);
				break;
			case 3:  //logout request
				printf("Case 3\n\n");
				if (User[index].status == 1) {
					User[index].nameuser[0] = '\0';
					User[index].status = 0;				//delete login status if user send BYE				
					User[index].usersock = NULL;
					strcpy_s(buff, BUFF_SIZE, SUCCESS_LOGOUT);
					strcat_s(buff, BUFF_SIZE, " You have successfully logged out");
					printf("buff: %s\n", buff);
					ret = send_Stream(connectedSocket, buff);
					break;
				}
				else if (User[index].status == 0) {
					strcpy_s(buff, BUFF_SIZE, "21");
					strcat_s(buff, BUFF_SIZE, " You are not logged in");
					printf("buff: %s\n", buff);
					ret = send_Stream(connectedSocket, buff);
					break;
				}
			}
		}
	}
	//delete login status if user turns off the login session
	User[index_user].nameuser[0] = '\0';
	User[index_user].status = 0;
	User[index_user].usersock = NULL;
	closesocket(connectedSocket);
	return 0;
}

int main(int argc, char* argv[])
{
	//Input server port
	char* port = argv[1];
	if (port == NULL) {
		printf("Missing server_port");
		return 0;
	}
	int SERVER_PORT = atoi(port);

	//Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Construct socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		return 0;
	}

	//Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error %d: Cannot place server socket in state LISTEN.", WSAGetLastError());
		return 0;
	}

	printf("Server started!\n");

	//Communicate with client
	SOCKET connSock;
	sockaddr_in clientAddr;
	char clientIP[INET_ADDRSTRLEN];
	int clientAddrLen = sizeof(clientAddr), clientPort;
	while (1) {
		connSock = accept(listenSock, (sockaddr *)& clientAddr, &clientAddrLen);
		if (connSock == SOCKET_ERROR)
			printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
		else {
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);
			printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
			_beginthreadex(0, 0, echoThread, (void *)connSock, 0, 0); //start thread
		}
	}

	closesocket(listenSock);

	WSACleanup();

	return 0;
}
