//float4 main() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}


//ピクセルシェーダーの入力
struct PS_IN
{
    float4 pos      : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 worldPos    : POSITION0;
};

//ライト情報を保持する定数バッファ
cbuffer Light : register(b0)
{
  
    float4 lightDiffuse;
    float4 lightAmbiment;
    float4 lightDir;
};

//カメラの定数バッファ
Texture2D tex : register(t0);

//サンプラーへのバインド
SamplerState samp : register(s0);

cbuffer Camer : register(b1)
{
    float4 camPos; //カメラの位置
    
};



float4 main(PS_IN pin) : SV_TARGET
{
    float4 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    color = tex.Sample(samp, pin.uv);

    float3 N = normalize(pin.normal);
    float3 L = normalize(-lightDir.xyz);
    float diffuse = saturate(dot(N, L));

    color *= diffuse * lightDiffuse + lightAmbiment;
    
    float3 V = normalize(camPos.xyz - pin.worldPos);
    float3 R = normalize(reflect(-L, N));
    
    float specular = saturate(dot(V, R));
    
    color += pow(specular, 5.0f);
    
    color.a = 1.0f;
    
    
    return color;
}