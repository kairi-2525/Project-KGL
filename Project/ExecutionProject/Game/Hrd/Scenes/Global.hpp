#pragma once

#include <DirectXMath.h>

struct FrameBuffer
{
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 proj;
	DirectX::XMFLOAT4X4 view_proj;
	DirectX::XMFLOAT3	eye_pos; float pad0;
	DirectX::XMFLOAT3	light_vec; float pad1;
	DirectX::XMFLOAT3	light_color; float pad2;
	DirectX::XMFLOAT3	ambient_light_color; float pad3;
};