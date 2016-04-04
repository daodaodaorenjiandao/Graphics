#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>

#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"D3DX10.lib")
#pragma comment(lib,"d3dx9.lib")

//窗口相关变量
const TCHAR g_szClassName[] = _T("DirextClass");	//窗口类名
const TCHAR g_szWindowTitle[] = _T("Application");	//窗口名
const int g_xPos = 400;								//窗口显示x坐标
const int g_yPos = 200;								//窗口显示y坐标
const int g_cxWindow = 600;							//窗口长
const int g_cyWindow = 480;							//窗口宽

//DirectX相关变量
IDirect3DDevice9 * g_pDevice;
IDirect3DVertexBuffer9 * g_pVertexBuffer;
ID3DXMesh* g_pMesh = nullptr;

//函数声明
BOOL InitD3D(HINSTANCE hInstance);
BOOL Setup();
void Render();
void Cleanup();

struct Vertex{		//顶点颜色的使用 1 定义顶点结构
	Vertex(float x, float y, float z,D3DCOLOR color)
	{
		_x = x;
		_y = y;
		_z = z;
		_color = color;		
	}
	float _x, _y, _z;
	D3DCOLOR _color;		//这里主要添加了颜色这一成员
	static const DWORD FVF;
};

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;	//使用的灵活顶点结构要加上 D3DFVF_DIFFUSE这一标记

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

	//取景变换
	D3DXVECTOR3 pos(0.f, 3.f, -5.f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMATRIX matrixView;
	D3DXMatrixLookAtLH(&matrixView, &pos, &target, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &matrixView);

	//设置投影变换
	D3DXMATRIX matrixPerspective;
	D3DXMatrixPerspectiveFovLH(&matrixPerspective,
							   D3DX_PI / 4,
							   (float)g_cxWindow / g_cyWindow,
							   1.f,
							   100.f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &matrixPerspective);
	return TRUE;
}

BOOL Setup()
{
	HRESULT hr;
	if (g_pDevice)
	{
		g_pDevice->CreateVertexBuffer(sizeof(Vertex)* 3,	//这里使用创建3个顶点的缓存，用于画一个三角形
									  0,
									  Vertex::FVF,
									  D3DPOOL_MANAGED,
									  &g_pVertexBuffer,
									  NULL);
		Vertex * pVertexs;
		g_pVertexBuffer->Lock(0, 0, (void**)&pVertexs, 0);
		pVertexs[0] = Vertex(0.f, 0.f, .5f, D3DCOLOR_ARGB(255, 255, 0, 0));
		pVertexs[1] = Vertex(.5f, .5f, .5f, D3DCOLOR_ARGB(255, 0, 0, 255));
		pVertexs[2] = Vertex(1.f, 0.f, .5f, D3DCOLOR_ARGB(255, 0, 255, 0));
		g_pVertexBuffer->Unlock();

		
		//图元的最终颜色是图元本身的颜色*光照强度  
		// 1 禁用光照后图元输出的是本身颜色没有光照强度的输出
		// 2 没有禁用光照，但是灯光和图元的法向量没有计算正确，就会算出错误的结果 比如说你的黑色应该就是没有定义灯光或者图元的法线有问题
		g_pDevice->SetRenderState(D3DRS_LIGHTING, false);	//关闭光照？
		g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		hr = D3DXCreatePolygon(g_pDevice, 0.5, 4, &g_pMesh, nullptr);
		//hr = D3DXCreateTeapot(g_pDevice, &g_pMesh, nullptr);
		if (FAILED(hr))
		{
			MessageBoxA(NULL, "创建网格失败", NULL, MB_OK);
			return false;
		}

	}
	return TRUE;
}

void Render()
{
	g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(255, 120,110, 110), 1, 0);
	g_pDevice->SetStreamSource(0, g_pVertexBuffer,0, sizeof(Vertex));
	g_pDevice->SetFVF(Vertex::FVF);
	
	g_pDevice->BeginScene();
	
	D3DXMATRIX martrix;
	D3DXMatrixTranslation(&martrix, -1, 0, 0);	//左平移一个单位
	g_pDevice->SetTransform(D3DTS_WORLD, &martrix);
	g_pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);	//模式1
	g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);		//绘制一个三角形

	D3DXMatrixTranslation(&martrix, 0, 0, 0);	//使用原来默认的位置
	g_pDevice->SetTransform(D3DTS_WORLD, &martrix);
	g_pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);	//模式2
	g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);		//绘制一个三角形

	g_pDevice->SetFVF(g_pMesh->GetFVF());
	g_pMesh->DrawSubset(0);
	
	g_pDevice->EndScene();

	g_pDevice->Present(NULL, NULL, NULL, NULL);
	
}

void Cleanup()
{
	if (g_pDevice) g_pDevice->Release();
	if (g_pMesh)
	{
		g_pMesh->Release();
		g_pMesh = nullptr;
	}

}