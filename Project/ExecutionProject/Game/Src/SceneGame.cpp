#include "../Hrd/SceneGame.hpp"
#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>

HRESULT SceneGame::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;

	const auto& device = desc.app->GetDevice();
	texture = std::make_shared<KGL::Texture>(device, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff);
	pmd_data = std::make_shared<KGL::PMD_Loader>("./Assets/Models/�����~�N.pmd");
	pmd_toon_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc(), "./Assets/Toons", &tex_mgr);
	pmd_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc(), &tex_mgr);
	pmd_renderer = std::make_shared<KGL::PMD_Renderer>(device);

	models.resize(1, { device, pmd_model->GetBoneMatrices() });
	for (auto& model : models)
	{
		{
			const auto& node = pmd_data->GetDesc().bone_node_table.at("���r");
			const auto& pos = node.start_pos;
			model.GetMappedBuffers()->bones[node.bone_idx] =
				XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
				* XMMatrixRotationZ(XM_PIDIV2)
				* XMMatrixTranslation(pos.x, pos.y, pos.z);
		}
		{
			const auto& node = pmd_data->GetDesc().bone_node_table.at("���Ђ�");
			const auto& pos = node.start_pos;

			model.GetMappedBuffers()->bones[node.bone_idx] =
				XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
				* XMMatrixRotationZ(-XM_PIDIV2)
				* XMMatrixTranslation(pos.x, pos.y, pos.z);
		}
		const auto& node = pmd_data->GetDesc().bone_node_table.at("�Z���^�[");
		model.RecursiveMatrixMultiply(
			&node,
			XMMatrixIdentity()
		);
	}

	return hr;
}

HRESULT SceneGame::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera.eye = { 0.f, 10.f, -15.f };
	camera.focus_vec = { 0.f, 0.f, 15.f };
	camera.up = { 0.f, 1.f, 0.f };

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// �A�X�y�N�g��
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

	return S_OK;
}

HRESULT SceneGame::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;

	for (auto& model : models)
	{
		//model.position.x -= elapsed_time * ((rand() % (20 + 1)) - 10);
		//model.position.z -= elapsed_time * ((rand() % (20 + 1)) - 10);
		model.Update();
	}

	return S_OK;
}

HRESULT SceneGame::Render(const SceneDesc& desc, const KGL::ComPtr<ID3D12GraphicsCommandList>& cmd_list)
{
	using KGL::SCAST;

	auto window_size = desc.window->GetClientSize();

	D3D12_VIEWPORT viewport = {};
	viewport.Width = SCAST<FLOAT>(window_size.x);
	viewport.Height = SCAST<FLOAT>(window_size.y);
	viewport.TopLeftX = 0;//�o�͐�̍�����WX
	viewport.TopLeftY = 0;//�o�͐�̍�����WY
	viewport.MaxDepth = 1.0f;//�[�x�ő�l
	viewport.MinDepth = 0.0f;//�[�x�ŏ��l

	auto scissorrect = CD3DX12_RECT(
		0, 0,
		window_size.x, window_size.y
	);

	cmd_list->RSSetViewports(1, &viewport);
	cmd_list->RSSetScissorRects(1, &scissorrect);

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pmd_renderer->SetState(cmd_list);

	HRESULT hr = S_OK;
	for (auto& model : models)
	{
		model.Render(cmd_list);

		hr = pmd_toon_model->Render(
			desc.app->GetDevice(),
			cmd_list
		);
		RCHECK(FAILED(hr), "pmd_model->Render�Ɏ��s", hr);
	}

	return hr;
}

HRESULT SceneGame::UnInit(const SceneDesc& desc)
{
	return S_OK;
}