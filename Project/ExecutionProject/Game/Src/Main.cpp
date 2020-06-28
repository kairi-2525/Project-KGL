#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <Window.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Dx12/Application.hpp>

#ifdef _DEBUG
#define DEBUG_LAYER (true)
#else
#define DEBUG_LAYER (false)
#endif

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	KGL::UseComPtr com;
	{
		KGL::Window window(KGL::Window::HD_WINDOWED_DESC);
		window.Show();
		{
			KGL::App app(window.GetHWND(), DEBUG_LAYER);

			while (window.Update())
			{

			}
		}
	}

	return 0;
};