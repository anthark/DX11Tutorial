cbuffer ModelBuffer : register(b0)
{
    float4x4 modelMatrix;
}

cbuffer SceneBuffer : register(b1)
{
    float4x4 VP;
}

Texture2D ColorTexture : register(t0);

SamplerState Sampler : register(s0);

struct VSInput
{
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD;
};

VSOutput VS(in VSInput vertex)
{
	VSOutput output;
	output.pos = mul(mul(vertex.pos, modelMatrix), VP);
	output.uv = vertex.uv;
	
	return output;
}

float4 PS(in VSOutput input) : SV_Target0
{
	//return float4(input.uv, 0, 0);
	return ColorTexture.Sample(Sampler, input.uv);
}
