#define PROFILE
#include <iostream>

#ifdef PROFILE
#include <chrono>
#include <map>
#include <string>
#include <iomanip>

class Profile
{
public:
	using clock = std::chrono::steady_clock;
	using duration = clock::duration;

	Profile(const char * name)
		: name(name), began(clock::now())
	{}

	~Profile()
	{
		auto ended = clock::now();
		int n = data[name].first;
		//running average
		data[name].second = ((ended - began) - comp + n*data[name].second)/(n+1);
		data[name].first = n + 1;
	}

	static void CalibrateProfiling();
	static void Print();

private:
	const char* name;
	const clock::time_point began;

	static duration comp;
	static std::map<const char*, std::pair<int, duration>> data;

	static inline std::string niceUnits(duration d);
};

inline void Profile::CalibrateProfiling()
{
	for (int i = 0; i<1000; ++i)
	{
		auto then = clock::now();
		auto now = clock::now();
		comp += now - then;
	}

	comp /= 1000;
}

inline std::string Profile::niceUnits(duration d)
{
	if (d < std::chrono::milliseconds(5))
		return std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(d).count()) + "us";
	else
		return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(d).count()) + "ms";
}

inline void Profile::Print()
{
	std::cout << std::setw(20) << "name"
		<< std::setw(8) << "total"
		<< std::setw(8) << "avg\n";

	for (const auto& datapt : data)
	{
		int n = datapt.second.first;
		duration t = datapt.second.second;
		std::cout << std::setw(20) << datapt.first
			<< std::setw(8) << niceUnits(t*n)
			<< std::setw(8) << niceUnits(t) << "\n";
	}
}

#else
class Profile
{
public:
	Profile(const char * name) {}
	~Profile() {}

	static void CalibrateProfiling() {}
	static void Print() {std::cout << "No profiling data\n";}
};
#endif