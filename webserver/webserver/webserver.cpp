// webserver.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#pragma comment ( lib, "Ws2_32.lib" )
#include <WinSock2.h>
#include <process.h>

#define BUF_SIZE	2048
#define BUF_SMALL	100

enum ErrorCode
{
	ERROR_400,
	ERROR_404,
};

unsigned WINAPI RequestHandler(void* arg);
char* ContentType(char* file);
void SendData(SOCKET sock, char* ct, char* fileName);
void SendErrorMSG(SOCKET sock, ErrorCode errorcode);
void ErrorHandling(char* message);


int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	HANDLE hThread;
	DWORD dwThreadID;
	int clntAdrSize = 0;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAStratup() error!");
	}

	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET)
	{
		ErrorHandling("WSASocket() error!");
		return 1;
	}
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(9190);

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		ErrorHandling("bind() error");
	}
	if (listen(hServSock, 5) == SOCKET_ERROR)
	{
		ErrorHandling("listen() error");
	}

	while (true)
	{
		clntAdrSize = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSize);
		if (hClntSock == INVALID_SOCKET)
		{
			perror("accept() error!");
			continue;
		}
		printf("Connection Request: %s:%d\n", inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
		hThread = (HANDLE)_beginthreadex(NULL, 0, RequestHandler, (void*)hClntSock, 0, (unsigned*)&dwThreadID);
		if (hThread == INVALID_HANDLE_VALUE)
		{
			perror("_beginthreadex() error!");
		}
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

unsigned WINAPI RequestHandler(void* arg)
{
	SOCKET hClntSock = (SOCKET)arg;
	char buf[BUF_SIZE] = { 0, };
	char method[BUF_SMALL] = { 0, };
	char* context = nullptr;
	char ct[BUF_SMALL] = { 0, };
	char fileName[BUF_SMALL] = { 0, };

	if (recv(hClntSock, buf, BUF_SIZE, 0) == SOCKET_ERROR)
	{
		perror("recv() error!");
		return 1;
	}
	if (strstr(buf, "HTTP/") == NULL)
	{
		SendErrorMSG(hClntSock, ERROR_400);
		closesocket(hClntSock);
		return 1;
	}

	strcpy_s(method, strtok_s(buf, " /", &context));
	if (strcmp(method, "GET"))
	{
		SendErrorMSG(hClntSock,ERROR_400);
	}

	strcpy_s(fileName, strtok_s(NULL, " /", &context));
	if (ContentType(fileName) == NULL)
	{
		SendErrorMSG(hClntSock, ERROR_404);
	}
	else
	{
		strcpy_s(ct, ContentType(fileName));
		SendData(hClntSock, ct, fileName);
	}
	SendData(hClntSock, ct, fileName);
	return 0;
}

void SendData(SOCKET sock, char* ct, char* fileName)
{
	char protocol[] = "HTTP/1.0 200 OK\r\n";
	char servName[] = "Server:simple web server\r\n";
	char cntLen[BUF_SMALL];
	char cntType[BUF_SMALL];
	char buf[BUF_SIZE];
	FILE* sendFile;
	int fileSize;
	errno_t err;

	sprintf_s(cntType, "Content-type:%s\r\n\r\n", ct);
	if ((err = fopen_s(&sendFile, fileName, "rt")) != 0)
	{
		SendErrorMSG(sock, ERROR_404);
		return;
	}

	fseek(sendFile, 0, SEEK_END);
	fileSize = ftell(sendFile);
	fseek(sendFile, 0, SEEK_SET);
	sprintf_s(cntLen, "Content-length:%d\r\n", fileSize);

	if (send(sock, protocol, strlen(protocol), 0) == SOCKET_ERROR)
	{
		perror("send() error!");
	}
	if (send(sock, servName, strlen(servName), 0) == SOCKET_ERROR)
	{
		perror("send() error!");
	}
	if (send(sock, cntLen, strlen(cntLen), 0) == SOCKET_ERROR)
	{
		perror("send() error!");
	}
	if (send(sock, cntType, strlen(cntType), 0) == SOCKET_ERROR)
	{
		perror("send() error!");
	}

	while (!feof(sendFile)) {
		fgets(buf, BUF_SIZE, sendFile);
		send(sock, buf, strlen(buf), 0);
	}
	fclose(sendFile);

// 	while (fgets(buf, BUF_SIZE, sendFile) != NULL)
// 	{
// 		send(sock, buf, strlen(buf), 0);
// 	}

	closesocket(sock);

}

void SendErrorMSG(SOCKET sock, ErrorCode errorcode)
{
	char* errorMsg;
	char protocol[BUF_SMALL] = { 0, };
	char servName[] = "Server:simple web server\r\n";
	char cntLen[] = "Content-length:2048\r\n";
	char cntType[] = "Content-type:text/html\r\n\r\n";
	char content[BUF_SIZE];

	switch (errorcode)
	{
	case ERROR_400:
		errorMsg = "400 Bad Request\r\n";
		break;
	case ERROR_404:
		errorMsg = "404 Not Found\r\n";
		break;
	}
	

	sprintf_s(protocol, "HTTP/1.0 %s", errorMsg);
	sprintf_s(content, "<html><head><title>NETWORK</title></head>"
		"<body><font size=+2><br>%s"
		"</font></body></html>", errorMsg);

	send(sock, protocol, strlen(protocol), 0);
	send(sock, servName, strlen(servName), 0);
	send(sock, cntLen, strlen(cntLen), 0);
	send(sock, cntType, strlen(cntType), 0);
	closesocket(sock);
}

char* ContentType(char* file)
{
	char extension[BUF_SMALL];
	char fileName[BUF_SMALL];
	char* context = nullptr;
	strcpy_s(fileName, file);
	strtok_s(fileName, ".", &context);
	strcpy_s(extension, strtok_s(NULL, ".", &context));
	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
	{
		return "text/html";
	}
	else
	{
		return "text/plain";
	}
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}