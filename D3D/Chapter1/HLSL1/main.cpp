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

ID3DXMesh * g_pTeapotMesh = nullptr;

ID3DXConstantTable * g_pConstantTable = nullptr;
IDirect3DVertexShader9 * g_pVertexShader = nullptr;

D3DXHANDLE g_hViewMatrix = nullptr;
D3DXHANDLE g_hViewProjMatrix = nullptr;
D3DXHANDLE g_hAmbientMateral = nullptr;
D3DXHANDLE g_hDiffuseMateral = nullptr;
D3DXHANDLE g_hLightDirection = nullptr;

D3DXMATRIX g_projectionMatrix;		//透视投影矩阵 


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
	HRESULT hr;
	hr = D3DXCreateTeapot(g_pDevice, &g_pTeapotMesh, nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建茶壶网格失败！"), NULL, MB_OK);
		return FALSE;
	}

	//编译文件
	ID3DXBuffer * pShaderBuf = nullptr;
	ID3DXBuffer * pErrorInfoBuf = nullptr;
	hr = D3DXCompileShaderFromFile(_T("diffuse.cpp"),
								   nullptr,
								   nullptr,
								   "Main",
								   "vs_1_1",
								   D3DXSHADER_DEBUG | D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,	//这里一定要加上这个参数，使之兼容全局变量可以在Main函数内可以修改！向后兼容
								   &pShaderBuf,
								   &pErrorInfoBuf,
								   &g_pConstantTable);
	if (pErrorInfoBuf)
	{
		MessageBoxA(NULL, (LPCSTR)pErrorInfoBuf->GetBufferPointer(), NULL, MB_OK);
		pErrorInfoBuf->Release();
	}
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("编译着色器文件失败！"), _T("Error"), MB_OK);
		return false;
	}

	//创建顶点着色器
	hr = g_pDevice->CreateVertexShader(static_cast<const DWORD*>(pShaderBuf->GetBufferPointer()),
									   &g_pVertexShader);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点着色器失败！"), NULL, MB_OK);
		return FALSE;
	}

	

	//获取常量表属性的句柄
	g_hViewMatrix = g_pConstantTable->GetConstantByName(nullptr, "ViewMatrix");
	g_hViewProjMatrix = g_pConstantTable->GetConstantByName(nullptr, "ViewProjMatrix");
	g_hAmbientMateral = g_pConstantTable->GetConstantByName(nullptr, "AmbientMtrl");
	g_hDiffuseMateral = g_pConstantTable->GetConstantByName(nullptr, "DiffuseMtrl");
	g_hLightDirection = g_pConstantTable->GetConstantByName(nullptr, "LightDirection");

	//设置常量表中的属性
	D3DXVECTOR4 lightDirection(-0.57f, 0.57f, -0.57f, 0.f);
	g_pConstantTable->SetVector(g_pDevice, g_hLightDirection, &lightDirection);

	//materal
	D3DXVECTOR4 ambientMtrl(0.f, 0.f, 1.f, 1.f);
	D3DXVECTOR4 diffuseMtrl(0.f, 0.f, 1.f, 1.f);

	g_pConstantTable->SetVector(g_pDevice, g_hAmbientMateral, &ambientMtrl);
	g_pConstantTable->SetVector(g_pDevice, g_hDiffuseMateral, &diffuseMtrl);

	g_pConstantTable->SetDefaults(g_pDevice);

	//取景
	//D3DXVECTOR3 pos(0.f, 0.f, -20.f);
	D3DXVECTOR3 pos(.0f, 0.0f, -5.0f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMATRIX v;
	D3DXMatrixLookAtLH(&v, &pos, &target, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &v);
	g_pConstantTable->SetMatrix(g_pDevice, g_hViewMatrix, &v);

	//投影
	/*D3DXMATRIX projection;
	D3DXMatrixPerspectiveFovLH(&projection,
							   D3DX_PI * 0.25f,
							   (float)g_cxWindow / (float)g_cyWindow,
							   1.f,
							   1000.f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &projection);
	g_pConstantTable->SetMatrix(g_pDevice, g_hViewProjMatrix, &(v * projection));*/

	D3DXMatrixPerspectiveFovLH(&g_projectionMatrix,
							   D3DX_PI * 0.25f,
							   (float)g_cxWindow / (float)g_cyWindow,
							   1.f,
							   1000.f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &g_projectionMatrix);

	return TRUE;
}

void Render()
{
	static float delta = 0.01;
	static float angle = 3.f * D3DX_PI / 2;
	static float height = .5f;

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		angle -= delta*.5f;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		angle += delta*.5f;
	if (GetAsyncKeyState(VK_UP) & 0x8000)
		height += delta;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		height -= delta;

	D3DXVECTOR3 pos(cos(angle) * 7, height, sin(angle) * 7);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMATRIX viewMatrix;
	D3DXMatrixLookAtLH(&viewMatrix, &pos, &target, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &viewMatrix);
	g_pConstantTable->SetMatrix(g_pDevice, g_hViewMatrix, &viewMatrix);
	g_pConstantTable->SetMatrix(g_pDevice, g_hViewProjMatrix, &(viewMatrix * g_projectionMatrix));
	

	g_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
	g_pDevice->BeginScene();
	g_pDevice->SetVertexShader(g_pVertexShader);
	g_pTeapotMesh->DrawSubset(0);
	g_pDevice->EndScene();
	g_pDevice->Present(0, 0, 0, 0);
}

void Cleanup()
{
	if (g_pDevice) g_pDevice->Release();
	if (g_pTeapotMesh) { g_pTeapotMesh->Release(); g_pTeapotMesh = nullptr; }
	if (g_pConstantTable) { g_pConstantTable->Release(); g_pConstantTable = nullptr; }
	if (g_pVertexShader){ g_pVertexShader->Release(); g_pVertexShader = nullptr; }
}
