#ifndef __SCENE_BURST_VECTOR_H__
#define __SCENE_BURST_VECTOR_H__

#include "SceneBase.hpp"
#include "Ball.h"
#include <vector>

class SceneBurstVectoe : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:
	float m_time = 0.0f;
	float m_speed = 1.0f;

	std::vector<Ball> m_Balls;
};

#endif // __SCENE_BURST_VECTOR_H___