#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>

#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"D3DX10.lib")

//窗口相关变量
const TCHAR g_szClassName[] = _T("DirextClass");	//窗口类名
const TCHAR g_szWindowTitle[] = _T("Application");	//窗口名
const int g_xPos = 400;								//窗口显示x坐标
const int g_yPos = 200;								//窗口显示y坐标
const int g_cxWindow = 600;							//窗口长
const int g_cyWindow = 480;							//窗口宽


//DirectX相关变量
IDirect3DDevice9 * g_pDevice;
IDirect3DVertexBuffer9 * g_pVertexBuffer;	//顶点缓存
IDirect3DIndexBuffer9  * g_pIndexBuffer;	//索引缓存


//函数声明
BOOL InitD3D(HINSTANCE hInstance);
BOOL Setup();
void Render();
void Cleanup();

struct Vertex{				//绘制 1 定义顶点结构
	Vertex(float x, float y, float z)
	{
		_x = x;
		_y = y;
		_z = z;
	}
	float _x;
	float _y;
	float _z;
	static const DWORD FVF;	
};

const DWORD Vertex::FVF = D3DFVF_XYZ;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
#include <time.h>
BOOL Setup()
{
	HRESULT hr;
	hr = g_pDevice->CreateVertexBuffer(sizeof(Vertex)* 30,	//绘制 2创建顶点缓存
									   0,
									   Vertex::FVF,
									   D3DPOOL_MANAGED,
									   &g_pVertexBuffer,
									   NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("Create Vertex Buffer Failed!"), NULL, MB_OK);
		return FALSE;
	}

	Vertex * pVertexs;
	g_pVertexBuffer->Lock(0, 0, (void**)&pVertexs, 0);

	srand(time(NULL));
	for (int i = 0; i < 30; ++i)	//设置顶点数据
	{
		/*pVertexs[i]._x = i * 0.03f;
		pVertexs[i]._y = (i % 2 ? 0.f : 0.03f );*/
		pVertexs[i]._x = rand() * 0.1 / 32767 + i * 0.1;
		pVertexs[i]._y = rand() * 0.1 / 32767;
		pVertexs[i]._z = 0.5f;		//这里设置Z值为0.5就可以不用进行投影，为什么？
	}

	g_pVertexBuffer->Unlock();

	hr  = g_pDevice->CreateIndexBuffer(sizeof(DWORD)* 30,		//--创建索引缓存
									   0,
									   D3DFMT_INDEX32,
									   D3DPOOL_MANAGED,
									   &g_pIndexBuffer,
									   NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("Create Index Buffer Failed!"), NULL, MB_OK);
		return FALSE;
	}
	DWORD *pIndecs;
	g_pIndexBuffer->Lock(0, 0, (void**)&pIndecs, 0);
	for (int i = 0; i < 3; ++i)
		pIndecs[i] = i;
	g_pIndexBuffer->Unlock();

/*
	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(&proj,
							   D3DX_PI * 0.5f,
							   (float)600 / (float)480,
							   1.0f,
							   1000.f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &proj); */

	g_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);	//绘制 3 设置绘制状态

	return TRUE;
}

void Render()
{
	if (g_pDevice)
	{
		g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 220, 220, 255), 1, 0);
		
		g_pDevice->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));	//绘制 4 将顶点缓存与数据流进行链接
		g_pDevice->SetFVF(Vertex::FVF);										//绘制 5 设置顶点格式和索引缓存
		g_pDevice->SetIndices(g_pIndexBuffer);
		g_pDevice->BeginScene();
		g_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,					//绘制 6 进行绘制
										0,
										0,
										30,
										0,
										10);
		//g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 10);
		
		g_pDevice->EndScene();
		g_pDevice->Present(NULL, NULL, NULL, NULL);
	}
}

void Cleanup()
{
	if (g_pDevice) g_pDevice->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
}