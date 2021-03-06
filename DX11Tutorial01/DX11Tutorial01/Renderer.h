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

	void MouseMove(int dx, int dy);
	void MouseWheel(int dz);

	void SwitchNormalMode();

private:
	HRESULT SetupBackBuffer();

	HRESULT CreateTransparentObjects();
	HRESULT CreateScene();
	void DestroyScene();
	void RenderScene();
	void RenderSceneTransparent();

	ID3D11VertexShader* CreateVertexShader(LPCTSTR shaderSource, ID3DBlob** ppBlob);
	ID3D11PixelShader*  CreatePixelShader(LPCTSTR shaderSource);

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;

	IDXGISwapChain* m_pSwapChain;
	ID3D11RenderTargetView* m_pBackBufferRTV;

	ID3D11Texture2D* m_pDepth;
	ID3D11DepthStencilView* m_pDepthDSV;

	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	ID3D11VertexShader* m_pVertexShader;
	ID3D11PixelShader* m_pPixelShader;
	ID3D11InputLayout* m_pInputLayout;

	ID3D11Resource* m_pTexture;
	ID3D11ShaderResourceView* m_pTextureSRV;

	ID3D11Resource* m_pTextureNM;
	ID3D11ShaderResourceView* m_pTextureNMSRV;

	ID3D11SamplerState* m_pSamplerState;

	ID3D11Buffer* m_pModelBuffer;
	ID3D11Buffer* m_pModelBuffer2;
	ID3D11Buffer* m_pSceneBuffer;

	ID3D11Buffer* m_pModelBuffer3;
	ID3D11Buffer* m_pModelBuffer4;
	ID3D11Buffer* m_pTransVertexBuffer;
	ID3D11Buffer* m_pTransIndexBuffer;
	ID3D11VertexShader* m_pTransVertexShader;
	ID3D11PixelShader* m_pTransPixelShader;
	ID3D11InputLayout* m_pTransInputLayout;
	ID3D11RasterizerState* m_pTransRasterizerState;
	ID3D11BlendState* m_pTransBlendState;
	ID3D11DepthStencilState* m_pTransDepthState;

	ID3D11RasterizerState* m_pRasterizerState;

	UINT m_width;
	UINT m_height;

	size_t m_usec;

	float m_lon;
	float m_lat;
	float m_dist;

	int m_mode;
};
