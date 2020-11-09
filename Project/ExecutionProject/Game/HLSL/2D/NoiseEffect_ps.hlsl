// Noise animation - Flow
// 2014 by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/MdlXRS
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
// Contact the author for other licensing options


//Somewhat inspired by the concepts behind "flow noise"
//every octave of noise is modulated separately
//with displacement using a rotated vector field

//normalization is used to created "swirls"
//usually not a good idea, depending on the type of noise
//you are going for.

//Sinus ridged fbm is used for better effect.

#include "Sprite.hlsli"

Texture2D<float4> tex : register (t0);
SamplerState smp : register (s0, space0);

cbuffer FrameBuffer : register (b0)
{
	float3 color;
	float time;
	float2 resolution;
	float rotate_scale;
}

static const float TAU = 6.2831853f;

static const row_major float2x2 M2 = { 0.8f, -0.6f, 0.6f, 0.8f };

float mod(float x, float y)
{
	return x - y * floor(x / y);
}

row_major float2x2 Makem2(float thata)
{
	float c = cos(thata);
	float s = sin(thata);

	return (row_major float2x2)( c, s, -s, c );
}

float Noise(float2 x)
{
	//return 0.f;
	return tex.Sample(smp, x * 0.01f).x;
}

float Grid(float2 p)
{
	return sin(p.x) * cos(p.y);
}

float Flow(float2 p)
{
	float z = 2.f;
	float rz = 0.f;
	float2 bp = p;

	for (uint i = 1; i < 7; i++)
	{
		bp += time * 1.5f;
		float2 gr;
		gr.x = Grid(p * 3.f - time * 2.f);
		gr.y = Grid(p * 3.f + 4.f - time * 2.f);

		gr = normalize(gr) * 0.4f;
		gr = mul(gr, Makem2((p.x + p.y) * 0.3f + time * 10.f));
		//gr = mul(Makem2((p.x + p.y) * 0.3f + time * 10.f), gr);
		p += gr * 0.5f;

		rz += (sin(Noise(p) * 8.f) * 0.5f + 0.5f) / z;

		p = lerp(bp, p, 0.5f);
		z *= 1.7f;
		p *= 2.5f;
		p = mul(p, M2);
		//p = mul(M2, p);
		bp *= 2.5f;
		bp = mul(bp, M2);
		//bp = mul(M2, bp);
	}
	return rz;
}

float Spiral(float2 p, float scl)
{
	float r = length(p);
	r = log(r);
	float a = atan2(p.y, p.x);
	return abs(mod(scl * (r - 2.f / scl * a), TAU) - 1.f) * 2.f;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float2 p;

	float2 uv = ((input.uv * resolution.xy) - 0.5 * resolution.xy) / resolution.y;
	
	p = uv;

	p *= 3.f;

	float rz = Flow(p);
	p /= exp(mod(time * 3.f * rotate_scale, 2.1f));
	rz *= (6.f - Spiral(p, 3.f)) * 0.9f;
	float3 col = color / rz;
	col = pow(abs(col), float3(1.01f, 1.01f, 1.01f));

	return float4(col, 1.0f);
}