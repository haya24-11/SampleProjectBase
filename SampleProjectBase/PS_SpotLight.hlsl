//float4 main() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}

struct PS_IN
{
    float4 pos : SV_POSITION0;
    float3 normal : NORMAL0;
    float2 uv : TEXCOOD0;
    float3 worldPos : POSITION0;
};

cbuffer Light : register(b0)
{
    float4 lightColor;
    float3 lightPos;
    float lightRange;
    float3 lightDir; //光の方向
    float lightAngle; //スポットライトの制限阿久戸
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

float4 main(PS_IN pin):SV_TARGET
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    color = tex.Sample(samp, pin.uv);
    
    
    //各ピクセルから点光源に向かうベクトルを計算
    float3 toLightVec = lightPos - pin.worldPos.xyz;
    float3 V = normalize(toLightVec);       //正規化したベクトル
    float toLightLen = length(toLightVec);  //光源までの距離
    
    //点光源に向かうベクトルと法線から明るさを計算
    float3 N = normalize(pin.normal);
    float dotNV = saturate(dot(N, V));
    
    //距離に応じて光の強さを変える
    
    float attenuation = saturate(1.0f - toLightLen / lightRange);
    
    
    attenuation = pow(attenuation, 2.0f);
    
    float3 L = normalize(-lightDir);
    
    //二つのベクトルが正規化されている場合、間の角度は
    //dot(v1,v2)=cosΘ
    float dotVL = dot(V, L);
    float angle = acos(dotVL);
    
    //角度に応じて明るさを計算
    float diffAngle = (lightAngle * 0.5f) * 0.1f;
    float spotAngle = ((lightAngle * 0.5f) + diffAngle);
    float spotRate = (spotAngle - angle) / diffAngle;
    spotRate = pow(saturate(spotRate), 2.0f);
    color.rgb *= dotNV * attenuation * spotRate;
    
    return color;

}
