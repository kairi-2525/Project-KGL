#pragma once

#include <DirectXMath.h>

namespace KGL
{
	inline namespace MATH
	{
		inline float CalcAngle2(DirectX::CXMVECTOR a, DirectX::CXMVECTOR b)
		{
			using namespace DirectX;
			float dist2, dot;
			XMStoreFloat(&dist2, (XMVector2Length(a) * XMVector2Length(b)));
			XMStoreFloat(&dot, XMVector2Dot(a, b));
			return acosf(dot / dist2);
		}
		inline float CalcAngle2(DirectX::XMFLOAT2 a, DirectX::XMFLOAT2 b)
		{
			using namespace DirectX;
			XMVECTOR xa, xb;
			xa = XMLoadFloat2(&a);
			xa = XMLoadFloat2(&b);
			return CalcAngle2(xa, xb);
		}
		inline float CalcAngle3(DirectX::CXMVECTOR a, DirectX::CXMVECTOR b)
		{
			using namespace DirectX;
			float dist2, dot;
			XMStoreFloat(&dist2, (XMVector3Length(a) * XMVector3Length(b)));
			XMStoreFloat(&dot, XMVector3Dot(a, b));
			return acosf(dot / dist2);
		}
		inline float CalcAngle3(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
		{
			using namespace DirectX;
			XMVECTOR xa, xb;
			xa = XMLoadFloat3(&a);
			xa = XMLoadFloat3(&b);
			return CalcAngle3(xa, xb);
		}

		inline DirectX::XMMATRIX CreateWorldMatrix(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 rotate)
		{
			using namespace DirectX;
			XMMATRIX S, R, T;
			S = XMMatrixScaling(scale.x, scale.y, scale.z);
			R = XMMatrixRotationRollPitchYaw(rotate.x, rotate.y, rotate.z);
			T = XMMatrixTranslation(pos.x, pos.y, pos.z);

			return  S * R * T;
		}

		// â~ÇÃñ êœ
		inline float CircleArea(float radius)
		{
			return (DirectX::XM_PI * radius) * (DirectX::XM_PI * radius);
		}
		// â~é¸ÇÃí∑Ç≥
		inline float Circumferencelength(float radius)
		{
			return DirectX::XM_2PI * radius;
		}
		// êÓå`ÇÃå ÇÃñ êœ
		inline float ArcArea(float radius, float radian)
		{
			return CircleArea(radius) * (radian / DirectX::XM_2PI);
		}
		// êÓå`ÇÃå ÇÃí∑Ç≥
		inline float ArcLength(float radius, float radian)
		{
			return Circumferencelength(radius) * (radian / DirectX::XM_2PI);
		}
	}
}