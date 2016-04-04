#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>
#include <windows.h>

#pragma comment(lib,"winmm.lib")

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Render();

BOOL Startup();
void Cleanup();

IDirect3DDevice9 *pDevice = nullptr;
IDirect3DVertexBuffer9 * VB = nullptr;		//顶点缓存

int WINAPI _tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPreInstance,
					 LPTSTR lpCmdLine,
					 int nShowCmd)
{
	TCHAR szClassName[] = _T("myClass");
	WNDCLASS wndClass;

	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.hInstance = hInstance;
	wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = szClassName;
	wndClass.lpszMenuName = NULL;

	BOOL bRet;
	bRet = RegisterClass(&wndClass);
	if (!bRet)
	{
		MessageBox(NULL, _T("Register window class fail"), NULL, MB_OK);
		return FALSE;
	}

	HWND  hWnd;
	hWnd = CreateWindow(szClassName,
						_T("Application"),
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						600,
						480,
						NULL,
						NULL,
						hInstance,
						NULL);
	if (!hWnd)
	{
		MessageBox(NULL, _T("Create Window FAil"), NULL, MB_OK);
		return FALSE;
	}

	ShowWindow(hWnd, nShowCmd);
	UpdateWindow(hWnd);


	IDirect3D9 *d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	D3DCAPS9 caps;
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT,
						D3DDEVTYPE_HAL,
						&caps);
	int type;
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		type = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		type = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	D3DPRESENT_PARAMETERS pp;
	pp.BackBufferWidth = 600;
	pp.BackBufferHeight = 480;
	pp.BackBufferFormat = D3DFMT_A8R8G8B8;
	pp.BackBufferCount = 1;
	pp.MultiSampleType = D3DMULTISAMPLE_NONE;
	pp.MultiSampleQuality = 0;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.hDeviceWindow = hWnd;
	pp.Windowed = true;
	pp.EnableAutoDepthStencil = true;
	pp.AutoDepthStencilFormat = D3DFMT_D24S8;
	pp.Flags = 0;
	pp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

	HRESULT hr;
	hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT,
							D3DDEVTYPE_HAL,
							hWnd,
							type,
							&pp,
							&pDevice);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("CreateDevice Failed"), NULL, MB_OK);
		return FALSE;
	}
	d3d9->Release();

	if (!Startup())
	{
		MessageBox(NULL, _T("Startup Failed"), NULL, MB_OK);
		return FALSE;
	}

	MSG msg;
	memset(&msg, 0, sizeof(msg));
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
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

struct Vertex		//1 定义顶点结构
{
	Vertex(float x, float y, float z)
	{
		_x = x;
		_y = y;
		_z = z;
	}
	float _x, _y, _z;
	static DWORD FVF;
};

DWORD Vertex::FVF = D3DFVF_XYZ;

BOOL Startup()		//初始化
{
	HRESULT hr;
	hr = pDevice->CreateVertexBuffer(3 * sizeof(Vertex),				// 2 创建顶点缓存
									 0,
									 Vertex::FVF,
									 D3DPOOL_MANAGED,
									 &VB,
									 NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("Create Vertex Buffer Failed!"), NULL, MB_OK);
		return FALSE;
	}

	Vertex * pVertexs;
	VB->Lock(0,
			 0,
			 reinterpret_cast<void**>(&pVertexs),
			 0);

	//初始化顶点数据
	//1 此处将z坐标的值设置为0.5时就可以正常显示，而设置其他值而不显示，当然这是在没有设置投影的情况，即没有下面的代码
	//2 这里Z值设置为0.5，后面也进行投影，也没有结果，为什么？

	pVertexs[0] = Vertex(-1, 0, 0.5);		//z坐标相同，在同一平面
	pVertexs[1] = Vertex(0, 1, 0.5);
	pVertexs[2] = Vertex(1, 0, 0.5);
	VB->Unlock();
	
	D3DXVECTOR3 pos(0.f, 0.f, -3.f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 1.f);
	D3DXMATRIX matrixView;
	D3DXMatrixLookAtLH(&matrixView, &pos, &target, &up);
	pDevice->SetTransform(D3DTS_VIEW, &matrixView);

	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(&proj,
							   D3DX_PI * 0.5f,
							   (float)600 / (float)480,
							   1.0f,
							   1000.f);
	pDevice->SetTransform(D3DTS_PROJECTION, &proj);

	//pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);		// 3 设置绘制状态

	return TRUE;
}

void Cleanup()
{
	if (pDevice) pDevice->Release();
	if (VB) VB->Release();
}


void Render()
{
	float angle = timeGetTime() % 10000 * 1.f / 10000 * 2 * D3DX_PI ;
	int r = 3;
	D3DXVECTOR3 pos(cos(angle) * r, 0.f, sin(angle) * r);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMATRIX matrixView;
	D3DXMatrixLookAtLH(&matrixView, &pos, &target, &up);
	pDevice->SetTransform(D3DTS_VIEW, &matrixView);

	pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 220, 0, 255), 1.0f, 0);
	pDevice->SetStreamSource(0, VB, 0, sizeof(Vertex));	//4 将顶点缓存与数据流进行链接，
	pDevice->SetFVF(Vertex::FVF);						//5 设置顶点格式
			
	pDevice->BeginScene();
	pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);	//6 绘制
	pDevice->EndScene();
	pDevice->Present(NULL, NULL, NULL, NULL);
}
