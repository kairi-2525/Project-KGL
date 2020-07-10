#pragma once

#include <DirectXMath.h>
#include <cmath>

namespace DirectX
{
	// z�������̕����Ɍ�����s���Ԃ��֐�
	// @param lookat	�����������x�N�g��
	// @param up		��x�N�g��
	// @param right		�E�׃N�g��
	XMMATRIX XMLookAtMatrix(
		CXMVECTOR lookat,
		CXMVECTOR up,
		CXMVECTOR right)
	{
		// ��������������(z��)
		XMVECTOR vz = lookat;
		// �������������������������Ƃ��̉���y���x�N�g��
		XMVECTOR vy = XMVector3Normalize(up);
		// �������������������������Ƃ���x��
		XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));

		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		// LookAt��up�����������������Ă�����right����ɂ��č�蒼��
		if (std::fabs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
		{
			// ����x������
			vx = XMVector3Normalize(right);
			// �������������������������Ƃ���y���x�N�g��
			vy = XMVector3Normalize(XMVector3Cross(vz, vx));
			// x���Čv�Z
			vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		}

		XMMATRIX ret = XMMatrixIdentity();
		ret.r[0] = vx;
		ret.r[1] = vy;
		ret.r[2] = vz;
		return ret;
	}

	// ����̃x�N�g�������̕����Ɍ����邽�߂̍s���Ԃ�
	// @param lookat	����̕���
	// @param up		��x�N�g��
	// @param right		�E�׃N�g��
	// @return			����x�N�g�������̕����Ɍ����邽�߂̍s��
	XMMATRIX XMLookAtMatrix(
		CXMVECTOR origin,
		CXMVECTOR lookat,
		CXMVECTOR up,
		CXMVECTOR right)
	{
		return
			XMMatrixTranspose(XMLookAtMatrix(origin, up, right))
			* XMLookAtMatrix(lookat, up, right);
	}
}