#pragma once

#include <d3d11.h>
#include <dxgi.h>

class Renderer
{
public:
	Renderer();

	bool Init(HWND hWnd);
	void Term();

	bool Render();

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;

	IDXGISwapChain* m_pSwapChain;
};
