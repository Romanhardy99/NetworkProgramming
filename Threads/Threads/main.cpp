#include<iostream>
#include<Windows.h>
using namespace std;

VOID Function()
{
	while (true)
	{
		cout << "Hello Threads from " << GetCurrentThreadId() << endl;
		system("PAUSE");
	}
}

void main()
{
	setlocale(LC_ALL, "");
	DWORD dwID = 0;
	HANDLE hThread = CreateThread
	(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)Function,
		NULL,
		NULL,
		&dwID
	);
	cout << "Thread ID from main(): " << dwID << endl;
	WaitForSingleObject(hThread, INFINITE);
}