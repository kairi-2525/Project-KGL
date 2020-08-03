#pragma once

#include "../Input/Direct.hpp"
#include "../Input/X.hpp"

namespace KGL
{
	inline namespace BASE
	{
		class Input
		{
		private:
			struct MouseState
			{
				bool visible;
				bool visible_out_side;
			};
			struct AnyState
			{
				bool any_hold;
				bool any_pressed;
				bool any_released;
				bool any_run;
			};
		private:
			INPUT::Direct m_direct_input;
			INPUT::X m_x_input;

			MouseState m_mouse_state;

			AnyState m_current_mouse_state;
			AnyState m_previous_mouse_state;
			AnyState m_current_keyboard_state;
			AnyState m_previous_keyboard_state;
		private:
			void AnyStateUpdate();
		public:
			Input(HWND hwnd);
			HRESULT Update(bool key_update = true, bool mouse_update = true, bool pad_update = true);

			//---------------
			//     マウス
			//---------------

			// Anyキーを保存するかどうか (Default:true)
			void SetMouseAnyUpdate(bool run);
			bool IsMouseAnyUpdateRun() const;

			bool IsMouseAnyHold() const;
			bool IsMouseAnyPressed() const;
			bool IsMouseAnyReleased() const;
			bool IsMouseHold(INPUT::MOUSE_BUTTONS button) const;
			bool IsMousePressed(INPUT::MOUSE_BUTTONS button) const;
			bool IsMouseReleased(INPUT::MOUSE_BUTTONS button) const;

			//ホイールの変位を取得
			LONG GetMouseWheel() const;

			//カーソルの移動量
			DirectX::XMINT2 GetMouseMove() const;
			//カーソルの座標
			DirectX::XMINT2 GetMousePos() const;
			//カーソルの座標
			DirectX::XMINT2 GetMousePos(HWND hwnd) const;
			//前回更新時のカーソルの座標
			DirectX::XMINT2 GetMouseOldPos() const;
			//前回更新時のカーソルの座標
			DirectX::XMINT2 GetMouseOldPos(HWND hwnd) const;

			//カーソルの表示/非表示を切り替える(trueで表示)
			void SetMouseVisible(bool visible);

			//---------------
			//   キーボード
			//---------------

			// Anyキーを保存するかどうか (Default:true)
			void SetKeyAnyUpdate(bool run);
			bool IsKeyAnyUpdateRun() const;

			bool IsKeyAnyHold() const;
			bool IsKeyAnyPressed() const;
			bool IsKeyAnyReleased() const;
			bool IsKeyHold(INPUT::KEYS button) const;
			bool IsKeyPressed(INPUT::KEYS button) const;
			bool IsKeyReleased(INPUT::KEYS button) const;
		};

		//--------------------------
		//     マウス：インライン実装
		//--------------------------
		inline void Input::SetMouseAnyUpdate(bool run)
		{
			m_current_mouse_state.any_run = run;
		}
		inline bool Input::IsMouseAnyUpdateRun() const
		{
			return m_current_mouse_state.any_run;
		}

		inline bool Input::IsMouseAnyHold() const
		{
			assert(m_current_mouse_state.any_run && "マウスのanyキーを保存していません。");
			return m_current_mouse_state.any_hold;
		}
		inline bool Input::IsMouseAnyPressed() const
		{
			assert(m_current_mouse_state.any_run && "マウスのanyキーを保存していません。");
			return m_current_mouse_state.any_pressed;
		}
		inline bool Input::IsMouseAnyReleased() const
		{
			assert(m_current_mouse_state.any_run && "マウスのanyキーを保存していません。");
			return m_current_mouse_state.any_released;
		}

		inline bool Input::IsMouseHold(INPUT::MOUSE_BUTTONS button) const
		{
			return m_direct_input.GetMouse()->IsHold(button);
		}
		inline bool Input::IsMousePressed(INPUT::MOUSE_BUTTONS button) const
		{
			return m_direct_input.GetMouse()->IsPressed(button);
		}
		inline bool Input::IsMouseReleased(INPUT::MOUSE_BUTTONS button) const
		{
			return m_direct_input.GetMouse()->IsReleased(button);
		}
		inline LONG Input::GetMouseWheel() const
		{
			return m_direct_input.GetMouse()->GetWheel();
		}
		inline DirectX::XMINT2 Input::GetMouseMove() const
		{
			return m_direct_input.GetMouse()->GetMove();
		}
		inline DirectX::XMINT2 Input::GetMousePos() const
		{
			return m_direct_input.GetMouse()->GetPos();
		}
		inline DirectX::XMINT2 Input::GetMousePos(HWND hwnd) const
		{
			return m_direct_input.GetMouse()->GetPos(hwnd);
		}
		inline DirectX::XMINT2 Input::GetMouseOldPos() const
		{
			return m_direct_input.GetMouse()->GetOldPos();
		}
		inline DirectX::XMINT2 Input::GetMouseOldPos(HWND hwnd) const
		{
			return m_direct_input.GetMouse()->GetOldPos(hwnd);
		}

		//--------------------------
		//   キーボード：インライン実装
		//--------------------------
		inline void Input::SetKeyAnyUpdate(bool run)
		{
			m_current_keyboard_state.any_run = run;
		}
		inline bool Input::IsKeyAnyUpdateRun() const
		{
			return m_current_keyboard_state.any_run;
		}

		inline bool Input::IsKeyAnyHold() const
		{
			assert(m_current_keyboard_state.any_run && "キーボードのanyキーを保存していません。");
			return m_current_keyboard_state.any_hold;
		}
		inline bool Input::IsKeyAnyPressed() const
		{
			assert(m_current_keyboard_state.any_run && "キーボードのanyキーを保存していません。");
			return m_current_keyboard_state.any_pressed;
		}
		inline bool Input::IsKeyAnyReleased() const
		{
			assert(m_current_keyboard_state.any_run && "キーボードのanyキーを保存していません。");
			return m_current_keyboard_state.any_released;
		}

		inline bool Input::IsKeyHold(INPUT::KEYS button) const
		{
			return m_direct_input.GetKeyBoard()->IsHold(button);
		}
		inline bool Input::IsKeyPressed(INPUT::KEYS button) const
		{
			return m_direct_input.GetKeyBoard()->IsPressed(button);
		}
		inline bool Input::IsKeyReleased(INPUT::KEYS button) const
		{
			return m_direct_input.GetKeyBoard()->IsReleased(button);
		}
	}
}