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
#include "Resource.h"

#define MAX_ACC 2048
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

/**
* @structure to save login status
* @param1[in] usersock: Socket identifier on the server
* @param2[in] nameuser: Account's name logged in at client
* @param3[in] status: true is login, false is not login
*/
typedef struct status_user {
	SOCKET usersock;
	char nameuser[100] = "";
	bool status;
} st_u;

/**
* @brief send message in stream by 4 bytes
* @param[in] socket: the connected socket client - server
* @param[in] buffer: message
* @return: if no error occurs, return the number of bytes sent or send a value of SOCKET_ERROR
*/
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
	//ret = recv(socket, (char *)bytes, 4, MSG_WAITALL);
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
* @return status account. return 1 if account active, return 0 if account locked, return 2 if account does not exist
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
* @brief login account
* @param[out] User[]: list User
* @param[in] index: the index of current User
* @param[in] username: account from client
* @return request message type. return LOGGED if logged in, ACCOUNT_BLOCKED if account is blocked, SUCCESS_LOGIN if success login, ACCOUNT_NOT_EXISTED if account is not existed
*/
int login(st_u User[], int index, char username[]) {
	if (User[index].status == 1) {   //logged in
		return LOGGED;
	}
	else {
		if (check_user(username) == 0) return ACCOUNT_BLOCKED;
		else if (check_user(username) == 1) {
			User[index].status = 1;
			strcat_s(User[index].nameuser, sizeof(User[index].nameuser), username + 5);
			return SUCCESS_LOGIN;
		}
		else return ACCOUNT_NOT_EXISTED;
	}
}

/**
* @brief post message
* @param[out] User[]: list User
* @param[in] index: the index of current User
* @param[in] buff: message from client
* @return request message type. return NOT_LOGIN if not login, SUCCESS_POST if success post
*/
int post(st_u User[], int index, char buff[]) {
	if (User[index].status == 1) return SUCCESS_POST;
	return NOT_LOGIN;
}

/**
* @brief logout account
* @param[out] User[]: list User
* @param[in] index: the index of current User
* @return request message type. return NOT_LOGIN if not login, SUCCESS_LOGOUT if success logout
*/
int logout(st_u User[], int index) {
	if (User[index].status == 1) {
		User[index].nameuser[0] = '\0';     //delete name user
		User[index].status = 0;
		return SUCCESS_LOGOUT;
	}
	else return NOT_LOGIN;
}

/**
* @brief show message rely from server
* @param[in] Rely: the message from server
*/
void showResult(int Rely) {
	switch (Rely) {
		case SUCCESS_LOGIN: {
			printf("%d : You have successfully logged in!\n", Rely);
			break;
		}
		case ACCOUNT_NOT_EXISTED: {
			printf("%d : Account does not exist!\n", Rely);
			break;
		}
		case ACCOUNT_BLOCKED: {
			printf("%d : Account has been locked!\n", Rely);
			break;
		}
		case LOGGED: {
			printf("%d : You are logging in another account!\n", Rely);
			break;
		}
		case SUCCESS_POST: {
			printf("%d : You have successfully posted!\n", Rely);
			break;
		}
		case NOT_LOGIN: {
			printf("%d : You are not logged in!\n", Rely);
			break;
		}
		case SUCCESS_LOGOUT: {
			printf("%d : You have successfully logged out!\n", Rely);
			break;
		}
		case REQUEST_UNKNOWN: {
			printf("%d : Message unidentified!\n", Rely);
			break;
		}
	}
}