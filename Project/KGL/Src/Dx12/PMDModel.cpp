#include <Dx12/PMDModel.hpp>

using namespace KGL;

PMD_Model::PMD_Model(ComPtr<ID3D12Device> device, const PMD::Desc& desc, TextureManager* mgr) noexcept
{
	// TODO
	m_tex_white = std::make_unique<Texture>(device);
	m_tex_black = std::make_unique<Texture>(device, 0x00, 0x00, 0x00, 0xff);
}