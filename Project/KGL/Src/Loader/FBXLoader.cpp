#include <Loader/FBXLoader.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Convert.hpp>

using namespace KGL;

//ボーン行列を FBX データから取得する
void FBX_Loader::FetchBoneMatrices(
	fbxsdk::FbxMesh* fbx_mesh,
	std::vector<FBX::Bone>& skeletal,
	fbxsdk::FbxTime time
) noexcept
{
	const int number_of_deformers = fbx_mesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int index_of_deformer = 0; index_of_deformer < number_of_deformers; ++index_of_deformer)
	{
		FbxSkin* skin = static_cast<FbxSkin*>(fbx_mesh->GetDeformer(index_of_deformer, FbxDeformer::eSkin));
		const int number_of_clusters = skin->GetClusterCount();
		skeletal.resize(number_of_clusters);
		for (int index_of_cluster = 0; index_of_cluster < number_of_clusters; ++index_of_cluster)
		{
			FBX::Bone& bone = skeletal.at(index_of_cluster);
			FbxCluster* cluster = skin->GetCluster(index_of_cluster);
			// this matrix trnasforms coordinates of the initial pose from mesh space to global space
			FbxAMatrix reference_global_init_position;
			cluster->GetTransformMatrix(reference_global_init_position);
			// this matrix trnasforms coordinates of the initial pose from bone space to global space
			FbxAMatrix cluster_global_init_position;
			cluster->GetTransformLinkMatrix(cluster_global_init_position);
			// this matrix trnasforms coordinates of the current pose from bone space to global space
			FbxAMatrix cluster_global_current_position;
			cluster_global_current_position = cluster->GetLink()->EvaluateGlobalTransform(time);
			// this matrix trnasforms coordinates of the current pose from mesh space to global space
			FbxAMatrix reference_global_current_position;
			reference_global_current_position = fbx_mesh->GetNode()->EvaluateGlobalTransform(time);
			// Matrices are defined using the Column Major scheme. When a FbxAMatrix represents a transformation
			// (translation, rotation and scale), the last row of the matrix represents the translation part of the
			// transformation.
			FbxAMatrix transform = reference_global_current_position.Inverse() * cluster_global_current_position
				* cluster_global_init_position.Inverse() * reference_global_init_position;
			// convert FbxAMatrix(transform) to XMDLOAT4X4(bone.transform)

			DirectX::XMFLOAT4X4 xm_transform;
			FbxMatrix_To_XMFLOAT4X4(transform, &xm_transform);
			bone.transform = DirectX::XMLoadFloat4x4(&xm_transform);
		}
	}
}

//アニメーションを取得する
void FBX_Loader::FetchAnimations(
	fbxsdk::FbxMesh* fbx_mesh,
	FBX::SkeletalAnimation& skeletal_animation,
	UINT sampling_rate
) noexcept
{
	// Get the list of all the animation stack.
	FbxArray<FbxString*> array_of_animation_stack_names;
	fbx_mesh->GetScene()->FillAnimStackNameArray(array_of_animation_stack_names);
	// Get the number of animations.
	int number_of_animations = array_of_animation_stack_names.Size();
	if (number_of_animations > 0)
	{
		// Get the FbxTime per animation's frame.
		FbxTime::EMode time_mode = fbx_mesh->GetScene()->GetGlobalSettings().GetTimeMode();
		FbxTime frame_time;
		frame_time.SetTime(0, 0, 0, 1, 0, time_mode);
		sampling_rate = sampling_rate > 0 ? sampling_rate : static_cast<UINT>(frame_time.GetFrameRate(time_mode));
		float sampling_time = 1.0f / sampling_rate;
		skeletal_animation.sampling_time = sampling_time;
		skeletal_animation.animation_tick = 0.0f;
		FbxString* animation_stack_name = array_of_animation_stack_names.GetAt(0);
		FbxAnimStack* current_animation_stack
			= fbx_mesh->GetScene()->FindMember<FbxAnimStack>(animation_stack_name->Buffer());
		fbx_mesh->GetScene()->SetCurrentAnimationStack(current_animation_stack);
		FbxTakeInfo* take_info = fbx_mesh->GetScene()->GetTakeInfo(animation_stack_name->Buffer());
		FbxTime start_time = take_info->mLocalTimeSpan.GetStart();
		FbxTime end_time = take_info->mLocalTimeSpan.GetStop();
		FbxTime sampling_step;
		sampling_step.SetTime(0, 0, 1, 0, 0, time_mode);
		sampling_step = static_cast<FbxLongLong>(sampling_step.Get() * sampling_time);
		for (FbxTime current_time = start_time; current_time < end_time; current_time += sampling_step)
		{
			FBX::Skeletal skeletal;
			FetchBoneMatrices(fbx_mesh, skeletal, current_time);
			skeletal_animation.push_back(skeletal);
		}
	}
	else
	{
		FbxTime::EMode time_mode = fbx_mesh->GetScene()->GetGlobalSettings().GetTimeMode();
		FbxTime frame_time;
		frame_time.SetTime(0, 0, 0, 1, 0, time_mode);
		FBX::Skeletal skeletal;
		FetchBoneMatrices(fbx_mesh, skeletal, frame_time * 0); // pose at frame 0
		skeletal_animation.push_back(skeletal);
	}
	for (int i = 0; i < number_of_animations; i++)
	{
		delete array_of_animation_stack_names[i];
	}
}

//ボーン影響度を FBX データから取得する
void FBX_Loader::FetchBoneInfluences(
	const FbxMesh* fbx_mesh,
	std::vector<FBX::Bone_Influences_Per_Control_Point>* influences
) noexcept
{
	const int number_of_control_points = fbx_mesh->GetControlPointsCount();
	influences->resize(number_of_control_points);
	const int number_of_deformers = fbx_mesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int index_of_deformer = 0; index_of_deformer < number_of_deformers; ++index_of_deformer)
	{
		FbxSkin* skin = static_cast<FbxSkin*>(fbx_mesh->GetDeformer(index_of_deformer, FbxDeformer::eSkin));
		const int number_of_clusters = skin->GetClusterCount();
		for (int index_of_cluster = 0; index_of_cluster < number_of_clusters; ++index_of_cluster)
		{
			FbxCluster* cluster = skin->GetCluster(index_of_cluster);
			const int number_of_control_point_indices = cluster->GetControlPointIndicesCount();
			const int* array_of_control_point_indices = cluster->GetControlPointIndices();
			const double* array_of_control_point_weights = cluster->GetControlPointWeights();
			for (int i = 0; i < number_of_control_point_indices; ++i)
			{
				FBX::Bone_Influences_Per_Control_Point& influences_per_control_point
					= influences->at(array_of_control_point_indices[i]);
				FBX::Bone_Influence influence;
				influence.index = index_of_cluster;
				influence.weight = static_cast<float>(array_of_control_point_weights[i]);
				influences_per_control_point.push_back(influence);
			}
		}
	}
}

void FBX_Loader::Traverse(std::vector<fbxsdk::FbxNode*>* fetched_meshes, fbxsdk::FbxNode* node) noexcept
{
	if (node)
	{
		FbxNodeAttribute* fbx_node_attribute = node->GetNodeAttribute();
		if (fbx_node_attribute) {
			switch (fbx_node_attribute->GetAttributeType()) {
				case FbxNodeAttribute::eMesh:
					fetched_meshes->push_back(node);
					break;
			}
		}
		for (int i = 0; i < node->GetChildCount(); i++)
			Traverse(fetched_meshes, node->GetChild(i));
	}
}

void FBX_Loader::FbxMatrix_To_XMFLOAT4X4(
	const fbxsdk::FbxAMatrix& fbxamatrix,
	DirectX::XMFLOAT4X4* xmfloat4x4
) noexcept
{
	assert(xmfloat4x4 && "xmfloat4x4がnullptr");
	for (int row = 0; row < 4; row++)
	{
		for (int column = 0; column < 4; column++)
		{
			xmfloat4x4->m[row][column] = static_cast<float>(fbxamatrix[row][column]);
		}
	}
}

FBX_Loader::FBX_Loader(std::filesystem::path path) noexcept
{
	std::shared_ptr<FBX::Desc> desc = std::make_shared<FBX::Desc>();
	m_desc = desc;
	desc->path = path;

	using namespace fbxsdk;
	FbxManager* fbx_mgr = FbxManager::Create();
	fbx_mgr->SetIOSettings(FbxIOSettings::Create(fbx_mgr, IOSROOT));

	// ここでパスを渡すのは名前を付けるためなので読み込みに関係はない
	FbxImporter* importer = FbxImporter::Create(fbx_mgr, ( "Importer [" + path.string() + "]" ).c_str());
	bool import_status = false;
	import_status = importer->Initialize(path.string().c_str(), -1, fbx_mgr->GetIOSettings());
	FbxScene* scene = nullptr;
	try
	{
		if (!import_status)
		{
			fbx_mgr->Destroy();
			throw std::runtime_error(
				"[ " + desc->path.string() + " ] の読み込み中にエラー : "
				+ importer->GetStatus().GetErrorString());
		}
		scene = FbxScene::Create(fbx_mgr, ("Scene [" + path.string() + "]").c_str());
		import_status = importer->Import(scene);
		if (!import_status)
		{
			fbx_mgr->Destroy();
			throw std::runtime_error(
				"[ " + desc->path.string() + " ] の読み込み中にエラー : "
				+ importer->GetStatus().GetErrorString());
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	//DirectX用座標系に変換
	fbxsdk::FbxAxisSystem::DirectX.ConvertScene(scene);
	//メートルに変換
	FbxSystemUnit::m.ConvertScene(scene);

	FbxGeometryConverter geometry_converter(fbx_mgr);
	geometry_converter.Triangulate(scene, /*replace*/true);

	std::vector<fbxsdk::FbxNode*> fetched_meshes;
	Traverse(&fetched_meshes, scene->GetRootNode());
	desc->meshes.resize(fetched_meshes.size());
	UINT total_mtr_count = 0u;
	for (size_t i = 0; i < fetched_meshes.size(); i++)
	{
		total_mtr_count += fetched_meshes.at(i)->GetMesh()->GetNode()->GetMaterialCount();
	}

	const size_t meshes_size = fetched_meshes.size();
	for (size_t i = 0; i < meshes_size; i++)
	{
		FbxMesh* fbx_mesh = fetched_meshes.at(i)->GetMesh();
		std::vector<FBX::Bone_Influences_Per_Control_Point> bone_influences;
		FetchBoneInfluences(fbx_mesh, &bone_influences);

		auto& mesh = desc->meshes.at(i);

		FbxStringList uv_names;
		fbx_mesh->GetUVSetNames(uv_names);
		const int number_of_materials = fbx_mesh->GetNode()->GetMaterialCount();
		mesh.subsets.resize(number_of_materials > 0 ? number_of_materials : 1);

		fbxsdk::FbxAMatrix global_transform = fbx_mesh->GetNode()->EvaluateGlobalTransform(0);
		DirectX::XMFLOAT4X4 xmtransform;
		FbxMatrix_To_XMFLOAT4X4(global_transform, &xmtransform);
		mesh.global_transform = DirectX::XMLoadFloat4x4(&xmtransform) * DirectX::XMLoadFloat4x4(&FBX::CORDINATE_CONVERSION);

		//マテリアルカラー/テクスチャの取得
		{
			auto directory_pass = path;
			directory_pass.remove_filename();
			if (number_of_materials == 0)
			{
				FBX::Subset& subset = mesh.subsets[0];
			}
			else
			{
				for (int index_of_material = 0; index_of_material < number_of_materials; ++index_of_material)
				{
					FBX::Subset& subset = mesh.subsets[index_of_material];
					const FbxSurfaceMaterial* surface_material = fbx_mesh->GetNode()->GetMaterial(index_of_material);
					const FbxProperty property = surface_material->FindProperty(FbxSurfaceMaterial::sDiffuse);
					const FbxProperty factor = surface_material->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);
					if (property.IsValid() && factor.IsValid())
					{
						FbxDouble3 color = property.Get<FbxDouble3>();
						double f = factor.Get<FbxDouble>();
						subset.diffuse.color.x = static_cast<float>(color[0] * f);
						subset.diffuse.color.y = static_cast<float>(color[1] * f);
						subset.diffuse.color.z = static_cast<float>(color[2] * f);
						subset.diffuse.color.w = 1.0f;
					}
					if (property.IsValid())
					{
						const int number_of_textures = property.GetSrcObjectCount<fbxsdk::FbxFileTexture>();
						if (number_of_textures)
						{
							const fbxsdk::FbxFileTexture* file_texture = property.GetSrcObject<fbxsdk::FbxFileTexture>();
							if (file_texture)
							{
								subset.diffuse.tex_file_pass = directory_pass;
								//std::string relative_file_name = file_texture->GetRelativeFileName();
								TCHAR data[512];
								data[0] = NULL;
								CHARToTCHAR(data, file_texture->GetRelativeFileName());
								subset.diffuse.tex_file_pass += data;
								subset.diffuse.tex_file_pass.make_preferred();
							}
						}
					}
					//各サブセットの開始インデックス番号を決定する
					if (number_of_materials > 0)
					{
						const int number_of_polygons = fbx_mesh->GetPolygonCount();
						for (int index_of_polygon = 0; index_of_polygon < number_of_polygons; ++index_of_polygon)
						{
							const UINT material_index = fbx_mesh->GetElementMaterial()->GetIndexArray().GetAt(index_of_polygon);
							mesh.subsets.at(material_index).index_count += 3;
						}
						int offset = 0;
						for (FBX::Subset& subset : mesh.subsets)
						{
							subset.index_start = offset;
							offset += subset.index_count;
							subset.index_count = 0;
						}
					}

					UINT vertex_count = 0u;
					const FbxVector4* array_of_control_points = fbx_mesh->GetControlPoints();
					const int number_of_polygons = fbx_mesh->GetPolygonCount();
					mesh.vertices.reserve(mesh.vertices.size() + number_of_polygons * 3);
					mesh.indexes.resize(mesh.indexes.size() + number_of_polygons * 3);

					const bool has_normal = fbx_mesh->GetElementNormalCount() > 0;
					const bool has_tex = fbx_mesh->GetElementUVCount() > 0;

					for (int index_of_polygon = 0; index_of_polygon < number_of_polygons; index_of_polygon++)
					{
						int index_of_material = 0;
						if (number_of_materials > 0)
						{
							index_of_material = fbx_mesh->GetElementMaterial()->GetIndexArray().GetAt(index_of_polygon);
						}
						auto& subset = mesh.subsets.at(index_of_material);
						const int index_offset = subset.index_start + subset.index_count;

						for (int index_of_vertex = 0; index_of_vertex < 3; index_of_vertex++)
						{
							int index_of_material = 0;
							if (number_of_materials > 0)
							{
								index_of_material = fbx_mesh->GetElementMaterial()->GetIndexArray().GetAt(index_of_polygon);
							}
							FBX::Vertex vertex;
							DirectX::XMFLOAT3 vertex_point;
							const int index_of_control_point = fbx_mesh->GetPolygonVertex(index_of_polygon, index_of_vertex);
							vertex_point.x = vertex.position.x = static_cast<float>(array_of_control_points[index_of_control_point][0]);
							vertex_point.y = vertex.position.y = static_cast<float>(array_of_control_points[index_of_control_point][1]);
							vertex_point.z = vertex.position.z = static_cast<float>(array_of_control_points[index_of_control_point][2]);

							FBX::Bone_Influences_Per_Control_Point influences_per_control_point = bone_influences.at(index_of_control_point);
							const size_t size_of_influence = influences_per_control_point.size();
							for (size_t index_of_influence = 0u; index_of_influence < size_of_influence; index_of_influence++)
							{
								if (index_of_influence < FBX::MAX_BONE_INFLUENCES)
								{
									vertex.bone_weights[index_of_influence] = influences_per_control_point.at(index_of_influence).weight;
									vertex.bone_indices[index_of_influence] = influences_per_control_point.at(index_of_influence).index;
								}
							}

							if (has_normal)
							{
								FbxVector4 normal;
								fbx_mesh->GetPolygonVertexNormal(index_of_polygon, index_of_vertex, normal);
								vertex.normal.x = static_cast<float>(normal[0]);
								vertex.normal.y = static_cast<float>(normal[1]);
								vertex.normal.z = static_cast<float>(normal[2]);
							}

							if (has_tex)
							{
								FbxVector2 uv;
								bool unmapped_uv;
								fbx_mesh->GetPolygonVertexUV(index_of_polygon, index_of_vertex, uv_names[0], uv, unmapped_uv);
								vertex.uv.x = static_cast<float>(uv[0]);
								vertex.uv.y = 1.0f - static_cast<float>(uv[1]);
							}

							mesh.vertices.push_back(vertex);
							mesh.indexes.at(index_offset + index_of_vertex) = static_cast<UINT>(vertex_count);
							vertex_count += 1;
						}
						subset.index_count += 3;
					}
					FetchAnimations(fbx_mesh, mesh.skeletal_animation);
				}
				fbx_mgr->Destroy();
			}
		}
	}
}