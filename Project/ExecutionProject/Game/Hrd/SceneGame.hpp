#pragma once

#include "Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Loader/PMDLoader.hpp>
#include <Dx12/PMDModel.hpp>

class SceneGame : public SceneBase
{
private:
	std::shared_ptr<KGL::Texture> texture;
	std::shared_ptr<KGL::PMD_Loader> pmd_data;
	std::shared_ptr<KGL::PMD_Model> pmd_model;
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc) override;
	HRESULT Render(const SceneDesc& desc) override;
	HRESULT UnInit(const SceneDesc& desc) override;

	SceneGame() = default;
	~SceneGame() = default;
};