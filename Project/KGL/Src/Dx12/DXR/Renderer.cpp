#include <Dx12/DXR/Renderer.hpp>
#include <Helper/Convert.hpp>
#include <Helper/ThrowAssert.hpp>
#include <unordered_set>

using namespace KGL;

HRESULT DXR::BaseRenderer::Create(
	ComPtrC<ID3D12Device5> device,
	const std::shared_ptr<DXC>& dxc,
	const Desc& desc
) noexcept
{
	HRESULT hr = S_OK;

	DXR::Signature signature(device, dxc, desc.signatures);
	DXR::DummySignature global_signature(device);
	DXR::DummySignature local_signature(device, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	const auto& sigs = signature.GetData();
	const size_t sig_size = sigs.size();

	const UINT64 subobject_count =
		3 * sigs.size() + // シェーダー+ルートシグネチャ宣言+関連付け
		3 * desc.hit_groups.size() +
		1 + // シェーダー構成
		1 + // シェーダーペイロード
		2 + // 空のグローバルおよびローカルルートシグネチャ
		1;	// 最終パイプラインサブオブジェクト

	std::vector<D3D12_STATE_SUBOBJECT> subobjects(subobject_count);
	std::vector<D3D12_DXIL_LIBRARY_DESC> libraries(sig_size);
	std::vector<std::vector<D3D12_EXPORT_DESC>> export_desc_list(sig_size);
	std::unordered_set<std::wstring> exports;
	std::vector<std::wstring> w_shader_names;
	std::vector<LPCWSTR> w_shader_name_pointers;
	w_shader_names.reserve(sig_size);
	w_shader_name_pointers.reserve(sig_size);

	UINT current_index = 0;
	for (const auto& sig : sigs)
	{
		// shader名をポインターで保存
		w_shader_name_pointers.emplace_back(
			w_shader_names.emplace_back(CONVERT::MultiToWide(sig.first)).c_str()
		);

		const auto& shader = sig.second.shader;
		const auto& ep_list = sig.second.entry_points;
		const size_t ep_size = ep_list.size();

		// シンボルごとに1つのエクスポート記述子を作成
		export_desc_list[current_index].resize(ep_size);
		for (size_t i = 0; i < ep_size; i++)
		{
			export_desc_list[current_index][i].Name = ep_list[i].c_str();
			export_desc_list[current_index][i].ExportToRename = nullptr;
			export_desc_list[current_index][i].Flags = D3D12_EXPORT_FLAG_NONE;

			RCHECK(exports.find(ep_list[i]) != exports.end(), "DXILライブラリが同名で複数定義されています。", E_FAIL)

			exports.insert(ep_list[i]);
		}

		// DXILコードとエクスポート名を組み合わせたライブラリ記述子を作成
		libraries[current_index].DXILLibrary.BytecodeLength = shader->GetBufferSize();
		libraries[current_index].DXILLibrary.pShaderBytecode = shader->GetBufferPointer();
		libraries[current_index].NumExports = SCAST<UINT>(ep_size);
		libraries[current_index].pExports = export_desc_list[current_index].data();

		// 作成したDXILライブラリを追加
		D3D12_STATE_SUBOBJECT lib_subobject = {};
		lib_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		lib_subobject.pDesc = &libraries[current_index];

		subobjects[current_index++] = lib_subobject;
	}

	std::vector<std::wstring> w_group_names;
	std::vector<std::wstring> w_group_ch_symbols;
	w_group_names.reserve(desc.hit_groups.size());

	std::vector<D3D12_HIT_GROUP_DESC> hit_groups;
	hit_groups.reserve(desc.hit_groups.size());

	// すべてのヒットグループ宣言を追加します
	for (const auto& group : desc.hit_groups)
	{
		const auto& group_name = w_group_names.emplace_back(CONVERT::MultiToWide(group.first));
		const auto& group_ch_symbol = w_group_ch_symbols.emplace_back(CONVERT::MultiToWide(group.second));
		auto& hit_group_desc = hit_groups.emplace_back();
		hit_group_desc.HitGroupExport = group_name.c_str();
		hit_group_desc.ClosestHitShaderImport = group_ch_symbol.c_str();
		hit_group_desc.AnyHitShaderImport = nullptr;
		hit_group_desc.IntersectionShaderImport = nullptr;

		D3D12_STATE_SUBOBJECT hit_group = {};
		hit_group.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		hit_group.pDesc = &hit_group_desc;

		subobjects[current_index++] = hit_group;
	}

#ifdef _DEBUG	// ヒットグループが不明なシェーダー名を参照していないことを確認する
	for (const auto& sig : sigs)
	{
		for (const auto& hit_group : w_group_ch_symbols)
		{
			RCHECK(exports.find(hit_group) == exports.end(), "存在しないシェーダーを参照", E_FAIL);
		}
	}
#endif

	// シェーダーペイロード構成のサブオブジェクトを追加します
	D3D12_STATE_SUBOBJECT shader_config_object = {};
	shader_config_object.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shader_config_object.pDesc = &desc.shader_config;
	subobjects[current_index++] = shader_config_object;

	// レイ生成、ミス、ヒットグループのすべてのシンボルのリストを作成します。
	// これらのシェーダーはペイロード定義に関連付ける必要があります。
	std::vector<LPCWSTR> exported_symbols;
	exported_symbols.reserve(exports.size());
	for (const auto& name : exports)
	{
		exported_symbols.push_back(name.c_str());
	}
	const WCHAR** shader_exports = exported_symbols.data();

	// シェーダーとペイロード間の関連付けのサブオブジェクトを追加します
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shader_payload_association = {};
	shader_payload_association.NumExports = static_cast<UINT>(exported_symbols.size());
	shader_payload_association.pExports = shader_exports;

	// シェーダーのセットを前のサブオブジェクトで定義されたペイロードに関連付けます
	shader_payload_association.pSubobjectToAssociate = &subobjects[(current_index - 1)];

	// ペイロード関連付けオブジェクトを作成して保存します
	D3D12_STATE_SUBOBJECT shader_payload_association_object = {};
	shader_payload_association_object.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shader_payload_association_object.pDesc = &shader_payload_association;
	subobjects[current_index++] = shader_payload_association_object;

	// ルートシグネチャの関連付けには、それぞれに2つのオブジェクトが必要です。
	// 1つはルートシグネチャを宣言するためのもので、
	// もう1つはそのルートシグネチャをシンボルのセットに関連付けるためのものです。
	std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations;
	associations.reserve(sig_size);
	{
		size_t index = 0u;
		for (const auto& sig : sigs)
		{
			// ルートシグネチャを宣言するサブオブジェクトを追加します
			D3D12_STATE_SUBOBJECT root_sig_object = {};
			root_sig_object.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			root_sig_object.pDesc = sig.second.rs.GetAddressOf();
			subobjects[current_index++] = root_sig_object;

			// エクスポートされたシェーダシンボルとルートシグネチャの関連付けのためのサブオブジェクトを追加します。
			auto& association = associations.emplace_back();
			association.NumExports = SCAST<UINT>(w_shader_name_pointers.size());
			association.pExports = w_shader_name_pointers.data();
			association.pSubobjectToAssociate = &subobjects[(current_index - 1)];

			D3D12_STATE_SUBOBJECT root_sig_association_object = {};
			root_sig_association_object.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			root_sig_association_object.pDesc = &association;
			subobjects[current_index++] = root_sig_association_object;

			index++;
		}
	}

	// パイプラインの構築には、常に空のグローバルルートシグネチャが必要
	D3D12_STATE_SUBOBJECT global_root_sig;
	global_root_sig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	global_root_sig.pDesc = global_signature.Data().GetAddressOf();
	subobjects[current_index++] = global_root_sig;

	// パイプラインの構築には、常に空のローカルルートシグネチャが必要
	D3D12_STATE_SUBOBJECT local_root_sig;
	local_root_sig.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	local_root_sig.pDesc = local_signature.Data().GetAddressOf();
	subobjects[current_index++] = local_root_sig;

	// レイトレーシングパイプライン構成のサブオブジェクトを追加します
	D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = {};
	pipeline_config.MaxTraceRecursionDepth = desc.max_trace_recursion_depth;

	D3D12_STATE_SUBOBJECT pipeline_config_pbject = {};
	pipeline_config_pbject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipeline_config_pbject.pDesc = &pipeline_config;
	subobjects[current_index++] = pipeline_config_pbject;

	// レイトレーシングパイプラインステートオブジェクトを記述します
	D3D12_STATE_OBJECT_DESC pipeline_desc = {};
	pipeline_desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipeline_desc.NumSubobjects = SCAST<UINT>(subobject_count);
	pipeline_desc.pSubobjects = subobjects.data();

	// ステートオブジェクトを作成
	hr = device->CreateStateObject(&pipeline_desc, IID_PPV_ARGS(m_state_object.ReleaseAndGetAddressOf()));
	RCHECK_HR(hr, "ステートオブジェクトの作成に失敗");

	return hr;
}