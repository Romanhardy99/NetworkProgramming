//Server
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
#define MTU 1500

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
	SOCKET client_socket = accept(listen_socket, NULL, NULL);
	if (client_socket == INVALID_SOCKET)
	{
		cout << "Accept failed with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(target);
		WSACleanup();
		return;
	}
	//7) Получем данные от клиента:
	CHAR recv_buffer[MTU] = {};
	CHAR send_buffer[MTU] = "Hello client";
	INT iReceivedBytes = 0;
	INT iSentBytes = 0;
	do
	{
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
	if (iResult == SOCKET_ERROR)
	{
		cout << "shutdown failed with error:\t" << WSAGetLastError();
	}

	//?) Освобождаем ресурсы, занятые WinSOCK
	closesocket(listen_socket);
	freeaddrinfo(target);
	WSACleanup();
}