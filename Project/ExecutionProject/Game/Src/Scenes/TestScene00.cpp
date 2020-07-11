#include "../../Hrd/Scenes/TestScene00.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>

HRESULT TestScene00::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	hr = KGL::HELPER::CreateCommandAllocatorAndList(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);

	tex_blur_w = std::make_shared<KGL::Texture>(device, desc.app->GetRtvBuffers().at(0), DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f));
	tex_blur_h = std::make_shared<KGL::Texture>(device, desc.app->GetRtvBuffers().at(0), DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f));
	{
		std::vector<KGL::ComPtr<ID3D12Resource>> resources(2);
		resources[0] = tex_blur_w->Data();
		resources[1] = tex_blur_h->Data();
		texture_rtv = std::make_shared<KGL::RenderTargetView>(device, resources);
	}
	pmd_data = std::make_shared<KGL::PMD_Loader>("./Assets/Models/初音ミク.pmd");
	vmd_data = std::make_shared<KGL::VMD_Loader>("./Assets/Motions/motion.vmd");
	pmd_toon_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc(), "./Assets/Toons", &tex_mgr);
	pmd_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc(), &tex_mgr);
	pmd_renderer = std::make_shared<KGL::PMD_Renderer>(device);

	renderer_sprite = std::make_shared<KGL::_2D::Renderer>(device);
	{
		auto add_ranges = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		std::vector<D3D12_ROOT_PARAMETER> add_rootparam(1);
		{
			CD3DX12_ROOT_PARAMETER root_param;
			root_param.InitAsDescriptorTable(1, &add_ranges);
			add_rootparam[0] = root_param;
		}
		renderer_blur_w = std::make_shared<KGL::_2D::Renderer>(
			device,
			KGL::BDTYPE::DEFAULT,
			KGL::_2D::Renderer::VS_DESC,
			KGL::Shader::Desc{ "./HLSL/2D/GaussianBlurW_ps.hlsl", "PSMain", "ps_5_0" },
			KGL::_2D::Renderer::INPUT_LAYOUTS,
			std::vector<D3D12_DESCRIPTOR_RANGE>(),
			add_rootparam
			);
		renderer_blur_h = std::make_shared<KGL::_2D::Renderer>(
			device,
			KGL::BDTYPE::DEFAULT,
			KGL::_2D::Renderer::VS_DESC,
			KGL::Shader::Desc{ "./HLSL/2D/GaussianBlurH_ps.hlsl", "PSMain", "ps_5_0" },
			KGL::_2D::Renderer::INPUT_LAYOUTS,
			std::vector<D3D12_DESCRIPTOR_RANGE>(),
			add_rootparam
			);
	}
	sprite = std::make_shared<KGL::Sprite>(device);

	models.resize(1, { device, *pmd_model });
	for (auto& model : models)
	{
		model.SetAnimation(vmd_data->GetDesc());
	}

	{
		blur_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
		blur_buff_handle = blur_desc_mgr->Alloc(device);

		// ガウス処理をあらかじめ計算しておく
		const auto& weights = KGL::MATH::GetGaussianWeights(8u, 5.f);

		const auto buff_size = ((weights.size() * sizeof(weights[0])) + 0xff) & ~0xff;
		hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(buff_size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(blur_const_buff.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateCommittedResourceに失敗", hr);
		float* mapped_weight = nullptr;
		hr = blur_const_buff->Map(0, nullptr, (void**)&mapped_weight);
		RCHECK(FAILED(hr), "blur_const_buff->Mapに失敗", hr);
		std::copy(weights.cbegin(), weights.cend(), mapped_weight);
		blur_const_buff->Unmap(0, nullptr);

		D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbv_desc = {};
		mat_cbv_desc.BufferLocation = blur_const_buff->GetGPUVirtualAddress();
		mat_cbv_desc.SizeInBytes = buff_size;
		device->CreateConstantBufferView(&mat_cbv_desc, blur_buff_handle.Cpu());
	}


	return hr;
}

HRESULT TestScene00::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera.eye = { 0.f, 10.f, -15.f };
	camera.focus_vec = { 0.f, 0.f, 15.f };
	camera.up = { 0.f, 1.f, 0.f };

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// アスペクト比
		1.0f, 100.0f // near, far
	);

	const auto& device = desc.app->GetDevice();

	for (auto& model : models)
	{
		model.SetViewProjection(
			KGL::CAMERA::GetView(camera), proj_mat
		);
		model.SetEye(camera.eye);
	}

	clear_color = { 1.f, 1.f, 1.f, 1.f };

	return S_OK;
}

HRESULT TestScene00::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;

	for (auto& model : models)
	{
		//model.position.x -= elapsed_time * ((rand() % (20 + 1)) - 10);
		//model.position.z -= elapsed_time * ((rand() % (20 + 1)) - 10);
		model.MotionUpdate(elapsed_time, true);
		model.Update(elapsed_time);
	}

	return Render(desc);
}

HRESULT TestScene00::Render(const SceneDesc& desc)
{
	using KGL::SCAST;
	HRESULT hr = S_OK;

	auto window_size = desc.window->GetClientSize();

	D3D12_VIEWPORT viewport = {};
	viewport.Width = SCAST<FLOAT>(window_size.x);
	viewport.Height = SCAST<FLOAT>(window_size.y);
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値

	auto scissorrect = CD3DX12_RECT(
		0, 0,
		window_size.x, window_size.y
	);

	{	// テクスチャ(W)にモデルを描画
		cmd_list->ResourceBarrier(1, &texture_rtv->GetRtvResourceBarrier(true, 0u));
		texture_rtv->Set(cmd_list, &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart(), 0u);
		texture_rtv->Clear(cmd_list, DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f), 0u);
		desc.app->ClearDsv(cmd_list);

		cmd_list->RSSetViewports(1, &viewport);
		cmd_list->RSSetScissorRects(1, &scissorrect);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		pmd_renderer->SetState(cmd_list);
		for (auto& model : models)
		{
			model.Render(cmd_list);

			hr = pmd_toon_model->Render(
				desc.app->GetDevice(),
				cmd_list
			);
			RCHECK(FAILED(hr), "pmd_model->Renderに失敗", hr);
		}

		cmd_list->ResourceBarrier(1, &texture_rtv->GetRtvResourceBarrier(false, 0u));
	}
	{	// テクスチャ(H)にテクスチャ(W)を描画
		cmd_list->ResourceBarrier(1, &texture_rtv->GetRtvResourceBarrier(true, 1u));
		texture_rtv->Set(cmd_list, nullptr, 1u);
		texture_rtv->Clear(cmd_list, DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f), 1u);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		renderer_blur_w->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, texture_rtv->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, texture_rtv->GetSRVGPUHandle(0u));
		cmd_list->SetDescriptorHeaps(1, blur_buff_handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(1, blur_buff_handle.Gpu());
		sprite->Render(cmd_list);

		cmd_list->ResourceBarrier(1, &texture_rtv->GetRtvResourceBarrier(false, 1u));
	}
	{	// モデルを描画したテクスチャをSwapchainのレンダーターゲットに描画
		desc.app->SetRtv(cmd_list);
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtv(cmd_list, clear_color);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		renderer_blur_h->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, texture_rtv->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, texture_rtv->GetSRVGPUHandle(1u));
		cmd_list->SetDescriptorHeaps(1, blur_buff_handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(1, blur_buff_handle.Gpu());

		sprite->Render(cmd_list);

		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
	}

	cmd_list->Close();
	ID3D12CommandList* cmd_lists[] = { cmd_list.Get() };
	desc.app->GetQueue()->Data()->ExecuteCommandLists(1, cmd_lists);
	desc.app->GetQueue()->Signal();
	desc.app->GetQueue()->Wait();

	cmd_allocator->Reset();
	cmd_list->Reset(cmd_allocator.Get(), nullptr);

	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);

	return hr;
}

HRESULT TestScene00::UnInit(const SceneDesc& desc)
{
	return S_OK;
}