#pragma once

#include <Dx12/3D/PMDActor.hpp>

class Obj3D : public KGL::PMD_Actor
{
public:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 scale;
	DirectX::XMFLOAT3 rotation;

	Obj3D(
		const KGL::ComPtr<ID3D12Device>& device,
		const KGL::PMD_Model& model
	) noexcept : 
		KGL::PMD_Actor(device, model),
		position(0.f, 0.f, 0.f),
		scale(1.f, 1.f, 1.f),
		rotation(0.f, 0.f, 0.f)
	{}
	void Update(float elapsed_time);
};