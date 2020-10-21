#pragma once

#include <algorithm>
#include <chrono>
#include <deque>
#include <vector>

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
		struct SCALES
		{
			UINT64 milli, micro, nano;

			SCALES operator+(const SCALES& s) { return { milli + s.milli, micro + s.micro, nano + s.nano }; }
			SCALES operator-(const SCALES& s) { return { milli - s.milli, micro - s.micro, nano - s.nano }; }
			SCALES& operator+=(const SCALES& s) { return *this = *this + s; }
			SCALES& operator-=(const SCALES& s) { return *this = *this - s; }
			SCALES operator/(UINT i) { return { milli / i, micro / i, nano / i }; }
			SCALES& operator/=(UINT i) { return *this = *this / i; }
			static SCALES Max(const SCALES& s0, const SCALES& s1) { return { (std::max)(s0.milli, s1.milli), (std::max)(s0.micro, s1.micro), (std::max)(s0.nano, s1.nano) }; }
			static SCALES Min(const SCALES& s0, const SCALES& s1) { return { (std::min)(s0.milli, s1.milli), (std::min)(s0.micro, s1.micro), (std::min)(s0.nano, s1.nano) }; }
		};
	private:
		std::chrono::system_clock::time_point	time_point;
		SCALES									max_time;
		SCALES									min_time;
		SCALES									average_time;
		SCALES									last_time;
		bool									inited;
		std::vector<SCALES>						average_frame_data;
	public:
		void Count() noexcept
		{
			auto last_time_point = std::chrono::system_clock::now() - time_point;
			last_time.milli = std::chrono::duration_cast<std::chrono::milliseconds>(last_time_point).count();
			last_time.micro = std::chrono::duration_cast<std::chrono::microseconds>(last_time_point).count();
			last_time.nano = std::chrono::duration_cast<std::chrono::nanoseconds>(last_time_point).count();

			max_time = SCALES::Max(max_time, last_time);
			min_time = SCALES::Min(min_time, last_time);

			average_frame_data.push_back(last_time);
			if (average_frame_data.size() >= average_frame_data.capacity())
			{
				average_time = { 0, 0, 0 };
				for (const auto& it : average_frame_data)
				{
					average_time += it;
				}
				average_time /= average_frame_data.capacity();
				average_frame_data.clear();
			}
		}
		void Clear()
		{
			inited = false;
			last_time = average_time = max_time = { 0, 0, 0 };
			min_time = { UINT64_MAX, UINT64_MAX, UINT64_MAX };
			average_frame_data.clear();
		}
		explicit Timer(UINT average_frame = 100u) noexcept { average_frame_data.reserve(average_frame); Clear(); Restart(); };
		void Restart() noexcept { time_point = std::chrono::system_clock::now(); }
		UINT64 GetTime(SEC sec_type = SEC::MILLI) noexcept
		{
			Count();
			switch (sec_type)
			{
			case SEC::MICRO:
				return last_time.micro;
			case SEC::NANO:
				return last_time.nano;
			default:
				return last_time.milli;
			}
		}
		const SCALES& Last() const noexcept { return last_time; }
		const SCALES& Max() const noexcept { return max_time; }
		const SCALES& Min() const noexcept { return min_time; }
		const SCALES& Average() const noexcept { return average_time; }
	};
}