#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <Base/Window.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Timer.hpp>
#include <Dx12/Application.hpp>
#include <DirectXTex/d3dx12.h>

#include "../Hrd/Scene.hpp"
#include "../Hrd/Scenes/TestScene00.hpp"
#include "../Hrd/SceneGame.hpp"

#ifdef _DEBUG
#define DEBUG_LAYER (true)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#define DEBUG_LAYER (false)
#endif

#ifdef _CONSOLE
int main()
#else
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
#endif
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

	KGL::UseComPtr com;
	{
		using KGL::ComPtr;

		std::shared_ptr<KGL::Window> window = std::make_shared<KGL::Window>(KGL::Window::HD_WINDOWED_DESC);
		KGL::RefreshRate fps_counter;
		window->Show();

		ComPtr<ID3D12Device> device;
		{
			HRESULT hr = S_OK;
			std::shared_ptr<KGL::App> app = std::make_shared<KGL::App>(window->GetHWND(), DEBUG_LAYER);
			{
				SceneManager scene_mgr;

				RCHECK(FAILED(hr), "シーンの初期化に失敗", -1);

				device = app->GetDevice();

				SceneDesc scene_desc = { app, window };
				hr = scene_mgr.Init<TestScene00>(scene_desc);

				DirectX::XMFLOAT4 clear_color = { 0.f, 0.f, 0.f, 1.f };
				HRESULT scene_hr = S_OK;
				while (window->Update())
				{
					fps_counter.Update();
					window->SetTitle("FPS : [" + std::to_string(fps_counter.GetRefreshRate()) + "]");
					scene_hr = scene_mgr.Update(scene_desc, fps_counter.GetElpasedTime());

					if (FAILED(scene_hr))
						break;
					
					scene_hr = scene_mgr.SceneChangeUpdate(scene_desc);
				}
				scene_mgr.UnInit(scene_desc);
			}
		}
#if DEBUG_LAYER
		ComPtr<ID3D12DebugDevice> debug_interface;
		HRESULT hr = device->QueryInterface(IID_PPV_ARGS(debug_interface.ReleaseAndGetAddressOf()));
		debug_interface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
#endif
	}

	return 0;
};