#pragma once

#include "Effects.hpp"
#include <Helper/Cast.hpp>
#include <Base/Directory.hpp>
#include "CelealHelper.hpp"
#include <map>

// Cereal���o�͗p�\����
struct FS_Obj_Desc
{
	std::string			set_name;
	std::string			name;			// �X�|�i�[�̖��O
	std::string			fw_name;		// �g�p����ԉ΂̖��O
	DirectX::XMFLOAT2	start_time;		// �����J�n
	DirectX::XMFLOAT2	time;			// �����J�n����̐�������
	DirectX::XMFLOAT2	wait_time;		// �������Ԍ�̍Đ����܂ł̎���
	float				spawn_radius;	// �����ʒu�͈�
	DirectX::XMFLOAT2	spawn_angle;	// �ˏo�p�x
	DirectX::XMFLOAT2	spawn_late;		// �����J�n���̐������[�g
	DirectX::XMFLOAT2	spawn_power;	// �ˏo���x��power�{���܂��B
	bool				infinity;		// �����J�n��A�������Ԃ�����
	bool				random_color;	// �F�������_���ɕύX����
};

// ���p�\����
class FS_Obj
{
	std::shared_ptr<FireworksDesc>  fw_desc;
	float							start_time;
	float							time;
	float							spawn_late;
	float							counter;
public:
	FS_Obj_Desc						obj_desc;
public:
	static void SetRandomColor(FireworksDesc* desc);
public:
	void SetDesc(std::shared_ptr<FireworksDesc> desc) { fw_desc = desc; }
	void Init();
	void Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list);
	void Update(float update_time, std::vector<Fireworks>* pout_fireworks);
	bool GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list);
};

// �X�|�i�[�̈ꗗ���Ǘ�����N���X
class FS
{
public:
	std::string						set_name;
	std::string						create_name;
	std::string						name;
	std::vector<FS_Obj>				objects;
private:
	void Create(std::string name, std::shared_ptr<FireworksDesc> desc);
public:
	void Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list);
	void Update(float update_time, std::vector<Fireworks>* pout_fireworks);
	bool GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list);
};

class FSManager : private KGL::Directory
{
private:
	std::string						set_name;
	std::list<std::shared_ptr<FS>>	fs_list;
	std::shared_ptr<FS>				select_fs;
private:
	void Create(std::string name);
	void Export();
public:
	FSManager(const std::filesystem::path& path, const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list) noexcept;
	void Update(float update_time, std::vector<Fireworks>* pout_fireworks);
	void GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list);
};

#define EV_0_FSODC_ARCHIVE \
	KGL_NVP("name", m.name), \
	KGL_NVP("fw_name", m.fw_name), \
	KGL_NVP("start_time", m.start_time), \
	KGL_NVP("time", m.time), \
	KGL_NVP("wait_time", m.wait_time), \
	KGL_NVP("spawn_radius", m.spawn_radius), \
	KGL_NVP("spawn_angle", m.spawn_angle), \
	KGL_NVP("spawn_late", m.spawn_late), \
	KGL_NVP("spawn_power", m.spawn_power), \
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
	KGL_NVP("name", m.name), \
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