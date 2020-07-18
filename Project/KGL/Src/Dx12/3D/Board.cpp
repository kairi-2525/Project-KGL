#include <Dx12/3D/Board.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

Board::Board(const ComPtrC<ID3D12Device>& device) noexcept
{
	using namespace DirectX;

	auto hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vert_buff.GetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResource‚ÉŽ¸”s");

	m_vbv.BufferLocation = m_vert_buff->GetGPUVirtualAddress();
	m_vbv.SizeInBytes = sizeof(Vertices);
	m_vbv.StrideInBytes = sizeof(Vertex);

	Vertices vertices =
	{
		{
			{ { -0.5f, -0.5f, 0.0f }, { 0.f, 0.f, -1.f,  }, { 0.f, 1.f } },	// ¶ã
			{ { -0.5f, +0.5f, 0.0f }, { 0.f, 0.f, -1.f,  }, { 0.f, 0.f } },	// ¶‰º
			{ { +0.5f, -0.5f, 0.0f }, { 0.f, 0.f, -1.f,  }, { 1.f, 1.f } },	// ‰E‰º
			{ { +0.5f, +0.5f, 0.0f }, { 0.f, 0.f, -1.f,  }, { 1.f, 0.f } }	// ‰Eã
		}
	};

	Vertex* mapped_vertex = nullptr;
	hr = m_vert_buff->Map(0, nullptr, (void**)&mapped_vertex);
	RCHECK(FAILED(hr), "Board‚ÌMap‚ÉŽ¸”s");
	std::copy(vertices.cbegin(), vertices.cend(), mapped_vertex);
	m_vert_buff->Unmap(0, nullptr);
}

void Board::Render(const ComPtr<ID3D12GraphicsCommandList>& cmd_list, UINT index_count) const noexcept
{
	cmd_list->IASetVertexBuffers(0, 1, &m_vbv);
	cmd_list->DrawInstanced(4, index_count, 0, 0);
}