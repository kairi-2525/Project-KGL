#pragma once

#include "../../Loader/Loader.hpp"

namespace KGL
{
	inline namespace DX12
	{
		inline namespace _3D
		{
			class StaticModel
			{
			private:
			public:
				explicit StaticModel(std::shared_ptr<const StaticModelLoader> loader) noexcept;
			};
		}
	}
}