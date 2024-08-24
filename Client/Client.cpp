#ifndef UNICODE
#define UNICODE
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <Windows.h>
#include <chrono>
#include <string>

using namespace std;
using namespace chrono;


// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
int cond = 0;

enum MsgTypes : char
{
	MSG_CLIENT_TO_SERVER_HELLO = 1,
	MSG_SERVER_TO_CLIENT_OK,
	MSG_PING,
	MSG_PONG
};

int initialize_win(int& iResult, WSADATA& wsaData)
{
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
}

int create_socket(SOCKET& SendSocket)
{
	SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SendSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
}

int close_socket(int& iResult, SOCKET SendSocket)
{
	wprintf(L"Finished sending. Closing socket.\n");
	iResult = closesocket(SendSocket);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"closesocket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	return 0;
}

void set_recv(sockaddr_in& RecvAddr, unsigned short Port)
{
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	//RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	RecvAddr.sin_addr.s_addr = inet_addr("3.216.94.65");
}

int send_dt(int& result, SOCKET& send_socket, char buffer[], int& len, sockaddr_in& recv)
{
	wprintf(L"Sending a datagram to the receiver...\n");
	result = sendto(send_socket,
		buffer, len, 0, (SOCKADDR*)&recv, sizeof(recv));
	if (result == SOCKET_ERROR) {
		wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
		closesocket(send_socket);
		WSACleanup();
		return 1;
	}
	else {

		if (buffer[0] == MSG_CLIENT_TO_SERVER_HELLO)
		{
			printf("-----------------------------------------------\n");
			printf("Am trimis server-ului un mesaj ""Hello\n""");
		}
		else
			if (buffer[0] == MSG_PONG)
			{
				printf("Am trimis server-ului un PONG\n");
				printf("-----------------------------------------------\n");
			}
	}
}

int recv_dt(int& result, SOCKET& recv_sock, char recv[], int& len, sockaddr_in& send, int& sender_addr)
{
	wprintf(L"Receiving datagrams...\n");
	result = recvfrom(recv_sock,
		recv, len, 0, (SOCKADDR*)&send, &sender_addr);

	if (result == SOCKET_ERROR) {
		int ierr = WSAGetLastError();
		if (ierr == WSAEWOULDBLOCK) {
			return 0;
		}
		wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
	{
		if (recv[0] == MSG_SERVER_TO_CLIENT_OK)
		{

			printf("Am primit de la server un mesaj ""You're connected""\n");
			printf("-----------------------------------------------\n");
			cond = 1;
		}
		else
			if (recv[0] == MSG_PING)
			{
				printf("-----------------------------------------------\n");
				printf("Am primit de la server un PING\n");
				recv[0] = MSG_PONG;
			}
	}
	return 0;
}

void update(int& result, SOCKET& send, char send_buff[], int& len, sockaddr_in& recv, bool& ok, bool& can_send, steady_clock::time_point& start, int& size)
{
	if (recv_dt(result, send, send_buff, len, recv, size) == 1 || send_dt(result, send, send_buff, len, recv) == 1)
		ok = false;
}
int main()
{

	int iResult;
	WSADATA wsaData;

	SOCKET SendSocket = INVALID_SOCKET;
	sockaddr_in RecvAddr;
	int size = sizeof(RecvAddr);

	unsigned short Port = 27015;

	char SendBuf[1024];
	SendBuf[0] = MSG_CLIENT_TO_SERVER_HELLO;
	int BufLen = 1024;


	if (initialize_win(iResult, wsaData) == 1)
		return 1;

	if (create_socket(SendSocket) == 1)
		return 1;

	set_recv(RecvAddr, Port);

	steady_clock::time_point start;
	bool ok = true;
	bool can_send = false;
	if (send_dt(iResult, SendSocket, SendBuf, BufLen, RecvAddr) == 1 ||
		recv_dt(iResult, SendSocket, SendBuf, BufLen, RecvAddr, size) == 1)
		goto error;
	if(cond)
	while (ok)
	{
		    cond = 0;
			update(iResult, SendSocket, SendBuf, BufLen, RecvAddr, ok, can_send, start, size);
			Sleep(100);
	}
error:

	if (close_socket(iResult, SendSocket) == 1)
		return 1;

	wprintf(L"Exiting.\n");
	WSACleanup();
	return 0;
}