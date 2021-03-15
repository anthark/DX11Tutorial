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

	bool Update();
	bool Render();

private:
	HRESULT CreateBackBufferRTV();

	HRESULT CreateScene();
	void DestroyScene();
	void RenderScene();

	ID3D11VertexShader* CreateVertexShader(LPCTSTR shaderSource, ID3DBlob** ppBlob);
	ID3D11PixelShader*  CreatePixelShader(LPCTSTR shaderSource);

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;

	IDXGISwapChain* m_pSwapChain;
	ID3D11RenderTargetView* m_pBackBufferRTV;

	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	ID3D11VertexShader* m_pVertexShader;
	ID3D11PixelShader* m_pPixelShader;
	ID3D11InputLayout* m_pInputLayout;

	ID3D11Buffer* m_pModelBuffer;

	ID3D11RasterizerState* m_pRasterizerState;

	UINT m_width;
	UINT m_height;

	size_t m_usec;
};
