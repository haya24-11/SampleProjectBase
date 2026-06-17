#ifndef __SCENE_PBR_H__
#define __SCENE_PBR_H__

#include "SceneBase.hpp"
#include "Texture.h"

class ScenePBR : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:
	float m_time = 0.0f;
	static const UINT SHADOW_SIZE = 1024;
};

#endif // __SCENE_PBR_H__
