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
	//Window::SetPointer�ŃZ�b�g����this�|�C���^���擾
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
	assert(SUCCEEDED(hr) && "�E�B���h�E�̍쐬�Ɏ��s");
}

HRESULT Window::Create() noexcept
{
	HRESULT hr = S_OK;

	WNDCLASSEX wcex{};
	HINSTANCE h_instance = (HINSTANCE)GetModuleHandle(NULL);
	wcex.cbSize = sizeof(WNDCLASSEX);   //���̍\���̎��̂̃T�C�Y�Ȃ̂ŁB
	wcex.cbClsExtra = 0;    //�g���p�H�Ƃ肠�����g��Ȃ��̂�0
	wcex.cbWndExtra = 0;    //�g���p�H�Ƃ肠�����g��Ȃ��̂�0
	wcex.lpfnWndProc = BaseWndProc;//�v���V�[�W�����w��B
	wcex.hInstance = h_instance;
	wcex.lpszClassName = m_desc.title.c_str();
	wcex.lpszMenuName = nullptr;    //���j���[�̖��O�B�g��Ȃ��Ȃ�Ƃ肠����NULL��OK
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.hCursor = ::LoadCursor(h_instance, IDC_ARROW);      //�J�[�\���A�C�R��

	if (m_desc.icon)
	{
		wcex.hIcon = m_desc.icon;    //�v���O�����A�C�R��
		wcex.hIconSm = m_desc.icon;  //�v���O�����A�C�R���i���j::�^�C�g���o�[�Ɏg�����H
	}
	else
	{
		wcex.hIcon = LoadIcon(h_instance, MAKEINTRESOURCE(DEFAULT_ICON_NUMBER));    //�v���O�����A�C�R��
		if (!wcex.hIcon) wcex.hIcon = ::LoadIcon(h_instance, IDI_APPLICATION);
		wcex.hIconSm = LoadIcon(h_instance, MAKEINTRESOURCE(DEFAULT_ICON_NUMBER));  //�v���O�����A�C�R���i���j::�^�C�g���o�[�Ɏg�����H
		if (!wcex.hIconSm) wcex.hIconSm = ::LoadIcon(h_instance, IDI_APPLICATION);
	}

	wcex.hbrBackground = (HBRUSH)::GetStockObject(NULL_BRUSH);      //�N���C�A���g�̈�̓h��Ԃ��F�i�u���V�j
	if (!RegisterClassEx(&wcex)) {
		const DWORD error_code = GetLastError();
		//���ɑ��݂��܂��Ƃ����G���[�͋��e����
		if (error_code != 1410)
		{
			hr = HRESULT_FROM_WIN32(error_code);
			if (FAILED(hr))
			{
				assert(!"[�E�B���h�E�̐����ɖ�肪����]RegisterClassEx(&wcex)�Ɏ��s���܂����B");
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
		m_desc.style | WS_MINIMIZE,    //�E�B���h�E�X�^�C���B�Ƃ肠�����f�t�H���g�Ȋ�����
		start_x, start_y,   //�����ʒu�B�K���ɂ���Ă����B
		rc.right - rc.left, rc.bottom - rc.top,      //�E�B���h�E�T�C�Y
		nullptr,    //�e�E�B���h�E�̃n���h���B���ɂȂ���ō����NULL
		nullptr,    //���j���[�n���h���B���ɂȂ��̂ō����NULL
		h_instance,
		this    //�g���b�N�̊́BCreateParameter�ɐݒ�
	);
	if (!m_hwnd)
	{
		assert(!"[�E�B���h�E�̐����ɖ�肪����]CreateWindow�Ɏ��s���܂����B");
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

		//�I������
		if (WM_QUIT == msg.message)
		{
			return false;
		}
	}
	//�X�V����
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