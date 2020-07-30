#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <Input/X.hpp>
#include <string>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

X::X() :
	m_pads{ 0, 1, 2, 3 }
{
	for (auto& pad : m_pads)
	{
		if (SUCCEEDED(pad.UpdatePad(false)))
		{
			const auto& state = pad.GetState();
			XINPUT_CAPABILITIES capabilities;
			XInputGetCapabilities(static_cast<DWORD>(pad.GetNum()), NULL, &capabilities);
			KGLDebugOutPutString(
				std::string(
					"XInput[ゲームパッド[" + std::to_string(pad.GetNum()) + "]]"
					"\n\n"
				).c_str()
			);
		}
	}
}

void X::Update(bool clear)
{
	for (auto& pad : m_pads)
	{
		pad.UpdatePad(clear);
	}
}

X::Pad::Pad(const int id) :
	INPUT::Pad(id)
{

}

//スティックの値をデッドゾーンを考慮してノーマライズ
void X::Pad::StickNormalize()
{
	for (int i = 0; i < 4; i++)
	{
		float* set_stick = nullptr;
		SHORT stick = 0;
		switch (i)
		{
			case 0:
				set_stick = &m_current_state.l_stick.x;
				stick = m_current_state.state.Gamepad.sThumbLX;
				break;
			case 1:
				set_stick = &m_current_state.l_stick.y;
				stick = m_current_state.state.Gamepad.sThumbLY;
				break;
			case 2:
				set_stick = &m_current_state.r_stick.x;
				stick = m_current_state.state.Gamepad.sThumbRX;
				break;
			case 3:
				set_stick = &m_current_state.r_stick.y;
				stick = m_current_state.state.Gamepad.sThumbRY;
				break;
		}

		if (set_stick)
		{
			if (stick < m_deadzone.x)
			{
				const int dist = -STICK_TILT_LIMIT.x + static_cast<int>(m_deadzone.x);
				const SHORT stick_dist = stick - static_cast<SHORT>(m_deadzone.x);
				*set_stick = static_cast<float>(stick_dist) / dist;
			}
			else if (stick > m_deadzone.y)
			{
				const int dist = STICK_TILT_LIMIT.y - static_cast<int>(m_deadzone.y);
				const SHORT stick_dist = stick - static_cast<SHORT>(m_deadzone.y);
				*set_stick = static_cast<float>(stick_dist) / dist;
			}
			else *set_stick = 0;
		}
	}
}

HRESULT X::Pad::UpdatePad(bool clear)
{
	DWORD dw_result;
	m_previous_state = m_current_state;
	std::memset(&m_current_state.state, 0, sizeof(XINPUT_STATE));
	if (clear) return S_OK;
	dw_result = XInputGetState(static_cast<DWORD>(m_id), &m_current_state.state);
	if (dw_result == ERROR_SUCCESS)
	{
		{
			XINPUT_CAPABILITIES capabilities = {};
			XInputGetCapabilities(m_id, NULL, &capabilities);
			m_current_state.connected = (capabilities.Type == XINPUT_DEVSUBTYPE_GAMEPAD && capabilities.SubType == XINPUT_DEVSUBTYPE_GAMEPAD);
		}
		if (m_current_state.connected)
		{
			StickNormalize();
		}
	}
	return m_current_state.connected ? S_OK : E_FAIL;
}