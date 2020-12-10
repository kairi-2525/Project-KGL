#pragma once

#include <string>

namespace KGL
{
	inline namespace MATH
	{
		struct Planet
		{
			float mass;
			float radius;
		};

		struct NamePlanet : public Planet
		{
			std::string name;
		};
		
		namespace PLANET
		{
			extern inline const NamePlanet SUN		= { 1.989e30f,	6.960e8f,	"‘¾—z"};
			extern inline const NamePlanet MERCURY	= { 3.285e23f,	2.4397e6f,	"…¯"};
			extern inline const NamePlanet VENUS	= { 4.867e24f,	6.0518e6f,	"‹à¯"};
			extern inline const NamePlanet EARTH	= { 5.9724e24f,	6.3781e6f,	"’n‹…"};
			extern inline const NamePlanet MOON		= { 7.3458e22f,	1.7371e6f,	"Œ"};
			extern inline const NamePlanet MARS		= { 6.39e23f,	3.3895e6f,	"‰Î¯"};
			extern inline const NamePlanet JUPITER	= { 1.8986e27f,	6.9911e7f,	"–Ø¯"};
			extern inline const NamePlanet SATURN	= { 5.683e26f,	5.8232e7f,	"“y¯"};
			extern inline const NamePlanet URANUS	= { 8.681e25f,	2.5362e7f,	"“V‰¤¯"};
			extern inline const NamePlanet NEPTUNE	= { 1.024e26f,	2.4622e7f,	"ŠC‰¤¯"};
		}
	}
}