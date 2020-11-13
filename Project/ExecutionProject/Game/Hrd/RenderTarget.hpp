#pragma once

#include <Dx12/Texture.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/Base/Renderer.hpp>

struct RenderTargetResource
{
	struct Texture
	{
		std::shared_ptr<KGL::Texture>		tex;
		KGL::DescriptorHandle				gui_srv_handle;
	};
	std::vector<Texture>						render_targets;
	std::shared_ptr<KGL::RenderTargetView>		rtvs;
	KGL::DescriptorHandle						dsv_handle;
	std::shared_ptr<KGL::Texture>				depth_stencil;
	KGL::DescriptorHandle						depth_srv_handle;
	KGL::DescriptorHandle						depth_gui_srv_handle;
};
struct BoardRenderers
{
	std::shared_ptr<KGL::BaseRenderer>			simple;
	std::shared_ptr<KGL::BaseRenderer>			add_pos;
	std::shared_ptr<KGL::BaseRenderer>			simple_wire;
	std::shared_ptr<KGL::BaseRenderer>			add_pos_wire;
	std::shared_ptr<KGL::BaseRenderer>			dsv;
	std::shared_ptr<KGL::BaseRenderer>			dsv_add_pos;
};