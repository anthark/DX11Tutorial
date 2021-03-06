#include "Renderer.h"

#include <stdio.h>
#include <tchar.h>
#include <assert.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "DDSTextureLoader11.h"

#include <chrono>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace DirectX;

static const UINT IndicesCount = 36;

static const XMVECTORF32 TransPos1{ 2.5f, 0, 0 };
static const XMVECTORF32 TransPos2{ 3.0f, 0.5f, 0.5f };

struct TextureVertex
{
	XMVECTORF32 pos;
	XMFLOAT2 uv;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
};

struct ModelBuffer
{
	XMMATRIX modelMatrix;
	XMMATRIX normalMatrix;
	XMVECTORF32 objColor;
};

struct Light
{
	XMVECTORF32 pos;
	XMVECTORF32 color;
};

struct SceneBuffer
{
	XMMATRIX VP;
	XMVECTORI32 lightParams; // x - lights count

	Light lights[4];
};

#define SAFE_RELEASE(p) \
if (p != NULL) { \
	p->Release(); \
	p = NULL;\
}

Renderer::Renderer()
	: m_pDevice(NULL)
	, m_pContext(NULL)
	, m_pSwapChain(NULL)
	, m_pBackBufferRTV(NULL)
	, m_pDepth(NULL)
	, m_pDepthDSV(NULL)
	, m_width(0)
	, m_height(0)
	, m_pVertexBuffer(NULL)
	, m_pIndexBuffer(NULL)
	, m_pVertexShader(NULL)
	, m_pPixelShader(NULL)
	, m_pInputLayout(NULL)
	, m_pTexture(NULL)
	, m_pTextureSRV(NULL)
	, m_pTextureNM(NULL)
	, m_pTextureNMSRV(NULL)
	, m_pSamplerState(NULL)
	, m_pModelBuffer(NULL)
	, m_pModelBuffer2(NULL)
	, m_pSceneBuffer(NULL)
	, m_pRasterizerState(NULL)
	, m_usec(0)
	, m_lon(0.0f)
	, m_lat(0.0f)
	, m_dist(10.0f)
	, m_mode(0)
	, m_pModelBuffer3(NULL)
	, m_pModelBuffer4(NULL)
	, m_pTransVertexBuffer(NULL)
	, m_pTransIndexBuffer(NULL)
	, m_pTransVertexShader(NULL)
	, m_pTransPixelShader(NULL)
	, m_pTransInputLayout(NULL)
	, m_pTransRasterizerState(NULL)
	, m_pTransDepthState(NULL)
{
}

bool Renderer::Init(HWND hWnd)
{
	HRESULT result;

	// Create a DirectX graphics interface factory.
	IDXGIFactory* pFactory = NULL;
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
	assert(SUCCEEDED(result));

	// Select hardware adapter
	IDXGIAdapter* pSelectedAdapter = NULL;
	if (SUCCEEDED(result))
	{
		IDXGIAdapter* pAdapter = NULL;
		UINT adapterIdx = 0;
		while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter)))
		{
			DXGI_ADAPTER_DESC desc;
			pAdapter->GetDesc(&desc);

			if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0)
			{
				pSelectedAdapter = pAdapter;
				break;
			}

			pAdapter->Release();

			adapterIdx++;
		}
	}
	assert(pSelectedAdapter != NULL);

	// Create DirectX 11 device
	D3D_FEATURE_LEVEL level;
	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
	if (SUCCEEDED(result))
	{
		result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
			D3D11_CREATE_DEVICE_DEBUG, levels, 1, D3D11_SDK_VERSION, &m_pDevice, &level, &m_pContext);
		assert(level == D3D_FEATURE_LEVEL_11_0);
		assert(SUCCEEDED(result));
	}

	// Create swapchain
	if (SUCCEEDED(result))
	{
		RECT rc;
		GetClientRect(hWnd, &rc);
		m_width = rc.right - rc.left;
		m_height = rc.bottom - rc.top;

		DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferDesc.Width = m_width;
		swapChainDesc.BufferDesc.Height = m_height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Windowed = true;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		result = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		assert(SUCCEEDED(result));
	}

	// Create render target views
	if (SUCCEEDED(result))
	{
		result = SetupBackBuffer();
	}

	// Create scene for render
	if (SUCCEEDED(result))
	{
		result = CreateScene();
	}

	// Create texture samplers
	if (SUCCEEDED(result))
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = 1000;
		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 16;

		result = m_pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState);
	}

	SAFE_RELEASE(pSelectedAdapter);
	SAFE_RELEASE(pFactory);

	return SUCCEEDED(result);
}

void Renderer::Term()
{
	DestroyScene();

	SAFE_RELEASE(m_pDepthDSV);
	SAFE_RELEASE(m_pDepth);
	SAFE_RELEASE(m_pBackBufferRTV);
	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pContext);
	SAFE_RELEASE(m_pDevice);
}

void Renderer::Resize(UINT width, UINT height)
{
	if (width != m_width || height != m_height)
	{
		SAFE_RELEASE(m_pDepthDSV);
		SAFE_RELEASE(m_pDepth);
		SAFE_RELEASE(m_pBackBufferRTV);

		HRESULT result = m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		if (SUCCEEDED(result))
		{
			m_width = width;
			m_height = height;

			result = SetupBackBuffer();
		}
		assert(SUCCEEDED(result));
	}
}

bool Renderer::Update()
{
	size_t usec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	if (m_usec == 0)
	{
		m_usec = usec; // Initial update
	}

	double elapsedSec = (usec - m_usec) / 1000000.0;

	ModelBuffer cb;
	XMMATRIX m = XMMatrixRotationAxis({ 0,1,0 }, (float)elapsedSec * 0.5f);

	cb.modelMatrix = XMMatrixTranspose(m);
	//cb.normalMatrix = XMMatrixTranspose(XMMatrixTranspose(XMMatrixInverse(NULL, m)));
	cb.normalMatrix = XMMatrixInverse(NULL, m);
	m_pContext->UpdateSubresource(m_pModelBuffer, 0, NULL, &cb, 0, 0);

	cb.modelMatrix = XMMatrixTranspose(XMMatrixTranslation(1.5f, 0, 0));
	cb.normalMatrix = XMMatrixIdentity();
	m_pContext->UpdateSubresource(m_pModelBuffer2, 0, NULL, &cb, 0, 0);

	cb.modelMatrix = XMMatrixTranspose(XMMatrixTranslation(TransPos1.f[0], TransPos1.f[1], TransPos1.f[2]));
	cb.objColor = XMVECTORF32{0.75f, 0, 0.75f, 0.15f};
	m_pContext->UpdateSubresource(m_pModelBuffer3, 0, NULL, &cb, 0, 0);

	cb.modelMatrix = XMMatrixTranspose(XMMatrixTranslation(TransPos2.f[0], TransPos2.f[1], TransPos2.f[2]));
	cb.objColor = XMVECTORF32{ 0.1f, 0, 0.6f, 0.8f };
	m_pContext->UpdateSubresource(m_pModelBuffer4, 0, NULL, &cb, 0, 0);

	// Setup scene buffer
	SceneBuffer scb;

	static const float nearPlane = 0.001f;
	static const float farPlane = 100.0f;
	static const float fov = (float)M_PI * 2.0 / 3.0;

	XMMATRIX view = XMMatrixInverse(NULL, XMMatrixTranslation(0, 0, -m_dist) * XMMatrixRotationAxis({ 1,0,0 }, m_lat) * XMMatrixRotationAxis({ 0,1,0 }, m_lon));

	float width = nearPlane / tanf(fov / 2.0);
	float height = ((float)m_height / m_width) * width;
	scb.VP = XMMatrixTranspose(view * XMMatrixPerspectiveLH(width, height, nearPlane, farPlane));

	scb.lightParams.i[1] = m_mode;

	// Setup lights
	/*
	scb.lightParams.i[0] = 1;
	scb.lights[0].pos = XMVECTORF32{ 0, 1, 0, 0 };
	scb.lights[0].color = XMVECTORF32{ 0.75f, 0, 0, 0 };
	*/

	scb.lightParams.i[0] = 3;
	scb.lights[0].pos = XMVECTORF32{ 0, 1, 0, 0 };
	scb.lights[0].color = XMVECTORF32{ 0.75f, 0, 0, 0 };
	scb.lights[1].pos = XMVECTORF32{ 3, 1, 0, 0 };
	scb.lights[1].color = XMVECTORF32{ 0, 0.75f, 0, 0 };
	scb.lights[2].pos = XMVECTORF32{ 0, 0, 2, 0 };
	scb.lights[2].color = XMVECTORF32{ 1, 1, 1, 0 };

	m_pContext->UpdateSubresource(m_pSceneBuffer, 0, NULL, &scb, 0, 0);

	return true;
}

bool Renderer::Render()
{
	m_pContext->ClearState();

	ID3D11RenderTargetView* views[] = {m_pBackBufferRTV};
	m_pContext->OMSetRenderTargets(1, views, m_pDepthDSV);

	static const FLOAT BackColor[4] = {0.25f, 0.25f, 0.25f, 1.0f};
	m_pContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);
	m_pContext->ClearDepthStencilView(m_pDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	D3D11_VIEWPORT viewport{0, 0, (float)m_width, (float)m_height, 0.0f, 1.0f};
	m_pContext->RSSetViewports(1, &viewport);
	D3D11_RECT rect{ 0, 0, (LONG)m_width, (LONG)m_height };
	m_pContext->RSSetScissorRects(1, &rect);

	RenderScene();

	HRESULT result = m_pSwapChain->Present(0, 0);
	assert(SUCCEEDED(result));

	return SUCCEEDED(result);
}

void Renderer::MouseMove(int dx, int dy)
{
	m_lon += (float)dx / m_width * 5.0f;

	m_lat += (float)dy / m_height * 5.0f;
	if (m_lat <= -(float)M_PI / 2)
	{
		m_lat = -(float)M_PI / 2;
	}
	if (m_lat >= (float)M_PI / 2)
	{
		m_lat = (float)M_PI / 2;
	}
}

void Renderer::MouseWheel(int dz)
{
	m_dist += dz / 100.0f;
	if (m_dist < 0)
	{
		m_dist = 0;
	}
}

void Renderer::SwitchNormalMode()
{
	m_mode = (m_mode + 1) % 2;
}

HRESULT Renderer::SetupBackBuffer()
{
	ID3D11Texture2D* pBackBuffer = NULL;
	HRESULT result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	assert(SUCCEEDED(result));
	if (SUCCEEDED(result))
	{
		result = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferRTV);
		assert(SUCCEEDED(result));

		SAFE_RELEASE(pBackBuffer);
	}
	if (SUCCEEDED(result))
	{
		D3D11_TEXTURE2D_DESC depthDesc = {};
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.ArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.Height = m_height;
		depthDesc.Width = m_width;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthDesc.CPUAccessFlags = 0;
		depthDesc.MiscFlags = 0;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.SampleDesc.Quality = 0;

		result = m_pDevice->CreateTexture2D(&depthDesc, NULL, &m_pDepth);
		if (SUCCEEDED(result))
		{
			result = m_pDevice->CreateDepthStencilView(m_pDepth, NULL, &m_pDepthDSV);
		}
	}

	return result;
}

HRESULT Renderer::CreateTransparentObjects()
{
	// Textured cube
	static const XMVECTORF32 Vertices[4] = {
		{0, -1, -1, 1},
		{0,  1, -1, 1},
		{0,  1,  1, 1},
		{0, -1,  1, 1}
	};
	static const UINT16 Indices[6] = {
		0, 2, 1, 0, 3, 2
	};

	// Create vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData = { 0 };
	vertexData.pSysMem = Vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT result = m_pDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &m_pTransVertexBuffer);
	assert(SUCCEEDED(result));

	// Create index buffer
	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC indexBufferDesc = { 0 };
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(Indices);
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexData = { 0 };
		indexData.pSysMem = Indices;
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		result = m_pDevice->CreateBuffer(&indexBufferDesc, &indexData, &m_pTransIndexBuffer);
		assert(SUCCEEDED(result));
	}

	// Create vertex shader
	ID3DBlob* pBlob = NULL;
	m_pTransVertexShader = CreateVertexShader(_T("TransColorShader.hlsl"), &pBlob);
	// Create pixel shader
	if (m_pTransVertexShader)
	{
		m_pTransPixelShader = CreatePixelShader(_T("TransColorShader.hlsl"));
	}
	assert(m_pTransVertexShader != NULL && m_pTransPixelShader != NULL);
	if (m_pTransVertexShader == NULL || m_pTransPixelShader == NULL)
	{
		result = E_FAIL;
	}

	// Create input layout
	if (SUCCEEDED(result))
	{
		D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[1] = {
			D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		result = m_pDevice->CreateInputLayout(inputLayoutDesc, 1, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &m_pTransInputLayout);
		assert(SUCCEEDED(result));
	}
	SAFE_RELEASE(pBlob);

	// Create model constant buffer
	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC cbDesc = { 0 };
		cbDesc.ByteWidth = sizeof(ModelBuffer);
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		result = m_pDevice->CreateBuffer(&cbDesc, NULL, &m_pModelBuffer3);
		if (SUCCEEDED(result))
		{
			result = m_pDevice->CreateBuffer(&cbDesc, NULL, &m_pModelBuffer4);
		}
	}

	// Create rasterizer state
	if (SUCCEEDED(result))
	{
		D3D11_RASTERIZER_DESC rasterizerDesc;
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.ScissorEnable = FALSE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;

		result = m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pTransRasterizerState);
	}

	// Create blend state
	if (SUCCEEDED(result))
	{
		D3D11_BLEND_DESC blendDesc = { 0 };
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = 0xF;

		result = m_pDevice->CreateBlendState(&blendDesc, &m_pTransBlendState);
	}

	// Create depth state
	if (SUCCEEDED(result))
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc = { 0 };
		dsDesc.DepthEnable = TRUE;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		dsDesc.StencilEnable = FALSE;

		result = m_pDevice->CreateDepthStencilState(&dsDesc, &m_pTransDepthState);
	}

	return result;
}

HRESULT Renderer::CreateScene()
{
	// Textured cube
	static const TextureVertex Vertices[24] = {
		// Bottom face
		{{-0.5, -0.5,  0.5, 1}, {0,1}, {0,-1,0}, {1,0,0}},
		{{ 0.5, -0.5,  0.5, 1}, {1,1}, {0,-1,0}, {1,0,0}},
		{{ 0.5, -0.5, -0.5, 1}, {1,0}, {0,-1,0}, {1,0,0}},
		{{-0.5, -0.5, -0.5, 1}, {0,0}, {0,-1,0}, {1,0,0}},
		// Top face
		{{-0.5,  0.5, -0.5, 1}, {0,1}, {0,1,0}, {1,0,0}},
		{{ 0.5,  0.5, -0.5, 1}, {1,1}, {0,1,0}, {1,0,0}},
		{{ 0.5,  0.5,  0.5, 1}, {1,0}, {0,1,0}, {1,0,0}},
		{{-0.5,  0.5,  0.5, 1}, {0,0}, {0,1,0}, {1,0,0}},
		// Front face
		{{ 0.5, -0.5, -0.5, 1}, {0,1}, {1,0,0}, {0,0,1}},
		{{ 0.5, -0.5,  0.5, 1}, {1,1}, {1,0,0}, {0,0,1}},
		{{ 0.5,  0.5,  0.5, 1}, {1,0}, {1,0,0}, {0,0,1}},
		{{ 0.5,  0.5, -0.5, 1}, {0,0}, {1,0,0}, {0,0,1}},
		// Back face
		{{-0.5, -0.5,  0.5, 1}, {0,1}, {-1,0,0}, {0,0,-1}},
		{{-0.5, -0.5, -0.5, 1}, {1,1}, {-1,0,0}, {0,0,-1}},
		{{-0.5,  0.5, -0.5, 1}, {1,0}, {-1,0,0}, {0,0,-1}},
		{{-0.5,  0.5,  0.5, 1}, {0,0}, {-1,0,0}, {0,0,-1}},
		// Left face
		{{ 0.5, -0.5,  0.5, 1}, {0,1}, {0,0,1}, {-1,0,0}},
		{{-0.5, -0.5,  0.5, 1}, {1,1}, {0,0,1}, {-1,0,0}},
		{{-0.5,  0.5,  0.5, 1}, {1,0}, {0,0,1}, {-1,0,0}},
		{{ 0.5,  0.5,  0.5, 1}, {0,0}, {0,0,1}, {-1,0,0}},
		// Right face
		{{-0.5, -0.5, -0.5, 1}, {0,1}, {0,0,-1}, {1,0,0}},
		{{ 0.5, -0.5, -0.5, 1}, {1,1}, {0,0,-1}, {1,0,0}},
		{{ 0.5,  0.5, -0.5, 1}, {1,0}, {0,0,-1}, {1,0,0}},
		{{-0.5,  0.5, -0.5, 1}, {0,0}, {0,0,-1}, {1,0,0}},
	};
	static const UINT16 Indices[IndicesCount] = {
		0, 2, 1, 0, 3, 2,
		4, 6, 5, 4, 7, 6,
		8, 10, 9, 8, 11, 10,
		12, 14, 13, 12, 15, 14, 
		16, 18, 17, 16, 19, 18,
		20, 22, 21, 20, 23, 22
	};

	// Create vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData = { 0 };
	vertexData.pSysMem = Vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT result = m_pDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &m_pVertexBuffer);
	assert(SUCCEEDED(result));

	// Create index buffer
	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC indexBufferDesc = { 0 };
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(Indices);
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexData = { 0 };
		indexData.pSysMem = Indices;
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		result = m_pDevice->CreateBuffer(&indexBufferDesc, &indexData, &m_pIndexBuffer);
		assert(SUCCEEDED(result));
	}

	// Create vertex shader
	ID3DBlob* pBlob = NULL;
	m_pVertexShader = CreateVertexShader(_T("ColorShader.hlsl"), &pBlob);
	// Create pixel shader
	if (m_pVertexShader)
	{
		m_pPixelShader = CreatePixelShader(_T("ColorShader.hlsl"));
	}
	assert(m_pVertexShader != NULL && m_pPixelShader != NULL);
	if (m_pVertexShader == NULL || m_pPixelShader == NULL)
	{
		result = E_FAIL;
	}

	// Create input layout
	if (SUCCEEDED(result))
	{
		D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[4] = {
			D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			D3D11_INPUT_ELEMENT_DESC{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMVECTORF32), D3D11_INPUT_PER_VERTEX_DATA, 0},
			D3D11_INPUT_ELEMENT_DESC{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMVECTORF32) + sizeof(XMFLOAT2), D3D11_INPUT_PER_VERTEX_DATA, 0},
			D3D11_INPUT_ELEMENT_DESC{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMVECTORF32) + sizeof(XMFLOAT2) + sizeof(XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		result = m_pDevice->CreateInputLayout(inputLayoutDesc, 4, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &m_pInputLayout);
		assert(SUCCEEDED(result));
	}
	SAFE_RELEASE(pBlob);

	// Create model constant buffer
	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC cbDesc = { 0 };
		cbDesc.ByteWidth = sizeof(ModelBuffer);
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		result = m_pDevice->CreateBuffer(&cbDesc, NULL, &m_pModelBuffer);
		if (SUCCEEDED(result))
		{
			result = m_pDevice->CreateBuffer(&cbDesc, NULL, &m_pModelBuffer2);
		}
	}

	// Create scene constant buffer
	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC cbDesc = { 0 };
		cbDesc.ByteWidth = sizeof(SceneBuffer);
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		result = m_pDevice->CreateBuffer(&cbDesc, NULL, &m_pSceneBuffer);
	}

	// Create rasterizer state
	if (SUCCEEDED(result))
	{
		D3D11_RASTERIZER_DESC rasterizerDesc;
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_BACK;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.ScissorEnable = FALSE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;

		result = m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pRasterizerState);
	}

	// Create textures
	if (SUCCEEDED(result))
	{
		//result = DirectX::CreateDDSTextureFromFile(m_pDevice, L"Rocks.dds", (ID3D11Resource**)&m_pTexture, &m_pTextureSRV);
		result = DirectX::CreateDDSTextureFromFile(m_pDevice, L"Brick.dds", (ID3D11Resource**)&m_pTexture, &m_pTextureSRV);
	}
	if (SUCCEEDED(result))
	{
		result = DirectX::CreateDDSTextureFromFile(m_pDevice, L"BrickNM.dds", (ID3D11Resource**)&m_pTextureNM, &m_pTextureNMSRV);
	}

	if (SUCCEEDED(result))
	{
		result = CreateTransparentObjects();
	}

	return result;
}

void Renderer::DestroyScene()
{
	SAFE_RELEASE(m_pTransDepthState);

	SAFE_RELEASE(m_pTransBlendState);
	SAFE_RELEASE(m_pTransRasterizerState);

	SAFE_RELEASE(m_pModelBuffer3);
	SAFE_RELEASE(m_pModelBuffer4);
	SAFE_RELEASE(m_pTransVertexBuffer);
	SAFE_RELEASE(m_pTransIndexBuffer);
	SAFE_RELEASE(m_pTransVertexShader);
	SAFE_RELEASE(m_pTransPixelShader);
	SAFE_RELEASE(m_pTransInputLayout);

	SAFE_RELEASE(m_pSamplerState);

	SAFE_RELEASE(m_pTextureNMSRV);
	SAFE_RELEASE(m_pTextureNM);

	SAFE_RELEASE(m_pTextureSRV);
	SAFE_RELEASE(m_pTexture);

	SAFE_RELEASE(m_pRasterizerState);
	SAFE_RELEASE(m_pModelBuffer2);
	SAFE_RELEASE(m_pModelBuffer);
	SAFE_RELEASE(m_pSceneBuffer);

	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexShader);

	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pVertexBuffer);
}

void Renderer::RenderScene()
{
	// Render transparents
	//RenderSceneTransparent();

	ID3D11Buffer* vertexBuffers[] = {m_pVertexBuffer};
	UINT stride = sizeof(TextureVertex);
	UINT offset = 0;

	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	m_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	m_pContext->IASetInputLayout(m_pInputLayout);

	m_pContext->VSSetShader(m_pVertexShader, NULL, 0);
	m_pContext->PSSetShader(m_pPixelShader, NULL, 0);

	{
		ID3D11Buffer* constBuffers[] = { m_pSceneBuffer };
		m_pContext->VSSetConstantBuffers(1, 1, constBuffers);
		m_pContext->PSSetConstantBuffers(1, 1, constBuffers);
	}

	m_pContext->RSSetState(m_pRasterizerState);
	m_pContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11ShaderResourceView* textures[] = {m_pTextureSRV, m_pTextureNMSRV};
	m_pContext->PSSetShaderResources(0, 2, textures);

	ID3D11SamplerState* samplers[] = {m_pSamplerState};
	m_pContext->PSSetSamplers(0, 1, samplers);

	// First triangle
	{
		ID3D11Buffer* constBuffers[] = { m_pModelBuffer };
		m_pContext->VSSetConstantBuffers(0, 1, constBuffers);

		m_pContext->DrawIndexed(IndicesCount, 0, 0);
	}

	// Second triangle
	{
		ID3D11Buffer* constBuffers[] = { m_pModelBuffer2 };
		m_pContext->VSSetConstantBuffers(0, 1, constBuffers);

		m_pContext->DrawIndexed(IndicesCount, 0, 0);
	}

	// Render transparents
	RenderSceneTransparent();
}

void Renderer::RenderSceneTransparent()
{
	ID3D11Buffer* vertexBuffers[] = { m_pTransVertexBuffer };
	UINT stride = sizeof(XMVECTORF32);
	UINT offset = 0;

	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	m_pContext->IASetIndexBuffer(m_pTransIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	m_pContext->IASetInputLayout(m_pTransInputLayout);

	m_pContext->VSSetShader(m_pTransVertexShader, NULL, 0);
	m_pContext->PSSetShader(m_pTransPixelShader, NULL, 0);

	{
		ID3D11Buffer* constBuffers[] = { m_pSceneBuffer };
		m_pContext->VSSetConstantBuffers(1, 1, constBuffers);
		m_pContext->PSSetConstantBuffers(1, 1, constBuffers);
	}

	m_pContext->RSSetState(m_pRasterizerState);
	m_pContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_pContext->RSSetState(m_pTransRasterizerState);
	m_pContext->OMSetBlendState(m_pTransBlendState, NULL, 0xFFFFFFFF);

	m_pContext->OMSetDepthStencilState(m_pTransDepthState, 0);

	XMMATRIX view = XMMatrixTranslation(0, 0, -m_dist) * XMMatrixRotationAxis({ 1,0,0 }, m_lat) * XMMatrixRotationAxis({ 0,1,0 }, m_lon);
	XMVECTOR cameraPos = XMVector4Transform(XMVECTOR{ 0,0,0,1 }, view);
	XMVECTOR cameraDir = -cameraPos / m_dist;

	float transDist1 = XMVector3Dot(TransPos1.v - cameraPos, cameraDir).m128_f32[0];
	float transDist2 = XMVector3Dot(TransPos2.v - cameraPos, cameraDir).m128_f32[0];

	if (transDist1 < transDist2)
	{
		// Second transparent quad
		{
			ID3D11Buffer* constBuffers[] = { m_pModelBuffer4 };
			m_pContext->VSSetConstantBuffers(0, 1, constBuffers);
			m_pContext->PSSetConstantBuffers(0, 1, constBuffers);

			m_pContext->DrawIndexed(6, 0, 0);
		}
		// First transparent quad
		{
			ID3D11Buffer* constBuffers[] = { m_pModelBuffer3 };
			m_pContext->VSSetConstantBuffers(0, 1, constBuffers);
			m_pContext->PSSetConstantBuffers(0, 1, constBuffers);

			m_pContext->DrawIndexed(6, 0, 0);
		}
	}
	else
	{
		// First transparent quad
		{
			ID3D11Buffer* constBuffers[] = { m_pModelBuffer3 };
			m_pContext->VSSetConstantBuffers(0, 1, constBuffers);
			m_pContext->PSSetConstantBuffers(0, 1, constBuffers);

			m_pContext->DrawIndexed(6, 0, 0);
		}
		// Second transparent quad
		{
			ID3D11Buffer* constBuffers[] = { m_pModelBuffer4 };
			m_pContext->VSSetConstantBuffers(0, 1, constBuffers);
			m_pContext->PSSetConstantBuffers(0, 1, constBuffers);

			m_pContext->DrawIndexed(6, 0, 0);
		}
	}
}

ID3D11VertexShader* Renderer::CreateVertexShader(LPCTSTR shaderSource, ID3DBlob** ppBlob)
{
	ID3D11VertexShader* pVertexShader = NULL;

	FILE* pFile = NULL;

	_tfopen_s(&pFile, shaderSource, _T("rb"));
	if (pFile != NULL)
	{
		fseek(pFile, 0, SEEK_END);
		int size = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		char* pSourceCode = (char*)malloc((size_t)size + 1);
		fread(pSourceCode, size, 1, pFile);
		pSourceCode[size] = 0;

		fclose(pFile);

		ID3DBlob* pError = NULL;
		HRESULT result = D3DCompile(pSourceCode, size, "", NULL, NULL, "VS", "vs_5_0", 0, 0, ppBlob, &pError);
		if (!SUCCEEDED(result))
		{
			const char *pMsg = (const char*)pError->GetBufferPointer();
			OutputDebugStringA(pMsg);
		}
		else
		{
			result = m_pDevice->CreateVertexShader((*ppBlob)->GetBufferPointer(), (*ppBlob)->GetBufferSize(), NULL, &pVertexShader);
			assert(SUCCEEDED(result));
		}

		SAFE_RELEASE(pError);
	}

	return pVertexShader;
}

ID3D11PixelShader* Renderer::CreatePixelShader(LPCTSTR shaderSource)
{
	ID3D11PixelShader* pPixelShader = NULL;

	FILE* pFile = NULL;

	_tfopen_s(&pFile, shaderSource, _T("rb"));
	if (pFile != NULL)
	{
		fseek(pFile, 0, SEEK_END);
		int size = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		char* pSourceCode = (char*)malloc((size_t)size + 1);
		fread(pSourceCode, size, 1, pFile);
		pSourceCode[size] = 0;

		fclose(pFile);

		ID3DBlob* pBlob = NULL;
		ID3DBlob* pError = NULL;
		HRESULT result = D3DCompile(pSourceCode, size, "", NULL, NULL, "PS", "ps_5_0", 0, 0, &pBlob, &pError);
		if (!SUCCEEDED(result))
		{
			const char* pMsg = (const char*)pError->GetBufferPointer();
			OutputDebugStringA(pMsg);
		}
		else
		{
			result = m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &pPixelShader);
			assert(SUCCEEDED(result));
		}

		SAFE_RELEASE(pError);
		SAFE_RELEASE(pBlob);
	}

	return pPixelShader;
}
