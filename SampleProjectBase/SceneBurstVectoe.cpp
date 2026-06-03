//#include <DirectXMath.h>
#include "math.h"
#include "SceneBurstVectoe.h"
#include "Geometory.h"
#include "DebugLog.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void SceneBurstVectoe::Init()
{
	Ball b1 = Ball(XMFLOAT3(-10.0f, 0.0f, 0.0f));
	b1.m_speedVec = (XMFLOAT3(5.0f, 0.0f, 0.0f));
	b1.SetColor(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
	m_Balls.push_back(b1);

	Ball b2 = Ball(XMFLOAT3(10.0f, 0.0f, 0.0f));
	b2.m_speedVec = (XMFLOAT3(-5.0f, 0.0f, 0.0f));
	b2.SetColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
	m_Balls.push_back(b2);

	Ball b3 = Ball(XMFLOAT3(-5.0f, 0.0f, 4.0f));
	b3.m_speedVec = (XMFLOAT3(3.0f, 0.0f, -2.0f));
	m_Balls.push_back(b3);
}
void SceneBurstVectoe::Uninit()
{

}
void SceneBurstVectoe::Update(float tick)
{
	m_time += tick;

	// 課題範囲--------------------------------------------------------------------------

    const float e = 1.0f;
    float remainTick = tick;

    for (int iter = 0; iter < 20; iter++)
    {
        float earliestTime = remainTick;
        int hitA = -1, hitB = -1;

        for (int i = 0; i < (int)m_Balls.size(); i++)
        {
            for (int j = i + 1; j < (int)m_Balls.size(); j++)
            {
                Ball& a = m_Balls[i];
                Ball& b = m_Balls[j];

                float minDist = a.GetRadius() + b.GetRadius();

                // ✅ GetPosition() で m_pos + m_basePos を取得
                XMFLOAT3 posA = a.GetPosition();
                XMFLOAT3 posB = b.GetPosition();

                float rx = posB.x - posA.x;
                float ry = posB.y - posA.y;
                float rz = posB.z - posA.z;

                // 速度は m_speedVec のまま（スカラー速度）
                float rvx = b.m_speedVec.x - a.m_speedVec.x;
                float rvy = b.m_speedVec.y - a.m_speedVec.y;
                float rvz = b.m_speedVec.z - a.m_speedVec.z;

                float A = rvx * rvx + rvy * rvy + rvz * rvz;
                float B = 2.0f * (rx * rvx + ry * rvy + rz * rvz);
                float C = rx * rx + ry * ry + rz * rz - minDist * minDist;

                if (C < 0.0f)
                {
                    if (B < 0.0f)
                    {
                        earliestTime = 0.0f;
                        hitA = i; hitB = j;
                    }
                    continue;
                }

                if (B >= 0.0f) continue;
                if (A < 0.0001f) continue;

                float disc = B * B - 4.0f * A * C;
                if (disc < 0.0f) continue;

                float t = (-B - sqrtf(disc)) / (2.0f * A);
                if (t < 0.0f || t > earliestTime) continue;

                earliestTime = t;
                hitA = i; hitB = j;
            }
        }

        // 衝突時刻まで全ボール移動（m_pos に加算）
        for (int i = 0; i < (int)m_Balls.size(); i++)
        {
            m_Balls[i].m_pos.x += m_Balls[i].m_speedVec.x * earliestTime;
            m_Balls[i].m_pos.y += m_Balls[i].m_speedVec.y * earliestTime;
            m_Balls[i].m_pos.z += m_Balls[i].m_speedVec.z * earliestTime;
        }
        remainTick -= earliestTime;

        if (hitA >= 0)
        {
            Ball& a = m_Balls[hitA];
            Ball& b = m_Balls[hitB];

            float minDist = a.GetRadius() + b.GetRadius();
            float m1 = a.m_mass;
            float m2 = b.m_mass;

            // ✅ 衝突後もGetPosition()で正確な位置を使う
            XMFLOAT3 posA = a.GetPosition();
            XMFLOAT3 posB = b.GetPosition();
            float dx = posB.x - posA.x;
            float dy = posB.y - posA.y;
            float dz = posB.z - posA.z;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);
            if (dist < 0.0001f) { remainTick = 0.0f; break; }

            float nx = dx / dist;
            float ny = dy / dist;
            float nz = dz / dist;

            float v1 = a.m_speedVec.x * nx + a.m_speedVec.y * ny + a.m_speedVec.z * nz;
            float v2 = b.m_speedVec.x * nx + b.m_speedVec.y * ny + b.m_speedVec.z * nz;

            if (v1 > v2)
            {
                float totalMass = m1 + m2;
                float v1_after = ((m1 - e * m2) * v1 + (1.0f + e) * m2 * v2) / totalMass;
                float v2_after = ((m2 - e * m1) * v2 + (1.0f + e) * m1 * v1) / totalMass;

                float dv1 = v1_after - v1;
                float dv2 = v2_after - v2;
                a.m_speedVec.x += dv1 * nx;
                a.m_speedVec.y += dv1 * ny;
                a.m_speedVec.z += dv1 * nz;
                b.m_speedVec.x += dv2 * nx;
                b.m_speedVec.y += dv2 * ny;
                b.m_speedVec.z += dv2 * nz;
            }

            // めり込み補正（m_posに適用）
            float overlap = minDist - dist;
            if (overlap > 0.0f)
            {
                a.m_pos.x -= nx * overlap * 0.5f;
                a.m_pos.y -= ny * overlap * 0.5f;
                a.m_pos.z -= nz * overlap * 0.5f;
                b.m_pos.x += nx * overlap * 0.5f;
                b.m_pos.y += ny * overlap * 0.5f;
                b.m_pos.z += nz * overlap * 0.5f;
            }
        }

        if (remainTick < 0.0001f) break;
        if (hitA < 0) break;
    }
	// ----------------------------------------------------------------------------------
}
void SceneBurstVectoe::Draw()
{
	for (int i = 0; i < m_Balls.size(); i++)
	{
		m_Balls[i].Drow();
	}
}
