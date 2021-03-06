#ifndef PROFILE_HPP
#define PROFILE_HPP

#define PROFILE

#ifdef PROFILE
#include <chrono>
#include <map>

class Profile
{
public:
	using clock = std::chrono::steady_clock;
	using duration = clock::duration;

	Profile(const char * name)
		: name(name), running(true), began(clock::now())
	{}

	~Profile()
	{
        Stop();
	}
    
    void Stop()
    {
        auto ended = clock::now();
        if (!running)
            return;
        int n = data[name].first;
        //running average
        data[name].second = ((ended - began) - comp + n*data[name].second)/(n+1);
        data[name].first = n + 1;
    }
    
	static void CalibrateProfiling();
	static void Print();

private:
	const char* name;
    bool running;
	const clock::time_point began;

	static duration comp;
	static std::map<const char*, std::pair<int, duration>> data;

	static inline std::string niceUnits(duration d);
};

inline void Profile::CalibrateProfiling()
{
    static const char* test = "test";
	for (int i = 0; i<1000; ++i)
        auto p = Profile(test);

    comp = data[test].second / 1000;
    data.erase(test);
}

#else
class Profile
{
public:
	Profile(const char * name) {}
	~Profile() {}

	static void CalibrateProfiling() {}
	static void Print();
};
#endif

#endif
