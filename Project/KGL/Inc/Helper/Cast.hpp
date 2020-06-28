#pragma once

namespace KGL
{
	template<typename out, typename in> __forceinline out SCAST(in value) { return static_cast<out>(value); }
	template<typename out, typename in> __forceinline out CCAST(in value) { return const_cast<out>(value); }
	template<typename out, typename in> __forceinline out RCAST(in value) { return reinterpret_cast<out>(value); }
	template<typename out, typename in> __forceinline out DCAST(in value) { return dynamic_cast<out>(value); }
}