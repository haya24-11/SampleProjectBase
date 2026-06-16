#ifndef __SCENE_PBR_H__
#define __SCENE_PBR_H__

#include "SceneBase.hpp"

class ScenePBR : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
};

#endif // __SCENE_PBR_H__
