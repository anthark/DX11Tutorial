cbuffer ModelBuffer : register(b0)
{
    float4x4 modelMatrix;
	float4x4 normalMatrix;
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

Texture2D ColorTexture : register(t0);

SamplerState Sampler : register(s0);

struct VSInput
{
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float4 worldPos : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
};

VSOutput VS(in VSInput vertex)
{
	VSOutput output;
	float4 worldPos = mul(vertex.pos, modelMatrix);
	output.pos = mul(worldPos, VP);
	output.worldPos = worldPos;
	output.uv = vertex.uv;
	output.normal = mul(vertex.normal, normalMatrix);
	
	return output;
}

float4 PS(in VSOutput input) : SV_Target0
{
	float4 color = float4(0,0,0,1);

	int lightCount = lightParams.x;

	float3 matColor = ColorTexture.Sample(Sampler, input.uv);
	for (int i = 0; i < lightCount; i++)
	{
		float3 l = lights[i].pos.xyz - input.worldPos.xyz;
		float dist = length(l);		
		l = l / dist;
		
		float ndotl = max(dot(l, input.normal),0);
		float atten = 1.0 / (0.2 + dist * dist);
	
		color.xyz += matColor * lights[i].color.xyz * ndotl * atten;
	}

	return color;
}
