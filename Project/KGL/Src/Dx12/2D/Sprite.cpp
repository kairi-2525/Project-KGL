#include <Dx12/2D/Sprite.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

Sprite::Sprite(const ComPtrC<ID3D12Device>& device) noexcept
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
			{ { -1.f, -1.f, 0.1f }, { 0.f, 1.f } },	// ¶ã
			{ { -1.f, +1.f, 0.1f }, { 0.f, 0.f } },	// ¶ã
			{ { +1.f, -1.f, 0.1f }, { 1.f, 1.f } },	// ‰E‰º
			{ { +1.f, +1.f, 0.1f }, { 1.f, 0.f } }	// ‰Eã
		}
	};

	SetVertices(vertices);
}

HRESULT Sprite::SetVertices(const Vertices& vertices) const noexcept
{
	HRESULT hr = S_OK;
	Vertex* mapped_vertex = nullptr;
	hr = m_vert_buff->Map(0, nullptr, (void**)&mapped_vertex);
	RCHECK_HR(hr, "Sprite‚ÌMap‚ÉŽ¸”s");
	std::copy(vertices.cbegin(), vertices.cend(), mapped_vertex);
	m_vert_buff->Unmap(0, nullptr);
	return hr;
}

HRESULT Sprite::GetVertices(Vertices* p_vertices) const noexcept
{
	HRESULT hr = S_OK;
	RCHECK(!p_vertices, "p_vertices ‚ª nullptr", E_FAIL);
	Vertex* mapped_vertex = nullptr;
	hr = m_vert_buff->Map(0, nullptr, (void**)&mapped_vertex);
	RCHECK_HR(hr, "Sprite‚ÌMap‚ÉŽ¸”s");
	std::copy(&mapped_vertex[0], &mapped_vertex[p_vertices->size() - 1], &(*p_vertices)[0]);
	m_vert_buff->Unmap(0, nullptr);
	return hr;
}

HRESULT Sprite::SetPos(const RECT& rect, float screen_w, float screen_h) const noexcept
{
	Vertices vertices;
	HRESULT hr = GetVertices(&vertices);
	RCHECK_HR(hr, "GetVertices‚ÉŽ¸”s");

	float screen_left = (((std::min)(SCAST<float>(rect.left), screen_w) / screen_w) * 2) - 1.f;
	float screen_right = (((std::min)(SCAST<float>(rect.right), screen_w) / screen_w) * 2) - 1.f;
	float screen_top = (((std::min)(SCAST<float>(rect.top), screen_h) / screen_h) * 2) - 1.f;
	float screen_bottom = (((std::min)(SCAST<float>(rect.right), screen_h) / screen_h) * 2) - 1.f;

	vertices[0].position = { screen_left, screen_top,		0.1f };
	vertices[1].position = { screen_left, screen_bottom,	0.1f };
	vertices[2].position = { screen_right, screen_top,		0.1f };
	vertices[3].position = { screen_right, screen_bottom,	0.1f };

	return SetVertices(vertices);
}

void Sprite::Render(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept
{
	cmd_list->IASetVertexBuffers(0, 1, &m_vbv);
	cmd_list->DrawInstanced(4, 1, 0, 0);
}