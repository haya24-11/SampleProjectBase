#include "SceneLight.h"
#include "Model.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Input.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Geometory.h"

void SceneLight::Init()
{
	// シェーダーの読み込み処理
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_Object"),
		CreateObj<PixelShader>("PS_TexColor"),
		CreateObj<VertexShader>("VS_WorldPosition"),
		CreateObj<PixelShader>("PS_PointLight"),
		CreateObj<PixelShader>("PS_SpotLight"),
		CreateObj<PixelShader>("PS_RimLight"),
	};
	const char* file[] = {
		"Assets/Shader/VS_Object.cso",
		"Assets/Shader/PS_TexColor.cso",
		"Assets/Shader/VS_WorldPosition.cso",
		"Assets/Shader/PS_PointLight.cso",
		"Assets/Shader/PS_SpotLight.cso",
		"Assets/Shader/PS_RimLight.cso"
	};

	int shaderNum = _countof(shader);
	for (int i = 0; i < shaderNum; ++i)
	{
		if (FAILED(shader[i]->Load(file[i])))
		{
			MessageBox(NULL, file[i], "Shader Error", MB_OK);
		}
	}
	// ポイントライトの作成
	for (int i = 0; i < 30; ++i)
	{
		std::string name = "PointLight" + std::to_string(i);
		PointLight* pPointLight = CreateObj<PointLight>(name.c_str());
		pPointLight->SetPos(DirectX::XMFLOAT3(
			((rand() % 21) - 10) * 0.1f,	// -1～1(0.1刻み
			(rand() % 21) * 0.1f,			// 0～2(0.1刻み
			((rand() % 21) - 10) * 0.1f));	// -1～1(0.1刻み
		pPointLight->SetRange((rand() % 3) * 0.5f + 1.0f); // 1～2(0.5刻み)
		pPointLight->SetDiffuse(DirectX::XMFLOAT4(
			(rand() % 7) * 0.1f + 0.4f,			// 0.4～1(0.1刻み)
			(rand() % 7) * 0.1f + 0.4f,			// 0.4～1(0.1刻み)
			(rand() % 7) * 0.1f + 0.4f,			// 0.4～1(0.1刻み)
			1.0f
		));
	}

	// スポットライトの作成
	SpotLight* pSpotLight = CreateObj<SpotLight>("SpotLight");
	pSpotLight->SetPos(DirectX::XMFLOAT3(2.0f, 2.0f, 0.0f));
	pSpotLight->SetDirection(DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f));
	pSpotLight->SetRange(10.0f);
	pSpotLight->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	pSpotLight->SetLightAngle(DirectX::XMConvertToRadians(30.0f));
}
void SceneLight::Uninit()
{
}
void SceneLight::Update(float tick)
{
}


void SceneLight::Draw()
{
	// 各種取得
	Model* pModel = GetObj<Model>("Model");
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");
	SpotLight* pSpotLight = GetObj<SpotLight>("SpotLight");

	// 定数バッファに渡す行列の情報
	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixIdentity());
	mat[1] = pCamera->GetView();
	mat[2] = pCamera->GetProj();

	// 定数バッファに渡すライトの情報
	DirectX::XMFLOAT3 lightDir =
		pLight->GetDirection();
	DirectX::XMFLOAT4 light[] = {
		pLight->GetDiffuse(),
		pLight->GetAmbient(),
		{lightDir.x, lightDir.y, lightDir.z, 0.0f}
	};

	// 定数バッファに渡すポイントライトの情報
	struct Light {
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT3 pos;
		float range;
	};
	Light lights[30];
	for (int i = 0; i < 30; ++i)
	{
		std::string name = "PointLight" + std::to_string(i);
		PointLight* pPointLight = GetObj<PointLight>(name.c_str());
		lights[i].color = pPointLight->GetDiffuse();
		lights[i].pos = pPointLight->GetPos();
		lights[i].range = pPointLight->GetRange();
	}

	// 定数バッファに渡すスポットライトの情報
	struct SpotLight {
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT3 pos;
		float range;
		DirectX::XMFLOAT3 dir;
		float angle;
	};

	SpotLight spotLight[1];
	spotLight->color = pSpotLight->GetDiffuse();
	spotLight->pos = pSpotLight->GetPos();
	spotLight->range = pSpotLight->GetRange();
	spotLight->dir = pSpotLight->GetDirection();
	spotLight->angle = pSpotLight->GetLightAngle();

	// 定数バッファに渡すカメラの情報
	DirectX::XMFLOAT3 camPos = pCamera->GetPos();
	DirectX::XMFLOAT4 camera[] = {
		{camPos.x, camPos.y, camPos.z, 0.0f}
	};

	// シェーダーの取得
	Shader* shader[] = {
		GetObj<Shader>("VS_Object"),
		GetObj<Shader>("PS_TexColor"),
		GetObj<Shader>("VS_WorldPosition"),
		GetObj<Shader>("PS_PointLight"),
		GetObj<Shader>("PS_SpotLight"),
		GetObj<Shader>("PS_RimLight")
	};
	// モデルの表示(点光源)
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(
		DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f)
	));
	shader[2]->WriteBuffer(0, mat);
	shader[3]->WriteBuffer(0, lights);
	pModel->SetVertexShader(shader[2]);
	pModel->SetPixelShader(shader[3]);
	pModel->Draw();

	// モデルの表示(スポットライト)
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(
		DirectX::XMMatrixTranslation(2.0f, 0.0f, 0.0f)
	));
	shader[2]->WriteBuffer(0, mat);
	shader[4]->WriteBuffer(0, spotLight);
	pModel->SetVertexShader(shader[2]);
	pModel->SetPixelShader(shader[4]);
	pModel->Draw();

	// 点光源の位置を表示
	for (int i = 0; i < 30; ++i)
	{
		DirectX::XMFLOAT4X4 mat;
		DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixTranspose(
			DirectX::XMMatrixScaling(0.1f, 0.1f, 0.1f) *
			DirectX::XMMatrixTranslation(lights[i].pos.x, lights[i].pos.y, lights[i].pos.z)
		));
		Geometory::SetWorld(mat);
		Geometory::SetColor(lights[i].color);
		Geometory::DrawSphere();
	}

	// スポットライトの位置を表示
	DirectX::XMFLOAT4X4 mat_spot;
	DirectX::XMStoreFloat4x4(&mat_spot, DirectX::XMMatrixTranspose(
		DirectX::XMMatrixScaling(0.25f, 0.25f, 0.25f)*
		DirectX::XMMatrixTranslation(spotLight->pos.x, spotLight->pos.y, spotLight->pos.z)
	));
	Geometory::SetWorld(mat_spot);
	Geometory::SetColor({ 1.0f, 1.0f, 1.0f, 1.5f });
	Geometory::DrawSphere();

	// モデルの表示(リムライト)
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(
		DirectX::XMMatrixTranslation(3.0f, 0.0f, 0.0f)
	));
	shader[2]->WriteBuffer(0, mat);
	shader[5]->WriteBuffer(0, light);
	shader[5]->WriteBuffer(1, camera);
	pModel->SetVertexShader(shader[2]);
	pModel->SetPixelShader(shader[5]);
	pModel->Draw();
}


