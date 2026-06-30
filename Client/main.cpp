﻿
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
//Если с библиотекой <WinSOCK2.h> подключается файл <Windows.h> или <IPhlpAPI>,
//то они тоже подключают файл <WinSOCK2.h>, что приводит к конфликтам.
//Для того чтобы <Windows.h> и <IPhlpAPI.h> не подтягивали WinSOCK, создается макроопределение.
#endif // !WIN32_LEAN_AND_MEAN


#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>
#include<FormatLastError.h>
#include<Messages.h>
using namespace std;

#pragma comment(lib, "WS2_32.lib")	//Встраиваем статическую библиотеку, для заголовка <WS2TCPIP.h>
#pragma comment(lib, "FormatLastError.lib")

#define MTU	1500	//Maximum Transfer Unit - Максимально-возможный размер Ethernet-кадра

CHAR send_buffer[MTU] = "Привет Server";
CHAR recv_buffer[MTU] = {};

VOID Receive(SOCKET connect_socket);

void main()
{
	setlocale(LC_ALL, "");
	cout << "CLIENT" << endl;
	DWORD dwError = 0;
	CHAR szError[256] = {};

	//1) Инициализация WinSOCK:
	WSAData wsaData;
	int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WinSOCK init failed with code: " << iResult;
		return;
	}

	//2) Определяем параметры подключения:
	addrinfo  hints;
	addrinfo* target;
	ZeroMemory(&hints, sizeof(hints));	//Обнуляем экземпляр стуктуры
	hints.ai_family = AF_INET;			//Стек протоколов TCP/IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;	//Определяем протокол транспортного уровня
	iResult = getaddrinfo("127.0.0.1", "27015", &hints, &target);
	if (iResult != 0)
	{
		cout << "getaddressinfo() failed with code " << iResult << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//3) Создаем сокет:
	//SOCKET - тип данных;
	//socket() - это функция;
	SOCKET connect_socket = socket(target->ai_family, target->ai_socktype, target->ai_protocol);
	dwError = WSAGetLastError();
	if (connect_socket == INVALID_SOCKET)
	{
		cout << "SOCKET creation failed with error:\t" << dwError << endl;
		cout << FormatLastError(dwError, szError) << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//4) Подключаемся к узлу:
	iResult = connect(connect_socket, target->ai_addr, target->ai_addrlen);
	dwError = WSAGetLastError();
	freeaddrinfo(target);
	if (iResult == SOCKET_ERROR)
	{
		//cout << "Error " << dwError << ":\t";
		cout << FormatLastError(dwError, szError) << endl;
		//cout << lpError << endl;

		//	WSAGetLastError() в обязатенльном порядке должна быть вызвана непосредственно 
		//	после вывоза функции, которая потенциально может выполниться с ошибкой.
		cout << "Unable to connect to server" << endl;
		closesocket(connect_socket);
		//freeaddrinfo(target);
		WSACleanup();
		return;
	}
	//freeaddrinfo(target);

	//5) Отправка:
	//CHAR send_buffer[MTU] = "Привет Server";
	//CHAR recv_buffer[MTU] = {};
	DWORD dwThreadID = 0;
	HANDLE hReceive = CreateThread
	(
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)Receive,
		(LPVOID)connect_socket,
		NULL,
		&dwThreadID
	);
	do
	{
		iResult = send(connect_socket, send_buffer, strlen(send_buffer), 0);
		dwError = WSAGetLastError();
		if (iResult == SOCKET_ERROR)
		{
			cout << "Send failed with error: " << WSAGetLastError() << endl;
			cout << FormatLastError(dwError, szError) << endl;
			closesocket(connect_socket);
			WSACleanup();
			return;
		}

		//6) Получение данных:

		ZeroMemory(send_buffer, MTU);
		if (strcmp(recv_buffer, DECLINE_MESSAGE) != 0)	cout << "Введите сообщение: ";
		else cout << "Для выхода нажмите 'Enter'" << endl;
		SetConsoleCP(1251);
		cin.getline(send_buffer, MTU);
		SetConsoleCP(866);
	} while (strcmp(send_buffer, "exit") != 0 && strcmp(recv_buffer, DECLINE_MESSAGE) != 0);//https://legacy.cplusplus.com/reference/cstring/strcmp/

	iResult = shutdown(connect_socket, SD_BOTH);//Закрываем сокет на получение и отправку данных (разрываем TCP-соединение):
	if (iResult == SOCKET_ERROR)
		cout << "Shutdown failed with " << FormatLastError(WSAGetLastError(), szError) << endl;
	//7) Освобождаем ресурсы WinSOCK:
	closesocket(connect_socket);
	WSACleanup();
}
VOID Receive(SOCKET connect_socket)
{
	INT iResult = 0;
	DWORD dwError = 0;
	CHAR szError[256] = {};
	ZeroMemory(recv_buffer, MTU);
	do
	{
		iResult = recv(connect_socket, recv_buffer, MTU, 0);
		dwError = WSAGetLastError();
		if (iResult > 0)
			cout << "Bytes received: " << iResult << "Message: " << recv_buffer << endl;
		else if (iResult == 0)cout << "Connection closed" << endl;
		else cout << "Receive failed with " << FormatLastError(dwError, szError) << endl;

	} while (iResult > 0);
}