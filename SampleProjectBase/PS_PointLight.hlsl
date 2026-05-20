//float4 main() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}

struct PS_IN
{
    float4 pos : SV_POSITION0;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
    float3 worldPos : POSSITION0;
};

struct Light
{
    float4 color;
    float3 pos;
    float range;
};

cbuffer LightBuf : register(b0)
{
    Light lights[30];
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

float4 main(PS_IN pin):SV_TARGET
{
    float4 color = float4(0.0f, 0.0f, 0.0f,1.0f);
    float4 texcolor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    texcolor = tex.Sample(samp, pin.uv);
    //ピクセルの1点に対して、全ての点光源と明るさを計算する
    for (int i = 0; i < 30;++i)
    {
        //各ピクセルから点光源に向かうベクトルを計算
        float3 toLightVec = lights[i].pos - pin.worldPos.xyz;
        float3 V = normalize(toLightVec);   //正規化したベクトル
        float toLightLen = length(toLightVec);//光源までの距離
        
       //点光源に向かうベクトルと法線から明るさを計算
        float3 N = normalize(pin.normal);
        float dotNV = saturate(dot(N, V));
        
        //距離に応じて光の強さを変える
        //距離が近ければ明るく離れていれば暗くなるよう計算
        float attenuation = saturate(1.0f - toLightLen / lights[i].range);
        
        //自然な減衰の計算
        //距離に応じて光の中麺が乗算で増えていく
        attenuation = pow(attenuation, 2.0f);
        
        //複数の点光源の光を合算していく
        color.rgb += lights[i].color.rgb * dotNV * attenuation;
        
        
    }
    texcolor.rgb *= color.rgb;//テクスチャの色に光の色を乗算して最終的な色を計算
    return texcolor;
}