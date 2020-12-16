#pragma once

#include <Cereal/cereal.hpp>
#include <filesystem>

namespace std
{
	namespace filesystem
	{
		template<class Archive>
		std::string save_minimal(
			Archive const&,
			path const& m)
		{
			return m.string();
		}
		template<class Archive>
		void load_minimal(
			Archive const&,
			path& m,
			std::string const& value)
		{
			m = value;
		}
	}
}