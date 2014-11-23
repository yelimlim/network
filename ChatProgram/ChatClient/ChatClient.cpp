// ChatClient.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#pragma warning (disable:4996)
#pragma comment ( lib, "Ws2_32.lib" )
#include <WinSock2.h>

#define BUF_SIZE	1024
#define NAME_SIZE	20

unsigned WINAPI SendMsg(void* arg);
unsigned WINAPI RecvMsg(void* arg);
void ErrorHandling(char *message);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];


int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAdr;
	HANDLE hSndThread, hRcvThread;
	char message[BUF_SIZE];
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAStartup() error!");
	}

	printf("What your name? \n");
	fgets(name, sizeof(name), stdin);
	name[strlen(name) - 1] = '\0';
	hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandling("socket() error!");
	}

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("192.168.0.7");
	servAdr.sin_port = htons(5000);

	if (connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		ErrorHandling("connect() error!");
	}

	hSndThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSocket, 0, NULL);
	if (hSndThread == INVALID_HANDLE_VALUE)
	{
		perror("_beginthreadex() error!");
	}
	hRcvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSocket, 0, NULL);
	if (hRcvThread == INVALID_HANDLE_VALUE)
	{
		perror("_beginthreadex() error!");
	}

	WaitForSingleObject(hSndThread, INFINITE);
	WaitForSingleObject(hRcvThread, INFINITE);

	closesocket(hSocket);
	WSACleanup();
	return 0;
}

unsigned WINAPI SendMsg(void* arg)
{
	SOCKET hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	while (true)
	{
		fgets(msg, BUF_SIZE, stdin);
		if (!strcmp(msg, "q\n") || !strcmp(msg,"Q\n"))
		{
			closesocket(hSock);
			exit(0);
		}
		sprintf_s(nameMsg, "%s %s", name, msg);
		if (send(hSock, nameMsg, strlen(nameMsg), 0) == SOCKET_ERROR)
		{
			perror("send() error!");
		}
	}
	
	return 0;
}

unsigned WINAPI RecvMsg(void* arg)
{
	int hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	int strLen;
	while (true)
	{
		strLen = recv(hSock, nameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		if (strLen == SOCKET_ERROR)
		{
			perror("recv() error!");
		}
		if (strLen == -1)
		{
			return -1;
		}
		nameMsg[strLen] = 0;
		fputs(nameMsg, stdout);
	}

	return 0;
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

