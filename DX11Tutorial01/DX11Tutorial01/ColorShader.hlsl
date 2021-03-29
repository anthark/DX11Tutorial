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
Texture2D NormalTexture : register(t1);

SamplerState Sampler : register(s0);

struct VSInput
{
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float4 worldPos : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

VSOutput VS(in VSInput vertex)
{
	VSOutput output;
	float4 worldPos = mul(vertex.pos, modelMatrix);
	output.pos = mul(worldPos, VP);
	output.worldPos = worldPos;
	output.uv = vertex.uv;
	output.normal = mul(vertex.normal, normalMatrix);
	output.tangent = mul(vertex.tangent, normalMatrix);
	
	return output;
}

float4 PS(in VSOutput input) : SV_Target0
{
	float4 color = float4(0,0,0,1);

	int lightCount = lightParams.x;
	int mode = lightParams.y;

	float3 matColor = ColorTexture.Sample(Sampler, input.uv);
	float3 nm = (NormalTexture.Sample(Sampler, input.uv) - float3(0.5,0.5,0.5)) * 2.0;
	
	float3 binormal = cross(input.normal, input.tangent);
	
	float3 normal = float3(0,0,0);
	if (mode == 0)
	{
		normal = nm.x * input.tangent + nm.y * binormal + input.normal;
	}
	else
	{
		normal = input.normal;
	}
	
	for (int i = 0; i < lightCount; i++)
	{
		float3 l = lights[i].pos.xyz - input.worldPos.xyz;
		float dist = length(l);		
		l = l / dist;
		
		float ndotl = max(dot(l, normal),0);
		float atten = 1.0 / (0.2 + dist * dist);
	
		color.xyz += matColor * lights[i].color.xyz * ndotl * atten;
	}
	
	//color.xyz = 0.5 * (normal + float3(1,1,1));

	return color;
}
