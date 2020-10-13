#pragma once

#define DIRECTINPUT_VERSION     0x0800
#include <dinput.h>
#include <memory>
#include <list>
#include <string>
#include <DirectXMath.h>
#include "../Helper/Cast.hpp"
#include "Pad.hpp"
#include <algorithm>


#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


namespace KGL
{
	inline namespace INPUT
	{

		class Direct
		{
		public:
			//-----------
			//  キー列挙
			//-----------
			enum class MOUSE_BUTTONS : int
			{
				left,
				right,
				center,
				button1
			};
			enum class KEYS : int
			{
				ESCAPE = DIK_ESCAPE,
				NUM_1 = DIK_1,
				NUM_2 = DIK_2,
				NUM_3 = DIK_3,
				NUM_4 = DIK_4,
				NUM_5 = DIK_5,
				NUM_6 = DIK_6,
				NUM_7 = DIK_7,
				NUM_8 = DIK_8,
				NUM_9 = DIK_9,
				NUM_0 = DIK_0,
				MINUS = DIK_MINUS,
				EQUALS = DIK_EQUALS,
				BACK = DIK_BACK,
				TAB = DIK_TAB,
				Q = DIK_Q,
				W = DIK_W,
				E = DIK_E,
				R = DIK_R,
				T = DIK_T,
				Y = DIK_Y,
				U = DIK_U,
				I = DIK_I,
				O = DIK_O,
				P = DIK_P,
				LBRACKET = DIK_LBRACKET,
				RBRACKET = DIK_RBRACKET,
				RETURN = DIK_RETURN,
				LCONTROL = DIK_LCONTROL,
				A = DIK_A,
				S = DIK_S,
				D = DIK_D,
				F = DIK_F,
				G = DIK_G,
				H = DIK_H,
				J = DIK_J,
				K = DIK_K,
				L = DIK_L,
				SEMICOLON = DIK_SEMICOLON,
				APOSTROPHE = DIK_APOSTROPHE,
				GRAVE = DIK_GRAVE,
				LSHIFT = DIK_LSHIFT,
				BACKSLASH = DIK_BACKSLASH,
				Z = DIK_Z,
				X = DIK_X,
				C = DIK_C,
				V = DIK_V,
				B = DIK_B,
				N = DIK_N,
				M = DIK_M,
				COMMA = DIK_COMMA,
				PERIOD = DIK_PERIOD,
				SLASH = DIK_SLASH,
				RSHIFT = DIK_RSHIFT,
				MULTIPLY = DIK_MULTIPLY,
				LMENU = DIK_LMENU,
				SPACE = DIK_SPACE,
				CAPITAL = DIK_CAPITAL,
				F1 = DIK_F1,
				F2 = DIK_F2,
				F3 = DIK_F3,
				F4 = DIK_F4,
				F5 = DIK_F5,
				F6 = DIK_F6,
				F7 = DIK_F7,
				F8 = DIK_F8,
				F9 = DIK_F9,
				F10 = DIK_F10,
				NUMLOCK = DIK_NUMLOCK,
				SCROLL = DIK_SCROLL,
				NUMPAD7 = DIK_NUMPAD7,
				NUMPAD8 = DIK_NUMPAD8,
				NUMPAD9 = DIK_NUMPAD9,
				SUBTRACT = DIK_SUBTRACT,
				NUMPAD4 = DIK_NUMPAD4,
				NUMPAD5 = DIK_NUMPAD5,
				NUMPAD6 = DIK_NUMPAD6,
				ADD = DIK_ADD,
				NUMPAD1 = DIK_NUMPAD1,
				NUMPAD2 = DIK_NUMPAD2,
				NUMPAD3 = DIK_NUMPAD3,
				NUMPAD0 = DIK_NUMPAD0,
				DECIMAL = DIK_DECIMAL,
				OEM_102 = DIK_OEM_102,
				F11 = DIK_F11,
				F12 = DIK_F12,
				F13 = DIK_F13,
				F14 = DIK_F14,
				F15 = DIK_F15,
				KANA = DIK_KANA,
				ABNT_C1 = DIK_ABNT_C1,
				CONVERT = DIK_CONVERT,
				NOCONVERT = DIK_NOCONVERT,
				YEN = DIK_YEN,
				ABNT_C2 = DIK_ABNT_C2,
				NUMPADEQUALS = DIK_NUMPADEQUALS,
				PREVTRACK = DIK_PREVTRACK,
				AT = DIK_AT,
				COLON = DIK_COLON,
				UNDERLINE = DIK_UNDERLINE,
				KANJI = DIK_KANJI,
				STOP = DIK_STOP,
				AX = DIK_AX,
				UNLABELED = DIK_UNLABELED,
				NEXTTRACK = DIK_NEXTTRACK,
				NUMPADENTER = DIK_NUMPADENTER,
				RCONTROL = DIK_RCONTROL,
				MUTE = DIK_MUTE,
				CALCULATOR = DIK_CALCULATOR,
				PLAYPAUSE = DIK_PLAYPAUSE,
				MEDIASTOP = DIK_MEDIASTOP,
				VOLUMEDOWN = DIK_VOLUMEDOWN,
				VOLUMEUP = DIK_VOLUMEUP,
				WEBHOME = DIK_WEBHOME,
				NUMPADCOMMA = DIK_NUMPADCOMMA,
				DIVIDE = DIK_DIVIDE,
				SYSRQ = DIK_SYSRQ,
				RMENU = DIK_RMENU,
				PAUSE = DIK_PAUSE,
				HOME = DIK_HOME,
				UP = DIK_UP,
				PRIOR = DIK_PRIOR,
				LEFT = DIK_LEFT,
				RIGHT = DIK_RIGHT,
				END = DIK_END,
				DOWN = DIK_DOWN,
				NEXT = DIK_NEXT,
				INSERT = DIK_INSERT,
				DEL = DIK_DELETE,
				LWIN = DIK_LWIN,
				RWIN = DIK_RWIN,
				APPS = DIK_APPS,
				POWER = DIK_POWER,
				SLEEP = DIK_SLEEP,
				WAKE = DIK_WAKE,
				WEBSEARCH = DIK_WEBSEARCH,
				WEBFAVORITES = DIK_WEBFAVORITES,
				WEBREFRESH = DIK_WEBREFRESH,
				WEBSTOP = DIK_WEBSTOP,
				WEBFORWARD = DIK_WEBFORWARD,
				WEBBACK = DIK_WEBBACK,
				MYCOMPUTER = DIK_MYCOMPUTER,
				MAIL = DIK_MAIL,
				MEDIASELECT = DIK_MEDIASELECT,

				//別名
				ENTER = RETURN,
				BACKSPACE = BACK,
				NUMPADSTAR = MULTIPLY,
				LALT = LMENU,
				CAPSLOCK = CAPITAL,
				NUMPADMINUS = SUBTRACT,
				NUMPADPLUS = ADD,
				NUMPADPERIOD = DECIMAL,
				NUMPADSLASH = DIVIDE,
				RALT = RMENU,
				UPARROW = UP,
				PGUP = PRIOR,
				LEFTARROW = LEFT,
				RIGHTARROW = RIGHT,
				DOWNARROW = DOWN,
				PGDN = NEXT,
				CIRCUMFLEX = PREVTRACK
			};
			enum class PADS
			{

			};

		private:
			//-----------------
			//  デバイスのベース
			//-----------------
			class Device
			{
			private:
				Device(const Device&) = delete;
				Device& operator=(const Device&) = delete;
			protected:
				LPDIRECTINPUTDEVICE8 m_lp_device;
				DIDEVICEINSTANCEA m_desc;
			protected:
				Device(LPDIRECTINPUTDEVICE8 lp_device, const std::string& device_name = "不明のデバイス");
				virtual ~Device();
			public:
				virtual HRESULT Update(bool clear) = 0;
				const DIDEVICEINSTANCE& GetInstanceDesc() const;
			};

			//-----------------
			//     Button
			//-----------------
			template <class T>
			class Button
			{
			private:
				BYTE& m_current_button;
				BYTE& m_previous_button;

			protected:
				Button(BYTE* current_button, BYTE* previous_button);
				virtual ~Button() = default;
			public:
				//押されていたらTRUE
				bool IsHold(T button) const;
				//押した瞬間だけTRUE
				bool IsPressed(T button) const;
				//離した瞬間だけTRUE
				bool IsReleased(T button) const;
			};

			//-----------------
			//      マウス
			//-----------------
			class Mouse :
				public Device,
				public Button<MOUSE_BUTTONS>
			{
			private:
				struct State
				{
					DIMOUSESTATE state;
				};
			private:
				State m_current_state;
				State m_previous_state;
				DirectX::XMINT2 m_pos;
				DirectX::XMINT2 m_old_pos;
			private:
				static DirectX::XMINT2 GetWindowPos(HWND hwnd, const DirectX::XMINT2& pos);
			public:
				Mouse(
					LPDIRECTINPUTDEVICE8 lp_device
				);
				HRESULT Update(bool clear) override;
				LONG GetWheel() const;
				DirectX::XMINT2 GetMove() const;
				DirectX::XMINT2 GetPos() const;
				DirectX::XMINT2 GetPos(HWND hwnd) const;
				DirectX::XMINT2 GetOldPos() const;
				DirectX::XMINT2 GetOldPos(HWND hwnd) const;
			};

			//-----------------
			//     キーボード
			//-----------------
			class KeyBoard :
				public Device,
				public Button<KEYS>
			{
			private:
				struct State
				{
					BYTE key[256] = {};
				};
			private:
				State m_current_state;
				State m_previous_state;
			public:
				KeyBoard(
					LPDIRECTINPUTDEVICE8 lp_device
				);
				HRESULT Update(bool clear) override;
			};

			//-----------------
			//   ゲームパッド
			//-----------------
			class GamePad :
				public INPUT::Pad,
				public Device
			{
			private:
				struct State
				{
					DIJOYSTATE state;

					DirectX::XMINT4 direction_button;
					float r_trigger;
					float l_trigger;

					DirectX::XMFLOAT2 r_stick;
					DirectX::XMFLOAT2 l_stick;

					bool connected = false;
				};
			public:
				static inline const float MAX_STICK_TILT = 32767.0f;
				static inline const float MAX_TRRIGER_TILT = 255.0f;
			private:
				State m_current_state;
				State m_previous_state;

				DirectX::XMFLOAT2 m_deadzone;
			public:
				GamePad(
					LPDIRECTINPUTDEVICE8 lp_device,
					const int id
				);
				HRESULT Update(bool clear) override;
				HRESULT UpdatePad(bool clear) override;
			};


		private:
			LPDIRECTINPUT8 m_lp_di = nullptr;
			std::unique_ptr<KeyBoard> m_keyboard;
			std::unique_ptr<Mouse> m_mouse;
			std::list<GamePad> m_gamepads;
		private:
			static BOOL PASCAL StaticEnumGamePadsDeviceProc(LPCDIDEVICEINSTANCE lpddi, LPVOID pv_ref);
			static BOOL IsXInputDevice(const GUID* pGuidProductFromDirectInput);
		private:
			BOOL EnumGamePadsDeviceProc(LPCDIDEVICEINSTANCE lpddi);
		public:
			Direct(HWND hwnd);
			~Direct();
			HRESULT Reload(HWND hwnd);
			HRESULT ReloadMouse(HWND hwnd);
			HRESULT ReloadKeyBoard(HWND hwnd);
			HRESULT ReloadGamePads();
			HRESULT UpdateMouse(bool clear);
			HRESULT UpdateKeyBoard(bool clear);
			HRESULT UpdateGamePads(bool clear);
			const KeyBoard* GetKeyBoard() const;
			const Mouse* GetMouse() const;
		};

		//------------------------------
		//     ゲッター：インライン実装
		//------------------------------
		inline const Direct::KeyBoard* Direct::GetKeyBoard() const
		{
			return m_keyboard.get();
		}
		inline const Direct::Mouse* Direct::GetMouse() const
		{
			return m_mouse.get();
		}

		//------------------------------
		//     デバイス：インライン実装
		//------------------------------
		inline const DIDEVICEINSTANCEA& Direct::Device::GetInstanceDesc() const
		{
			return m_desc;
		}

		//--------------------------
		//    Button：インライン実装
		//--------------------------
		template<class T>
		Direct::Button<T>::Button(BYTE* current_button, BYTE* previous_button) :
			m_current_button(*current_button),
			m_previous_button(*previous_button)
		{
		}
		template<class T>
		inline bool Direct::Button<T>::IsHold(T button) const
		{
			return (&m_current_button)[static_cast<int>(button)] & 0x80;
		}
		template<class T>
		inline bool Direct::Button<T>::IsPressed(T button) const
		{
			return
				(&m_current_button)[static_cast<int>(button)] & 0x80
				&&
				!((&m_previous_button)[static_cast<int>(button)] & 0x80);
		}
		template<class T>
		inline bool Direct::Button<T>::IsReleased(T button) const
		{
			return
				!((&m_current_button)[static_cast<int>(button)] & 0x80)
				&&
				(&m_previous_button)[static_cast<int>(button)] & 0x80;
		}

		//--------------------------
		//     マウス：インライン実装
		//--------------------------
		inline DirectX::XMINT2 Direct::Mouse::GetWindowPos(HWND hwnd, const DirectX::XMINT2& pos)
		{
			WINDOWINFO info = {};
			::GetWindowInfo(hwnd, &info);

			DirectX::XMINT2 ret_pos;
			ret_pos.x = SCAST<int32_t>((std::min)((std::max)(SCAST<LONG>(pos.x), info.rcClient.left), info.rcClient.right) - info.rcClient.left);
			ret_pos.y = SCAST<int32_t>((std::min)((std::max)(SCAST<LONG>(pos.y), info.rcClient.top), info.rcClient.bottom) - info.rcClient.top);

			return ret_pos;
		}

		inline LONG Direct::Mouse::GetWheel() const
		{
			return m_current_state.state.lZ;
		}
		inline DirectX::XMINT2 Direct::Mouse::GetMove() const
		{
			return { m_current_state.state.lX, -m_current_state.state.lY };
		}
		inline DirectX::XMINT2 Direct::Mouse::GetPos() const
		{
			return m_pos;
		}
		inline DirectX::XMINT2 Direct::Mouse::GetPos(HWND hwnd) const
		{
			return GetWindowPos(hwnd, m_pos);
		}
		inline DirectX::XMINT2 Direct::Mouse::GetOldPos() const
		{
			return m_old_pos;
		}
		inline DirectX::XMINT2 Direct::Mouse::GetOldPos(HWND hwnd) const
		{
			return GetWindowPos(hwnd, m_old_pos);
		}

		using KEYS = Direct::KEYS;
		using MOUSE_BUTTONS = Direct::MOUSE_BUTTONS;
	}
}