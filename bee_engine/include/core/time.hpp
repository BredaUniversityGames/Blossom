#pragma once
#include <chrono>

namespace bee
{

class Time
{
public:

	using DeltaMS = std::chrono::duration<float, std::milli>;

	Time() { m_prevFrameTime = std::chrono::high_resolution_clock::now(); };
	~Time() = default;

	//Call every frame to update values
	void Tick();

	//Returns the total time since the start of the program
	DeltaMS GetTotalTime() const { return m_timeSinceStartup; }

	//Gets time interval between last frame and the one before
	DeltaMS GetDeltaTime() const { return m_deltatime; }

	//Gets fixed defined time step for use in physics multiplied by the amount of updates to keep physics in sync
	DeltaMS GetFixedTimeStep() const { return m_fixedTimestep; };
	uint32_t GetFixedStepsNeeded() const { return m_physicsStepsNecessary; }

	void SetFixedTimeStep(DeltaMS interval) { m_fixedTimestep = interval; }

private:

	std::chrono::high_resolution_clock::time_point m_prevFrameTime{};

	DeltaMS m_deltatime{};
	DeltaMS m_fixedTimeAccumulator{};
	DeltaMS m_fixedTimestep = DeltaMS(16.0f); //60 FPS by default
	DeltaMS m_timeSinceStartup{};

	uint32_t m_physicsStepsNecessary{};

};

} // namespace bee