#include "stdafx.h"
#include "Time.hpp"

Time::Time()
    : currentTime(clock::now()), accumulator(dt) //simulate one frame before rendering
	, simTime(0)
{
}

Time::clock::duration Time::SimTime()
{
    return simTime;
}

float Time::SimTimeMs()
{
    return std::chrono::duration_cast<millifloat>(simTime).count();
}

const Time::clock::duration Time::dt = std::chrono::milliseconds(30);
const Time::clock::duration Time::frameLimit = std::chrono::milliseconds(250);