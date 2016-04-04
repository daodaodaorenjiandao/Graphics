#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Render();		//绘制

IDirect3DDevice9 *pDevice = nullptr;


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

	
	IDirect3D9 *d3d9 = Direct3DCreate9(D3D_SDK_VERSION);		// 1 创建IDirect3D9对象

	D3DCAPS9 caps;												// 2 检查设备能力
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT,
						D3DDEVTYPE_HAL,
						&caps);
	int type;
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		type = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		type = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		
	D3DPRESENT_PARAMETERS pp;									// 3 初始化D3DPRESENT_PARAMETERS结构体参数
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
	hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT,					//4 创建IDirect3DDevice9设备对象
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
			Render();											//5 绘制
		}
	}
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

void Render()
{
	pDevice->Clear(0,
				   NULL,
				   D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
				   D3DCOLOR_ARGB(0, 220, 220, 255),		//背景颜色
				   1.0f, 
				   0);
	pDevice->Present(NULL, NULL, NULL, NULL);
}