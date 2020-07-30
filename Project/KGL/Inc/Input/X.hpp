#pragma once

#include <Xinput.h>
#include <array>
#include <DirectXMath.h>
#include "Pad.hpp"

#pragma comment(lib, "xinput.lib")

namespace KGL
{
	inline namespace INPUT
	{
		class X
		{
		public:
			enum class PADS : WORD
			{
				UP = 0b0000000000000001,
				DOWN = 0b0000000000000010,
				LEFT = 0b0000000000000100,
				RIGHT = 0b0000000000001000,
				START = 0b0000000000010000,
				BACK = 0b0000000000100000,
				L_STICK = 0b0000000001000000,
				R_STICK = 0b0000000010000000,
				LB = 0b0000000100000000,
				RB = 0b0000001000000000,
				LT = 0b0000010000000000,
				RT = 0b0000100000000000,
				A = 0b0001000000000000,
				B = 0b0010000000000000,
				X = 0b0100000000000000,
				Y = 0b1000000000000000,
			};
		private:
			class Pad :
				public INPUT::Pad
			{
			private:
				struct State
				{
					XINPUT_STATE state{};

					DirectX::XMFLOAT2 r_stick;
					DirectX::XMFLOAT2 l_stick;

					bool connected = false;
				};
			private:
				static inline const DirectX::XMINT2 STICK_TILT_LIMIT = { -32768, 32767 };
				static inline const DirectX::XMINT2 STICK_DEAD_ZONE = { 120 - 7000, 120 + 7000 };
			private:
				State m_current_state;
				State m_previous_state;

				DirectX::XMFLOAT2 m_deadzone;
			private:
				void StickNormalize();
			public:
				Pad(const int id);
				HRESULT UpdatePad(bool clear) override;
				bool IsConnected() const;
				const XINPUT_STATE& GetState() const;
			};
		public:
			std::array<Pad, XUSER_MAX_COUNT> m_pads;
		public:
			X();
			void Update(bool clear);
		};

		inline bool X::Pad::IsConnected() const
		{
			return m_current_state.connected;
		}
		inline const XINPUT_STATE& X::Pad::GetState() const
		{
			return m_current_state.state;
		}
	}
}