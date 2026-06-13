#include<iostream>
#include<Windows.h>
#include<thread>
#include<chrono>
using std::cin;
using std::cout;
using std::endl;
using namespace std::chrono_literals;

bool finish = false;


VOID Function()
{
	while (!finish)
	{
		cout << "Hello Threads from " << GetCurrentThreadId() << endl;
		//system("PAUSE");
	}
}

struct Point
{
	Point(int x, int y) :x(x), y(y) {}
	int x;
	int y;
};

VOID Collision(Point* point)
{
	while (point->x != point->y)
	{
		cout << "X = " << point->x++ << "\t = " << point->y-- << endl;
		Sleep(10);
	}
}

VOID Decrement(int* i)
{
	while (i)cout << i-- << "\t";
}

void Plus()
{
	while (!finish)
	{
		cout << "+ ";
		std::this_thread::sleep_for(100ms);
	}
}
void Minus()
{
	while (!finish)
	{
		cout << "- ";
		std::this_thread::sleep_for(100ms);
	}
}

//#define WINDOWS_THREADS_1
//#define WINDOWS_THREADS_2

void main()
{
	setlocale(LC_ALL, "");
#ifdef WINDOWS_THREADS_1
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
	cin.get();
	finish = true;
	cout << "Thread ID from main(): " << dwID << endl;
	WaitForSingleObject(hThread, INFINITE);
#endif // WINDOWS_THREADS_1

#ifdef WINDOWS_THREADS_2
	Point A(0, 100000);
	int i = 10000;
	DWORD dwThreadID = 0;
	HANDLE hThread = CreateThread
	(
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)Decrement,
		(LPVOID)i, //lpvoid - long pointer to void. void-pointer может хранить указатель на абсолютно любой тип данных.
		NULL,
		&dwThreadID
	);
	WaitForSingleObject(hThread, INFINITE);
#endif // WINDOWS_THREADS_2

	//Plus();
	//Minus();

	std::thread plus_thread = std::thread(Plus);
	std::thread minus_thread = std::thread(Minus);

	cin.get();
	finish = true;

	if(plus_thread.joinable())plus_thread.join();
	if(minus_thread.joinable())minus_thread.join();
}