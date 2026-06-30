#include<Windows.h>
#include<iostream>
#include<thread>//Конкурентное выполнение
#include<mutex>	//Mutual execution - взаимное выполнение
#include<chrono>
using std::cin;
using std::cout;
using std::endl;
using namespace std::chrono_literals;

bool finish = false;
std::mutex mtx;
HANDLE ghMutex = NULL;

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
		cout << "X = " << point->x++ << "\tY = " << point->y-- << endl;
		//Sleep(5);
	}
}
VOID Decrement(int i)
{
	while (i)cout << i-- << "\t";
}

void Plus()
{
	while (!finish)
	{
		//mtx.lock();
		WaitForSingleObject(ghMutex, INFINITE);
		cout << "+ ";
		Sleep(10);
		ReleaseMutex(ghMutex);
		//std::this_thread::sleep_for(100ms);
		//mtx.unlock();
	}
}
void Minus()
{
	while (!finish)
	{
		//mtx.lock();
		WaitForSingleObject(ghMutex, INFINITE);
		cout << "- ";
		Sleep(10);
		ReleaseMutex(ghMutex);
		//std::this_thread::sleep_for(100ms);
		//mtx.unlock();
	}
}

//#define WINDOWS_THREADS_1
//#define WINDOWS_THREADS_2
//#define CPP_THREADS

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
	Point A(0, 10000);
	int i = 10000;
	DWORD dwThreadID = 0;
	HANDLE hThread = CreateThread
	(
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)Decrement,
		(LPVOID)i,	//LPVOID - LongPointer to VOID.
		//VOID-pointer может хранить указатель на абсолютно любой тип данных;
		NULL,
		&dwThreadID
	);
	WaitForSingleObject(hThread, INFINITE);
#endif // WINDOWS_THREADS_2

#ifdef CPP_THREADS
	//Plus();
	//Minus();

	std::thread plus_thread = std::thread(Plus);
	std::thread minus_thread = std::thread(Minus);

	cout << "Start" << endl;
	cin.get();
	finish = true;
	cout << "Finish" << endl;

	if (plus_thread.joinable())plus_thread.join();
	if (minus_thread.joinable())minus_thread.join();

	/*while (true)
	{
		cout << std::this_thread::get_id() << "\t";
		std::this_thread::sleep_for(100ms);
		Sleep(100);
	}*/
#endif // CPP_THREADS

	ghMutex = CreateMutex(NULL, FALSE, NULL);
	HANDLE hThreads[2] = {};
	hThreads[0] = CreateThread
	(
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)Plus,
		NULL,
		NULL,
		0
	);
	hThreads[1] = CreateThread
	(
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)Minus,
		NULL,
		NULL,
		0
	);
	WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);
}