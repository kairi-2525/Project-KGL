#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <string>
#include <functional>
#include <DirectXMath.h>

namespace KGL
{
	inline namespace BASE
	{
		class Window;
		inline namespace WINDOW
		{
			using WindowProcFunc = std::function<LRESULT(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, Window* window)>;

			// プライマリモニター = メインのモニター
			extern DirectX::XMUINT2 GetPrimaryMonitorSize() noexcept;
		}


		class Window
		{
		public:
			struct Desc
			{
				std::string			title;
				DirectX::XMUINT2	size;
				DWORD				style;
				bool				adjust;
				HICON				icon;
			};
		public:
			static const DWORD		WINDOWED_STYLE;
			static const DWORD		FULLSCREEN_STYLE;

			static const Desc		FULLSCREEN_DESC;
			static const Desc		FULLHD_WINDOWED_DESC;
			static const Desc		HD_WINDOWED_DESC;
			static const Desc		FULLHD_WINDOWED_ADJ_DESC;
			static const Desc		HD_WINDOWED_ADJ_DESC;
			static const WORD		DEFAULT_ICON_NUMBER;
		private:
			Desc					m_desc;
			HWND					m_hwnd;
			WindowProcFunc			m_user_proc;
			bool					m_user_proc_return;
		protected:
			//ベースになるウィンドウプロシージャ。これから個別のWidProcを呼び出す。
			static LRESULT CALLBACK BaseWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) noexcept;
		protected:
			//個別のウィンドウプロシージャ
			LRESULT LocalWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) noexcept;
			//プロシージャ呼び出し用にポインターとhwndを関連付けする。
			void SetMyPointerToUserData() noexcept;
			HRESULT Create() noexcept;
		private:
			Window() = delete;
			Window(const Window&) = delete;
			Window& operator=(const Window&) = delete;
		public:
			explicit Window(const Desc& desc) noexcept;
			virtual ~Window() = default;

			//成功時true
			virtual bool Update() noexcept;

			HWND GetHWND() const noexcept;
			BOOL SetTitle(const std::string& title) noexcept;
			std::string GetTitle() const noexcept;

			//ユーザー用のウィンドウプロシージャを設定する。(第二引数をtrueにするとこのプロシージャの戻り値を使用する。)
			void SetUserProc(const WindowProcFunc& proc, bool use_return = false) noexcept;
			void ResetUserProc() noexcept;

			//表示状態を変更する(SW_SHOWDEFAULTでウィンドウを前面に展開する。)
			BOOL Show(int show_cmd = SW_SHOWDEFAULT) const noexcept;

			static DirectX::XMUINT2 GetWindowSize(HWND hwnd) noexcept;
			DirectX::XMUINT2 GetWindowSize() noexcept;
			static DirectX::XMUINT2 GetClientSize(HWND hwnd) noexcept;
			DirectX::XMUINT2 GetClientSize() noexcept;
			static RECT GetWindowRect(HWND hwnd) noexcept;
			RECT GetWindowRect() noexcept;
			static RECT GetClientRect(HWND hwnd) noexcept;
			RECT GetClientRect() noexcept;
		};
	}
}

namespace KGL
{
	inline void Window::SetMyPointerToUserData() noexcept
	{
		SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	}
	inline HWND Window::GetHWND() const noexcept
	{
		return m_hwnd;
	}
	inline BOOL Window::SetTitle(const std::string& title) noexcept
	{
		m_desc.title = title;
		return SetWindowTextA(m_hwnd, m_desc.title.c_str());
	}
	inline std::string Window::GetTitle() const noexcept
	{
		return m_desc.title;
	}
	inline void Window::SetUserProc(const WindowProcFunc& proc, bool use_return) noexcept
	{
		m_user_proc = proc;
		m_user_proc_return = use_return;
	}
	inline void Window::ResetUserProc() noexcept
	{
		m_user_proc_return = false;
	}
	inline BOOL Window::Show(int show_cmd) const noexcept
	{
		return ShowWindow(m_hwnd, show_cmd);
	}
	inline DirectX::XMUINT2 Window::GetWindowSize() noexcept
	{
		return GetWindowSize(m_hwnd);
	}
	inline DirectX::XMUINT2 Window::GetClientSize() noexcept
	{
		return GetClientSize(m_hwnd);
	}
	inline RECT Window::GetWindowRect() noexcept
	{
		return GetWindowRect(m_hwnd);
	}
	inline RECT Window::GetClientRect() noexcept
	{
		return GetClientRect(m_hwnd);
	}
}