#include <Base/Window.hpp>
#include <Helper/Cast.hpp>
#include <Helper/ThrowAssert.hpp>
#include <atlcomcli.h>
#ifdef DEBUG
#undef DEBUG
#endif

using namespace KGL;

DirectX::XMUINT2 WINDOW::GetPrimaryMonitorSize() noexcept
{
	return {
			static_cast<unsigned int>(GetSystemMetrics(SM_CXSCREEN)),
			static_cast<unsigned int>(GetSystemMetrics(SM_CYSCREEN)),
	};
}

const DWORD Window::WINDOWED_STYLE = WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME | WS_VISIBLE;
const DWORD Window::FULLSCREEN_STYLE = WS_VISIBLE | WS_POPUP;

const Window::Desc Window::FULLSCREEN_DESC =
	{ "no title", { 0u, 0u },		FULLSCREEN_STYLE,	true };
const Window::Desc Window::FULLHD_WINDOWED_DESC =
	{ "no title", { 1920u, 1080u },	WINDOWED_STYLE,		false };
const Window::Desc Window::HD_WINDOWED_DESC =
	{ "no title", { 1280u, 720u },	WINDOWED_STYLE,		false };
const Window::Desc Window::FULLHD_WINDOWED_ADJ_DESC =
	{ "no title", { 1920u, 1080u }, WINDOWED_STYLE,		true };
const Window::Desc Window::HD_WINDOWED_ADJ_DESC =
	{ "no title", { 1280u, 720u },	WINDOWED_STYLE,		true };

const WORD Window::DEFAULT_ICON_NUMBER = 333;

LRESULT CALLBACK Window::BaseWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) noexcept
{
	//Window::SetPointerでセットしたthisポインタを取得
	Window* window = (Window*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (window)
	{
		return window->LocalWndProc(hwnd, msg, wp, lp);
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT Window::LocalWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) noexcept
{
	switch (msg) {
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			break;
		}
	}
	LRESULT def_result = 0, result = 0;
	if (m_user_proc) result = m_user_proc(hwnd, msg, wp, lp, this);
	def_result = DefWindowProc(hwnd, msg, wp, lp);
	if (m_user_proc && m_user_proc_return) return result;
	return def_result;
}

Window::Window(const Desc& desc) noexcept
{
	m_desc = desc;
	if (m_desc.size.x == 0u || m_desc.size.y == 0u)
		m_desc.size = GetPrimaryMonitorSize();
	ResetUserProc();
	auto hr = Create();
	assert(SUCCEEDED(hr) && "ウィンドウの作成に失敗");
}

HRESULT Window::Create() noexcept
{
	HRESULT hr = S_OK;

	WNDCLASSEX wcex{};
	HINSTANCE h_instance = (HINSTANCE)GetModuleHandle(NULL);
	wcex.cbSize = sizeof(WNDCLASSEX);   //この構造体自体のサイズなので。
	wcex.cbClsExtra = 0;    //拡張用？とりあえず使わないので0
	wcex.cbWndExtra = 0;    //拡張用？とりあえず使わないので0
	wcex.lpfnWndProc = BaseWndProc;//プロシージャを指定。
	wcex.hInstance = h_instance;
	wcex.lpszClassName = m_desc.title.c_str();
	wcex.lpszMenuName = nullptr;    //メニューの名前。使わないならとりあえずNULLでOK
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.hCursor = ::LoadCursor(h_instance, IDC_ARROW);      //カーソルアイコン

	if (m_desc.icon)
	{
		wcex.hIcon = m_desc.icon;    //プログラムアイコン
		wcex.hIconSm = m_desc.icon;  //プログラムアイコン（小）::タイトルバーに使われるやつ？
	}
	else
	{
		wcex.hIcon = LoadIcon(h_instance, MAKEINTRESOURCE(DEFAULT_ICON_NUMBER));    //プログラムアイコン
		if (!wcex.hIcon) wcex.hIcon = ::LoadIcon(h_instance, IDI_APPLICATION);
		wcex.hIconSm = LoadIcon(h_instance, MAKEINTRESOURCE(DEFAULT_ICON_NUMBER));  //プログラムアイコン（小）::タイトルバーに使われるやつ？
		if (!wcex.hIconSm) wcex.hIconSm = ::LoadIcon(h_instance, IDI_APPLICATION);
	}

	wcex.hbrBackground = (HBRUSH)::GetStockObject(NULL_BRUSH);      //クライアント領域の塗りつぶし色（ブラシ）
	if (!RegisterClassEx(&wcex)) {
		const DWORD error_code = GetLastError();
		//既に存在しますというエラーは許容する
		if (error_code != 1410)
		{
			hr = HRESULT_FROM_WIN32(error_code);
			if (FAILED(hr))
			{
				assert(!"[ウィンドウの生成に問題が発生]RegisterClassEx(&wcex)に失敗しました。");
				return hr;
			}
		}
	}

	RECT rc = { 0, 0, (LONG)m_desc.size.x, (LONG)m_desc.size.y };
	if (m_desc.adjust) AdjustWindowRect(&rc, m_desc.style, FALSE);

	LONG window_x = rc.right - rc.left;
	LONG window_y = rc.bottom - rc.top;
	LONG start_x = CW_USEDEFAULT;
	LONG start_y = CW_USEDEFAULT;

	const auto pm_size = GetPrimaryMonitorSize();
	if (pm_size.x < SCAST<uint32_t>(window_x))
	{
		start_x = -((window_x - SCAST<LONG>(pm_size.x)) / 2);
		start_y = 0;
	}

	m_hwnd = CreateWindow(
		m_desc.title.c_str(),
		m_desc.title.c_str(),
		m_desc.style | WS_MINIMIZE,    //ウィンドウスタイル。とりあえずデフォルトな感じで
		start_x, start_y,   //初期位置。適当にやってくれる。
		rc.right - rc.left, rc.bottom - rc.top,      //ウィンドウサイズ
		nullptr,    //親ウィンドウのハンドル。特にないんで今回はNULL
		nullptr,    //メニューハンドル。特にないので今回はNULL
		h_instance,
		this    //トリックの肝。CreateParameterに設定
	);
	if (!m_hwnd)
	{
		assert(!"[ウィンドウの生成に問題が発生]CreateWindowに失敗しました。");
		return E_FAIL;
	}

	UnregisterClass(wcex.lpszClassName, wcex.hInstance);

	SetMyPointerToUserData();
	return hr;
}

bool Window::Update() noexcept
{
	MSG msg{};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		//終了処理
		if (WM_QUIT == msg.message)
		{
			return false;
		}
	}
	//更新処理
	if (WM_QUIT != msg.message)
	{
		return true;
	}
	return false;
}

DirectX::XMUINT2 Window::GetWindowSize(HWND hwnd) noexcept
{
	RECT rc;
	::GetWindowRect(hwnd, &rc);
	return { SCAST<UINT>(rc.right - rc.left), SCAST<UINT>(rc.bottom - rc.top) };
}
DirectX::XMUINT2 Window::GetClientSize(HWND hwnd) noexcept
{
	RECT rc;
	::GetClientRect(hwnd, &rc);
	return { SCAST<UINT>(rc.right), SCAST<UINT>(rc.bottom) };
}
RECT Window::GetWindowRect(HWND hwnd) noexcept
{
	WINDOWINFO info = {};
	::GetWindowInfo(hwnd, &info);

	return info.rcWindow;
}
RECT Window::GetClientRect(HWND hwnd) noexcept
{
	WINDOWINFO info = {};
	::GetWindowInfo(hwnd, &info);

	return info.rcClient;
}