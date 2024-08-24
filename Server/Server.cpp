#ifndef UNICODE
#define UNICODE
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <Windows.h>
#include <chrono>
#include <vector>

using namespace std;
using namespace chrono;

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
int cond = 0;
int size_vec = 0;
int first_time = 1;
int var = 0;
steady_clock::time_point start_delay;
steady_clock::time_point end_delay;

enum MsgTypes : char
{
	MSG_CLIENT_TO_SERVER_HELLO = 1,
	MSG_SERVER_TO_CLIENT_OK,
	MSG_PING,
	MSG_PONG
};

class Client
{
private:
	sockaddr_in clientAddr;
	int index;
	bool is_connected;
public:
	sockaddr_in getAddr();
	void setAddr(sockaddr_in clientAddr);
	void setIndex(int index);
	int getIndex();
	void setConnect(bool is_connected);
	bool getConnect();
};

sockaddr_in Client::getAddr()
{
	return this->clientAddr;
}

void Client::setAddr(sockaddr_in clientAddr)
{
	this->clientAddr = clientAddr;
}

int Client::getIndex()
{
	return this->index;
}

void Client::setIndex(int index)
{
	this->index = index;
}

void Client::setConnect(bool is_connected)
{
	this->is_connected = is_connected;
}

bool Client::getConnect()
{
	return this->is_connected;
}

int initialize_win(int& iResult, WSADATA& wsaData)
{
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error %d\n", iResult);
		return 1;
	}
	return 0;
}

int create_recv_sock(SOCKET& RecvSocket)
{
	RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	u_long mode = 1;
	ioctlsocket(RecvSocket, FIONBIO, &mode);
	if (RecvSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

void create_addr(sockaddr_in& RecvAddr, int Port)
{
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
}

int make_bind(int& iResult, sockaddr_in RecvAddr, SOCKET RecvSocket)
{
	iResult = bind(RecvSocket, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
	if (iResult != 0) {
		wprintf(L"bind failed with error %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

int close_sock(int& iResult, SOCKET RecvSocket)
{
	wprintf(L"Finished receiving. Closing socket.\n");
	iResult = closesocket(RecvSocket);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"closesocket failed with error %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}


int recv_dt(int& result, SOCKET& recv_sock, char recv[], int& len, sockaddr_in& send, int& sender_addr, vector<Client>& clients, int& iter)
{
		result = recvfrom(recv_sock,
			recv, len, 0, (SOCKADDR*)&send, &sender_addr);
			if (result == SOCKET_ERROR) {
				int ierr = WSAGetLastError();
				if (ierr == WSAEWOULDBLOCK || ierr == 10054) {
					if (ierr == 10054)
					{
						if (clients.size() > 1)
						{
							first_time = 0;
							goto disc;
						}
						if (first_time)
						{
							disc:
							clients.back().setConnect(false);
							clients.pop_back();
						}
					}
					return 0;
				}

				wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
				return 1;
			}
		else
		{
			if (recv[0] == MSG_CLIENT_TO_SERVER_HELLO)
			{
				Client client;
				client.setAddr(send);
				client.setConnect(false);
				size_vec++;
				clients.push_back(client);
				printf("-----------------------------------------------\n");
				printf("Am primit de la clientul cu index-ul %d si adresa %s un mesaj ""Hello""\n", clients.back().getIndex(),inet_ntoa((in_addr)(clients.back().getAddr().sin_addr)));
				recv[0] = MSG_SERVER_TO_CLIENT_OK;
				cond = 1;
			}
			else
				if (recv[0] == MSG_PONG)
				{
					printf("Am primit de la clientul cu adresa %s un PONG\n", inet_ntoa((in_addr)(clients[var].getAddr().sin_addr)));
					recv[0] = MSG_PING;
					end_delay = high_resolution_clock::now();
					printf("Delay-ul este: %d ms\n", (int)duration_cast<milliseconds>(end_delay - start_delay).count());
					printf("-----------------------------------------------\n");
					var++;
				}
	}
	return 0;
}

int send_dt(int& result, SOCKET& send_socket, char buffer[], int& len, sockaddr_in recv, steady_clock::time_point& start, int i, vector<Client>& clients)
{
	result = sendto(send_socket,
		buffer, len, 0, (SOCKADDR*)&recv, sizeof(recv));
	var = 0;
	if (result == SOCKET_ERROR) {
		wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
		closesocket(send_socket);
		WSACleanup();
		return 1;
	}
	else {
		start = high_resolution_clock::now();
		if (buffer[0] == MSG_SERVER_TO_CLIENT_OK && clients[i].getConnect() == false)
		{
			printf("Am trimis clientului cu index-ul %d si adresa %s un mesaj ""You're connected\n""",clients[i].getIndex(), inet_ntoa((in_addr)(clients[i].getAddr().sin_addr)));
			printf("-----------------------------------------------\n");
			clients[i].setConnect(true);
		}
		else
			if (buffer[0] == MSG_PING && clients[i].getConnect() == true)
			{
				printf("Am trimis clientului cu index-ul %d si adresa %s un PING\n",clients[i].getIndex(), inet_ntoa((in_addr)(clients[i].getAddr().sin_addr)));
				start_delay = high_resolution_clock::now();
			}
		buffer[0] = MSG_PING;
	}
	return 0;
}

void update(int& result, SOCKET& recv, char buffer[], int& len, sockaddr_in& sender, int& sender_addr, bool& ok, bool& can_send, int& iter, vector<Client>& clients, steady_clock::time_point& start)
{
		if (recv_dt(result, recv, buffer, len, sender, sender_addr, clients, iter) == 1)
			ok = false;
		if (cond == 1)
		{
			
			if (can_send)
			{
				for (int i = clients.size() -1 ; i >= 0; i--)
				{
					if (send_dt(result, recv, buffer, len, clients[i].getAddr(), start, i, clients) == 1)
						ok = false;
					else
						can_send = false;
				}
			}
			else
			{
				auto end = high_resolution_clock::now();
				if (duration_cast<seconds>(end - start).count() >= 3)
					can_send = true;
			}
		}
}

int main()
{

	int iResult = 0;

	WSADATA wsaData;

	SOCKET RecvSocket;
	struct sockaddr_in RecvAddr;

	unsigned short Port = 27015;

	char RecvBuf[1024];
	int BufLen = 1024;

	struct sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);

	int counter = 0;
	vector<Client> clients;

	if (initialize_win(iResult, wsaData) == 1)
		return 1;

	if (create_recv_sock(RecvSocket) == 1)
		return 1;

	create_addr(RecvAddr, Port);

	if (make_bind(iResult, RecvAddr, RecvSocket) == 1)
		return 1;

	bool ok = true;
	bool can_send = true;
	steady_clock::time_point start;
	while (ok)
	{
		update(iResult, RecvSocket, RecvBuf, BufLen, SenderAddr, SenderAddrSize, ok, can_send, counter, clients, start);
		Sleep(100);

	}
	if (close_sock(iResult, RecvSocket) == 1)
		return 1;

	wprintf(L"Exiting.\n");
	WSACleanup();
	return 0;
}