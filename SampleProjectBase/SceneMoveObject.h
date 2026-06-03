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
    DirectX::SimpleMath::Vector3 m_pos;
    DirectX::SimpleMath::Vector3 m_v;
    DirectX::SimpleMath::Vector3 m_v0;       // © ‰‘¬“x
    DirectX::SimpleMath::Vector3 m_a;
    DirectX::SimpleMath::Vector3 m_startPos; // © ‰ŠúˆÊ’u
    float m_t;
    int m_mode;
    float m_theta; // Î•û“ŠË‚ÌŠp“x
};

#endif // __SceneMoveObject___