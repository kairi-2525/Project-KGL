#include <Dx12/2D/Sprite.hpp>
#include <array>
#include <Dx12/2D/Renderer.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

Sprite::Sprite(const ComPtr<ID3D12Device>& device) noexcept
{
	using namespace DirectX;
	std::array<Vertex, 4> vertices =
	{
		{
			{ { -1.f, -1.f, 0.1f }, { 0.f, 1.f } },	// ¶ã
			{ { -1.f, +1.f, 0.1f }, { 0.f, 0.f } },	// ¶ã
			{ { +1.f, -1.f, 0.1f }, { 1.f, 1.f } },	// ‰E‰º
			{ { +1.f, +1.f, 0.1f }, { 1.f, 0.f } }	// ‰Eã
		} 
	};

	auto hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vert_buff.GetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResource‚ÉŽ¸”s");

	m_vbv.BufferLocation = m_vert_buff->GetGPUVirtualAddress();
	m_vbv.SizeInBytes = sizeof(vertices);
	m_vbv.StrideInBytes = sizeof(Vertex);

	Vertex* mapped_vertex = nullptr;
	m_vert_buff->Map(0, nullptr, (void**)&mapped_vertex);
	std::copy(vertices.begin(), vertices.end(), mapped_vertex);
	m_vert_buff->Unmap(0, nullptr);
}

void Sprite::Render(const ComPtr<ID3D12GraphicsCommandList>& cmd_list)
{
	cmd_list->IASetVertexBuffers(0, 1, &m_vbv);
	cmd_list->DrawInstanced(4, 1, 0, 0);
}