#ifndef TIME_HPP
#define TIME_HPP

#include <chrono>

class Time
{
    using clock = std::chrono::system_clock;
    clock::time_point currentTime;
    clock::duration accumulator;
    
public:
    Time();
    static const clock::duration dt;
    
    template<class PhysTick, class RenderTick>
    void MainLoop(const PhysTick& physTick, const RenderTick& renderTick)
    {
        while (true)
        {
            auto newTime = clock::now();
            auto frameTime = newTime - currentTime;
            if (frameTime > std::chrono::milliseconds(250))
                frameTime = std::chrono::milliseconds(250);
            currentTime = newTime;
            
            accumulator += frameTime;
            
            while (accumulator >= dt)
            {
                if (!physTick())
                    return;
                accumulator -= dt;
            }
            
            using millifloat = std::chrono::duration<float, std::milli>;
            float alpha = std::chrono::duration_cast<millifloat>(accumulator).count()
            / std::chrono::duration_cast<millifloat>(dt).count();
            renderTick(alpha);
        }
    }
};

#endif