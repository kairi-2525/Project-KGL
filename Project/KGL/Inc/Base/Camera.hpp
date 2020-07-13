#pragma once

#include <DirectXMath.h>

namespace KGL
{
	inline namespace Base
	{
		struct Camera
		{
			DirectX::XMFLOAT3 eye, focus, up;
		};

		// Focusをベクトルとして扱います
		struct VecCamera
		{
			DirectX::XMFLOAT3 eye, focus_vec, up;
		};

		namespace CAMERA
		{
			inline DirectX::XMMATRIX GetView(const Camera& camera)
			{
				using namespace DirectX;
				return XMMatrixLookAtLH(
					XMVectorSet(camera.eye.x, camera.eye.y, camera.eye.z, 1.f),			// 始点
					XMVectorSet(camera.focus.x, camera.focus.y, camera.focus.z, 1.f),	// 注視点
					XMVectorSet(camera.up.x, camera.up.y, camera.up.z, 0.f)				// 上ベクトル
				);
			}

			inline DirectX::XMMATRIX GetView(const VecCamera& camera)
			{
				using namespace DirectX;
				return XMMatrixLookAtLH(
					XMVectorSet(camera.eye.x, camera.eye.y, camera.eye.z, 1.f),			// 始点
					XMVectorSet(
						camera.eye.x + camera.focus_vec.x,
						camera.eye.y + camera.focus_vec.y,
						camera.eye.z + camera.focus_vec.z,
						1.f),															// 注視点
					XMVectorSet(camera.up.x, camera.up.y, camera.up.z, 0.f)				// 上ベクトル
				);
			}

			inline DirectX::XMFLOAT3 GetFocusPos(const VecCamera& camera)
			{
				using namespace DirectX;
				return { 
					camera.eye.x + camera.focus_vec.x,
					camera.eye.y + camera.focus_vec.y,
					camera.eye.z + camera.focus_vec.z
				};
			}
			inline DirectX::XMFLOAT3 GetFocusVec(const Camera& camera)
			{
				using namespace DirectX;
				return {
					camera.focus.x - camera.eye.x,
					camera.focus.y - camera.eye.y,
					camera.focus.z - camera.eye.z
				};
			}
		}
	}
}