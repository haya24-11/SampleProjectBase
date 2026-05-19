#ifndef __SCENE_LIGHT_H__
#define __SCENE_LIGHT_H__

#include "SceneBase.hpp"

class SceneLight : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:
};

#endif // __SCENE_LIGHT_H__