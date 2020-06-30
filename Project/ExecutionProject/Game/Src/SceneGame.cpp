#include "../Hrd/SceneGame.hpp"

HRESULT SceneGame::Load(const SceneDesc& desc)
{
	texture = std::make_shared<KGL::Texture>(desc.app->GetDevice(), 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff);
	pmd_data = std::make_shared<KGL::PMDLoader>("./Assets/Models/‹¾‰¹ƒŠƒ“.pmd");
	return S_OK;
}

HRESULT SceneGame::Init(const SceneDesc& desc)
{
	return S_OK;
}

HRESULT SceneGame::Update(const SceneDesc& desc)
{
	return S_OK;
}

HRESULT SceneGame::Render(const SceneDesc& desc)
{
	return S_OK;
}

HRESULT SceneGame::UnInit(const SceneDesc& desc)
{
	return S_OK;
}