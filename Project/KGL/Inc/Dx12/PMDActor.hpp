#pragma once

#include <DirectXMath.h>
#include "../Helper/ComPtr.hpp"

#include "PMDModel.hpp"
#include "../Base/VMD.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class PMD_Actor
		{
		protected:
			struct ConstantBuffers
			{
				DirectX::XMMATRIX wvp;
				DirectX::XMMATRIX world;
				DirectX::XMMATRIX view;
				DirectX::XMMATRIX proj;
				DirectX::XMMATRIX bones[512];
				DirectX::XMFLOAT3 eye;	// 視点座標
			};
		private:
			std::shared_ptr<const PMD::Desc>		m_model_desc;
			std::shared_ptr<const VMD::Desc>		m_anim_desc;
		protected:
			ConstantBuffers*						m_map_buffers;
			ComPtr<ID3D12DescriptorHeap>			m_desc_heap;
			ComPtr<ID3D12Resource>					m_const_buff;
			float									m_anim_counter;
		private:
			static float GetYFromXOnBezier(
				float x,
				const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b,
				UINT8 n					
			) noexcept;
		private:
			// 場合分け
			void IKSolve(UINT frame_no) noexcept;
			// CCD-IKによりボーン方向を解決
			void SolveCCDIK(const PMD::IK& ik) noexcept;
			// 余弦定理IKによりボーン方向を解決
			void SolveCosineIK(const PMD::IK& ik) noexcept;
			// LookAt行列によりボーン方向を解決
			void SolveLookAt(const PMD::IK& ik) noexcept;
		public:
			explicit PMD_Actor(
				const ComPtr<ID3D12Device>& device,
				const PMD_Model& model
			) noexcept;
			virtual ~PMD_Actor() = default;

			void SetViewProjection(DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
			{ m_map_buffers->view = view; m_map_buffers->proj = proj; }
			void SetEye(const DirectX::XMFLOAT3& eye) { m_map_buffers->eye = eye; }
			void SetAnimation(const std::shared_ptr<const VMD::Desc>& desc) noexcept;
			void ClearAnimation() noexcept;
			void MotionUpdate(float elapsed_time, bool loop = true, bool bezier = true) noexcept;
			void UpdateWVP();
			void Render(
				const ComPtr<ID3D12GraphicsCommandList>& cmd_list
			) const noexcept;
			ConstantBuffers* GetMappedBuffers() { return m_map_buffers; }
			void RecursiveMatrixMultiply(
				const PMD::BoneNode* node,
				const DirectX::XMMATRIX& mat
			) noexcept;
		};
	}
}