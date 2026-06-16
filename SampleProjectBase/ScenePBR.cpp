#include "ScenePBR.h"
#include "Model.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Input.h"
#include "Geometory.h"

// 法線マップ・PBR用の頂点データ（タンジェントベクトル付き）
struct PBRVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT3 tangent;
};

// PBRパラメータ定数バッファ
struct PBRParam
{
	float metallic;
	float roughness;
	float dummy[2];
};

void ScenePBR::Init()
{
	// シェーダー読み込み
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_Object"),
		CreateObj<PixelShader>("PS_TexColor"),
		CreateObj<VertexShader>("VS_Bumpmap"),
		CreateObj<PixelShader>("PS_Bumpmap"),
		CreateObj<VertexShader>("VS_PBR"),
		CreateObj<PixelShader>("PS_PBR"),
	};
	const char* file[] = {
		"Assets/Shader/VS_Object.cso",
		"Assets/Shader/PS_TexColor.cso",
		"Assets/Shader/VS_Bumpmap.cso",
		"Assets/Shader/PS_Bumpmap.cso",
		"Assets/Shader/VS_PBR.cso",
		"Assets/Shader/PS_PBR.cso",
	};
	for (int i = 0; i < _countof(shader); ++i)
	{
		if (FAILED(shader[i]->Load(file[i])))
		{
			MessageBox(NULL, file[i], "Shader Error", MB_OK);
		}
	}

	// タンジェントベクトル計算ラムダ（SceneBumpmapと同様）
	auto calcTangent = [](Model::RemakeInfo& data)
	{
		PBRVertex* destVtx = reinterpret_cast<PBRVertex*>(data.dest);
		const Model::Vertex* srcVtx = reinterpret_cast<const Model::Vertex*>(data.source);
		for (UINT i = 0; i < data.vtxNum; ++i)
		{
			destVtx[i].pos = srcVtx[i].pos;
			destVtx[i].normal = srcVtx[i].normal;
			destVtx[i].uv = srcVtx[i].uv;
			destVtx[i].tangent = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
		}

		using TanList = std::vector<DirectX::XMFLOAT3>;
		std::vector<TanList> tangentVtx(data.vtxNum);
		const UINT* idx = reinterpret_cast<const UINT*>(data.idx);
		for (UINT i = 0; i < data.idxNum; i += 3)
		{
			UINT i0 = idx[i], i1 = idx[i + 1], i2 = idx[i + 2];
			auto& p0 = destVtx[i0].pos; auto& p1 = destVtx[i1].pos; auto& p2 = destVtx[i2].pos;
			auto& t0 = destVtx[i0].uv;  auto& t1 = destVtx[i1].uv;  auto& t2 = destVtx[i2].uv;

			DirectX::XMFLOAT3 V1(p1.x-p0.x, p1.y-p0.y, p1.z-p0.z);
			DirectX::XMFLOAT3 V2(p2.x-p0.x, p2.y-p0.y, p2.z-p0.z);
			DirectX::XMFLOAT2 ST1(t1.x-t0.x, t1.y-t0.y);
			DirectX::XMFLOAT2 ST2(t2.x-t0.x, t2.y-t0.y);
			float f = ST1.x * ST2.y - ST2.x * ST1.y;
			if (fabsf(f) < 1e-8f) continue;
			DirectX::XMFLOAT3 T(
				(ST2.y*V1.x - ST1.y*V2.x) / f,
				(ST2.y*V1.y - ST1.y*V2.y) / f,
				(ST2.y*V1.z - ST1.y*V2.z) / f
			);
			float len = sqrtf(T.x*T.x + T.y*T.y + T.z*T.z);
			if (len > 1e-8f) { T.x /= len; T.y /= len; T.z /= len; }
			tangentVtx[i0].push_back(T);
			tangentVtx[i1].push_back(T);
			tangentVtx[i2].push_back(T);
		}
		for (UINT i = 0; i < data.vtxNum; ++i)
		{
			if (tangentVtx[i].empty()) continue;
			DirectX::XMFLOAT3 sum(0,0,0);
			for (auto& t : tangentVtx[i]) { sum.x+=t.x; sum.y+=t.y; sum.z+=t.z; }
			float n = (float)tangentVtx[i].size();
			destVtx[i].tangent = { sum.x/n, sum.y/n, sum.z/n };
		}
	};

	// planeモデルにタンジェント付き頂点を生成
	Model* pSrcPlane = GetObj<Model>("ModelPlane");
	Model* pPBRPlane = CreateObj<Model>("PBRPlane");
	*pPBRPlane = *pSrcPlane;
	pPBRPlane->RemakeVertex(sizeof(PBRVertex), calcTangent);

	// 法線マップ読み込み
	Texture* pNormalMap = CreateObj<Texture>("NormalMap");
	pNormalMap->Create("Assets/Model/plane/normal.png");
}

void ScenePBR::Uninit()
{
}

void ScenePBR::Update(float tick)
{
}

void ScenePBR::Draw()
{
	Model* pPlane = GetObj<Model>("PBRPlane");
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");

	// 行列
	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixIdentity());
	mat[1] = pCamera->GetView();
	mat[2] = pCamera->GetProj();

	// ライト情報
	DirectX::XMFLOAT3 lightDir = pLight->GetDirection();
	DirectX::XMFLOAT4 light[] = {
		pLight->GetDiffuse(),
		pLight->GetAmbient(),
		{lightDir.x, lightDir.y, lightDir.z, 0.0f}
	};

	// カメラ情報
	DirectX::XMFLOAT3 camPos = pCamera->GetPos();
	DirectX::XMFLOAT4 camera[] = {
		{camPos.x, camPos.y, camPos.z, 0.0f}
	};

	// PBRパラメータ (非金属・中程度の粗さ)
	PBRParam pbrParam = { 0.0f, 0.5f, {0.0f, 0.0f} };

	Shader* vsObject  = GetObj<Shader>("VS_Object");
	Shader* psTexColor= GetObj<Shader>("PS_TexColor");
	Shader* vsBumpmap = GetObj<Shader>("VS_Bumpmap");
	Shader* psBumpmap = GetObj<Shader>("PS_Bumpmap");
	Shader* vsPBR     = GetObj<Shader>("VS_PBR");
	Shader* psPBR     = GetObj<Shader>("PS_PBR");

	Texture* pNormalMap = GetObj<Texture>("NormalMap");

	// 3つの描画モード:
	//  0: テクスチャカラーのみ
	//  1: バンプマップ(法線マップ + Diffuse)
	//  2: PBR + 法線マップ
	struct DrawMode { Shader* vs; Shader* ps; };
	DrawMode modes[] = {
		{ vsObject,  psTexColor },
		{ vsBumpmap, psBumpmap  },
		{ vsPBR,     psPBR      },
	};
	const int modeCount = _countof(modes);

	for (int i = 0; i < modeCount; ++i)
	{
		DirectX::XMStoreFloat4x4(&mat[0],
			DirectX::XMMatrixTranspose(
				DirectX::XMMatrixScaling(0.98f, 0.98f, 0.98f) *
				DirectX::XMMatrixTranslation((modeCount - 1) * 0.5f - i, 0.0f, 0.0f)
			));
		mat[0] = mat[0] * pPlane->GetScaleBaseMatrix();

		Shader* vs = modes[i].vs;
		Shader* ps = modes[i].ps;

		vs->WriteBuffer(0, mat);
		vs->WriteBuffer(1, light);
		vs->WriteBuffer(2, camera);
		ps->WriteBuffer(0, light);
		ps->WriteBuffer(1, camera);

		if (i == 2)
		{
			// PBRパラメータをb2に渡す
			ps->WriteBuffer(2, &pbrParam);
		}

		// 法線マップをセット (TexColorモードは使わないが設定しても問題なし)
		ps->SetTexture(1, pNormalMap);

		pPlane->SetVertexShader(vs);
		pPlane->SetPixelShader(ps);
		pPlane->Draw();
	}
}
