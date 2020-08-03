#include <Input/Direct.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Other.hpp>

#include <string>

#ifdef RETURN_DEBUG
#include <sstream>
#include <bitset>
#endif

using namespace KGL;

//-------------
//  管理用クラス
//-------------
Direct::Direct(HWND hwnd)
{
	// IDirect8の作成
	HRESULT hr = DirectInput8Create(
		(HINSTANCE)GetModuleHandle(0),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(LPVOID*)&m_lp_di,
		NULL
	);
	RCHECK(FAILED(hr), "IDirect8の作成に失敗");

	hr = Reload(hwnd);
	RCHECK(FAILED(hr), "IDirectのReloadに失敗");
}
Direct::~Direct()
{
	m_keyboard.reset();
	m_gamepads.clear();
	if (m_lp_di)
		m_lp_di->Release();
}

//更新
HRESULT Direct::UpdateMouse(bool clear)
{
	if (m_mouse)
	{
		return m_mouse->Update(clear);
	}
	return S_FALSE;
}
HRESULT Direct::UpdateKeyBoard(bool clear)
{
	if (m_keyboard)
	{
		return m_keyboard->Update(clear);
	}
	return S_FALSE;
}
HRESULT Direct::UpdateGamePads(bool clear)
{
	HRESULT hr = S_FALSE;
	for (auto& gamepad : m_gamepads)
	{
		hr = gamepad.Update(clear);
		if (FAILED(hr)) return hr;
	}
	return hr;
}

//各デバイスの生成/再生成
HRESULT Direct::Reload(HWND hwnd)
{
	HRESULT hr;

	hr = ReloadMouse(hwnd);
	RCHECK_HR(hr, "ReloadMouseに失敗");
	hr = ReloadKeyBoard(hwnd);
	RCHECK_HR(hr, "ReloadKeyBoardに失敗");
	hr = ReloadGamePads();
	RCHECK_HR(hr, "ReloadGamePadsに失敗");
	return hr;
}

//マウスデバイスの生成/再生成
HRESULT Direct::ReloadMouse(HWND hwnd)
{
	HRESULT hr;

	m_mouse.reset();

	LPDIRECTINPUTDEVICE8 lp_mouse;
	hr = m_lp_di->CreateDevice(GUID_SysMouse, &lp_mouse, NULL);
	RCHECK_HR(hr, "DirectInput:マウスのデバイスの作成に失敗");

	// 入力データ形式のセット
	hr = lp_mouse->SetDataFormat(&c_dfDIMouse);
	if (FAILED(hr))
	{
		lp_mouse->Release();
		assert(!"DirectInput:マウスの入力データ形式のセットに失敗");
		return hr;
	}

	// 排他制御のセット
	hr = lp_mouse->SetCooperativeLevel(
		hwnd,
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
	);
	if (FAILED(hr))
	{
		lp_mouse->Release();
		assert(!"DirectInput:マウスの排他制御のセットに失敗");
		return hr;
	}

	// デバイスの設定
	DIPROPDWORD diprop;
	diprop.diph.dwSize = sizeof(diprop);
	diprop.diph.dwHeaderSize = sizeof(diprop.diph);
	diprop.diph.dwObj = 0;
	diprop.diph.dwHow = DIPH_DEVICE;
	diprop.dwData = DIPROPAXISMODE_REL;	// 相対値モードで設定（絶対値はDIPROPAXISMODE_ABS）

	hr = lp_mouse->SetProperty(DIPROP_AXISMODE, &diprop.diph);
	if (FAILED(hr)) {
		lp_mouse->Release();
		assert(!"DirectInput:マウスのプロパティーのセットに失敗");
		return hr;
	}

	m_mouse = std::make_unique<Mouse>(lp_mouse);
	return hr;
}

//キーボードデバイスの生成/再生成
HRESULT Direct::ReloadKeyBoard(HWND hwnd)
{
	HRESULT hr;

	m_keyboard.reset();

	LPDIRECTINPUTDEVICE8 lp_keyboard;
	hr = m_lp_di->CreateDevice(GUID_SysKeyboard, &lp_keyboard, NULL);

	RCHECK_HR(hr, "DirectInput:キーボードのデバイスの作成に失敗");

	// 入力データ形式のセット
	hr = lp_keyboard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr))
	{
		lp_keyboard->Release();
		assert(!"DirectInput:キーボードの入力データ形式のセットに失敗");
		return hr;
	}
	// 排他制御のセット
	hr = lp_keyboard->SetCooperativeLevel(
		hwnd,
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
	);

	if (FAILED(hr))
	{
		lp_keyboard->Release();
		assert(!"DirectInput:キーボードの排他制御のセットに失敗");
		return hr;
	}

	m_keyboard = std::make_unique<KeyBoard>(lp_keyboard);
	return hr;
}

//ゲームパッドデバイスの生成/再生成
HRESULT Direct::ReloadGamePads()
{
	m_gamepads.clear();
	// ジョイスティックの列挙
	HRESULT hr = m_lp_di->EnumDevices(DI8DEVCLASS_GAMECTRL, Direct::StaticEnumGamePadsDeviceProc, this, DIEDFL_ATTACHEDONLY);
	RCHECK_HR(hr, "DirectInput:ジョイスティックの列挙に失敗");

	return hr;
}

//--------------------------
//  ゲームパッド用プロシージャ
//--------------------------
BOOL PASCAL Direct::StaticEnumGamePadsDeviceProc(
	LPCDIDEVICEINSTANCE lpddi, LPVOID pv_ref
)
{
	Direct* p_this = (Direct*)pv_ref;
	return p_this->EnumGamePadsDeviceProc(lpddi);
}

BOOL Direct::EnumGamePadsDeviceProc(LPCDIDEVICEINSTANCE lpddi)
{
	LPDIRECTINPUTDEVICE8 lp_gamepad;


	//XINPUTに対応しているならそちらで処理する
	if (IsXInputDevice(&lpddi->guidProduct))
		return DIENUM_CONTINUE;

	// ゲームパッドデバイスの作成
	HRESULT ret = m_lp_di->CreateDevice(lpddi->guidInstance, &lp_gamepad, NULL);
	if (FAILED(ret)) {

		return DIENUM_STOP;
	}

	// 入力データ形式のセット
	ret = lp_gamepad->SetDataFormat(&c_dfDIJoystick);
	if (FAILED(ret)) {
		lp_gamepad->Release();
		return DIENUM_STOP;
	}

	// 入力範囲のセット
	DIPROPRANGE	diprg;
	diprg.diph.dwSize = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMax = 32767L;
	diprg.lMin = -32767L;

	// X axis
	diprg.diph.dwObj = DIJOFS_X;
	lp_gamepad->SetProperty(DIPROP_RANGE, &diprg.diph);

	// Y axis
	diprg.diph.dwObj = DIJOFS_Y;
	lp_gamepad->SetProperty(DIPROP_RANGE, &diprg.diph);

	// Z axis
	diprg.diph.dwObj = DIJOFS_Z;
	lp_gamepad->SetProperty(DIPROP_RANGE, &diprg.diph);

	// RZ axis
	diprg.diph.dwObj = DIJOFS_RZ;
	lp_gamepad->SetProperty(DIPROP_RANGE, &diprg.diph);

	diprg.lMax = 255L;
	diprg.lMin = 0L;
	// RX axis
	diprg.diph.dwObj = DIJOFS_RX;
	lp_gamepad->SetProperty(DIPROP_RANGE, &diprg.diph);

	// RY axis
	diprg.diph.dwObj = DIJOFS_RY;
	lp_gamepad->SetProperty(DIPROP_RANGE, &diprg.diph);

	lp_gamepad->Poll();
	m_gamepads.emplace_back(lp_gamepad, static_cast<int>(m_gamepads.size()));
	// 最初の1つのみで終わる
	return DIENUM_CONTINUE;			// 次のデバイスを列挙するにはDIENUM_CONTINUEを返す
}

//-----------------
//  デバイスのベース
//-----------------
Direct::Device::Device(LPDIRECTINPUTDEVICE8 lp_device, const std::string& device_name)
{
	assert(lp_device && "lp_deviceがnullptrです。");
	m_lp_device = lp_device;
	m_desc.dwSize = sizeof(DIDEVICEINSTANCE);
	HRESULT hr = m_lp_device->GetDeviceInfo(&m_desc);
	RCHECK(FAILED(hr), "[Direct::Device]DESCの取得に失敗");
	
	KGLDebugOutPutString(
		"DirectInput[" + device_name + "]" +
		"\n    Instance / Product           : " + m_desc.tszInstanceName + " / " + m_desc.tszProductName +
		"\n    Driver UUID                  : " + HELPER::CreateGUIDToStr(m_desc.guidFFDriver) +
		"\n    Instance UUID / Product UUID : " + HELPER::CreateGUIDToStr(m_desc.guidInstance) + " / " + HELPER::CreateGUIDToStr(m_desc.guidProduct) +
		"\n\n"
	);
}
Direct::Device::~Device()
{
	if (m_lp_device) m_lp_device->Release();
}

//-----------------
//      マウス
//-----------------
Direct::Mouse::Mouse(
	LPDIRECTINPUTDEVICE8 lp_device
) :
	Device(lp_device, "Mouse"),
	Button<MOUSE_BUTTONS>(
		m_current_state.state.rgbButtons,
		m_previous_state.state.rgbButtons
		)
{
	m_lp_device->Acquire();
}
HRESULT Direct::Mouse::Update(bool clear)
{
	m_previous_state = m_current_state;
	HRESULT	hr = S_OK;
	const size_t size = sizeof(DIMOUSESTATE);
	ZeroMemory(&m_current_state.state, size);
	if (clear) return hr;
	hr = m_lp_device->GetDeviceState(size, &m_current_state.state);
	if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
	{
		// 失敗なら再開させてもう一度取得
		hr = m_lp_device->Acquire();
		if (SUCCEEDED(hr))
		{
			m_lp_device->GetDeviceState(size, &m_current_state.state);
		}
		else if (hr != DIERR_OTHERAPPHASPRIO)
		{
			return hr;
		}
	}
	if (SUCCEEDED(hr))
	{
		m_old_pos = m_pos;
		GetCursorPos(LPPOINT(&m_pos));
	}
	return hr;
}

//-----------------
//     キーボード
//-----------------
Direct::KeyBoard::KeyBoard(
	LPDIRECTINPUTDEVICE8 lp_device
) :
	Device(lp_device, "KeyBoard"),
	Button<KEYS>(
		m_current_state.key,
		m_previous_state.key
		)
{
	m_lp_device->Acquire();
}
HRESULT Direct::KeyBoard::Update(bool clear)
{
	m_previous_state = m_current_state;
	HRESULT hr = S_OK;
	const size_t size = sizeof(m_current_state.key);

	ZeroMemory(m_current_state.key, size);
	if (clear) return hr;
	hr = m_lp_device->GetDeviceState(size, m_current_state.key);
	if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
	{
		// 失敗なら再開させてもう一度取得
		hr = m_lp_device->Acquire();
		if (SUCCEEDED(hr))
		{
			m_lp_device->GetDeviceState(size, m_current_state.key);
		}
		else if (hr != DIERR_OTHERAPPHASPRIO)
		{
			return hr;
		}
	}
#ifdef RETURN_DEBUG
	m_current_state.key[static_cast<int>(KEYS::RETURN)];
	std::stringstream ss;
	ss << std::bitset<8>(m_current_state.key[static_cast<int>(KEYS::RETURN)]);
	OutputDebugStringA(ss.str().c_str());
	OutputDebugStringA("\n");
#endif
	return hr;
}

//-----------------
//   ゲームパッド
//-----------------
Direct::GamePad::GamePad(
	LPDIRECTINPUTDEVICE8 lp_device,
	const int id
) :
	INPUT::Pad(id),
	Device(lp_device, std::string("GamePad[" + std::to_string(id) + "]"))
{
	m_lp_device->Acquire();
}
HRESULT Direct::GamePad::UpdatePad(bool clear)
{
	return Update(clear);
}
HRESULT Direct::GamePad::Update(bool clear)
{
	m_previous_state = m_current_state;
	HRESULT hr = S_OK;

	const size_t size = sizeof(DIJOYSTATE);

	ZeroMemory(&m_current_state.state, size);
	if (clear) return hr;
	hr = m_lp_device->GetDeviceState(size, &m_current_state.state);
	if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
	{
		// 失敗なら再開させてもう一度取得
		hr = m_lp_device->Acquire();
		if (SUCCEEDED(hr))
		{
			m_lp_device->GetDeviceState(size, &m_current_state.state);
		}
		else if (hr != DIERR_OTHERAPPHASPRIO)
		{
			return hr;
		}
	}
	return hr;
}