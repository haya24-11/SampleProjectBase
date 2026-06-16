#include "ScenePBR.h"
#include "Model.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Input.h"

// PBR用頂点レイアウト（タンジェントベクトル付き）
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
	float dummy[2];
};

// タンジェントベクトルを計算して PBRVertex に詰め直すラムダを生成
// uvScale: UV座標のスケール係数（タイリング用）
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
			UINT i0 = idx[i], i1 = idx[i + 1], i2 = idx[i + 2];
			auto& p0 = dst[i0].pos; auto& p1 = dst[i1].pos; auto& p2 = dst[i2].pos;
			auto& t0 = dst[i0].uv;  auto& t1 = dst[i1].uv;  auto& t2 = dst[i2].uv;

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

			tanPerVtx[i0].push_back(T);
			tanPerVtx[i1].push_back(T);
			tanPerVtx[i2].push_back(T);
		}

		for (UINT i = 0; i < data.vtxNum; ++i)
		{
			if (tanPerVtx[i].empty()) continue;
			DirectX::XMFLOAT3 sum(0, 0, 0);
			for (auto& t : tanPerVtx[i]) { sum.x += t.x; sum.y += t.y; sum.z += t.z; }
			float n = (float)tanPerVtx[i].size();
			dst[i].tangent = { sum.x/n, sum.y/n, sum.z/n };
		}
	};
}

void ScenePBR::Init()
{
	// シェーダー読み込み
	Shader* shaders[] = {
		CreateObj<VertexShader>("VS_PBR"),
		CreateObj<PixelShader>("PS_PBR"),
	};
	const char* files[] = {
		"Assets/Shader/VS_PBR.cso",
		"Assets/Shader/PS_PBR.cso",
	};
	for (int i = 0; i < _countof(shaders); ++i)
	{
		if (FAILED(shaders[i]->Load(files[i])))
			MessageBox(NULL, files[i], "Shader Error", MB_OK);
	}

	// 地面: FieldModel をタンジェント付き頂点に変換（UVx4 タイリング）
	Model* pFieldSrc = GetObj<Model>("FieldModel");
	Model* pGround = CreateObj<Model>("PBRGround");
	*pGround = *pFieldSrc;
	pGround->RemakeVertex(sizeof(PBRVertex), MakeTangentCalc(4.0f));

	// スポットモデル: タンジェント付き頂点に変換
	Model* pModelSrc = GetObj<Model>("Model");
	Model* pPBRModel = CreateObj<Model>("PBRModel");
	*pPBRModel = *pModelSrc;
	pPBRModel->RemakeVertex(sizeof(PBRVertex), MakeTangentCalc(1.0f));

	// 法線マップ（plane フォルダのものを流用）
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
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase*  pLight  = GetObj<LightBase>("Light");
	Shader*     vsPBR   = GetObj<Shader>("VS_PBR");
	Shader*     psPBR   = GetObj<Shader>("PS_PBR");
	Texture*    pNormal = GetObj<Texture>("NormalMap");

	// 定数バッファ共通データ
	DirectX::XMFLOAT4X4 mat[3];
	mat[1] = pCamera->GetView();
	mat[2] = pCamera->GetProj();

	DirectX::XMFLOAT3 lightDir = pLight->GetDirection();
	DirectX::XMFLOAT4 light[] = {
		pLight->GetDiffuse(),
		pLight->GetAmbient(),
		{ lightDir.x, lightDir.y, lightDir.z, 0.0f }
	};
	DirectX::XMFLOAT3 camPos = pCamera->GetPos();
	DirectX::XMFLOAT4 camera[] = {
		{ camPos.x, camPos.y, camPos.z, 0.0f }
	};

	// --- 1. 地面を最初に描画 ---
	{
		Model* pGround = GetObj<Model>("PBRGround");
		DirectX::XMStoreFloat4x4(&mat[0],
			DirectX::XMMatrixTranspose(
				DirectX::XMMatrixScaling(5.0f, 1.0f, 5.0f) *
				DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f)
			));
		mat[0] = mat[0] * pGround->GetScaleBaseMatrix();

		PBRParam groundParam = { 0.0f, 0.85f, { 0.0f, 0.0f } };

		vsPBR->WriteBuffer(0, mat);
		vsPBR->WriteBuffer(1, light);
		vsPBR->WriteBuffer(2, camera);
		psPBR->WriteBuffer(0, light);
		psPBR->WriteBuffer(1, camera);
		psPBR->WriteBuffer(2, &groundParam);
		psPBR->SetTexture(1, pNormal);

		pGround->SetVertexShader(vsPBR);
		pGround->SetPixelShader(psPBR);
		pGround->Draw();
	}

	// --- 2. スポットモデルを地面の上に描画 ---
	{
		Model* pModel = GetObj<Model>("PBRModel");
		DirectX::XMStoreFloat4x4(&mat[0],
			DirectX::XMMatrixTranspose(
				DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f)
			));
		mat[0] = mat[0] * pModel->GetScaleBaseMatrix();

		PBRParam modelParam = { 0.0f, 0.4f, { 0.0f, 0.0f } };

		vsPBR->WriteBuffer(0, mat);
		vsPBR->WriteBuffer(1, light);
		vsPBR->WriteBuffer(2, camera);
		psPBR->WriteBuffer(0, light);
		psPBR->WriteBuffer(1, camera);
		psPBR->WriteBuffer(2, &modelParam);
		psPBR->SetTexture(1, pNormal);

		pModel->SetVertexShader(vsPBR);
		pModel->SetPixelShader(psPBR);
		pModel->Draw();
	}
}
