//Server
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>
#include"FormatLastError.h"
using namespace std;

#pragma comment(lib, "WS2_32.lib")

#define MTU				 1500
#define MAX_CONNECTIONS		3

VOID ClientHandle(SOCKET client_socket);

SOCKET client_sockets[MAX_CONNECTIONS] = {};
DWORD dwThreadIDs[MAX_CONNECTIONS] = {}; //идентификаторы потоков 
HANDLE hThreads[MAX_CONNECTIONS] = {}; //дескрипторы потоков

INT g_ActiveClients = 0;

void main()
{
	setlocale(LC_ALL, "");
	cout << "SERVER" << endl;
	//1)Инициализация WinSOCK:
	WSADATA wsaData;
	INT iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup failed with error: " << iResult << endl;
		return;
	}

	//2) Параметры подключения:
	addrinfo hints;
	addrinfo* target;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; //Соединение будет работать в режиме 'LISTENING';

	iResult = getaddrinfo(NULL, "27015", &hints, &target);
	if (iResult != 0)
	{
		cout << "getaddrinfo() failed with error: " << iResult << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//Создание серверного сокета, который он будет постоянно прослушивать: 
	SOCKET listen_socket = socket(target->ai_family, target->ai_socktype, target->ai_protocol);
	if (listen_socket == INVALID_SOCKET)
	{
		cout << "SOCKET creation failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//4)Привязывает сокет к интерфейсу и порту: 
	iResult = bind(listen_socket, target->ai_addr, target->ai_addrlen);
	if (iResult != 0)
	{
		cout << "bind failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(target);
		closesocket(listen_socket);
		WSACleanup();
		return;
	}

	//5)Запускание прослушивание порта:
	if (listen(listen_socket, 1) == SOCKET_ERROR) // 1 - Максимальное кол-во одновременно подключенных клиентов
	{
		cout << "Listen failed with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//6) Принимаем подключение от клиента
	do
	{
		SOCKADDR_IN client_address;
		INT client_address_len = sizeof(client_address);
		SOCKET client_socket = accept(listen_socket, (SOCKADDR*) & client_address, &client_address_len);
		if (client_socket == INVALID_SOCKET)
		{
			cout << "Accept failed with error: " << WSAGetLastError() << endl;
			closesocket(listen_socket);
			freeaddrinfo(target);
			WSACleanup();
			return;
		}
		CHAR sz_client_address[32];
		cout << inet_ntop(AF_INET, &client_address.sin_addr, sz_client_address, 32) << ":" << ntohs(client_address.sin_port) << endl;
		
		//Получаем данные от клиента
		//ClientHandle(client_socket);
		client_sockets[g_ActiveClients] = client_socket;
		hThreads[g_ActiveClients] = CreateThread
		(
			NULL, 
			0,
			(LPTHREAD_START_ROUTINE)ClientHandle,
			(LPVOID)client_socket,
			NULL,
			&dwThreadIDs[g_ActiveClients]
		);
		g_ActiveClients++;
	} while (true);

	WaitForMultipleObjects(g_ActiveClients, hThreads, TRUE, INFINITE);

	//9) Освобождаем ресурсы, занятые WinSOCK
	closesocket(listen_socket);
	freeaddrinfo(target);
	WSACleanup();
}

VOID ClientHandle(SOCKET client_socket)
{
	INT iResult = 0;
	DWORD dwError = 0;
	CHAR szError[256] = {};
	//7) Получем данные от клиента:
	CHAR send_buffer[MTU] = "Hello client";
	INT iReceivedBytes = 0;
	INT iSentBytes = 0;
	do
	{
		CHAR recv_buffer[MTU] = {};
		iReceivedBytes = recv(client_socket, recv_buffer, MTU, 0);
		//Функция recv() - Receive ожидает получения данных по указаному сокету, И возвращает количество полученных байт.
		if (iReceivedBytes > 0)
		{
			cout << "Received " << iReceivedBytes << " " << recv_buffer << endl;
			iSentBytes = send(client_socket, send_buffer, strlen(send_buffer), 0);
			if (iSentBytes == SOCKET_ERROR)
			{
				cout << "Send failed with error:\t" << WSAGetLastError() << endl;
			}
			else
			{
				cout << iSentBytes << "Bytes sent " << endl;
			}
		}
		else if (iReceivedBytes == 0)
		{
			cout << "Connection closing...." << endl;
		}
		else
		{
			cout << "Receive failed with error: " << WSAGetLastError() << endl;
		}
	} while (iReceivedBytes > 0);

	//8) Разрываем TCP - соединение: 
	iResult = shutdown(client_socket, SD_BOTH);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)
	{
		cout << "shutdown failed with error:\t" << FormatLastError(dwError, szError);
	}
}