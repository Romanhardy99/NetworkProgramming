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
#include<Messages.h>
using namespace std;

#pragma comment(lib, "WS2_32.lib")

#define MTU				 1500
#define MAX_CONNECTIONS		3

VOID ClientHandle(LPVOID lpParam);

// TODO #1: чтобы поток клиента знал, КТО прислал сообщение (IP:Port),
// в поток нужно передать не только сокет, но и адрес клиента.
// Функция потока принимает один параметр, поэтому "упаковываем" всё в структуру.
struct ClientContext
{
	SOCKET socket;
	CHAR   address[32];	// строка вида "127.0.0.1:54321"
};

SOCKET client_sockets[MAX_CONNECTIONS] = {};
DWORD dwThreadIDs[MAX_CONNECTIONS] = {}; //идентификаторы потоков
HANDLE hThreads[MAX_CONNECTIONS] = {}; //дескрипторы потоков

INT g_ActiveClients = 0;

// TODO #2 + #3: глобальный Mutex.
// Несколько потоков (по одному на клиента) рассылают сообщения во ВСЕ сокеты,
// значит один и тот же сокет может оказаться в send() из разных потоков одновременно.
// Mutex делает рассылку (и работу с массивом сокетов) критической секцией:
// в каждый момент времени send() в общий список выполняет только один поток.
HANDLE g_hMutex = NULL;

// Рассылка сообщения всем подключённым клиентам.
// Весь цикл send() защищён мьютексом => один сокет не пишется из разных потоков сразу.
VOID Broadcast(const CHAR* message, INT length)
{
	WaitForSingleObject(g_hMutex, INFINITE);	// вход в критическую секцию
	for (INT i = 0; i < MAX_CONNECTIONS; i++)
	{
		if (client_sockets[i] != 0 && client_sockets[i] != INVALID_SOCKET)
		{
			send(client_sockets[i], message, length, 0);
		}
	}
	ReleaseMutex(g_hMutex);						// выход из критической секции
}

void main()
{
	setlocale(LC_ALL, "");
	DWORD dwError = 0;
	CHAR szError[256] = {};
	cout << "SERVER" << endl;

	// TODO #3: создаём Mutex (изначально свободен).
	g_hMutex = CreateMutex(NULL, FALSE, NULL);
	if (g_hMutex == NULL)
	{
		cout << "CreateMutex failed with error: " << GetLastError() << endl;
		return;
	}

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
	if (listen(listen_socket, MAX_CONNECTIONS) == SOCKET_ERROR)
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
		SOCKET client_socket = accept(listen_socket, (SOCKADDR*)&client_address, &client_address_len);
		if (client_socket == INVALID_SOCKET)
		{
			cout << "Accept failed with error: " << WSAGetLastError() << endl;
			closesocket(listen_socket);
			freeaddrinfo(target);
			WSACleanup();
			return;
		}
		CHAR sz_client_address[32];
		inet_ntop(AF_INET, &client_address.sin_addr, sz_client_address, 32);
		USHORT client_port = ntohs(client_address.sin_port);
		cout << sz_client_address << ":" << client_port << " connected" << endl;

		//7) Ищем свободный слот и регистрируем клиента (под мьютексом,
		//   т.к. массив client_sockets[] параллельно читает Broadcast() из других потоков).
		INT slot = -1;
		WaitForSingleObject(g_hMutex, INFINITE);
		for (INT i = 0; i < MAX_CONNECTIONS; i++)
		{
			if (client_sockets[i] == 0)
			{
				slot = i;
				client_sockets[i] = client_socket;	// сохраняем сокет клиента
				g_ActiveClients++;
				break;
			}
		}
		ReleaseMutex(g_hMutex);

		if (slot != -1)
		{
			// TODO #1: упаковываем сокет + "IP:Port" и передаём в поток.
			ClientContext* ctx = new ClientContext();
			ctx->socket = client_socket;
			sprintf(ctx->address, "%s:%d", sz_client_address, client_port);

			hThreads[slot] = CreateThread
			(
				NULL,	//атрибут безопасности
				0,		//размер стека (0 - по умолчанию)
				(LPTHREAD_START_ROUTINE)ClientHandle,	//функция потока
				(LPVOID)ctx,							//параметр: указатель на ClientContext
				0,
				&dwThreadIDs[slot]
			);
		}
		else
		{
			// мест нет - вежливо отказываем
			iResult = send(client_socket, DECLINE_MESSAGE, strlen(DECLINE_MESSAGE), 0);
			dwError = WSAGetLastError();
			if (iResult == SOCKET_ERROR) cout << FormatLastError(dwError, szError) << endl;
			shutdown(client_socket, SD_BOTH);
			closesocket(client_socket);
			cout << "DECLINE" << endl;
		}
	} while (true);

	//9) Освобождаем ресурсы (код ниже недостижим из-за while(true),
	//   оставлен как пример корректного завершения)
	closesocket(listen_socket);
	freeaddrinfo(target);
	CloseHandle(g_hMutex);
	WSACleanup();
}

VOID ClientHandle(LPVOID lpParam)
{
	// TODO #1: достаём сокет и адрес отправителя из переданной структуры.
	ClientContext* ctx = (ClientContext*)lpParam;
	SOCKET client_socket = ctx->socket;

	CHAR recv_buffer[MTU] = {};
	CHAR out_buffer[MTU + 64] = {};	// "IP:Port : сообщение"
	INT iReceivedBytes = 0;

	do
	{
		ZeroMemory(recv_buffer, MTU);
		iReceivedBytes = recv(client_socket, recv_buffer, MTU, 0);
		if (iReceivedBytes > 0)
		{
			// на сервере выводим, кто и что прислал
			cout << ctx->address << " : " << recv_buffer << endl;

			// TODO #1: добавляем к сообщению отправителя в виде IP:Port
			INT length = sprintf(out_buffer, "%s : %s", ctx->address, recv_buffer);

			// TODO #2/#3: рассылаем всем; send() сериализован мьютексом внутри Broadcast()
			Broadcast(out_buffer, length);
		}
		else if (iReceivedBytes == 0)
		{
			cout << ctx->address << " disconnected" << endl;
		}
		else
		{
			cout << "Receive failed with error: " << WSAGetLastError() << endl;
		}
	} while (iReceivedBytes > 0);

	//8) Клиент отключился - убираем его сокет из массива (под мьютексом),
	//   чтобы Broadcast() в него больше не писал.
	WaitForSingleObject(g_hMutex, INFINITE);
	for (INT i = 0; i < MAX_CONNECTIONS; i++)
	{
		if (client_sockets[i] == client_socket)
		{
			client_sockets[i] = 0;
		}
	}
	if (g_ActiveClients > 0) g_ActiveClients--;
	ReleaseMutex(g_hMutex);

	shutdown(client_socket, SD_BOTH);
	closesocket(client_socket);
	delete ctx;	// освобождаем память, выделенную под контекст в main()
}