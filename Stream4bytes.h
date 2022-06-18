#pragma once
#include<stdio.h>
#include<math.h>
#include<ctime>
#include<string>
#include<process.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#include<conio.h>
#include <cstdint>

#define BUFF_SIZE 1024
//define code request
#define SUCCESS_LOGIN 10
#define SUCCESS_POST 20
#define SUCCESS_LOGOUT 30
#define ACCOUNT_BLOCKED 11
#define ACCOUNT_NOT_EXISTED 12
#define LOGGED 13
#define NOT_LOGIN 21
#define REQUEST_UNKNOWN 99

int send_Stream(SOCKET &socket, char *buff) {
	unsigned char bytes[4];
	int ret, buffLen;

	buffLen = strlen(buff);
	uint32_t len = htonl(buffLen);
	memcpy_s(bytes, 4, &len, 4);

	//Send the length of the message
	ret = send(socket, (char *)bytes, 4, 0);
	if (ret == SOCKET_ERROR) {
		return ret;
	}
	//Send the message
	for (int i = 0; i < buffLen; i += ret) {
		ret = send(socket, buff + i, buffLen - i, 0);
		if (ret == SOCKET_ERROR)
			return ret;
	}
}

/**
* @brief send message in stream by 4 bytes
* @param[in] sock: the connected socket client - server
* @param[in] buffer: message
* @return: if no error occurs, return the number of bytes sent or send a value of SOCKET_ERROR
*/
int recv_Stream(SOCKET socket, char* buff) {
	unsigned char bytes[4];
	char temp[BUFF_SIZE];
	int ret;
	uint32_t len = 0;

	//Receive the length of the message
	ret = recv(socket, (char *)bytes, 4, 0);
	if (ret < 1) {
		printf("Error %d: Can't receive data", WSAGetLastError());
		return ret;
	}

	memcpy_s(&len, 4, bytes, 4);
	len = ntohl(len);

	strcpy_s(buff, BUFF_SIZE, "");

	//Recive the message
	while (len != 0) {
		ret = recv(socket, temp, len, 0);
		if (ret < 1)
			return ret;

		temp[ret] = 0;
		strcat_s(buff, BUFF_SIZE, temp);

		//left bytes to receive
		len -= ret;
	}
	return ret;
}

/**
* @brief show message rely from server
* @param[in] buff: the message from server
*/
void showResult(char *buff) {
	int Rely = atoi(buff);
	switch (Rely) {
		case SUCCESS_LOGIN: {
			printf("You have successfully logged in!\n");
			break;
		}
		case ACCOUNT_NOT_EXISTED: {
			printf("Account does not exist!\n");
			break;
		}
		case ACCOUNT_BLOCKED: {
			printf("Account has been locked!\n");
			break;
		}
		case LOGGED: {
			printf("You are logging in another account!\n");
			break;
		}
		case SUCCESS_POST: {
			printf("You have successfully posted!\n");
			break;
		}
		case NOT_LOGIN: {
			printf("You are not logged in!\n");
			break;
		}
		case SUCCESS_LOGOUT: {
			printf("You have successfully logged out!\n");
			break;
		}
		case REQUEST_UNKNOWN: {
			printf("Message unidentified!\n");
			break;
		}
	}
}