#include <Helper/Timer.hpp>

using namespace KGL;

void RefreshRate::Update() noexcept
{
	if (!m_inited)
	{
		time_point = std::chrono::system_clock::now();
		m_inited = true;
	}
	m_elapsed_time_nano = static_cast<double>(
		std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::system_clock::now() - time_point
			).count()
		);
	const double scaling_nano_sec = 1000000000.0;
	m_elapsed_time_sec = m_elapsed_time_nano / scaling_nano_sec;
	time_point = std::chrono::system_clock::now();

	auto itr = elpased_time_log.emplace(elpased_time_log.begin(), m_elapsed_time_nano);
	{
		double total = 0;
		unsigned int num = 0;
		const auto end = elpased_time_log.end();
		const unsigned int count_frame = static_cast<unsigned int>(1.0 / m_elapsed_time_sec);
		while (itr != end && num < count_frame)
		{
			total += *itr;
			num++;
			itr++;
		}
		if (itr != end)
			elpased_time_log.erase(itr, end);
		if (num > 0u)
		{
			m_average_time = scaling_nano_sec / (total / num);
			if (m_update_counter < UPDATE_RATE)
			{
				m_update_counter += m_elapsed_time_sec;
				if (m_update_counter >= UPDATE_RATE)
				{
					m_update_counter = 0.0;
					m_refresh_rate = m_average_time;
				}
			}
		}
	}
}