#include <windows.h>
#include <DirectXMath.h>
#include <time.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>
#include <dxerr.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxerr.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11d.lib")
#pragma comment(lib, "Effects11d.lib")

#include <d3dx11effect.h>

using namespace DirectX;

void CreateDepthStencilTexture();

struct MyVertex
{
	XMFLOAT3 pos;
	XMFLOAT4 color;

	XMFLOAT3 normal;
	XMFLOAT2 tex;
};

struct ConstantBuffer
{
	XMMATRIX wvp;
	XMMATRIX world;
	XMFLOAT4 lightDir;
	XMFLOAT4 lightColor;
};

XMFLOAT4 lightDirection =
{
	XMFLOAT4(1.f, 1.f, 1.f, 1.f),
};

XMFLOAT4 lightColor =
{
	XMFLOAT4(1.f, 1.f, 0.f, 1.f),
};

HWND                      g_hWnd = NULL;
IDXGISwapChain*           g_pSwapChain = NULL;
ID3D11Device*             g_pd3dDevice = NULL;
ID3D11DeviceContext*      g_pImmediateContext = NULL;
ID3D11RenderTargetView*   g_pRenderTargetView = NULL;
D3D_FEATURE_LEVEL         g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11VertexShader*       g_pVertexShader = NULL;
ID3D11InputLayout*		  g_pVertexLayout = NULL;
ID3D11Buffer*			  g_pVertexBuffer;
ID3D11PixelShader*		  g_pPixelShader = NULL;
ID3D11Buffer*			  g_pIndexBuffer;
XMMATRIX				  g_World;
XMMATRIX				  g_World2;
XMMATRIX				  g_View;
XMMATRIX				  g_Projection;
ID3D11Buffer*			  g_pConstantBuffer;
ID3D11Texture2D*		  g_pDepthStencil = NULL;
ID3D11DepthStencilView*	  g_pDepthStencilView = NULL;
ID3D11RasterizerState*    g_pSolidRS;
ID3D11RasterizerState*    g_pWireFrameRS;
ID3D11ShaderResourceView* g_pTextureRV = NULL;
ID3D11SamplerState*		  g_pSamplerLinear = NULL;

HRESULT InitDevice()
{
	HRESULT hr = S_OK;
	
	// Flag 설정
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// 내가 지원할 수 있는 버전을 써놓는다.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// Swap Chain에 대한 설명. (WNDCLASS를 생각해보자)
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;

	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	
	hr = D3D11CreateDeviceAndSwapChain(
		0,										// 기본 티스플레이 어댑터 사용
		D3D_DRIVER_TYPE_HARDWARE,	            // 3D 하드웨어 가속
		0,	                                    // 소프트웨어 구동 안함
		createDeviceFlags,
		featureLevels,
		numFeatureLevels,
		D3D11_SDK_VERSION,
		&sd,
		&g_pSwapChain,
		&g_pd3dDevice,
		&g_featureLevel,
		&g_pImmediateContext);

	if (FAILED(hr)) return hr;

	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	if (FAILED(hr)) return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();

	if (FAILED(hr)) return hr;

	CreateDepthStencilTexture();
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	D3D11_VIEWPORT vp;
	vp.Width = 800;
	vp.Height = 600;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);
	
	return hr;
}

HRESULT LoadTexture()
{
	HRESULT hr = D3DX11CreateShaderResourceViewFromFile(
		g_pd3dDevice,
		L"./images.jpg",
		NULL,
		NULL, &g_pTextureRV, NULL);

	if (FAILED(hr)) return hr;

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr)) return hr;

	return hr;
}

void CreateShader()
{
	ID3DBlob *pErrorBlob = NULL;
	ID3DBlob *pVSBlob = NULL;
	HRESULT hr = D3DX11CompileFromFile(L"MyShader.fx", 0, 0, "VS", "vs_5_0", 0, 0, 0, &pVSBlob, &pErrorBlob, 0);

	hr = g_pd3dDevice->CreateVertexShader(
		pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(),
		0,
		&g_pVertexShader);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	UINT numElements = ARRAYSIZE(layout);
	hr = g_pd3dDevice->CreateInputLayout(
		layout, numElements,
		pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(),
		&g_pVertexLayout);
	pVSBlob->Release();

	ID3DBlob* pPSBlob = NULL;
	D3DX11CompileFromFile(
		L"MyShader.fx", 0, 0,
		"PS", "ps_5_0", 0, 0, 0,
		&pPSBlob, &pErrorBlob, 0);

	hr = g_pd3dDevice->CreatePixelShader(
		pPSBlob->GetBufferPointer(),
		pPSBlob->GetBufferSize(),
		0, &g_pPixelShader);

	pPSBlob->Release();

	return;
}

void CreateVertexBuffer()
{
	HRESULT hr;
	MyVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT3(-0.33f, 0.33f, -0.33f), XMFLOAT2(1.f, 1.f) }, 
		{ XMFLOAT3(1.0f , 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT3(0.33f, 0.33f, -0.33f), XMFLOAT2(0.f, 1.f) }, 
		{ XMFLOAT3(1.0f , 1.0f,  1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.33f, 0.33f, 0.33f), XMFLOAT2(0.f, 0.f) },
		{ XMFLOAT3(-1.0f, 1.0f,  1.0f), XMFLOAT4(1.0f, 0.0f,  0.f,  1.f), XMFLOAT3(-0.33f, 0.33f, 0.33f), XMFLOAT2(1.f, 0.f) },
		{ XMFLOAT3(-1.f , -1.f,  -1.f), XMFLOAT4( 1.f,  0.f,  1.f,  1.f), XMFLOAT3(-0.33f, -0.33f, -0.33f), XMFLOAT2(0.f, 0.f) },
		{ XMFLOAT3(1.f  , -1.f,  -1.f), XMFLOAT4( 1.f,  1.f,  0.f,  1.f), XMFLOAT3(0.33f, -0.33f, -0.33f), XMFLOAT2(1.f, 0.f) },
		{ XMFLOAT3(1.f  , -1.f,   1.f), XMFLOAT4( 1.f,  1.f,  1.f,  1.f), XMFLOAT3(0.33f, -0.33f, -0.33f), XMFLOAT2(1.f, 1.f) },
		{ XMFLOAT3(-1.f , -1.f,   1.f), XMFLOAT4( 0.f,  0.f,  0.f,  1.f), XMFLOAT3(-0.33f, -0.33f, 0.33f), XMFLOAT2(0.f, 1.f) },
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(vertices);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pVertexBuffer);
	
	return;
}

void CreateIndexBuffer()
{
	UINT indices[] =
	{
		3, 1, 0,
		2, 1, 3,
		0, 5, 4,
		1, 5, 0, 
		3, 4, 7,
		0, 4, 3,
		1, 6, 5,
		2, 6, 1,
		2, 7, 6,
		3, 7, 2,
		6, 4, 5,
		7, 4, 6,
	};

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.ByteWidth = sizeof(indices);
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	ZeroMemory(&iinitData, sizeof(iinitData));
	iinitData.pSysMem = indices;
	g_pd3dDevice->CreateBuffer(&ibd, &iinitData, &g_pIndexBuffer);

	return;
}

void CreateDepthStencilTexture()
{
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = 800;
	descDepth.Height = 600;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	descDSV.Flags = 0;
	g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);

	g_pDepthStencil->Release();

	return;
}

void InitMatrix()
{
	g_World = XMMatrixIdentity();

	XMVECTOR pos = XMVectorSet(0.f, 0.f, -8.f, 1.f);
	XMVECTOR target = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	g_View = XMMatrixLookAtLH(pos, target, up);

	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, 800.f / (FLOAT)600.f, 0.3f, 1000.f);
}

void CreateConstantBuffer()
{
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));

	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;
	g_pd3dDevice->CreateBuffer(&cbd, NULL, &g_pConstantBuffer);
}

void CreateRenderState()
{
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = false;

	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pSolidRS);

	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pWireFrameRS);
}

void CalculateMatrixForBox(float dt)
{
	XMMATRIX mat = XMMatrixRotationY(dt);
	mat *= XMMatrixRotationX(-dt);
	g_World = mat;

	XMMATRIX wvp = g_World * g_View * g_Projection;
	ConstantBuffer cb;
	cb.wvp = XMMatrixTranspose(wvp);

	cb.world = XMMatrixTranspose(g_World);
	cb.lightDir = lightDirection;
	cb.lightColor = lightColor;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
}

void CalculateMatrixForBox2(float dt)
{
	XMMATRIX scale = XMMatrixScaling(1.f, 1.f, 1.f);
	XMMATRIX rotate = XMMatrixRotationZ(dt);
	float moveValue = 5.0f;
	XMMATRIX position = XMMatrixTranslation(moveValue, 0.f, 0.f);
	g_World2 = scale * rotate * position;

	XMMATRIX rotate2 = XMMatrixRotationY(-dt);
	g_World2 *= rotate2;

	XMMATRIX wvp = g_World2 * g_View * g_Projection;
	ConstantBuffer cb;

	cb.wvp = XMMatrixTranspose(wvp);
	cb.world = XMMatrixTranspose(g_World2);
	cb.lightDir = lightDirection;
	cb.lightColor = lightColor;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &cb, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
}

void Render(const float deltaTime)
{
	// Clear View
	float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
	g_pImmediateContext->ClearDepthStencilView(
		g_pDepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	// Set Input Assembler
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(MyVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set Shader and Draw
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	g_pImmediateContext->RSSetState(g_pSolidRS);

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	CalculateMatrixForBox(deltaTime);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	//g_pImmediateContext->RSSetState(g_pWireFrameRS);

	//CalculateMatrixForBox2(deltaTime);
	//g_pImmediateContext->DrawIndexed(36, 0, 0);

	g_pSwapChain->Present(0, 0);

	return;
}

void CleanupDevice()
{
	if (g_pSolidRS) g_pSolidRS->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();

	if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();

	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	if (g_pTextureRV) g_pTextureRV->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();

	return;
}

#define szWindowClass	TEXT("First")
#define szTitle			TEXT("First App")

LRESULT CALLBACK WndProc( HWND hWnd
						 , UINT message
						 , WPARAM wParam
						 , LPARAM lParam );

int APIENTRY WinMain( HINSTANCE hInstance,
					  HINSTANCE hPrevInstance,
					  LPSTR lpszCmdParam,
					  int nCmdShow)
{
	WNDCLASSEX wcex;

	wcex.cbSize		= sizeof(WNDCLASSEX);
	wcex.style		= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon	= LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hIconSm= LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;

	if( !RegisterClassEx(&wcex) )
		return 0;

	HWND	hWnd = CreateWindowEx( WS_EX_APPWINDOW
		, szWindowClass
		, szTitle
		, WS_OVERLAPPEDWINDOW
		, CW_USEDEFAULT
		, CW_USEDEFAULT
		, 800
		, 600
		, NULL
		, NULL
		, hInstance
		, NULL );

	if( !hWnd )
		return 0;

	ShowWindow(hWnd, nCmdShow);

	g_hWnd = hWnd;
	InitDevice();
	CreateShader();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateConstantBuffer();
	CreateRenderState();
	InitMatrix();
	LoadTexture();

	MSG			msg;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			static float deltaTime = 0;
			deltaTime += 0.0005f;
			Render(deltaTime);
		}
	}

	CleanupDevice();
	return (int)msg.wParam;
}

// 메시지 처리 함수
LRESULT CALLBACK WndProc( HWND hWnd
						 , UINT message
						 , WPARAM wParam
						 , LPARAM lParam )
{
	HDC	hdc;
	PAINTSTRUCT	ps;

	switch(message)
	{
	case WM_CREATE:

		break;
	case WM_PAINT:
		{
			hdc = BeginPaint( hWnd, &ps );

			EndPaint( hWnd, &ps );
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
