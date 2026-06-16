static const float PI = 3.14159265358979f;

cbuffer Light : register(b0)
{
	float4 lightColor;
	float4 lightAmbient;
	float4 lightDir;
};
cbuffer Camera : register(b1)
{
	float4 camPos;
};
cbuffer PBRParam : register(b2)
{
	float metallic;
	float roughness;
	float2 dummy;
};

SamplerState samp     : register(s0);
Texture2D    albedoTex : register(t0);
Texture2D    normalMap : register(t1);

struct PS_IN
{
	float4 pos      : SV_POSITION0;
	float2 uv       : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
	float3 lightT   : NORMAL0;
	float3 viewT    : NORMAL1;
};

// GGX 法線分布関数
float D_GGX(float NdotH, float r)
{
	float a  = r * r;
	float a2 = a * a;
	float d  = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
	return a2 / (PI * d * d);
}

// Smith-Schlick 幾何減衰
float G_SchlickGGX(float NdotV, float r)
{
	float k = (r + 1.0f) * (r + 1.0f) / 8.0f;
	return NdotV / (NdotV * (1.0f - k) + k);
}
float G_Smith(float NdotV, float NdotL, float r)
{
	return G_SchlickGGX(NdotV, r) * G_SchlickGGX(NdotL, r);
}

// Schlick フレネル近似
float3 F_Schlick(float HdotV, float3 F0)
{
	return F0 + (1.0f - F0) * pow(1.0f - HdotV, 5.0f);
}

float4 main(PS_IN pin) : SV_TARGET
{
	// 法線マップからタンジェント空間の法線を取得
	float3 N = normalMap.Sample(samp, pin.uv).rgb;
	N = normalize(N * 2.0f - 1.0f);

	float3 L = normalize(-pin.lightT);
	float3 V = normalize(-pin.viewT);
	float3 H = normalize(L + V);

	float NdotL = saturate(dot(N, L));
	float NdotV = saturate(dot(N, V)) + 1e-5f;
	float NdotH = saturate(dot(N, H));
	float HdotV = saturate(dot(H, V));

	float4 albedo = albedoTex.Sample(samp, pin.uv);

	// 基本反射率: 非金属=0.04、金属=アルベド色
	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, metallic);

	// Cook-Torrance BRDF
	float  D = D_GGX(NdotH, roughness);
	float  G = G_Smith(NdotV, NdotL, roughness);
	float3 F = F_Schlick(HdotV, F0);

	float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 1e-4f);
	float3 kD       = (1.0f - F) * (1.0f - metallic);
	float3 diffuse  = kD * albedo.rgb / PI;

	float3 Lo      = (diffuse + specular) * lightColor.rgb * NdotL;
	float3 ambient = lightAmbient.rgb * albedo.rgb;
	float3 color   = Lo + ambient;

	// Reinhard トーンマッピング
	color = color / (color + 1.0f);

	return float4(color, albedo.a);
}
