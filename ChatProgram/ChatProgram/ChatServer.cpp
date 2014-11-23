// ChatProgram.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#pragma comment ( lib, "Ws2_32.lib" )
#include <WinSock2.h>
#include <windows.h>

#define BUF_SIZE	100
#define READ		3
#define WRITE		5
#define MAX_CLNT	100

struct LPPER_HANDLE_DATA
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
};

struct LPPER_IO_DATA
{
	OVERLAPPED overlapped;
	WSABUF wsabuf;
	char buffer[BUF_SIZE];
	int rwMode;
};

unsigned int  WINAPI	EchoThreadMain(LPVOID CompletionPortIO);
void					RemoveClnt(SOCKET removesoket);
void					ErrorHandling(char* message);

int clntCnt = 0;
SOCKET clntSocks[MAX_CLNT];
HANDLE hMutex;

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsaData;
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA* ioInfo;
	LPPER_HANDLE_DATA* handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	unsigned long int recvBytes, flags = 0;
	int i = 0;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAStartup() error!");
	}

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&sysInfo);
	for (i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
	{
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}

	hMutex = CreateMutex(NULL, FALSE, NULL);
	hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hServSock == INVALID_SOCKET)
	{
		ErrorHandling("Sock() error!");
	}
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(5000);
	
	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		ErrorHandling("bind() error!");
	}
	if (listen(hServSock, 5) == SOCKET_ERROR)
	{
		ErrorHandling("litsen() error!");
	}

	while (true)
	{
		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);

		hClntSock = accept(hServSock, (SOCKADDR*)& clntAdr, &addrLen);
		if (hClntSock == INVALID_SOCKET)
		{
			perror("accept() error");
		}


		handleInfo = (LPPER_HANDLE_DATA*)malloc(sizeof(LPPER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		ioInfo = (LPPER_IO_DATA*)malloc(sizeof(LPPER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsabuf.len = BUF_SIZE;
		ioInfo->wsabuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ;

		WaitForSingleObject(hMutex, INFINITE);
		clntSocks[clntCnt++] = hClntSock; 
		ReleaseMutex(hMutex);

		WSARecv(handleInfo->hClntSock, &(ioInfo->wsabuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
	}

	closesocket(hServSock);
	WSACleanup();
	return 0;
}

unsigned int WINAPI EchoThreadMain(LPVOID pComPort)
{
	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA* handleInfo;
	LPPER_IO_DATA* ioInfo;
	DWORD flags = 0;
	int count = 0;

	while (true)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;
		printf("socknum: %d\n", sock);

		

		if (ioInfo->rwMode == READ)
		{
			puts("message received!");
			if (bytesTrans == 0)
			{
				closesocket(sock);
				free(handleInfo);
				free(ioInfo);
				continue;
			}

			//printf("socknum: %d\n", sock);
			for (int i = 0; i < clntCnt; ++i)
			{
				memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
				ioInfo->wsabuf.len = bytesTrans;
				ioInfo->rwMode = WRITE;
				
				if (WSASend(clntSocks[i], &(ioInfo->wsabuf), 1, NULL, 0, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						perror("send() error!");
					}
				}
			}
			

			ioInfo = (LPPER_IO_DATA*)malloc(sizeof(LPPER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsabuf.len = BUF_SIZE;
			ioInfo->wsabuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;
			if (WSARecv(sock, &(ioInfo->wsabuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					perror("send() error!");
				}
			}

			if (!strcmp(ioInfo->wsabuf.buf, "q") || !strcmp(ioInfo->wsabuf.buf, "Q"))
			{
				RemoveClnt(sock);
				break;
			}
		}
		else
		{
			puts("message sent!");
			count++;
			if (sock == clntSocks[count])
			{
				free(ioInfo);
				count = 0;
			}
		}
	}

	return 0;
}

void RemoveClnt(SOCKET removesoket)
{
	WaitForSingleObject(hMutex, INFINITE);
	for (int i = 0; i < clntCnt; ++i)
	{
		if (removesoket == clntSocks[i])
		{
			while (i++ < clntCnt - 1)
			{
				clntSocks[i] = clntSocks[i + 1];
			}
			break;
		}
	}
	clntCnt--;
	ReleaseMutex(hMutex);
	closesocket(removesoket);
	return;
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}