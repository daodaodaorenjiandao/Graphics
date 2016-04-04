#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>
#include <D3DX10Math.h>
#include <memory.h>

#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"D3DX10.lib")
#pragma comment(lib,"D3dx9.lib")

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
const int g_cxWindow = 600;							//窗口长
const int g_cyWindow = 480;							//窗口宽

//DirectX相关变量
IDirect3DDevice9 *g_pDevice = nullptr;
ID3DXMesh * g_pMesh = nullptr;

struct Vertex
{
	Vertex(){}
	Vertex(float x, float y, float z)
	{
		_x = x;  _y = y;  _z = z;

	}
	float _x, _y, _z;

	static const DWORD FVF;
};
const DWORD Vertex::FVF = D3DFVF_XYZ;


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
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

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
	HDC hdc;
	LOGFONT lf;
	HFONT hFont;
	HFONT hOldFont;
	hdc = CreateCompatibleDC(NULL);
	

	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = 25;
	lf.lfWidth = 12;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = 0;
	lf.lfItalic = false;
	lf.lfUnderline = false;
	lf.lfStrikeOut = false;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = 0;
	lf.lfClipPrecision = 0;
	lf.lfQuality = 0;
	lf.lfPitchAndFamily = 0;
	_tcscpy(lf.lfFaceName, _T("Times New Roman"));

	hFont = CreateFontIndirect(&lf);
	hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

	hr = D3DXCreateText(g_pDevice, hdc,_T("hello world!!!"),0.001,0.4f,&g_pMesh,NULL,NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("D3DXCreateText Failed!!!"), NULL, MB_OK);
		return false;
	}
	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
	DeleteDC(hdc);

	D3DXVECTOR3 pos(0.0f, 1.5f, -3.3f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMATRIX V;
	D3DXMatrixLookAtLH(&V, &pos, &target, &up);

	g_pDevice->SetTransform(D3DTS_VIEW, &V);

	//
	// Set projection matrix.
	//

	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
		&proj,
		D3DX_PI * 0.5f, // 90 - degree
		(float)g_cxWindow / (float)g_cyWindow,
		1.0f,
		1000.0f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &proj);

	return TRUE;
}

void Render()
{
	g_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xffffffff, 1.0f, 0);
	g_pDevice->BeginScene();

	g_pMesh->DrawSubset(0);

	g_pDevice->EndScene();
	g_pDevice->Present(0, 0, 0, 0);
}

void Cleanup()
{
	if (g_pDevice) g_pDevice->Release();
}