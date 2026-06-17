#include "ScenePBR.h"
#include "Model.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Input.h"
#include "Main.h"
#include "Texture.h"
#include <cmath>

// ----------------------------------------------------------------
// HLSL ソース (ランタイムコンパイル)
// ----------------------------------------------------------------

static const char* VS_SHADOW_SRC = R"(
struct VS_IN {
    float3 pos     : POSITION0;
    float3 normal  : NORMAL0;
    float2 uv      : TEXCOORD0;
    float3 tangent : TANGENT0;
};
struct VS_OUT {
    float4 pos   : SV_POSITION;
    float  depth : TEXCOORD0;
};
cbuffer WVP : register(b0) {
    float4x4 world;
    float4x4 view;
    float4x4 proj;
};
VS_OUT main(VS_IN vin) {
    VS_OUT o;
    float4 p = mul(float4(vin.pos, 1.0f), world);
    p = mul(p, view);
    p = mul(p, proj);
    o.pos   = p;
    o.depth = p.z / p.w;
    return o;
}
)";

static const char* PS_SHADOW_SRC = R"(
struct PS_IN { float4 pos : SV_POSITION; float depth : TEXCOORD0; };
float4 main(PS_IN pin) : SV_TARGET { return float4(pin.depth, 0, 0, 1); }
)";

static const char* VS_PBR_SRC = R"(
struct VS_IN {
    float3 pos     : POSITION0;
    float3 normal  : NORMAL0;
    float2 uv      : TEXCOORD0;
    float3 tangent : TANGENT0;
};
struct VS_OUT {
    float4 pos       : SV_POSITION0;
    float2 uv        : TEXCOORD0;
    float3 worldPos  : TEXCOORD1;
    float3 lightT    : NORMAL0;
    float3 viewT     : NORMAL1;
    float4 shadowPos : TEXCOORD2;
};
cbuffer WVP : register(b0) {
    float4x4 world;
    float4x4 view;
    float4x4 proj;
};
cbuffer Light : register(b1) {
    float4 lightColor;
    float4 lightAmbient;
    float4 lightDir;
};
cbuffer Camera : register(b2) {
    float4 camPos;
};
cbuffer LightMatrix : register(b3) {
    float4x4 lightViewProj;
};
VS_OUT main(VS_IN vin) {
    VS_OUT vout;
    float4 wp = mul(float4(vin.pos, 1.0f), world);
    vout.worldPos  = wp.xyz;
    vout.shadowPos = mul(wp, lightViewProj);
    vout.pos = mul(wp, view);
    vout.pos = mul(vout.pos, proj);
    vout.uv = vin.uv;
    float3 N = normalize(mul(vin.normal,  (float3x3)world));
    float3 T = normalize(mul(vin.tangent, (float3x3)world));
    float3 B = normalize(cross(T, N));
    float3x3 TBN = transpose(float3x3(T, B, N));
    vout.lightT = mul(normalize(lightDir.xyz), TBN);
    vout.viewT  = mul(normalize(wp.xyz - camPos.xyz), TBN);
    return vout;
}
)";

static const char* PS_PBR_SRC = R"(
static const float PI = 3.14159265358979f;
cbuffer Light : register(b0) {
    float4 lightColor;
    float4 lightAmbient;
    float4 lightDir;
};
cbuffer Camera : register(b1) {
    float4 camPos;
};
cbuffer PBRParam : register(b2) {
    float metallic;
    float roughness;
    float dayFactor;
    float dummy;
};
SamplerState samp      : register(s0);
Texture2D    albedoTex : register(t0);
Texture2D    normalMap : register(t1);
Texture2D    shadowMap : register(t2);
struct PS_IN {
    float4 pos       : SV_POSITION0;
    float2 uv        : TEXCOORD0;
    float3 worldPos  : TEXCOORD1;
    float3 lightT    : NORMAL0;
    float3 viewT     : NORMAL1;
    float4 shadowPos : TEXCOORD2;
};
float D_GGX(float NdotH, float r) {
    float a = r*r; float a2 = a*a;
    float d = NdotH*NdotH*(a2-1.0f)+1.0f;
    return a2/(PI*d*d);
}
float G_SchlickGGX(float NdotV, float r) {
    float k = (r+1.0f)*(r+1.0f)/8.0f;
    return NdotV/(NdotV*(1.0f-k)+k);
}
float G_Smith(float NdotV, float NdotL, float r) {
    return G_SchlickGGX(NdotV,r)*G_SchlickGGX(NdotL,r);
}
float3 F_Schlick(float HdotV, float3 F0) {
    return F0+(1.0f-F0)*pow(1.0f-HdotV,5.0f);
}
float4 main(PS_IN pin) : SV_TARGET {
    float3 N = normalMap.Sample(samp, pin.uv).rgb;
    N = normalize(N*2.0f-1.0f);
    float3 L = normalize(-pin.lightT);
    float3 V = normalize(-pin.viewT);
    float3 H = normalize(L+V);
    float NdotL = saturate(dot(N,L));
    float NdotV = saturate(dot(N,V))+1e-5f;
    float NdotH = saturate(dot(N,H));
    float HdotV = saturate(dot(H,V));
    float4 albedo = albedoTex.Sample(samp, pin.uv);
    float3 F0 = lerp(float3(0.04f,0.04f,0.04f), albedo.rgb, metallic);
    float  D = D_GGX(NdotH, roughness);
    float  G = G_Smith(NdotV, NdotL, roughness);
    float3 F = F_Schlick(HdotV, F0);
    float3 specular = (D*G*F)/max(4.0f*NdotV*NdotL, 1e-4f);
    float3 kD = (1.0f-F)*(1.0f-metallic);
    float3 diffuse = kD*albedo.rgb/PI;
    // シャドウ (昼間のみ)
    float shadow = 1.0f;
    if (dayFactor > 0.1f) {
        float3 proj = pin.shadowPos.xyz / pin.shadowPos.w;
        float2 suv  = proj.xy * 0.5f + 0.5f;
        suv.y = 1.0f - suv.y;
        if (suv.x > 0.0f && suv.x < 1.0f && suv.y > 0.0f && suv.y < 1.0f
            && proj.z >= 0.0f && proj.z <= 1.0f) {
            float shadowDepth = 0.0f;
            float texelSize = 1.0f / 1024.0f;
            for (int sy = -1; sy <= 1; ++sy)
                for (int sx = -1; sx <= 1; ++sx) {
                    float2 off = float2(sx, sy) * texelSize;
                    float closest = shadowMap.Sample(samp, suv + off).r;
                    shadowDepth += (proj.z - 0.0005f > closest) ? 1.0f : 0.0f;
                }
            shadowDepth /= 9.0f;
            shadow = 1.0f - shadowDepth * dayFactor * 0.95f;
        }
    }
    float3 Lo     = (diffuse+specular)*lightColor.rgb*NdotL*shadow;
    float3 ambient = lightAmbient.rgb*albedo.rgb;
    float3 color  = Lo+ambient;
    color = color/(color+1.0f);
    return float4(color, albedo.a);
}
)";

// ----------------------------------------------------------------
// 頂点構造体 / パラメータ構造体
// ----------------------------------------------------------------

struct PBRVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT3 tangent;
};

struct PBRParam
{
	float metallic;
	float roughness;
	float dayFactor;
	float dummy;
};

// ----------------------------------------------------------------
// タンジェント計算ラムダ生成
// ----------------------------------------------------------------

static auto MakeTangentCalc(float uvScale = 1.0f)
{
	return [uvScale](Model::RemakeInfo& data)
	{
		PBRVertex* dst = reinterpret_cast<PBRVertex*>(data.dest);
		const Model::Vertex* src = reinterpret_cast<const Model::Vertex*>(data.source);

		for (UINT i = 0; i < data.vtxNum; ++i)
		{
			dst[i].pos     = src[i].pos;
			dst[i].normal  = src[i].normal;
			dst[i].uv      = { src[i].uv.x * uvScale, src[i].uv.y * uvScale };
			dst[i].tangent = { 1.0f, 0.0f, 0.0f };
		}

		using TanList = std::vector<DirectX::XMFLOAT3>;
		std::vector<TanList> tanPerVtx(data.vtxNum);
		const UINT* idx = reinterpret_cast<const UINT*>(data.idx);

		for (UINT i = 0; i < data.idxNum; i += 3)
		{
			UINT i0 = idx[i], i1 = idx[i+1], i2 = idx[i+2];
			auto& p0 = dst[i0].pos; auto& p1 = dst[i1].pos; auto& p2 = dst[i2].pos;
			auto& t0 = dst[i0].uv;  auto& t1 = dst[i1].uv;  auto& t2 = dst[i2].uv;
			DirectX::XMFLOAT3 V1(p1.x-p0.x, p1.y-p0.y, p1.z-p0.z);
			DirectX::XMFLOAT3 V2(p2.x-p0.x, p2.y-p0.y, p2.z-p0.z);
			DirectX::XMFLOAT2 ST1(t1.x-t0.x, t1.y-t0.y);
			DirectX::XMFLOAT2 ST2(t2.x-t0.x, t2.y-t0.y);
			float f = ST1.x*ST2.y - ST2.x*ST1.y;
			if (fabsf(f) < 1e-8f) continue;
			DirectX::XMFLOAT3 T(
				(ST2.y*V1.x - ST1.y*V2.x)/f,
				(ST2.y*V1.y - ST1.y*V2.y)/f,
				(ST2.y*V1.z - ST1.y*V2.z)/f
			);
			float len = sqrtf(T.x*T.x + T.y*T.y + T.z*T.z);
			if (len > 1e-8f) { T.x/=len; T.y/=len; T.z/=len; }
			tanPerVtx[i0].push_back(T);
			tanPerVtx[i1].push_back(T);
			tanPerVtx[i2].push_back(T);
		}

		for (UINT i = 0; i < data.vtxNum; ++i)
		{
			if (tanPerVtx[i].empty()) continue;
			DirectX::XMFLOAT3 sum(0,0,0);
			for (auto& t : tanPerVtx[i]) { sum.x+=t.x; sum.y+=t.y; sum.z+=t.z; }
			float n = (float)tanPerVtx[i].size();
			dst[i].tangent = { sum.x/n, sum.y/n, sum.z/n };
		}
	};
}

// ----------------------------------------------------------------
// Init
// ----------------------------------------------------------------

void ScenePBR::Init()
{
	// シェーダーをソースからコンパイル
	struct ShaderDef { const char* name; const char* src; bool isVS; };
	ShaderDef defs[] = {
		{ "VS_Shadow", VS_SHADOW_SRC, true  },
		{ "PS_Shadow", PS_SHADOW_SRC, false },
		{ "VS_PBR",    VS_PBR_SRC,    true  },
		{ "PS_PBR",    PS_PBR_SRC,    false },
	};
	for (auto& d : defs)
	{
		Shader* s = d.isVS
			? (Shader*)CreateObj<VertexShader>(d.name)
			: (Shader*)CreateObj<PixelShader>(d.name);
		if (FAILED(s->Compile(d.src)))
			MessageBox(NULL, d.name, "Shader Compile Error", MB_OK);
	}

	// シャドウマップ: 深度値をR32_FLOATのカラーテクスチャに書き込む方式
	RenderTarget* pShadowRT = CreateObj<RenderTarget>("ShadowMap");
	pShadowRT->Create(DXGI_FORMAT_R32_FLOAT, SHADOW_SIZE, SHADOW_SIZE);
	DepthStencil* pShadowDS = CreateObj<DepthStencil>("ShadowDS");
	pShadowDS->Create(SHADOW_SIZE, SHADOW_SIZE, false);

	// 地面: FieldModel をタンジェント付き頂点に変換 (UVx4 タイリング)
	Model* pGround = CreateObj<Model>("PBRGround");
	*pGround = *GetObj<Model>("FieldModel");
	pGround->RemakeVertex(sizeof(PBRVertex), MakeTangentCalc(4.0f));

	// スポットモデル: タンジェント付き頂点に変換
	Model* pPBRModel = CreateObj<Model>("PBRModel");
	*pPBRModel = *GetObj<Model>("Model");
	pPBRModel->RemakeVertex(sizeof(PBRVertex), MakeTangentCalc(1.0f));

	// 法線マップ
	Texture* pNormalMap = CreateObj<Texture>("NormalMap");
	pNormalMap->Create("Assets/Model/plane/normal.png");
}

// ----------------------------------------------------------------
// Uninit / Update
// ----------------------------------------------------------------

void ScenePBR::Uninit()
{
}

void ScenePBR::Update(float tick)
{
	const float CycleTime = 20.0f;
	m_time += tick;
	if (m_time > CycleTime) m_time -= CycleTime;
}

// ----------------------------------------------------------------
// ヘルパー: PBR で1つのモデルを描画
// ----------------------------------------------------------------

static void DrawPBR(
	Model* pModel,
	const DirectX::XMMATRIX& worldMat,
	Shader* vsPBR, Shader* psPBR,
	const DirectX::XMFLOAT4X4 viewMat,
	const DirectX::XMFLOAT4X4 projMat,
	const DirectX::XMFLOAT4 light[3],
	const DirectX::XMFLOAT4 camera[1],
	const DirectX::XMFLOAT4X4& lightVPMat,
	PBRParam& param,
	Texture* pNormal,
	RenderTarget* pShadow)
{
	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(worldMat));
	mat[0] = mat[0] * pModel->GetScaleBaseMatrix();
	mat[1] = viewMat;
	mat[2] = projMat;

	vsPBR->WriteBuffer(0, mat);
	vsPBR->WriteBuffer(1, (void*)light);
	vsPBR->WriteBuffer(2, (void*)camera);
	vsPBR->WriteBuffer(3, (void*)&lightVPMat);
	psPBR->WriteBuffer(0, (void*)light);
	psPBR->WriteBuffer(1, (void*)camera);
	psPBR->WriteBuffer(2, &param);
	psPBR->SetTexture(1, pNormal);
	psPBR->SetTexture(2, pShadow);

	pModel->SetVertexShader(vsPBR);
	pModel->SetPixelShader(psPBR);
	pModel->Draw();
}

// ----------------------------------------------------------------
// Draw
// ----------------------------------------------------------------

void ScenePBR::Draw()
{
	CameraBase*   pCamera = GetObj<CameraBase>("Camera");
	Shader*       vsPBR   = GetObj<Shader>("VS_PBR");
	Shader*       psPBR   = GetObj<Shader>("PS_PBR");
	Shader*       vsShadow= GetObj<Shader>("VS_Shadow");
	Shader*       psShadow= GetObj<Shader>("PS_Shadow");
	Texture*      pNormal = GetObj<Texture>("NormalMap");
	RenderTarget* pShadowRT = GetObj<RenderTarget>("ShadowMap");
	DepthStencil* pShadowDS = GetObj<DepthStencil>("ShadowDS");
	Model*        pGround = GetObj<Model>("PBRGround");
	Model*        pModel  = GetObj<Model>("PBRModel");

	// --- 昼夜パラメータ ---
	const float PI = 3.14159265f;
	const float CycleTime = 20.0f;
	float dayFactor = (sinf(m_time / CycleTime * 2.0f * PI - PI * 0.5f) + 1.0f) * 0.5f;

	// 空の色
	SetClearColor(
		0.4f * dayFactor + 0.01f * (1.0f - dayFactor),
		0.7f * dayFactor + 0.01f * (1.0f - dayFactor),
		1.0f * dayFactor + 0.05f * (1.0f - dayFactor)
	);

	// 太陽/月の方向 (弧を描く)
	float sunAngle = dayFactor * PI;
	DirectX::XMFLOAT3 lightDir = { 0.0f, -sinf(sunAngle), cosf(sunAngle) };

	// ライト色
	DirectX::XMFLOAT4 light[] = {
		{ 1.0f*dayFactor + 0.15f*(1-dayFactor), 0.92f*dayFactor + 0.15f*(1-dayFactor), 0.75f*dayFactor + 0.3f*(1-dayFactor), 1.0f },
		{ 0.25f*dayFactor + 0.02f*(1-dayFactor), 0.25f*dayFactor + 0.02f*(1-dayFactor), 0.3f*dayFactor + 0.04f*(1-dayFactor), 1.0f },
		{ lightDir.x, lightDir.y, lightDir.z, 0.0f }
	};
	DirectX::XMFLOAT3 camPos = pCamera->GetPos();
	DirectX::XMFLOAT4 camera[] = { { camPos.x, camPos.y, camPos.z, 0.0f } };

	// ライト空間行列 (シャドウ用)
	DirectX::XMMATRIX lightViewMat = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX lightProjMat = DirectX::XMMatrixIdentity();
	DirectX::XMFLOAT4X4 lightVPMat;
	DirectX::XMStoreFloat4x4(&lightVPMat, DirectX::XMMatrixIdentity());

	const bool doShadow = (dayFactor > 0.2f);

	// --- シャドウパス ---
	if (doShadow)
	{
		// 太陽位置からライトを照らす
		DirectX::XMVECTOR lPos = DirectX::XMVectorSet(
			-lightDir.x * 100.0f, -lightDir.y * 100.0f, -lightDir.z * 100.0f, 1.0f);
		DirectX::XMVECTOR lTarget = DirectX::XMVectorZero();
		DirectX::XMVECTOR lUp = (fabsf(lightDir.y) > 0.95f)
			? DirectX::XMVectorSet(1, 0, 0, 0)
			: DirectX::XMVectorSet(0, 1, 0, 0);
		lightViewMat = DirectX::XMMatrixLookAtLH(lPos, lTarget, lUp);
		lightProjMat = DirectX::XMMatrixOrthographicLH(150.0f, 150.0f, 1.0f, 250.0f);
		DirectX::XMMATRIX lvp = lightViewMat * lightProjMat;
		DirectX::XMStoreFloat4x4(&lightVPMat, DirectX::XMMatrixTranspose(lvp));

		// 現在のビューポートと RT を保存
		UINT numVP = 1;
		D3D11_VIEWPORT oldVP;
		GetContext()->RSGetViewports(&numVP, &oldVP);
		ID3D11RenderTargetView* pOldRTV = nullptr;
		ID3D11DepthStencilView* pOldDSV = nullptr;
		GetContext()->OMGetRenderTargets(1, &pOldRTV, &pOldDSV);

		// シャドウマップ用ビューポート設定
		D3D11_VIEWPORT svp = {};
		svp.Width    = (float)SHADOW_SIZE;
		svp.Height   = (float)SHADOW_SIZE;
		svp.MaxDepth = 1.0f;
		GetContext()->RSSetViewports(1, &svp);

		// 前フレームで t2 に残っている shadow SRV を解放
		ID3D11ShaderResourceView* nullSRV = nullptr;
		GetContext()->PSSetShaderResources(2, 1, &nullSRV);

		// シャドウ RT + DS をバインド
		ID3D11RenderTargetView* shadowRTV = pShadowRT->GetView();
		GetContext()->OMSetRenderTargets(1, &shadowRTV, pShadowDS->GetView());
		float clearDepth[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		pShadowRT->Clear(clearDepth);
		pShadowDS->Clear();

		// シャドウキャスター描画
		DirectX::XMFLOAT4X4 sm[3];
		DirectX::XMStoreFloat4x4(&sm[1], DirectX::XMMatrixTranspose(lightViewMat));
		DirectX::XMStoreFloat4x4(&sm[2], DirectX::XMMatrixTranspose(lightProjMat));

		auto drawShadow = [&](Model* pM, const DirectX::XMMATRIX& world)
		{
			DirectX::XMStoreFloat4x4(&sm[0], DirectX::XMMatrixTranspose(world));
			sm[0] = sm[0] * pM->GetScaleBaseMatrix();
			vsShadow->WriteBuffer(0, sm);
			pM->SetVertexShader(vsShadow);
			pM->SetPixelShader(psShadow);
			pM->Draw();
		};

		drawShadow(pGround, DirectX::XMMatrixScaling(5.0f, 1.0f, 5.0f));
		drawShadow(pModel,  DirectX::XMMatrixScaling(1.0f, 3.0f, 1.0f));

		// RT / ビューポートを復元
		GetContext()->OMSetRenderTargets(1, &pOldRTV, pOldDSV);
		GetContext()->RSSetViewports(1, &oldVP);
		if (pOldRTV) pOldRTV->Release();
		if (pOldDSV) pOldDSV->Release();
	}

	// --- メインパス ---
	DirectX::XMFLOAT4X4 viewMat = pCamera->GetView();
	DirectX::XMFLOAT4X4 projMat = pCamera->GetProj();

	// 1. 地面
	{
		PBRParam p = { 0.0f, 0.85f, dayFactor, 0.0f };
		DrawPBR(pGround,
			DirectX::XMMatrixScaling(5.0f, 1.0f, 5.0f),
			vsPBR, psPBR, viewMat, projMat,
			light, camera, lightVPMat, p, pNormal, pShadowRT);
	}

	// 2. スポットモデル
	{
		PBRParam p = { 0.0f, 0.4f, dayFactor, 0.0f };
		DrawPBR(pModel,
			DirectX::XMMatrixScaling(1.0f, 3.0f, 1.0f),
			vsPBR, psPBR, viewMat, projMat,
			light, camera, lightVPMat, p, pNormal, pShadowRT);
	}
}
