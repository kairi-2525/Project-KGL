#pragma once

#include "Effects.hpp"
#include <Helper/Cast.hpp>
#include <Base/Directory.hpp>
#include "CelealHelper.hpp"
#include <map>

// Cereal入出力用構造体
struct FS_Obj_Desc
{
	std::string			name;		// スポナーの名前
	std::string			fw_name;	// 使用する花火の名前
	DirectX::XMFLOAT2	start_time;	// 生成開始
	DirectX::XMFLOAT2	time;		// 生成開始からの生存時間
	DirectX::XMFLOAT2	wait_time;	// 生存時間後の再生成までの時間
	DirectX::XMFLOAT2	spawn_late;	// 生成開始時の生成レート
	bool				infinity;	// 生成開始後、生存時間が無限
};

// 実用構造体
class FS_Obj
{
	std::shared_ptr<FireworksDesc>  fw_desc;
public:
	FS_Obj_Desc						obj_desc;
public:
	void Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list);
	void Update(float update_time, std::vector<Fireworks>* pout_fireworks);
};

// スポナーの一覧を管理するクラス
class FS
{
public:
	std::vector<FS_Obj>				objects;
public:
	void Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list);
	void Update(float update_time, std::vector<Fireworks>* pout_fireworks);
};

class FSManager : private KGL::Directory
{
private:
	std::list<std::shared_ptr<FS>>	fs_list;
	std::shared_ptr<FS>				select_fs;
public:
	FSManager(const std::filesystem::path& path, const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list) noexcept;
	void Update(float update_time, std::vector<Fireworks>* pout_fireworks);
	void GUIUpdate();
};

#define EV_0_FSODC_ARCHIVE \
	KGL_NVP("name", m.name), \
	KGL_NVP("fw_name", m.fw_name), \
	KGL_NVP("start_time", m.start_time), \
	KGL_NVP("time", m.time), \
	KGL_NVP("wait_time", m.wait_time), \
	KGL_NVP("spawn_late", m.spawn_late), \
	KGL_NVP("infinity", m.infinity)

enum class FSODC_VERSION
{
	EV_0 = 1u,
};

#define EV_0_FSDC_ARCHIVE \
	KGL_NVP("obj_desc", m.obj_desc)

enum class FSDC_VERSION
{
	EV_0 = 1u,
};

#define EV_0_FS_ARCHIVE \
	KGL_NVP("objects", m.objects)

enum class FS_VERSION
{
	EV_0 = 1u,
};

template<class Archive>
void serialize(Archive& archive,
	FS_Obj_Desc& m, std::uint32_t const version)
{
	using namespace DirectX;
	switch (SCAST<FSODC_VERSION>(version))
	{
		case FSODC_VERSION::EV_0:
		{
			archive(EV_0_FSODC_ARCHIVE);
			break;
		}
	}
};
CEREAL_CLASS_VERSION(FS_Obj_Desc, SCAST<UINT>(FSODC_VERSION::EV_0));

template<class Archive>
void serialize(Archive& archive,
	FS_Obj& m, std::uint32_t const version)
{
	using namespace DirectX;
	switch (SCAST<FSDC_VERSION>(version))
	{
		case FSDC_VERSION::EV_0:
		{
			archive(EV_0_FSDC_ARCHIVE);
			break;
		}
	}
};
CEREAL_CLASS_VERSION(FS_Obj, SCAST<UINT>(FSDC_VERSION::EV_0));

template<class Archive>
void serialize(Archive& archive,
	FS& m, std::uint32_t const version)
{
	using namespace DirectX;
	switch (SCAST<FS_VERSION>(version))
	{
		case FS_VERSION::EV_0:
		{
			archive(EV_0_FS_ARCHIVE);
			break;
		}
	}
};
CEREAL_CLASS_VERSION(FS, SCAST<UINT>(FS_VERSION::EV_0));