#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <Base/Window.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Timer.hpp>
#include <Dx12/Application.hpp>
#include <DirectXTex/d3dx12.h>
#include <Base/Input.hpp>

#include "../Hrd/Scene.hpp"
#include "../Hrd/Scenes/TestScene04.hpp"
#include "../Hrd/Scenes/LoadScene00.hpp"

#include "../Hrd/SceneGame.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <JIS_X_0208.h>
#include <Dx12/DescriptorHeap.hpp>

#ifdef _DEBUG
#define DEBUG_LAYER (true)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#define DEBUG_LAYER (false)
#endif

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KGL::Window* window)
{
	return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
}

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
#ifdef _DEBUG
		std::shared_ptr<KGL::Window> window = std::make_shared<KGL::Window>(KGL::Window::FULLSCREEN_DESC);
		// std::shared_ptr<KGL::Window> window = std::make_shared<KGL::Window>(KGL::Window::HD_WINDOWED_ADJ_DESC);
#else
		std::shared_ptr<KGL::Window> window = std::make_shared<KGL::Window>(KGL::Window::FULLSCREEN_DESC);
#endif
		std::shared_ptr<KGL::Input> input = std::make_shared<KGL::Input>(window->GetHWND());

		KGL::RefreshRate fps_counter;
		window->Show();

		std::shared_ptr<KGL::DXC>	dxc;
#ifdef _WIN64
		SetDllDirectoryA("./DLL/x64");
#else
		SetDllDirectoryA("./DLL/Win32");
#endif
		dxc = std::make_shared<KGL::DXC>("./dxcompiler.dll");
		ComPtr<ID3D12Device> device;
		{
			HRESULT hr = S_OK;
			std::shared_ptr<KGL::App> app = std::make_shared<KGL::App>(window->GetHWND(), DEBUG_LAYER, false);
#if _CONSOLE
			{
				std::filesystem::path path = app->GetDesc().Description;
				KGLDebugOutPutString("ÉAÉ_ÉvÉ^Å[èÓïÒ : " + path.string());
			}
#endif
			device = app->GetDevice();
			{
				std::shared_ptr<KGL::DescriptorManager> imgui_heap;
				KGL::DescriptorHandle imgui_handle;
				{
					imgui_heap = std::make_shared<KGL::DescriptorManager>(device, 1000u);
					imgui_handle = imgui_heap->Alloc();

					ImGuiContext* context_result = ImGui::CreateContext();
					RCHECK(context_result == nullptr, "ImGui::CreateContextÇ…é∏îs", -1);

					bool bln_result = ImGui_ImplWin32_Init(window->GetHWND());
					RCHECK(!bln_result, "ImGui_ImplWin32_InitÇ…é∏îs", -1);

					bln_result = ImGui_ImplDX12_Init(
						app->GetDevice().Get(), 3,
						DXGI_FORMAT_R8G8B8A8_UNORM,
						imgui_handle.Heap().Get(),
						imgui_handle.Cpu(), imgui_handle.Gpu()
					);
					RCHECK(!bln_result, "ImGui_ImplDX12_InitÇ…é∏îs", -1);

					window->SetUserProc(&WindowProc);

					ImGuiIO& io = ImGui::GetIO();
					io.Fonts->AddFontFromFileTTF("./Assets/Fonts/APJapanesefont.ttf", 20.0f, nullptr, glyphRangesJapanese);
				}

				SceneManager scene_mgr;

				RCHECK(FAILED(hr), "ÉVÅ[ÉìÇÃèâä˙âªÇ…é∏îs", -1);

				SceneDesc scene_desc = { app, window, input, imgui_heap, imgui_handle, dxc };
				hr = scene_mgr.Init<LoadScene00<TestScene04>>(scene_desc);

				DirectX::XMFLOAT4 clear_color = { 0.f, 0.f, 0.f, 1.f };
				HRESULT scene_hr = S_OK;
				while (window->Update())
				{
					auto io = ImGui::GetIO();
					input->Update(!io.WantCaptureKeyboard, !io.WantCaptureMouse);
					fps_counter.Update();
					window->SetTitle("FPS : [" + std::to_string(fps_counter.GetRefreshRate()) + "]");
					scene_hr = scene_mgr.Update(scene_desc, fps_counter.GetElpasedTime());

					if (FAILED(scene_hr))
						break;
					
					scene_hr = scene_mgr.SceneChangeUpdate(scene_desc);
				}
				scene_mgr.UnInit(scene_desc, nullptr);
			}
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}
#if DEBUG_LAYER
		ComPtr<ID3D12DebugDevice> debug_interface;
		HRESULT hr = device->QueryInterface(IID_PPV_ARGS(debug_interface.ReleaseAndGetAddressOf()));
		debug_interface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
#endif
	}

	return 0;
};