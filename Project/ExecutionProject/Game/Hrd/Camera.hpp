#pragma once

#include <Base/Camera.hpp>
#include <Base/Input.hpp>
#include <Base/Window.hpp>

class FPSCamera : protected KGL::VecCamera
{
protected:
	DirectX::XMFLOAT2 angle;
	bool mouse_clipped;
protected:
	void AngleUpdate(const DirectX::XMFLOAT2& input, const DirectX::XMFLOAT2& speed, float limit_y, float elapsed_time);
	void MoveUpdate(bool front, bool back, bool right, bool left, bool up, bool down, float speed, float elapsed_time);
public:
	explicit FPSCamera(const DirectX::XMFLOAT3 pos) noexcept;
	virtual ~FPSCamera() = default;
	virtual void Update(
		const std::shared_ptr<KGL::Window>& window,
		const std::shared_ptr<KGL::Input>& input,
		float elapsed_time, float speed, bool mouse_update = true,
		const DirectX::XMFLOAT2& mouse_speed = { 0.1f, 0.1f },
		float limit_y = 90.f - 1.f) noexcept;
	DirectX::XMMATRIX GetView() const noexcept
	{
		return KGL::CAMERA::GetView(*this);
	}
	DirectX::XMFLOAT3& GetPos() noexcept { return eye; };
	DirectX::XMFLOAT3& GetFront() noexcept { return focus_vec; };
	const DirectX::XMFLOAT3& GetPos() const noexcept { return eye; };
	void SetPos(const DirectX::XMFLOAT3& pos) noexcept { eye = pos; };
	void SetMouseClipped(bool flg) noexcept;
};

class DemoCamera : public FPSCamera
{
private:
	float rotate_angle;
	float input_timer;
	float timer_max;
public:
	DirectX::XMFLOAT3 center;
public:
	explicit DemoCamera(
		const DirectX::XMFLOAT3 center,
		const DirectX::XMFLOAT3 pos,
		float timer_max = 30.f) noexcept;
	void Update(
		const std::shared_ptr<KGL::Window>& window,
		const std::shared_ptr<KGL::Input>& input,
		float elapsed_time, float speed, bool mouse_update = true,
		const DirectX::XMFLOAT2& mouse_speed = { 0.1f, 0.1f },
		float limit_y = 90.f - 1.f
	) noexcept override;
};