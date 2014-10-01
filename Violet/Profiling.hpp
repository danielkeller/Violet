#define PROFILE
#include <iostream>

#ifdef PROFILE
#include <chrono>
#include <map>

class Profile
{
public:
	using clock = std::chrono::high_resolution_clock;
	using duration = clock::duration;

	Profile(const char * name)
		: name(name), began(clock::now())
	{}

	~Profile()
	{
		auto ended = clock::now();
		data[name] += (ended - began) - comp;
	}

	static void CalibrateProfiling();
	static void Print();

private:
	const char* name;
	const std::chrono::high_resolution_clock::time_point began;

	static duration comp;
	static std::map<const char*, duration> data;
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

inline void Profile::Print()
{
	for (const auto& datapt : data)
		std::cout << datapt.first << ": "
			<< std::chrono::duration_cast<std::chrono::milliseconds>(datapt.second).count() << "ms \n";
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