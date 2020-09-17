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

void FPSCamera::AngleUpdate(const DirectX::XMFLOAT2& input, const DirectX::XMFLOAT2& speed, float limit_y, float elapsed_time)
{
	limit_y = XMConvertToRadians(limit_y);

	angle.x += XMConvertToRadians(speed.x) * input.x;
	angle.y += XMConvertToRadians(speed.y) * -input.y;
	angle.y = std::clamp(angle.y, -limit_y, +limit_y);
	XMMATRIX vertical = XMMatrixRotationX(angle.y);
	XMMATRIX horizontal = XMMatrixRotationY(angle.x);

	static const XMVECTOR z_vector = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	XMStoreFloat3(&focus_vec, XMVector3Transform(z_vector, vertical * horizontal));
}

void FPSCamera::MoveUpdate(bool front, bool back, bool right, bool left, bool up, bool down,
	float speed, float elapsed_time)
{
	CXMVECTOR forward = XMLoadFloat3(&focus_vec);
	XMVECTOR move_vec = g_XMZero;
	UINT key_counter = 0u;

	if (front)
	{
		move_vec += forward;
		key_counter++;
	}
	if (back)
	{
		move_vec += -forward;
		key_counter++;
	}

	if (right || left)
	{
		static CXMVECTOR up_v = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		CXMVECTOR right_v = XMVector3Normalize(XMVector3Cross(up_v, forward));

		if (right)
		{
			move_vec += right_v;
			key_counter++;
		}
		if (left)
		{
			move_vec += -right_v;
			key_counter++;
		}
	}

	if (up || down)
	{
		static CXMVECTOR up_v = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		if (up)
		{
			move_vec += up_v;
			key_counter++;
		}
		if (down)
		{
			move_vec += -up_v;
			key_counter++;
		}
	}

	if (key_counter > 0u)
	{
		XMStoreFloat3(&eye, XMLoadFloat3(&eye) + XMVector3Normalize(move_vec) * speed * elapsed_time);
	}
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
			if (window && !mouse_clipped)
			{
				input->SetMouseVisible(false);
				auto rect = window->GetClientRect();
				ClipCursor(&rect);
				mouse_clipped = true;
			}
			const auto mouse = input->GetMouseMove();
			const DirectX::XMFLOAT2 mouse_f(SCAST<float>(mouse.x), SCAST<float>(mouse.y));
			AngleUpdate(mouse_f, mouse_speed, limit_y, elapsed_time);
		}
		else if (mouse_clipped)
		{
			input->SetMouseVisible(true);
			ClipCursor(nullptr);
			mouse_clipped = false;
		}

		const bool move_front = input->IsKeyHold(KEYS::W);
		const bool move_back = input->IsKeyHold(KEYS::S);
		const bool move_right = input->IsKeyHold(KEYS::D);
		const bool move_left = input->IsKeyHold(KEYS::A);
		const bool move_up = input->IsKeyHold(KEYS::SPACE);
		const bool move_down = input->IsKeyHold(KEYS::LCONTROL);

		MoveUpdate(
			move_front, move_back,
			move_right, move_left,
			move_up, move_down,
			speed, elapsed_time
		);
	}
}

DemoCamera::DemoCamera(const DirectX::XMFLOAT3 pos, float timer_max) noexcept :
	FPSCamera(pos)
{
	start_pos = pos;
	this->timer_max = timer_max;
	input_timer = timer_max;
}

void DemoCamera::Update(const std::shared_ptr<KGL::Window>& window,
	const std::shared_ptr<KGL::Input>& input,
	float elapsed_time, float speed, bool mouse_update,
	const DirectX::XMFLOAT2& mouse_speed,
	float limit_y) noexcept
{
	if (input)
	{
		using KGL::KEYS;
		const auto move = input->GetMouseMove();
		const bool move_front = input->IsKeyHold(KEYS::W);
		const bool move_back = input->IsKeyHold(KEYS::S);
		const bool move_right = input->IsKeyHold(KEYS::D);
		const bool move_left = input->IsKeyHold(KEYS::A);
		const bool move_up = input->IsKeyHold(KEYS::SPACE);
		const bool move_down = input->IsKeyHold(KEYS::LCONTROL);

		bool input_exist = false;

		input_exist = input_exist || (move.x != 0 || move.y != 0);
		input_exist = input_exist || (move_front || move_back);
		input_exist = input_exist || (move_right || move_left);
		input_exist = input_exist || (move_up || move_down);

		if (input_exist)
		{
			input_timer = timer_max;
			FPSCamera::Update(window, input, elapsed_time, speed, mouse_update, mouse_speed, limit_y);
		}
		else
		{
			const float before_input_timer = input_timer;
			input_timer = std::max<float>(0.f, input_timer - elapsed_time);

			if (input_timer <= 0.f)
			{
				if (before_input_timer > 0.f)
				{
					eye = start_pos;
					angle = {};
				}
				const float update_time = elapsed_time - before_input_timer;
				AngleUpdate({ 200 * elapsed_time, 0.f }, mouse_speed, limit_y, elapsed_time);
				MoveUpdate(false, false, false, true, false, false, speed, update_time);
			}
		}
	}
}