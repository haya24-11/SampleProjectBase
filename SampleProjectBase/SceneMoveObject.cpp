#include "math.h"
#include "SceneMoveObject.h"
#include "Geometory.h"
#include "DebugLog.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void SceneMoveObject::Init()
{

}
void SceneMoveObject::Uninit()
{

}
void SceneMoveObject::Update(float tick)
{
	
}
void SceneMoveObject::Draw()
{
	
	
	/*Geometory::AddLine(Vector3(2, 1, 0), Vector3(-2, 1, 0));
	Geometory::DrawLines();*/


	DirectX::XMFLOAT4X4 mat;
	//球の座標行列作成
	DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixTranspose(
		DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) *	//球のスケール
		DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f)//球の座標
	));

	Geometory::SetWorld(mat);		//行列セット
	Geometory::DrawSphere();		//球描画

}
