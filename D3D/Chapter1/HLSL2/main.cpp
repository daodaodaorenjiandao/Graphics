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
IDirect3DVertexShader9 * g_pVertexShader = nullptr;
IDirect3DTexture9 * g_pTexture = nullptr;

ID3DXMesh* g_pMeshs[4] = { 0 };
D3DXMATRIX g_matrixs[4];

ID3DXConstantTable * g_pConstantTable = nullptr;
//variable handle
D3DXHANDLE g_hViewMatrix = nullptr;
D3DXHANDLE g_hProjViewMatrix = nullptr;
D3DXHANDLE g_hLightDirection = nullptr;
D3DXHANDLE g_hColor = nullptr;

D3DXVECTOR4 g_colors[4];

D3DXMATRIX g_viewMatrix;
D3DXMATRIX g_projectionMatrix;

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
	//创建网格数据
	D3DXCreateTeapot(g_pDevice, g_pMeshs, nullptr);
	D3DXCreateSphere(g_pDevice, 1.f, 20, 20, g_pMeshs + 1, nullptr);
	D3DXCreateTorus(g_pDevice, 0.5f, 1.f, 20, 20, g_pMeshs + 2, nullptr);
	D3DXCreateCylinder(g_pDevice, 0.5f, 0.5f, 2.f, 20, 20, g_pMeshs + 3, nullptr);

	hr = D3DXCreateTextureFromFile(g_pDevice,
								   _T("toonshade.bmp"),
								   &g_pTexture);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建纹理失败！"), NULL, MB_OK);
		return FALSE;
	}
	g_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	g_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	g_pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	//初始化世界变换矩阵
	D3DXMatrixTranslation(g_matrixs, 0.f, 2.f, 0.f);
	D3DXMatrixTranslation(g_matrixs + 1, 0.f, -2.f, 0.f);
	D3DXMatrixTranslation(g_matrixs + 2, -3.f, 0.f, 0.f);
	D3DXMatrixTranslation(g_matrixs + 3, 3.f, 0.f, 0.f);

	//初始化颜色向量
	g_colors[0] = D3DXVECTOR4(1.f, 0.f, 0.f, 1.f);
	g_colors[1] = D3DXVECTOR4(0.f, 1.f, 0.f, 1.f);
	g_colors[2] = D3DXVECTOR4(0.f, 0.f, 1.f, 1.f);
	g_colors[3] = D3DXVECTOR4(1.f, 1.f, 0.f, 1.f);

	//编译顶点着色器文件
	ID3DXBuffer * pShaderBuf = nullptr;
	ID3DXBuffer * pErrorInfoBuf = nullptr;

	D3DXCompileShaderFromFile(_T("vertexShader.txt"),
							  nullptr,
							  nullptr,
							  "Main",
							  "vs_1_1",
							  D3DXSHADER_DEBUG | D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
							  &pShaderBuf,
							  &pErrorInfoBuf,
							  &g_pConstantTable);
	if (pErrorInfoBuf)
	{
		MessageBoxA(NULL, static_cast<LPCSTR>(pErrorInfoBuf->GetBufferPointer()), NULL, MB_OK);
		pErrorInfoBuf->Release();
	}
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("编译着色器文件失败！"), NULL, MB_OK); 
		return FALSE;
	}
	

	//创建顶点着色器
	hr = g_pDevice->CreateVertexShader(static_cast<const DWORD*>(pShaderBuf->GetBufferPointer()), &g_pVertexShader);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点着色器失败！"), NULL, MB_OK);
		return FALSE;
	}
	pShaderBuf->Release();

	//获取常量表
	g_hProjViewMatrix = g_pConstantTable->GetConstantByName(nullptr, "ViewProjMatrix");
	g_hViewMatrix = g_pConstantTable->GetConstantByName(nullptr, "ViewMatrix");
	g_hLightDirection = g_pConstantTable->GetConstantByName(nullptr, "lightDirection");
	g_hColor = g_pConstantTable->GetConstantByName(nullptr, "Color");

	//取景
	//D3DXVECTOR3 pos(0.f, 0.f, -20.f);
	D3DXVECTOR3 pos(0.0f, 0.0f, -5.0f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMATRIX v;
	D3DXMatrixLookAtLH(&v, &pos, &target, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &v);
	g_viewMatrix = v;

	//投影
	D3DXMATRIX projection;
	D3DXMatrixPerspectiveFovLH(&projection,
							   D3DX_PI * 0.5f,
							   (float)g_cxWindow / (float)g_cyWindow,
							   1.f,
							   1000.f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &projection);
	g_projectionMatrix = projection;

	//设置常量表属性
	D3DXVECTOR4 vecLightDirection(-0.57f, 0.57f, -0.57f, 0.f);
	g_pConstantTable->SetVector(g_pDevice, g_hLightDirection, &vecLightDirection);
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
	g_viewMatrix = viewMatrix;

	HRESULT hr;
	g_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
	g_pDevice->BeginScene();
	g_pDevice->SetVertexShader(g_pVertexShader);
	g_pDevice->SetTexture(0, g_pTexture);

	for (int i = 0; i < sizeof(g_pMeshs) / sizeof(g_pMeshs[0]); ++i)
	{
		//g_pDevice->SetTransform(D3DTS_WORLD, &g_matrixs[i]);
		g_pConstantTable->SetMatrix(g_pDevice, g_hViewMatrix, &(g_matrixs[i] * g_viewMatrix));
		g_pConstantTable->SetMatrix(g_pDevice, g_hProjViewMatrix, &(g_matrixs[i] * g_viewMatrix * g_projectionMatrix));
		g_pConstantTable->SetVector(g_pDevice, g_hColor, &g_colors[i]);
		g_pMeshs[i]->DrawSubset(0);
	}

	g_pDevice->EndScene();
	g_pDevice->Present(0, 0, 0, 0);
}

void Cleanup()
{
	if (g_pDevice) { g_pDevice->Release(); g_pDevice = nullptr; }
	for (int i = 0; i < sizeof(g_pMeshs) / sizeof(g_pMeshs[0]); ++i)
	{
		g_pMeshs[i]->Release();
		g_pMeshs[i] = nullptr;
	}
	if (g_pConstantTable)
	{
		g_pConstantTable->Release();
		g_pConstantTable = nullptr;
	}

	if (g_pVertexShader)
	{
		g_pVertexShader->Release();
		g_pVertexShader = nullptr;
	}
	if (g_pTexture)
	{
		g_pTexture->Release();
		g_pTexture = nullptr;
	}
}
