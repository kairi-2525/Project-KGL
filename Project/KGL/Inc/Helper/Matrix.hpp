#pragma once

#include <DirectXMath.h>
#include <cmath>

namespace DirectX
{
	// z軸を特定の方向に向ける行列を返す関数
	// @param lookat	向かせたいベクトル
	// @param up		上ベクトル
	// @param right		右べクトル
	XMMATRIX XMLookAtMatrix(
		CXMVECTOR lookat,
		CXMVECTOR up,
		CXMVECTOR right)
	{
		// 向かせたい方向(z軸)
		XMVECTOR vz = lookat;
		// 向かせたい方向を向かせたときの仮のy軸ベクトル
		XMVECTOR vy = XMVector3Normalize(up);
		// 向かせたい方向を向かせたときのx軸
		XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));

		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		// LookAtとupが同じ方向を向いていたらrightを基準にして作り直す
		if (std::fabs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
		{
			// 仮のx軸方向
			vx = XMVector3Normalize(right);
			// 向かせたい方向を向かせたときのy軸ベクトル
			vy = XMVector3Normalize(XMVector3Cross(vz, vx));
			// x軸再計算
			vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		}

		XMMATRIX ret = XMMatrixIdentity();
		ret.r[0] = vx;
		ret.r[1] = vy;
		ret.r[2] = vz;
		return ret;
	}

	// 特定のベクトルを特定の方向に向けるための行列を返す
	// @param lookat	特定の方向
	// @param up		上ベクトル
	// @param right		右べクトル
	// @return			特定ベクトルを特定の方向に向けるための行列
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