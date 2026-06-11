struct PS_IN
{
	float4 pos : SV_POSITION0;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL0;
	float4 worldPos : POSITION0;
	float4 screenPos : POSITION1;
};

cbuffer Camera : register(b0)
{
	float4 camPos;
};

Texture2D tex : register(t0);
Texture2D ditherTex : register(t1);
SamplerState samp : register(s0);

float4 main(PS_IN pin) : SV_TARGET
{
	float4 color = tex.Sample(samp, pin.uv);

	// 距離に応じて透明度を変更
	float len = length(pin.worldPos.xyz - camPos.xyz);
	float alpha = color.a * saturate(len * 0.2f);

	// スクリーンの座標に応じて、
	// ディザパターンを適用する位置を計算
	float2 screenUV = pin.screenPos.xy / pin.screenPos.w;
	screenUV = screenUV * 0.5f + 0.5f;
	screenUV *= float2(1280.0f, 720.f) / 4.0f;

	// ディザパターンの取得
	float dither = ditherTex.Sample(samp, screenUV).r;

	if (alpha < dither)
	{
		// 透明度0で見えないけどそこにある、ではなく完全に表示を行わない
		discard;
	}

	return color;
}



