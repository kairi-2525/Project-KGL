#include <Dx12/PipelineState.hpp>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;
//
//HRESULT PipelineStates::SetDesc(ComPtr<ID3D12Device> device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) noexcept
//{
//	HRESULT hr = S_OK;
//	m_desc = desc;
//	auto itr = m_states.find(m_desc);
//	if (itr != m_states.end())
//	{
//		m_state = itr->second;
//		return hr;
//	}
//	RCHECK(!device, "SetDesc��PipelineState�����������ۂ̓f�o�C�X��n���Ă��������B", E_FAIL);
//	auto& set_state = m_states[m_desc];
//	hr = device->CreateGraphicsPipelineState(&m_desc, IID_PPV_ARGS(set_state.ReleaseAndGetAddressOf()));
//	RCHECK(FAILED(hr), "�p�C�v���C���X�e�[�g�̍쐬�Ɏ��s", hr);
//	m_state = set_state;
//	return hr;
//}