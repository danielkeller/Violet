#include "stdafx.h"
#include "Profiling.hpp"

#include <iostream>

#ifdef PROFILE
#include <chrono>
#include <map>
#include <iomanip>

std::string Profile::niceUnits(duration d)
{
	if (d < std::chrono::milliseconds(5))
		return std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(d).count()) + "us";
	else
		return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(d).count()) + "ms";
}

void Profile::Print()
{
	std::cout << std::setw(20) << "name"
		<< std::setw(8) << "total"
		<< std::setw(8) << "avg" << '\n';

	for (const auto& datapt : data)
	{
		int n = datapt.second.first;
		duration t = datapt.second.second;
		std::cout << std::setw(20) << datapt.first
			<< std::setw(8) << niceUnits(t*n)
			<< std::setw(8) << niceUnits(t) << "\n";
	}
}

Profile::duration Profile::comp;
std::map<const char*, std::pair<int, Profile::duration>> Profile::data;

#else

void Profile::Print()
{
	std::cout << "No profiling data\n";
}
#endif