#include "precompiled/engine_precompiled.hpp"
#include <core/time.hpp>

void bee::Time::Tick()
{
    //DeltaTime
    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed = current_time - m_prevFrameTime;
    m_prevFrameTime = current_time;

    m_deltatime = std::chrono::duration_cast<DeltaMS>(elapsed);
    m_timeSinceStartup += m_deltatime;
    m_fixedTimeAccumulator += m_deltatime;

    //Number of Physics steps required
    m_physicsStepsNecessary = 0;
    while (m_fixedTimeAccumulator >= m_fixedTimestep)
    {
        m_fixedTimeAccumulator -= m_fixedTimestep;
        m_physicsStepsNecessary++;
    }
}
