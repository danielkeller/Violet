#include "stdafx.h"
#include "Time.hpp"

Time::Time()
    : currentTime(clock::now()), accumulator(0)
{
}

const Time::clock::duration Time::dt =std::chrono::milliseconds(30);