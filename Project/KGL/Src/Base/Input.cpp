#include <Base/Input.hpp>

using namespace KGL;

Input::Input(HWND hwnd) :
	m_direct_input(hwnd)
{
	m_mouse_state.visible = true;
	m_mouse_state.visible_out_side = false;

	m_current_mouse_state.any_hold = false;
	m_current_mouse_state.any_pressed = false;
	m_current_mouse_state.any_released = false;
	m_current_mouse_state.any_run = true;
	m_previous_mouse_state = m_current_mouse_state;

	m_current_keyboard_state.any_hold = false;
	m_current_keyboard_state.any_pressed = false;
	m_current_keyboard_state.any_released = false;
	m_current_keyboard_state.any_run = true;
	m_previous_keyboard_state = m_current_keyboard_state;
}

//anyキーを保存
void Input::AnyStateUpdate()
{
	//Mouse用anyキーを保存
	{
		auto* state = &m_current_mouse_state;
		state->any_hold = false;
		state->any_pressed = false;
		state->any_released = false;
		if (state->any_run)
		{
			for (int i = 0; i < 4; i++)
			{
				state->any_hold = state->any_hold || IsMouseHold(static_cast<INPUT::MOUSE_BUTTONS>(i));
				state->any_pressed = state->any_pressed || IsMousePressed(static_cast<INPUT::MOUSE_BUTTONS>(i));
				state->any_released = state->any_released || IsMouseReleased(static_cast<INPUT::MOUSE_BUTTONS>(i));
			}
		}
		m_previous_mouse_state = m_current_mouse_state;
	}

	//KeyBoard用anyキーを保存
	{
		auto* state = &m_current_keyboard_state;
		state->any_hold = false;
		state->any_pressed = false;
		state->any_released = false;
		if (state->any_run)
		{
			for (int i = 0; i <= 0xED; i++)
			{
				state->any_hold = state->any_hold || IsKeyHold(static_cast<INPUT::KEYS>(i));
				state->any_pressed = state->any_pressed || IsKeyPressed(static_cast<INPUT::KEYS>(i));
				state->any_released = state->any_released || IsKeyReleased(static_cast<INPUT::KEYS>(i));
			}
		}
		m_previous_keyboard_state = m_current_keyboard_state;
	}
}

HRESULT Input::Update(bool key_update, bool mouse_update, bool pad_update)
{
	HRESULT hr;
	hr = m_direct_input.UpdateMouse(!mouse_update);
	//ウィンドウが選択されていない時非表示状態から復帰する
	if (FAILED(hr))
	{
		if (hr == DIERR_OTHERAPPHASPRIO && !m_mouse_state.visible)
		{
			SetMouseVisible(true);
			m_mouse_state.visible_out_side = true;
		}
		else return hr;
	}
	else if (m_mouse_state.visible_out_side)
	{
		SetMouseVisible(false);
		m_mouse_state.visible_out_side = false;
	}

	hr = m_direct_input.UpdateKeyBoard(!key_update);
	AnyStateUpdate();
	if (FAILED(hr) && hr != DIERR_OTHERAPPHASPRIO) return hr;

	hr = m_direct_input.UpdateGamePads(!pad_update);
	if (FAILED(hr)) return hr;
	m_x_input.Update(pad_update);

	return hr;
}
void Input::SetMouseVisible(bool visible)
{
	if (m_mouse_state.visible != visible)
	{
		m_mouse_state.visible = visible;
		ShowCursor(visible);
	}
}