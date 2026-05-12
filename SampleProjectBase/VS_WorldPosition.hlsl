//頂点シェーダーへの入力
struct VS_IN
{
    float3 pos      : POSITIONT0;
    float3 normal   : NORMAL0;
    float2 uv : TEXCOORD0;
};


//頂点シェーダーからの出力→ピクセルシェーダーに出力
struct VS_OUT
{
    float4 pos    : SV_POSITION;
    float3 normal : NORMAL0;
    float2 uv     : TEXCOORD0;
    float3 worldPos: POSITION0;
};

//定数バッファ
cbuffer wvp : register(b0)
{
    float4x4 world;     //ワールド行列
    float4x4 view;     //ビュー行列
    float4x4 proj;      //プロジェクト行列
}


//頂点シェーダーのメイン処理
//この処理を頂点ごとに実行します   
VS_OUT main(VS_IN vin)
{
    VS_OUT vout;
    
    //座標変換
    vout.pos = float4(vin.pos, 1.0f);
    vout.pos = mul(vout.pos, world);
    vout.worldPos = vout.pos;
    vout.pos = mul(vout.pos, view);
    vout.pos = mul(vout.pos, proj);

    vout.normal = mul(vin.normal, (float3x3) world);
    
    vout.uv=vin.uv;
    
    
    return vout;
}