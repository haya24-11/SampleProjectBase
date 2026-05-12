#ifndef _SceneMoveObject_
#define _SceneMoveObject_
#include "SceneBase.hpp"
#include "Ball.h"
#include <vector>

class SceneMoveObject : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:

};

#endif // __SceneMoveObject___