cbuffer FrameBuffer : register(b0)
{
	uint value_size;
}

cbuffer StepBuffer : register(b1)
{
	uint block;
	uint step;
}


// 出力バッファ
RWStructuredBuffer<int> in_values : register(u0);
RWStructuredBuffer<int> out_values : register(u1);

[numthreads(64, 1, 1)]
void CSMain( uint3 idx_tid : SV_DispatchThreadID )
{
    uint idx = idx_tid.x;

	if (idx >= 1 << value_size)
		return;

	uint e = idx ^ step;

	if (e > idx)
	{
		uint v1 = in_values[idx];
		uint v2 = in_values[e];

		if ((idx & block) != 0)
		{
			if (v1 < v2)
			{
				in_values[e] = v1;
				in_values[idx] = v2;
			}
		}
		else
		{
			if (v1 > v2)
			{
				in_values[e] = v1;
				in_values[idx] = v2;
			}
		}
	}
}