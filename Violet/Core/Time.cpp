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

const Time::clock::duration Time::dt = 30ms;
const Time::clock::duration Time::frameLimit = 250ms;