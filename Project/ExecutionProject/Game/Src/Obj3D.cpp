#include "../Hrd/Obj3D.hpp"

void Obj3D::Update()
{
	using namespace DirectX;

	XMMATRIX S, R, T;
	S = XMMatrixScaling(scale.x, scale.y, scale.z);
	R = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	T = XMMatrixTranslation(position.x, position.y, position.z);

	m_map_buffers->world = S * R * T;
	PMD_Actor::UpdateWVP();
}