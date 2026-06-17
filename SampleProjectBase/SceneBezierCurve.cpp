#include "SceneBezierCurve.h"
#include "Geometory.h"
#include "DebugLog.h"

void SceneBezierCurve::Init()
{
	m_positions.push_back(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));	//Q0
	m_positions.push_back(DirectX::XMFLOAT3(2.0f, 4.0f, 0.0f));	//Q1
	m_positions.push_back(DirectX::XMFLOAT3(6.0f, 4.0f, 0.0f));	//Q2
	m_positions.push_back(DirectX::XMFLOAT3(8.0f, 0.0f, 0.0f));	//Q3

	m_ball.push_back(Ball(m_positions[0], 0.2f));
	m_ball.push_back(Ball(m_positions[1], 0.2f));
	m_ball.push_back(Ball(m_positions[2], 0.2f));
	m_ball.push_back(Ball(m_positions[3], 0.2f));
}
void SceneBezierCurve::Uninit()
{

}
void SceneBezierCurve::Update(float tick)
{
	m_time += tick;
	
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
	DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(0.2f, 1.0f, 0.0f, 1.0f);

	t += tick * 0.4f;
	if (t >= 1.0f) t = 0.0f;

	end = GetBezierCurvePosition(t);

	float t1 = t - 0.02f;
	if (t1 <= 0.0f) t1 = 0.0f;

	start = GetBezierCurvePosition(t1);
	Geometory::AddLine(start, color, end, color);


	Beem(t1, start, 100);
}
void SceneBezierCurve::Draw()
{

	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
	DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

	// �����\��(�f�o�b�O)
	for (int i=0;i< m_positions.size() -1; i++)
	{
		start = DirectX::XMFLOAT3(m_positions[i].x, m_positions[i].y, m_positions[i].z);
		end = DirectX::XMFLOAT3(m_positions[i+1].x, m_positions[i+1].y, m_positions[i+1].z);
		Geometory::AddLine(start,color,end, color);
	}
	Geometory::DrawLines();

	for (Ball ball : m_ball)
	{
		ball.Drow();
	}
}

DirectX::XMFLOAT3 SceneBezierCurve::GetBezierCurvePosition(float t)
{

	
	DirectX::XMFLOAT3 outpos;

	
	/*outpos.x = (1-t)
	outpos.y = 
	outpos.z = */

	// ベジエ曲線計算
	// P(t).x = (1-t)^3 * Q0.x + 3(1-t)^2 * t * Q1.x + 3(1-t) * t^2 * Q2.x + t^3 * Q3.x
	// P(t).y = (1-t)^3 * Q0.y + 3(1-t)^2 * t * Q1.y + 3(1-t) * t^2 * Q2.y + t^3 * Q3.y
	// P(t).z = (1-t)^3 * Q0.z + 3(1-t)^2 * t * Q1.z + 3(1-t) * t^2 * Q2.z + t^3 * Q3.z
	float t1    = 1.0f - t;
	float t1_3  = t1 * t1 * t1;
	float t1_2t = 3.0f * t1 * t1 * t;
	float t1_t2 = 3.0f * t1 * t * t;
	float t3    = t * t * t;

	outpos.x = t1_3 * m_positions[0].x + t1_2t * m_positions[1].x + t1_t2 * m_positions[2].x + t3 * m_positions[3].x;
	outpos.y = t1_3 * m_positions[0].y + t1_2t * m_positions[1].y + t1_t2 * m_positions[2].y + t3 * m_positions[3].y;
	outpos.z = t1_3 * m_positions[0].z + t1_2t * m_positions[1].z + t1_t2 * m_positions[2].z + t3 * m_positions[3].z;

	return outpos;
}

void SceneBezierCurve::Beem(float t, DirectX::XMFLOAT3 end, int loop)
{
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(0.2f, 1.0f, 0.0f, 1.0f);

	float t1 = t - 0.02f;
	if (t1 <= 0.0f) t1 = 0.0f;

	start = GetBezierCurvePosition(t1);
	Geometory::AddLine(start, color, end, color);

	loop--;

	if(loop > 0) Beem(t1, start, loop);
}