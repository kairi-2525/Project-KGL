#pragma once

#include "Scene.hpp"
#include <Dx12/Texture.hpp>

class SceneGame : public SceneBase
{
private:
	std::shared_ptr<KGL::Texture> texture;
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc) override;
	HRESULT Render(const SceneDesc& desc) override;
	HRESULT UnInit(const SceneDesc& desc) override;



	SceneGame() = default;
	~SceneGame() = default;
};