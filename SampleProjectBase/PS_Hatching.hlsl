struct PS_IN
{
	float4 pos : SV_POSITION0;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL0;
	float4 worldPos : POSITION0;
};

// ライトの情報
cbuffer Light : register(b0)
{
	float4 lightColor;
	float4 lightAmbient;
	float4 lightDir;
};

SamplerState samp : register(s0);
Texture2D tex : register(t0);

// コメントアウトを解除すると追加で横線が入る
float4 main(PS_IN pin) : SV_TARGET
{
	float4 color = tex.Sample(samp, pin.uv);

	//// y座標をそのまま出力すると、真っすぐな直線になるので
	//// x座標を利用して揺らぎをだす
	//float sideLineRandom = sin(pin.worldPos.x * 10.0f) * 10.0f;

	//// モデルのy座標をsinに渡して、周期的な直線として表現
	//float sideLine = sin(
	//	pin.worldPos.y * 1000.0f + // 線の本数を変更
	//	sideLineRandom // 線の揺らぎ
	//);
	//sideLine = step(-0.8f, sideLine); // 線の太さを変更

	// 別方向からの直線を追加
	// なるべく同じタッチにしたいので似たパラメータを設定
	float obliqueLine = sin(
		pin.worldPos.y * 1000.0f +
		pin.worldPos.x * 300.0f // x方向に向かって斜線になるよう補正
	);
	obliqueLine = step(-0.8f, obliqueLine);

	
	// 基本的な影の計算
	float3 N = normalize(pin.normal);
	float3 L = normalize(-lightDir.xyz);
	float dotNL = dot(N, L);

	// 絵のタッチになるように、テクスチャをモノクロ化
	color.rgb = color.r * 0.6f + color.g * 0.3f + color.b * 0.1f;

	//// 影の濃さに応じて、ハッチングを掛ける
	//color.rgb = lerp(
	//	color.rgb * sideLine, // 元のテクスチャにハッチングを適用
	//	color.rgb, // 元のテクスチャ(ハッチングなし
	//	1.0f - pow(1.0 - saturate(dotNL), 2.0f)
	//);

	// より暗くなる部分に、さらにハッチングをかける
	color.rgb = lerp(
		color.rgb, // 直前に計算した比較的明るいハッチング
		color.rgb * obliqueLine, // 斜線を重ねたハッチング
		1.0f - pow(1.0f - saturate(-dotNL), 3.0f)
	);

	return color;
}