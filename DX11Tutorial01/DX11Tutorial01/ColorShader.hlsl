cbuffer ModelBuffer : register(b0)
{
    float4x4 modelMatrix;
}

struct VSInput
{
	float4 pos : POSITION;
	float4 color : COLOR;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
};

VSOutput VS(in VSInput vertex)
{
	VSOutput output;
	output.pos = mul(vertex.pos, modelMatrix);
	output.color = vertex.color;
	
	return output;
}

float4 PS(in VSOutput input) : SV_Target0
{
	return input.color;
}
