#define  _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>
#include <D3DX10Math.h>
#include <memory.h>
#include <vector>
#include <cassert>

#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"D3dx9.lib")
#pragma comment(lib,"Winmm.lib")

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//函数声明
BOOL InitD3D(HINSTANCE hInstance);
BOOL Setup();
void Render();
void Cleanup();


//窗口相关变量
const TCHAR g_szClassName[] = _T("DirextClass");	//窗口类名
const TCHAR g_szWindowTitle[] = _T("Application");	//窗口名
const int g_xPos = 400;								//窗口显示x坐标
const int g_yPos = 200;								//窗口显示y坐标
const int g_cxWindow = 640;							//窗口长
const int g_cyWindow = 480;							//窗口宽

//DirectX相关变量
IDirect3DDevice9 *g_pDevice = nullptr;
IDirect3DVertexBuffer9 * g_pVertexBuffer = nullptr;
IDirect3DPixelShader9 * g_pPixShader = nullptr;

IDirect3DTexture9 * g_pCrateTexture = nullptr;
IDirect3DTexture9 * g_pSpotlightTexture = nullptr;
IDirect3DTexture9 * g_pStringTexture = nullptr;

ID3DXConstantTable * g_pConstantTable = nullptr;

//过滤器相关句柄
D3DXHANDLE g_hBaseSampler = nullptr;
D3DXHANDLE g_hSpotlightSampler = nullptr;
D3DXHANDLE g_hTextSampler = nullptr;
//过滤器相关描述
D3DXCONSTANT_DESC g_descBaseSampler;
D3DXCONSTANT_DESC g_descSpotlithSampler;
D3DXCONSTANT_DESC g_descTextSampler;

struct Vertex
{
	Vertex(float x, float y, float z,
	float u1, float v1,
	float u2, float v2,
	float u3, float v3)
	{
		_x = x; _y = y; _z = z;
		_u1 = u1; _v1 = v1;
		_u2 = u2; _v2 = v2;
		_u3 = u3; _v3 = v3;
	}
	float _x, _y, _z;
	float _u1, _v1;
	float _u2, _v2;
	float _u3, _v3;

	static const DWORD FVF;
};

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_TEX3;

int WINAPI _tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPreInstance,
					 LPTSTR lpCmdLine,
					 int nCmdShow)
{
	if (!InitD3D(hInstance))			//初始化D3D
	{
		MessageBox(NULL, _T("InitD3D Failed"), NULL, MB_OK);
		return FALSE;
	}

	MSG msg;
	while (true)						//开始消息循环
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
			Render();
		}
	}

	Cleanup();
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL InitD3D(HINSTANCE hInstance)
{
	WNDCLASS wndClass;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.hInstance = hInstance;
	wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = g_szClassName;
	wndClass.lpszMenuName = NULL;

	BOOL bRet;
	bRet = RegisterClass(&wndClass);
	if (!bRet)
	{
		MessageBox(NULL, _T("RegisterClass Failed"), NULL, MB_OK);
		return FALSE;
	}

	HWND hWnd;
	hWnd = CreateWindow(g_szClassName,
						g_szWindowTitle,
						WS_OVERLAPPEDWINDOW | WS_EX_TOPMOST,
						g_xPos,
						g_yPos,
						g_cxWindow,
						g_cyWindow,
						NULL,
						NULL,
						hInstance,
						NULL);
	if (!hWnd)
	{
		MessageBox(NULL, _T("CreateWindow Failed"), NULL, MB_OK);
		return FALSE;
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	//DirectX初始化 1 获取Direct3D9接口指针
	IDirect3D9 * pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D)
	{
		MessageBox(NULL, _T("Direct3DCreate9 Failed"), NULL, MB_OK);
		return FALSE;
	}

	//DirectX初始化 2 检查设备能力
	D3DCAPS9 caps;
	pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT,
						D3DDEVTYPE_HAL,
						&caps);
	int vp;		//顶点处理类型
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	//DirectX初始化 3 初始化D3DPRESENT_PARAMETERS结构体
	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth = g_cxWindow;
	d3dpp.BackBufferHeight = g_cyWindow;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality = 0;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.Windowed = true;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = /*D3DPRESENT_INTERVAL_DEFAULT*/D3DPRESENT_INTERVAL_IMMEDIATE;


	//DirectX初始化 4 创建IDirect3DDevice对象
	LRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT,
									D3DDEVTYPE_HAL,
									hWnd,
									vp,
									&d3dpp,
									&g_pDevice);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("CreateDevice Failed"), NULL, MB_OK);
		return FALSE;
	}
	pD3D->Release();

	//设置
	if (!Setup())
	{
		MessageBox(NULL, _T("Setup Failed"), NULL, MB_OK);
		return FALSE;
	}
	return TRUE;
}

BOOL Setup()
{
	HRESULT hr = 0;
	//创建顶点缓存
	hr = g_pDevice->CreateVertexBuffer(6 * sizeof(Vertex),
									   D3DUSAGE_WRITEONLY,
									   Vertex::FVF,
									   D3DPOOL_MANAGED,
									   &g_pVertexBuffer,
									   nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点还粗失败！"), NULL, MB_OK);
		return FALSE;
	}

	//初始化顶点数据
	Vertex * pVertex = nullptr;
	g_pVertexBuffer->Lock(0, 0, (void**)&pVertex, 0);
	pVertex[0] = Vertex(-10.0f, -10.0f, 5.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	pVertex[1] = Vertex(-10.0f, 10.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	pVertex[2] = Vertex(10.0f, 10.0f, 5.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);

	pVertex[3] = Vertex(-10.0f, -10.0f, 5.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	pVertex[4] = Vertex(10.0f, 10.0f, 5.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	pVertex[5] = Vertex(10.0f, -10.0f, 5.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	g_pVertexBuffer->Unlock();

	//编译像素着色器文件
	ID3DXBuffer * pPixelShaderBuf = nullptr;
	ID3DXBuffer * pErrorInfoBuf = nullptr;
	hr = D3DXCompileShaderFromFile(_T("multiTexture.cpp"),
								   nullptr,
								   nullptr,
								   "Main",
								   "ps_2_0",
								   D3DXSHADER_DEBUG | D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
								   &pPixelShaderBuf,
								   &pErrorInfoBuf,
								   &g_pConstantTable);
	if (pErrorInfoBuf)
	{
		MessageBoxA(NULL, static_cast<LPCSTR>(pErrorInfoBuf->GetBufferPointer()), NULL, MB_OK);
		pErrorInfoBuf->Release();
	}
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("编译像素着色器文件失败!"), NULL, MB_OK);
		return FALSE;
	}

	//创建像素着色器
	hr = g_pDevice->CreatePixelShader(static_cast<const DWORD*>(pPixelShaderBuf->GetBufferPointer()), &g_pPixShader);
	pPixelShaderBuf->Release();
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建像素着色器失败！"), NULL, MB_OK);
		return FALSE;
	}

	//创建纹理数据
	D3DXCreateTextureFromFile(g_pDevice, _T("crate.bmp"), &g_pCrateTexture);
	D3DXCreateTextureFromFile(g_pDevice, _T("spotlight.bmp"), &g_pSpotlightTexture);
	D3DXCreateTextureFromFile(g_pDevice, _T("text.bmp"), &g_pStringTexture);

	g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);	//关闭光照
	
	g_hBaseSampler = g_pConstantTable->GetConstantByName(nullptr, "baseTex");
	g_hSpotlightSampler = g_pConstantTable->GetConstantByName(nullptr, "spotLightTex");
	g_hTextSampler = g_pConstantTable->GetConstantByName(nullptr, "stringTex");

	UINT count = 0;
	g_pConstantTable->GetConstantDesc(g_hBaseSampler, &g_descBaseSampler, &count);
	g_pConstantTable->GetConstantDesc(g_hSpotlightSampler, &g_descSpotlithSampler, &count);
	g_pConstantTable->GetConstantDesc(g_hTextSampler, &g_descTextSampler, &count);

	g_pConstantTable->SetDefaults(g_pDevice);

	//取景
	//D3DXVECTOR3 pos(0.f, 0.f, -20.f);
	D3DXVECTOR3 pos(0.0f, 0.0f, -5.0f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMATRIX v;
	D3DXMatrixLookAtLH(&v, &pos, &target, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &v);

	//投影
	D3DXMATRIX projection;
	D3DXMatrixPerspectiveFovLH(&projection,
							   D3DX_PI * 0.5f,
							   (float)g_cxWindow / (float)g_cyWindow,
							   1.f,
							   1000.f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &projection);

	return TRUE;
}

void Render()
{
	static float delta = 0.01f;
	static float angle = D3DX_PI;
	static float height = 0.f;

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		angle -= delta;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		angle += delta;
	if (GetAsyncKeyState(VK_UP) & 0x8000)
		height += delta;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		height -= delta;

	D3DXVECTOR3 pos(sin(angle) * 7.f, height, cos(angle) * 7.f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMATRIX viewMatrix;
	D3DXMatrixLookAtLH(&viewMatrix, &pos, &target, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &viewMatrix);

	HRESULT hr;
	g_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
	g_pDevice->BeginScene();

	g_pDevice->SetPixelShader(g_pPixShader);	//设置像素着色器
	g_pDevice->SetFVF(Vertex::FVF);
	g_pDevice->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));

	//baseSampler
	g_pDevice->SetTexture(g_descBaseSampler.RegisterIndex, g_pCrateTexture);
	g_pDevice->SetSamplerState(g_descBaseSampler.RegisterIndex, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(g_descBaseSampler.RegisterIndex, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(g_descBaseSampler.RegisterCount, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	//spotlightSampler
	g_pDevice->SetTexture(g_descSpotlithSampler.RegisterIndex, g_pSpotlightTexture);
	g_pDevice->SetSamplerState(g_descSpotlithSampler.RegisterIndex, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(g_descSpotlithSampler.RegisterIndex, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(g_descSpotlithSampler.RegisterCount, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	//textSampler
	g_pDevice->SetTexture(g_descTextSampler.RegisterIndex, g_pSpotlightTexture);
	g_pDevice->SetSamplerState(g_descTextSampler.RegisterIndex, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(g_descTextSampler.RegisterIndex, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(g_descTextSampler.RegisterCount, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

	g_pDevice->EndScene();
	g_pDevice->Present(0, 0, 0, 0);
}

void Cleanup()
{
	if (g_pDevice) { g_pDevice->Release(); g_pDevice = nullptr; }
	if (g_pPixShader) { g_pPixShader->Release(); g_pPixShader = nullptr; }
	if (g_pConstantTable){ g_pConstantTable->Release(); g_pConstantTable = nullptr; }
	if (g_pCrateTexture){ g_pCrateTexture->Release(); g_pCrateTexture = nullptr; }
	if (g_pSpotlightTexture){ g_pSpotlightTexture->Release(); g_pSpotlightTexture = nullptr; }
	if (g_pStringTexture){ g_pStringTexture->Release(); g_pStringTexture = nullptr; }
	if (g_pVertexBuffer){ g_pVertexBuffer->Release(); g_pVertexBuffer = nullptr; }
}
