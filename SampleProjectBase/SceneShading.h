#ifndef __SCENE_SHADING_H__
#define __SCENE_SHADING_H__

#include "SceneBase.hpp"

class SceneShading : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:

};

#endif // __SCENE_SHADING_H__