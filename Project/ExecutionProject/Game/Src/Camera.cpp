#include "../Hrd/Camera.hpp"

using namespace DirectX;

FPSCamera::FPSCamera(const DirectX::XMFLOAT3 pos) noexcept
{
	eye = pos;
	focus_vec = { 0.f, 0.f, 1.f };
	up = { 0.f, 1.f, 0.f };
	angle = { 0.f, 0.f };
	mouse_clipped = false;
}

void FPSCamera::Update(const std::shared_ptr<KGL::Window>& window, 
	const std::shared_ptr<KGL::Input>& input,
	float elapsed_time, float speed, bool mouse_update,
	const DirectX::XMFLOAT2& mouse_speed, float limit_y) noexcept
{
	if (input)
	{
		using KGL::KEYS;

		if (mouse_update)
		{
			limit_y = XMConvertToRadians(limit_y);

			const auto& mouse_move = input->GetMouseMove();
			if (window && !mouse_clipped)
			{
				input->SetMouseVisible(false);
				auto rect = window->GetClientRect();
				ClipCursor(&rect);
				mouse_clipped = true;
			}

			angle.x += XMConvertToRadians(mouse_speed.x) * mouse_move.x;
			angle.y += XMConvertToRadians(mouse_speed.y) * -mouse_move.y;
			angle.y = std::clamp(angle.y, -limit_y, +limit_y);
			XMMATRIX vertical = XMMatrixRotationX(angle.y);
			XMMATRIX horizontal = XMMatrixRotationY(angle.x);

			static const XMVECTOR z_vector = XMVectorSet(0.f, 0.f, 1.f, 0.f);
			XMStoreFloat3(&focus_vec, XMVector3Transform(z_vector, vertical * horizontal));
		}
		else if (mouse_clipped)
		{
			input->SetMouseVisible(true);
			ClipCursor(nullptr);
			mouse_clipped = false;
		}

		CXMVECTOR forward = XMLoadFloat3(&focus_vec);
		XMVECTOR move_vec = g_XMZero;
		UINT key_counter = 0u;

		const bool move_front = input->IsKeyHold(KEYS::W);
		const bool move_back = input->IsKeyHold(KEYS::S);
		const bool move_right = input->IsKeyHold(KEYS::D);
		const bool move_left = input->IsKeyHold(KEYS::A);
		const bool move_up = input->IsKeyHold(KEYS::SPACE);
		const bool move_down = input->IsKeyHold(KEYS::LCONTROL);

		if (move_front)
		{
			move_vec += forward;
			key_counter++;
		}
		if (move_back)
		{
			move_vec += -forward;
			key_counter++;
		}

		if (move_right || move_left)
		{
			static CXMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
			CXMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));

			if (move_right)
			{
				move_vec += right;
				key_counter++;
			}
			if (move_left)
			{
				move_vec += -right;
				key_counter++;
			}
		}

		if (move_up || move_down)
		{
			static CXMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
			if (move_up)
			{
				move_vec += up;
				key_counter++;
			}
			if (move_down)
			{
				move_vec += -up;
				key_counter++;
			}
		}

		if (key_counter > 0u)
		{
			XMStoreFloat3(&eye, XMLoadFloat3(&eye) + XMVector3Normalize(move_vec) * speed * elapsed_time);
		}
	}
}