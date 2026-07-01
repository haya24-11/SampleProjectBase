#include "SceneSpring.h"
#include "Geometory.h"
#include "Input.h"

void SceneSpring::Init()
{
	m_springPos = DirectX::XMFLOAT3(0.0f, 10.0f, 0.0f);
	m_ballPos = DirectX::XMFLOAT3(0.0f, 5.0f, 0.0f);
}
void SceneSpring::Uninit()
{

}
void SceneSpring::Update(float tick)
{
	m_time += tick;
	bool isInput = false;

	if (IsKeyPress(VK_UP)){
		m_ballPos.z += 0.1f;
		isInput = true;
	}
	if (IsKeyPress(VK_LEFT)) {
		m_ballPos.x += -0.1f;
		isInput = true;
	}
	if (IsKeyPress(VK_DOWN)) {
		m_ballPos.z += -0.1f;
		isInput = true;
	}
	if (IsKeyPress(VK_RIGHT)) {
		m_ballPos.x += 0.1f;
		isInput = true;
	}
	if (IsKeyPress(VK_SPACE)) {
		m_ballPos.y += -0.1f;
		isInput = true;
	}

	// ばねの処理-------------------------------------------------------------------------------------
	// スプリングの長さを計算

	
	// スプリングのテンションを計算

	
	// 抵抗力を求める


	// かかる力を合成（合力＝重力＋張力＋抵抗力）


	// 入力がある場合は以下の処理を止めて球の移動処理に逃がす


	// 合力と質量から加速度を求める


	// 速度に加速度を加える


	// 速度から座標を移動

	
	// ばねの処理-------------------------------------------------------------------------------------
}
void SceneSpring::Draw()
{
	DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	Geometory::AddLine(m_springPos, color, m_ballPos, color);
	Geometory::DrawLines();

	// 球の座標行列作成
	DirectX::XMFLOAT4X4 mat;
	DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixTranspose(
		DirectX::XMMatrixTranslation(m_ballPos.x, m_ballPos.y, m_ballPos.z)	// 球の座標
	));

	Geometory::SetWorld(mat);		// 行列セット
	Geometory::DrawSphere();		// 球描画
}
