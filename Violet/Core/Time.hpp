#ifndef TIME_HPP
#define TIME_HPP

#include <chrono>

using namespace std::chrono_literals;

using millifloat = std::chrono::duration<float, std::milli>;

class Time
{
public:
    using clock = std::chrono::system_clock;
    
    Time();
    
    //time simulated per step
    static const clock::duration dt;
    //total simulation time as of just before current step
    clock::duration SimTime();
    float SimTimeMs();
    
    template<class PhysTick, class RenderTick>
    void MainLoop(const PhysTick& physTick, const RenderTick& renderTick)
    {
        while (true)
        {
            auto newTime = clock::now();
            auto frameTime = newTime - currentTime;
            if (frameTime > frameLimit) frameTime = frameLimit;
            
            //By setting currentTime to newTime we "warp" the time we say we've simulated up to clock::now(),
            //even if the amount of time is less than what we've actually simulated. This is fine in a single-player
            //environment, but in a networked environment it could cause our simulation to fall behind everyone
            //else's. By instead setting it to currentTime + frameTime, it would force the physics to take extra
            //steps (limited by frameLimit) to catch back up to clock::now(). This would appear as the physics
            //slowing down under load, then speeding back up to compensate. Alternatively, if we can simply override
            //the entire physics state, we could just skip simulating those ticks and warp right up to now.
            //On the other hand, after leaving it paused in the debugger for a while this could really screw things
            //up
            currentTime = newTime;
            
            accumulator += frameTime;
            
            while (accumulator >= dt)
            {
                if (!physTick())
                    return;
                simTime += dt;
                accumulator -= dt;
            }
            
            //leftover unsimulated time as a fraction of the step size
            float alpha = std::chrono::duration_cast<millifloat>(accumulator).count()
            / std::chrono::duration_cast<millifloat>(dt).count();
            renderTick(alpha);
        }
    }
    
private:
    
    //maximum amount of physics time to simulate before drawing a frame
    static const clock::duration frameLimit;
    
    //time point before last set of physics/draw steps
    clock::time_point currentTime;
    clock::duration accumulator; //total unsimulated time
    clock::duration simTime; //total simulated time
};

#endif