cbuffer ModelBuffer : register(b0)
{
    float4x4 modelMatrix;
	float4x4 normalMatrix;
	float4 objColor;
}

struct Light
{
	float4 pos;
	float4 color;
};

cbuffer SceneBuffer : register(b1)
{
    float4x4 VP;
	int4 lightParams;
	Light lights[4];
}

struct VSInput
{
	float4 pos : POSITION;
};

struct VSOutput
{
	float4 pos : SV_Position;
};

VSOutput VS(in VSInput vertex)
{
	VSOutput output;
	float4 worldPos = mul(vertex.pos, modelMatrix);
	output.pos = mul(worldPos, VP);
	
	return output;
}

float4 PS(in VSOutput input) : SV_Target0
{
	return objColor;
}
