#ifndef __SCENE_BUMPMAP_H__
#define __SCENE_BUMPMAP_H__

#include "SceneBase.hpp"

class SceneBumpmap : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:

};

#endif // __SCENE_BUMPMAP_H__