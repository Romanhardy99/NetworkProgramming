
#include<Windows.h>
#include<iostream>
#include<conio.h>
#include<chrono>
#include<thread>
using namespace std::chrono_literals;
using std::cin;
using std::cout;
using std::endl;
#define Escape	27
#define Enter	13

#define MIN_TANK_CAPACITY	20
#define MAX_TANK_CAPACITY	120
class Tank
{
	const int CAPACITY;
	double fuel_level;
public:
	Tank(int capacity):
		CAPACITY
		(
			capacity < MIN_TANK_CAPACITY ? MIN_TANK_CAPACITY :
			capacity > MAX_TANK_CAPACITY ? MAX_TANK_CAPACITY :
			capacity
		)
	{

		//this->CAPACITY = capacity;
		this->fuel_level = 0;
		cout << "Tank  is ready " << this << endl;
	}
	~Tank()
	{
		cout << "Tank is over " << this << endl;
	}
	double get_fuel_level()const
	{
		return fuel_level;
	}
	void fill(double amount)
	{
		if (amount < 0) return;
		fuel_level += amount;
		if (fuel_level > CAPACITY)fuel_level = CAPACITY;
	}
	double give_fuel(double amount)
	{
		if (amount < 0) return fuel_level;
		fuel_level -= amount;
		if (fuel_level < 0) fuel_level = 0;
		return fuel_level;
	}
	void info()const
	{
		cout << "Capacity:\t" << CAPACITY << " liters.\n";
		cout << "Fuel level:\t" << fuel_level << " leters.\n";
	}
};

#define MIN_ENGINE_CONSUMPTION	4
#define	MAX_ENGINE_CONSUMPTION	30

class Engine
{
	const double CONSUMPTION; //расход на 100км
	double consumption_per_second; //расход за 1 секунду.
	bool is_started;
public:
	Engine(double consumption) :CONSUMPTION
	(
		consumption < MIN_ENGINE_CONSUMPTION ? MIN_ENGINE_CONSUMPTION :
		consumption > MAX_ENGINE_CONSUMPTION ? MAX_ENGINE_CONSUMPTION :
		consumption
	)
	{
		consumption_per_second = CONSUMPTION * 3e-5;
		is_started = false;
		cout << "Engine is ready:\t\t" << this << endl;
	}
	~Engine()
	{
		cout << "Engine is over:\t" << this << endl;
	}
	double get_consumption_per_second()
	{
		return consumption_per_second;
	}
	void start()
	{
		is_started = true;
	}

	void stop()
	{
		is_started = false;
	}

	bool started()const
	{
		return is_started;
	}

	void info()const
	{
		cout << "Consumption:\t\t\t" << CONSUMPTION << " literals/km.\n";
		cout << "Consumption per sec:\t\t" << consumption_per_second << " liters/sec.\n";
	}
		
};
class Car
{
	Engine engine;
	Tank tank;
	bool driver_inside;
	struct 
	{
		std::thread panel_thread;
		std::thread engine_edle_thread;
	}car_threads;
public:
	Car(double consumption, int capacity = 50):engine(consumption),tank(capacity)
	{
		driver_inside = false;
		cout << "Your car is ready to go, press Enter to get in\t" << this << endl;
	}
	~Car()
	{
		cout << "Car is over:\t\t\t\t\t " << this << endl;
	}
	void panel()
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hConsole, &csbi);

		WORD oldAttributes = csbi.wAttributes;

		CONSOLE_CURSOR_INFO cursorInfo;
		GetConsoleCursorInfo(hConsole, &cursorInfo);
		bool cursorWasVisible = cursorInfo.bVisible;
		cursorInfo.bVisible = FALSE;
		SetConsoleCursorInfo(hConsole, &cursorInfo);

		while (driver_inside)
		{
			GetConsoleScreenBufferInfo(hConsole, &csbi);

			COORD coord = { 0,0 };
			SetConsoleCursorPosition(hConsole, coord);

			
			std::string output = "Fuel level: " + std::to_string(tank.get_fuel_level()) + " liters.\t";
			
			if (tank.get_fuel_level() < 5)
			{
				SetConsoleTextAttribute(hConsole, 0x4F);
				output += "LOW FUEL";
				SetConsoleTextAttribute(hConsole, oldAttributes);
			}
			output += "\nEngine is " 
				+ std::string(engine.started() ? "started" : "stopped") + " \n";
			DWORD written;
			WriteConsoleA(hConsole, output.c_str(), output.length(), &written, NULL);

			std::this_thread::sleep_for(100ms);
		}
		cursorInfo.bVisible = cursorWasVisible; 
		SetConsoleCursorInfo(hConsole, &cursorInfo);

	}
	void get_in()
	{
		driver_inside = true;
		//panel();
		if (!car_threads.panel_thread.joinable())
			car_threads.panel_thread = std::thread(&Car::panel, this);
	}
	void get_out()
	{
		driver_inside = false;
		if (car_threads.panel_thread.joinable())
			car_threads.panel_thread.join();
		system("CLS");
		cout << "You are out of the car" << endl;
	}
	void startup()
	{
		if (tank.give_fuel(0))
		{
			engine.start();
			if(!car_threads.engine_edle_thread.joinable())
				car_threads.engine_edle_thread = std::thread(&Car::engine_idle, this);
		}
	}
	void shutdown()
	{
		engine.stop();
		if (car_threads.engine_edle_thread.joinable())
			car_threads.engine_edle_thread.join();
	}
	void control()
	{
		char key;
		do
		{
			key = 0; 
			if(_kbhit())key = _getche(); // Функция _getch() ожидает нажатие клавиши и возвращает ASCI-код нажатой клавиши
			switch (key)
			{
				case Enter:
				{
					if (driver_inside) get_out();
					else get_in();
					break;
				case 'F':
				case 'f':
					if (!driver_inside && !engine.started())
					{
						double amount;
						cout << "Введите объём топлива: "; cin >> amount;
						tank.fill(amount);
					}
					else cout << "Нужно заглушить двигатель и выйти из машины, у нас только самообслуживание" << endl;
					break;
				}
				case 'I':
				case 'i':
					if (driver_inside && !engine.started())startup();
					else if(driver_inside)shutdown();
					break;
				case Escape:
					shutdown();
					get_out();
			}
			if (tank.get_fuel_level() == 0 && engine.started())shutdown();
		} while (key != Escape);
	}

	void engine_idle()
	{
		while (engine.started() && tank.give_fuel(engine.get_consumption_per_second()))
			std::this_thread::sleep_for(1s);
	}
};

//#define TANK_CHECK
#define ENGINE_CHECK
void main()
{
	setlocale(LC_ALL, "");
#ifdef TANK_CHECK
	Tank tank(40);
	int amount;
	while (true)
	{
		cout << "Введите объем топлива: "; cin >> amount;
		tank.fill(amount);
		tank.info();
	}
#endif // TANK_CHECK

#ifdef ENGINE_CHECK
	Engine engine(10);
	engine.info();
#endif // ENGINE_CHECK

	Car bmw(10, 70);
	bmw.control();

}