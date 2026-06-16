struct VS_IN
{
	float3 pos     : POSITION0;
	float3 normal  : NORMAL0;
	float2 uv      : TEXCOORD0;
	float3 tangent : TANGENT0;
};

struct VS_OUT
{
	float4 pos      : SV_POSITION0;
	float2 uv       : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 lightT   : NORMAL0;   // タンジェント空間のライト方向
	float3 viewT    : NORMAL1;   // タンジェント空間の視線方向
};

cbuffer WVP : register(b0)
{
	float4x4 world;
	float4x4 view;
	float4x4 proj;
};
cbuffer Light : register(b1)
{
	float4 lightColor;
	float4 lightAmbient;
	float4 lightDir;
};
cbuffer Camera : register(b2)
{
	float4 camPos;
};

VS_OUT main(VS_IN vin)
{
	VS_OUT vout;

	vout.pos = float4(vin.pos, 1.0f);
	vout.pos = mul(vout.pos, world);
	vout.worldPos = vout.pos.xyz;
	vout.pos = mul(vout.pos, view);
	vout.pos = mul(vout.pos, proj);
	vout.uv = vin.uv;

	// TBN行列を構築してタンジェント空間へ変換
	float3 N = normalize(mul(vin.normal,  (float3x3)world));
	float3 T = normalize(mul(vin.tangent, (float3x3)world));
	float3 B = normalize(cross(T, N));
	float3x3 TBN = transpose(float3x3(T, B, N));

	vout.lightT = mul(normalize(lightDir.xyz), TBN);
	vout.viewT  = mul(normalize(vout.worldPos - camPos.xyz), TBN);

	return vout;
}
