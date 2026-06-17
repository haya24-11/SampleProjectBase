#ifndef __MAIN_H__
#define __MAIN_H__

#include <string>
#include <Windows.h>

HRESULT Init(HWND hWnd, UINT width, UINT height);
void SetClearColor(float r, float g, float b);
void Uninit();
void Update(HWND hWnd, float tick);
void Draw();

#endif // __MAIN_H__