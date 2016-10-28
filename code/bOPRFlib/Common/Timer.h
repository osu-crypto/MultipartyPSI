#pragma once 
#include <list>
#include <chrono>
#include <string>

namespace bOPRF 
{ 


	class Timer
	{
		typedef std::chrono::system_clock::time_point timeUnit;

		timeUnit mStart;
		std::list< std::pair<timeUnit, std::string>> mTimes;

	public: 
		Timer()
		:mStart(Timer::timeUnit::clock::now()) 
		{}

		const timeUnit& setTimePoint(const std::string& msg);


		friend std::ostream& operator<<(std::ostream& out, const Timer& timer);


		void reset();
	};
	 
}