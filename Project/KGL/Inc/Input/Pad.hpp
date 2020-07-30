#pragma once

namespace KGL
{
	inline namespace INPUT
	{
		class Pad
		{
		protected:
			int m_id;
		protected:
			Pad(const int id);
			virtual ~Pad() = default;
		public:
			int GetNum() const;
			virtual long UpdatePad(bool clear) = 0;
		};

		inline int Pad::GetNum() const
		{
			return m_id;
		}
	}
}