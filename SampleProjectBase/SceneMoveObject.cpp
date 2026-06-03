#include "math.h"
#include "SceneMoveObject.h"
#include "Geometory.h"
#include "DebugLog.h"
#include "Input.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void SceneMoveObject::Init()
{
	

	m_startPos = DirectX::SimpleMath::Vector3(-3.0f, 0.0f, 0.0f);
	m_pos = m_startPos;
	m_v0 = DirectX::SimpleMath::Vector3(8.0f, 0.0f, 0.0f); // X方向のみ
	m_v = m_v0;
	m_a = DirectX::SimpleMath::Vector3(10.0f, 0.0f, 0.0f);
	m_t = 0.0f;
	m_mode = 0;
	m_theta = 45.0f * (XM_PI / 180.0f); // 45度をラジアンに変換
}
void SceneMoveObject::Uninit()
{

}
void SceneMoveObject::Update(float tick)
{
	switch (m_mode)
	{
	case 0:
		m_t += tick;
		// x = v0 * t（速度一定）
		m_pos.x = m_startPos.x + m_v0.x * m_t;
		break;
	case 1:
		// v = v0 + at

		m_t += tick;  

		// v = v0 + at
		m_v.x = m_v0.x + m_a.x * m_t;

		// x = v0t + 1/2 * at^2
		m_pos.x = m_startPos.x + m_v0.x * m_t + 0.5f * m_a.x * m_t * m_t;
		

		break;

	case 2:
		m_t += tick;  // 経過時間を加算

		// v = -gt
		m_v.y = -9.8f * m_t;

		// y = -1/2 * g * t^2
		m_pos.y = 5.0f + (-0.5f * 9.8f * m_t * m_t);
		break;

	case 3: // 鉛直投げ上げ
		m_t += tick;

		// v = v0 - gt  (初速度を5.0fで固定)
		m_v.y = 5.0f - 9.8f * m_t;

		// y = v0t - 1/2 * g * t^2
		m_pos.y = m_startPos.y + 5.0f * m_t - 0.5f * 9.8f * m_t * m_t;
		break;

	case 4: // 水平投射
		m_t += tick;

		// vx = v0, vy = -gt
		m_v.x = m_v0.x;
		m_v.y = -9.8f * m_t;

		// x = v0t, y = -1/2 * g * t^2
		m_pos.x = m_startPos.x + m_v0.x * m_t;
		m_pos.y = m_startPos.y + (-0.5f * 9.8f * m_t * m_t);
		break;

	case 5: // 斜方投射
		m_t += tick;

		// vx = v0cosθ, vy = v0sinθ - gt
		m_v.x = m_v0.x * cosf(m_theta);
		m_v.y = m_v0.x * sinf(m_theta) - 9.8f * m_t;

		// x = t * v0cosθ, y = t * v0sinθ - 1/2 * g * t^2
		m_pos.x = m_startPos.x + m_t * m_v0.x * cosf(m_theta);
		m_pos.y = m_startPos.y + m_t * m_v0.x * sinf(m_theta) - 0.5f * 9.8f * m_t * m_t;
		break;

	default:
		break;
	}
	if (IsKeyTrigger(VK_SPACE))
	{
		m_mode = (m_mode + 1) % 6; // 0→1→2→3→4→5→0...

		m_t = 0.0f;
		m_v = m_v0;

		switch (m_mode)
		{
		case 0: // 等速直線運動
		case 1: // 等加速度直線運動
			m_startPos = DirectX::SimpleMath::Vector3(-3.0f, 0.0f, 0.0f);
			break;
		case 2: // 自由落下
		case 3: // 鉛直投げ上げ
			m_startPos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
			break;
		case 4: // 水平投射（高い位置からスタート）
			m_startPos = DirectX::SimpleMath::Vector3(-3.0f, 3.0f, 0.0f);
			break;
		case 5: // 斜方投射
			m_startPos = DirectX::SimpleMath::Vector3(-3.0f, 0.0f, 0.0f);
			break;
		}

		m_pos = m_startPos;
	}
}
void SceneMoveObject::Draw()
{
	
	
	/*Geometory::AddLine(Vector3(2, 1, 0), Vector3(-2, 1, 0));
	Geometory::DrawLines();*/


	DirectX::XMFLOAT4X4 mat;
	//球の座標行列作成
	DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixTranspose(
		DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) *	//球のスケール
		DirectX::XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z)//球の座標
	));

	Geometory::SetWorld(mat);		//行列セット
	Geometory::DrawSphere();		//球描画

}
