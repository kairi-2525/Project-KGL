#pragma once

#include <Math/Planets.hpp>

struct PlayerShotParametor
{

	static inline float BLACK_HOLL_MASS = KGL::PLANET::EARTH.mass / 100000000;
	static inline float WHITE_HOLL_MASS = -BLACK_HOLL_MASS;
public:
	bool random_color;
	bool use_mass;
	float mass;
};