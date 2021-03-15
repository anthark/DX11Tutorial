#pragma once

#include <d3d11.h>
#include <dxgi.h>

class Renderer
{
public:
	Renderer();

	bool Init(HWND hWnd);
	void Term();

	void Resize(UINT width, UINT height);

	bool Render();

private:
	HRESULT CreateBackBufferRTV();

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;

	IDXGISwapChain* m_pSwapChain;
	ID3D11RenderTargetView* m_pBackBufferRTV;

	UINT m_width;
	UINT m_height;
};
