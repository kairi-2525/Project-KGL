#pragma once

using UINT = unsigned int;

class MSAASelector
{
public:
	enum TYPE : UINT
	{
		MSAA_OFF,
		MSAAx2,
		MSAAx4,
		MSAAx8,
		MSAAx16,
		MSAAx32
	};
private:
	TYPE max_scale;
	TYPE set_scale;
public:
	static UINT TypeToCount(TYPE type);
	static TYPE CountToType(UINT count);
public:
	MSAASelector(UINT max_sample_count);
	TYPE GetMaxScale() const { return max_scale; }
	void SetScale(TYPE scale);
	TYPE GetScale() const { return set_scale; }
};