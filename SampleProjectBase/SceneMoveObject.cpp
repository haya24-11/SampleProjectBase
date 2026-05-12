#include "math.h"
#include "SceneMoveObject.h"
#include "Geometory.h"
#include "DebugLog.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void SceneMoveObject::Init()
{

}
void SceneMoveObject::Uninit()
{

}
void SceneMoveObject::Update(float tick)
{

}
void SceneMoveObject::Draw()
{
	Geometory::AddLine(Vector3(2, 1, 0), Vector3(-2, 1, 0));
	Geometory::DrawLines();
}
