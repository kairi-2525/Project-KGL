#pragma once

#include <Base/Camera.hpp>
#include <Base/Input.hpp>
#include <Base/Window.hpp>

class FPSCamera : private KGL::VecCamera
{
private:
	DirectX::XMFLOAT2 angle;
	bool mouse_clipped;
public:
	explicit FPSCamera(const DirectX::XMFLOAT3 pos) noexcept;
	void Update(const std::shared_ptr<KGL::Window>& window,
		const std::shared_ptr<KGL::Input>& input,
		float elapsed_time, float speed, bool mouse_update = true,
		const DirectX::XMFLOAT2& mouse_speed = { 0.1f, 0.1f },
		float limit_y = 90.f - 1.f) noexcept;
	DirectX::XMMATRIX GetView() const noexcept
	{
		return KGL::CAMERA::GetView(*this);
	}
	DirectX::XMFLOAT3& GetPos() noexcept { return eye; };
	const DirectX::XMFLOAT3& GetPos() const noexcept { return eye; };
	void SetPos(const DirectX::XMFLOAT3& pos) noexcept { eye = pos; };
};