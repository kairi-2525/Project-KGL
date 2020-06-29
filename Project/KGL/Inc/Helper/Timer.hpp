#pragma once

#include <chrono>
#include <deque>

#include "Cast.hpp"

namespace KGL
{
	inline namespace Timer
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
}