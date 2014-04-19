#ifndef SE_TIMER__
#define SE_TIMER__

# include <chrono>

class Timer
{
private:
    
    using ClockType = std::chrono::high_resolution_clock;
    using Duration = ClockType::duration;
    using TimePoint = ClockType::time_point;
    
    TimePoint m_start;
public:
    Timer()
      : m_start(ClockType::now())
    {}
    
    void ResetClock() 
    { 
        m_start = ClockType::now(); 
    }
    
    Float64 CalcSeconds() const 
    { 
        auto const dur = std::chrono::duration_cast<std::chrono::seconds>(
                ClockType::now() - m_start 
        );
        return static_cast<Float64>(dur.count());
    }
};

#endif
