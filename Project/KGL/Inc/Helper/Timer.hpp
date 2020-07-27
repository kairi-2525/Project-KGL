#pragma once

#include <chrono>
#include <deque>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include "Cast.hpp"

namespace KGL
{
	inline namespace TIMER
	{
		class RefreshRate
		{
		private:
			static inline const double UPDATE_RATE = 0.1;
		private:
			bool m_inited = false;
			std::chrono::system_clock::time_point time_point;
			std::deque<double> elpased_time_log;
			double m_elapsed_time_nano = 0.0;
			double m_elapsed_time_sec = 0.0;
			double m_average_time = 0.0;
			double m_refresh_rate = 0.0;
			double m_update_counter = 0.0;
		private:
			RefreshRate(const RefreshRate&) = delete;
			RefreshRate& operator=(const RefreshRate&) = delete;
		public:
			RefreshRate() = default;

			void Update() noexcept;
			template<class T = float>
			T GetElpasedTime() const noexcept { return SCAST<T>(m_elapsed_time_sec); }
			template<class T = float>
			T GetRefreshRate() const noexcept { return SCAST<T>(m_refresh_rate); }
		};
	}

	class Timer
	{
	public:
		enum class SEC
		{
			MILLI,
			MICRO,
			NANO
		};
	private:
		std::chrono::system_clock::time_point time_point;
	public:
		explicit Timer() noexcept { Restart(); };
		void Restart() noexcept { time_point = std::chrono::system_clock::now(); }
		UINT64 GetTime(SEC sec_type = SEC::MILLI) const noexcept
		{
			switch (sec_type)
			{
			case SEC::MICRO:
				return std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::system_clock::now() - time_point
					).count();
			case SEC::NANO:
				return std::chrono::duration_cast<std::chrono::nanoseconds>(
					std::chrono::system_clock::now() - time_point
					).count();
			default:
				return std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now() - time_point
					).count();
			}
		}
	};
}