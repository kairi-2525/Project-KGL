#include <Dx12/3D/Board.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

Board::Board(const ComPtrC<ID3D12Device>& device) noexcept
{
	using namespace DirectX;

	const auto& heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto& res_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertices));

	auto hr = device->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vert_buff.GetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResourceに失敗");

	m_vbv.BufferLocation = m_vert_buff->GetGPUVirtualAddress();
	m_vbv.SizeInBytes = sizeof(Vertices);
	m_vbv.StrideInBytes = sizeof(Vertex);

	Vertices vertices =
	{
		{
			{ { -0.5f, -0.5f, 0.0f }, { 0.f, 0.f, -1.f,  }, { 0.f, 1.f } },	// 左上
			{ { -0.5f, +0.5f, 0.0f }, { 0.f, 0.f, -1.f,  }, { 0.f, 0.f } },	// 左下
			{ { +0.5f, -0.5f, 0.0f }, { 0.f, 0.f, -1.f,  }, { 1.f, 1.f } },	// 右下
			{ { +0.5f, +0.5f, 0.0f }, { 0.f, 0.f, -1.f,  }, { 1.f, 0.f } }	// 右上
		}
	};

	Vertex* mapped_vertex = nullptr;
	hr = m_vert_buff->Map(0, nullptr, (void**)&mapped_vertex);
	RCHECK(FAILED(hr), "BoardのMapに失敗");
	std::copy(vertices.cbegin(), vertices.cend(), mapped_vertex);
	m_vert_buff->Unmap(0, nullptr);
}

void Board::Render(const ComPtr<ID3D12GraphicsCommandList>& cmd_list, UINT index_count) const noexcept
{
	cmd_list->IASetVertexBuffers(0, 1, &m_vbv);
	cmd_list->DrawInstanced(4, index_count, 0, 0);
}