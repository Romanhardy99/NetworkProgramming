
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
//Если с библиотекой <WinSOCK2.h> подключается файл <Windows.h> или <IPhlAPI>,
//то они тоже подключают файл <WinSOCK2.h>, что приводит к конфликтам.
// Для того чтобы <Windows.h> и <IPhlpAPI.h> не подтягивали WinSOCK, создается макроопределение.
#endif // !WIN32_LEAN_AND_MEAN

#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>
#include<FormatLastError.h>
#include<Messages.h>
#pragma comment(lib, "WS2_32.lib") //Встраиваем статическую библиотеку, для заголовка <WS2_32.lib>

#define MTU 1500 //Maximum transfer unit - максимально возможный размер интернет-кадра

void main()
{
	setlocale(LC_ALL, "Russian");
	std::cout << "CLIENT" << std::endl;
	DWORD dwError = 0;
	CHAR szError[256] = {};

	//1) Инициализация WinSOCK:
	WSAData wsaData;
	int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WinSOCK init failed with code: " << iResult;
		return;
	}

	// 2) Определяем параметры подключения:
	addrinfo hints;
	addrinfo* target;
	ZeroMemory(&hints, sizeof(hints)); //Обнуляем экземпляр структуры
	hints.ai_family = AF_INET; //Стек протоколов TCP/IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP; //Определяем протокол транспортного уровня
	iResult = getaddrinfo("127.0.0.1", "27015", &hints, &target);
	if (iResult != 0)
	{
		std::cout << "getaddressinfo() failed with code " << iResult << std::endl;
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
		std::cout << "SOCKET creation failed with error:\t" << WSAGetLastError() << std::endl;
		std::cout << FormatLastError(dwError, szError) << std::endl;
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
		//std::cout << "Error " << dwError << ":\t";
		std::cout << FormatLastError(dwError, szError) << std::endl;

		//WSAGetLastError в обязательном порядке должна быть вызвана непосредственно 
		//после вызова функции, которая потенциально может выполнится с ошибкой.
		std::cout << "Unable to connect to server" << std::endl;
		closesocket(connect_socket);
		//freeaddrinfo(target);
		WSACleanup();
		return;
	}
	//freeaddrinfo(target);

	//5) Отправка:
		CHAR send_buffer[MTU] = "Privet Server";
	do
	{
		iResult = send(connect_socket, send_buffer, strlen(send_buffer), 0);
		dwError = WSAGetLastError();
		if (iResult == SOCKET_ERROR)
		{
			std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
			std::cout << FormatLastError(dwError, szError) << std::endl;
			closesocket(connect_socket);
			WSACleanup();
			return;
		}

		//6) Получение данных:
		CHAR recv_buffer[MTU] = {};
		//do
		{
			iResult = recv(connect_socket, recv_buffer, MTU, 0);
			dwError = WSAGetLastError();
			if (iResult > 0)
				std::cout << "Bytes received: " << iResult << "Message: " << recv_buffer << std::endl;
			else if (iResult == 0) std::cout << "Connection closed" << std::endl;
			else std::cout << "Receive failed with error " << FormatLastError(dwError, szError) << std::endl;

		}// while (iResult > 0);
		ZeroMemory(send_buffer, MTU);
		//ZeroMemory(recv_buffer, MTU);
		if (strcmp(recv_buffer, DECLINE_MESSAGE) == 0) std::cout << "Введите сообщение: ";
		else std::cout << "Для входа нажмите 'Enter: " << std::endl;
		SetConsoleCP(1251);
		std::cin.getline(send_buffer, MTU);
		SetConsoleCP(866);
	} while (strcmp(send_buffer, "exit") != 0);

	iResult = shutdown(connect_socket, SD_BOTH);//Закрываем сокет на получение и отправку данных (разрываем TCP-соединение):
	if (iResult == SOCKET_ERROR)
		std::cout << "Shutdown failed with " << FormatLastError(dwError, szError) << std::endl;

	// ?) Освобождаем ресурсы WinSOCK
	closesocket(connect_socket);
	WSACleanup();
}