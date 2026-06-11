#ifndef __SCENE_TOON_H__
#define __SCENE_TOON_H__

#include "SceneBase.hpp"

class SceneToon : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:
	int m_Frame = 0;
};

#endif // __SCENE_TOON_H__