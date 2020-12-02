#pragma once

#include <DirectXMath.h>

namespace KGL
{
	inline namespace BASE
	{
		struct Actor
		{
			DirectX::XMFLOAT3					position;
			DirectX::XMFLOAT3					scale;
			DirectX::XMFLOAT3					rotate;

		public:
			Actor() noexcept : position(0.f, 0.f, 0.f), scale(1.f, 1.f, 1.f), rotate(0.f, 0.f, 0.f) {}
			// ÉèÅ[ÉãÉhçsóÒÇéÊìæ
			_NODISCARD DirectX::XMMATRIX GetWorldMatrix() const noexcept
			{
				using namespace DirectX;
				XMMATRIX S, R, T;
				S = XMMatrixScaling(scale.x, scale.y, scale.z);
				R = XMMatrixRotationRollPitchYaw(rotate.x, rotate.y, rotate.z);
				T = XMMatrixTranslation(position.x, position.y, position.z);
				return S * R * T;
			}
		};
	}
}