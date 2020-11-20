cbuffer FrameBuffer : register(b0)
{
	uint value_size;
}

cbuffer StepBuffer : register(b1)
{
	uint block_step;
	uint sub_block_step;
}


// 出力バッファ
RWStructuredBuffer<int> in_values : register(u0);
RWStructuredBuffer<int> out_values : register(u1);

[numthreads(64, 1, 1)]
void CSMain( uint3 idx_tid : SV_DispatchThreadID )
{
    uint idx = idx_tid.x;

	// idx がオーバーしている
	if (idx >= value_size) return;

	uint d = 1u << (block_step - sub_block_step);

	bool up = ((idx >> block_step) & 2u) == 0u;

	uint target_index = 0u;
    if ((idx & d) == 0u)
    {
        target_index = idx | d;
    }
    else
    {
        target_index = idx & ~d;
        up = !up;
    }

    float a = in_values[idx];
    float b = in_values[target_index];

    if ((a > b) == up || a == b)
    {
        out_values[idx] = in_values[target_index];
    }
    else
    {
        out_values[idx] = in_values[idx];
    }
}