#include "../Hrd/SkyMap.hpp"

SkyManager::SkyManager(std::string folder, std::string name1, std::string name2)
{

}

void SkyManager::Update()
{

}

void SkyManager::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list)
{
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	renderer->SetState(cmd_list);
	//cmd_list->SetDescriptorHeaps(1, buffer.handle.Heap().GetAddressOf());
	//cmd_list->SetGraphicsRootDescriptorTable(0, buffer.handle.Gpu());
	for (int i = 0; i < CUBE::NUM; i++)
	{
		cmd_list->IASetVertexBuffers(0, 1, &vbv[i]);
		cmd_list->SetDescriptorHeaps(1, select->handle[i].Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(1, select->handle[i].Gpu());
		cmd_list->DrawInstanced(4, 1, 0, 0);
	}
}