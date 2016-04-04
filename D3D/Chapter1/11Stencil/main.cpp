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
void RenderMirror();

//窗口相关变量
const TCHAR g_szClassName[] = _T("DirextClass");	//窗口类名
const TCHAR g_szWindowTitle[] = _T("Application");	//窗口名
const int g_xPos = 400;								//窗口显示x坐标
const int g_yPos = 200;								//窗口显示y坐标
const int g_cxWindow = 600;							//窗口长
const int g_cyWindow = 480;							//窗口宽

//DirectX相关变量
IDirect3DDevice9 *g_pDevice = nullptr;
IDirect3DVertexBuffer9 * g_pVertexBuffer = nullptr;	

IDirect3DTexture9 * g_pTextureFloor = nullptr;
IDirect3DTexture9 * g_pTextureWall = nullptr;
IDirect3DTexture9 * g_pTextureMirror = nullptr;

D3DMATERIAL9 g_materialFloor;
D3DMATERIAL9 g_materialWall;
D3DMATERIAL9 g_materialMirror;

ID3DXMesh* g_pMeshTeapot = 0;
D3DXVECTOR3 g_teapotPosition(0.0f, 3.0f, -7.5f);
D3DMATERIAL9 g_materalTeapot;


struct Vertex
{
	Vertex(){}
	Vertex(float x, float y, float z,
		   float nx,float ny,float nz,
		   float u,float v)
	{
		_x = x;  _y = y;  _z = z;
		_nx = nx; _ny = ny; _nz = nz;
		_u = u; _v = v;
	}
	float _x, _y, _z;
	float _nx, _ny, _nz;
	float _u, _v;
	static const DWORD FVF;
};
const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;


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
	hr = g_pDevice->CreateVertexBuffer(sizeof(Vertex)* 24,
									   0,
									   Vertex::FVF,
									   D3DPOOL_MANAGED,
									   &g_pVertexBuffer,
									   NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点缓存失败！"), NULL, MB_OK);
		return FALSE;
	}

	Vertex * pVertexs;
	g_pVertexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pVertexs), 0);

	// floor
	pVertexs[0] = Vertex(-7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	pVertexs[1] = Vertex(-7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	pVertexs[2] = Vertex(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	
	pVertexs[3] = Vertex(-7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	pVertexs[4] = Vertex(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	pVertexs[5] = Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
	
	// wall
	pVertexs[6] = Vertex(-7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	pVertexs[7] = Vertex(-7.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	pVertexs[8] = Vertex(-2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	
	pVertexs[9] = Vertex(-7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	pVertexs[10] = Vertex(-2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	pVertexs[11] = Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Note: We leave gap in middle of walls for mirror
		
	pVertexs[12] = Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	pVertexs[13] = Vertex(2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	pVertexs[14] = Vertex(7.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	
	pVertexs[15] = Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	pVertexs[16] = Vertex(7.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	pVertexs[17] = Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
	
	// mirror
	pVertexs[18] = Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	pVertexs[19] = Vertex(-2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	pVertexs[20] = Vertex(2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	
	pVertexs[21] = Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	pVertexs[22] = Vertex(2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	pVertexs[23] = Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	g_pVertexBuffer->Unlock();

	g_materialWall.Ambient = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255));
	g_materialWall.Diffuse = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255));
	g_materialWall.Specular = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255));
	g_materialWall.Emissive = D3DXCOLOR(D3DCOLOR_XRGB(0, 0, 0));
	g_materialWall.Power = 2.f;

	g_materialFloor = g_materialWall;
	g_materialMirror = g_materialWall;

	g_materialWall.Specular = 0.2f *D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255));

	
	g_materalTeapot.Ambient = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 0));
	g_materalTeapot.Diffuse = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 0));
	g_materalTeapot.Specular = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 0));
	g_materalTeapot.Emissive = D3DXCOLOR(D3DCOLOR_XRGB(0, 0, 0));
	g_materalTeapot.Power = 2.f;

	//创建纹理
	D3DXCreateTextureFromFile(g_pDevice,
							  _T("brick0.jpg"),
							  &g_pTextureWall);
	D3DXCreateTextureFromFile(g_pDevice,
							  _T("checker.jpg"),
							  &g_pTextureFloor);
	D3DXCreateTextureFromFile(g_pDevice,
							  _T("ice.bmp"),
							  &g_pTextureMirror);
	g_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	
	D3DXCreateTeapot(g_pDevice, &g_pMeshTeapot, 0);

	//设置方向光
	D3DLIGHT9 light;
	D3DXCOLOR lightColor(1.0f, 1.0f, 1.0f,1.0f);
	memset(&light, 0, sizeof(light));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Ambient = lightColor* 0.4f;
	light.Diffuse = lightColor;
	light.Specular = lightColor * 0.6f;
	light.Direction = D3DXVECTOR3(0.707f, -0.707f, 0.707);

	g_pDevice->SetLight(0, &light);
	g_pDevice->LightEnable(0, TRUE);

	g_pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
	g_pDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);

	//设置摄像机
	D3DXVECTOR3    pos(-10.0f, 3.0f, -15.0f);
	D3DXVECTOR3 target(0.0, 0.0f, 0.0f);
	D3DXVECTOR3     up(0.0f, 1.0f, 0.0f);

	D3DXMATRIX V;
	D3DXMatrixLookAtLH(&V, &pos, &target, &up);

	g_pDevice->SetTransform(D3DTS_VIEW, &V);

	//
	// Set projection matrix.
	//
	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
		&proj,
		D3DX_PI / 4.0f, // 45 - degree
		(float)g_cxWindow / (float)g_cyWindow,
		1.0f,
		1000.0f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &proj);
	
	return TRUE;
}

void Render()
{
	static float radius = 20.0f;
	float timeDelta = 0.1001;

	if (::GetAsyncKeyState(VK_LEFT) & 0x8000f)
		g_teapotPosition.x -= 3.0f * timeDelta;

	if (::GetAsyncKeyState(VK_RIGHT) & 0x8000f)
		g_teapotPosition.x += 3.0f * timeDelta;

	if (::GetAsyncKeyState(VK_UP) & 0x8000f)
	//	radius -= 2.0f * timeDelta;
		g_teapotPosition.z += 3.0f * timeDelta;

	if (::GetAsyncKeyState(VK_DOWN) & 0x8000f)
//		radius += 2.0f * timeDelta;
g_teapotPosition.z -= 3.0f * timeDelta;


	static float angle = (3.0f * D3DX_PI) / 2.0f;

	if (::GetAsyncKeyState('A') & 0x8000f)
		angle -= 0.5f * timeDelta;

	if (::GetAsyncKeyState('D') & 0x8000f)
		angle += 0.5f * timeDelta;

	D3DXVECTOR3 position(cosf(angle) * radius, 3.0f, sinf(angle) * radius);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMATRIX V;
	D3DXMatrixLookAtLH(&V, &position, &target, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &V);

	g_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xff000000, 1.0f, 0);
	g_pDevice->BeginScene();

	g_pDevice->SetMaterial(&g_materalTeapot);
	g_pDevice->SetTexture(0, 0);
	D3DXMATRIX W;
	D3DXMatrixTranslation(&W,
						  g_teapotPosition.x,
						  g_teapotPosition.y,
						  g_teapotPosition.z);

	g_pDevice->SetTransform(D3DTS_WORLD, &W);
	g_pMeshTeapot->DrawSubset(0);

	D3DXMATRIX I;
	D3DXMatrixIdentity(&I);
	g_pDevice->SetTransform(D3DTS_WORLD, &I);
	
	g_pDevice->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
	g_pDevice->SetFVF(Vertex::FVF);

	g_pDevice->SetTexture(0, g_pTextureFloor);
	g_pDevice->SetMaterial(&g_materialFloor);
	g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

	g_pDevice->SetTexture(0, g_pTextureWall);
	g_pDevice->SetMaterial(&g_materialWall);
	g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 6, 4);

	g_pDevice->SetTexture(0, g_pTextureMirror);
	g_pDevice->SetMaterial(&g_materialMirror);
	g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 18, 2);

	RenderMirror();
	g_pDevice->EndScene();
	g_pDevice->Present(0, 0, 0, 0);
}

void Cleanup()
{
	if (g_pDevice) g_pDevice->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pTextureWall) g_pTextureWall->Release();
	if (g_pTextureFloor) g_pTextureFloor->Release();
	if (g_pTextureMirror) g_pTextureMirror->Release();

}

void RenderMirror()
{
	g_pDevice->SetRenderState(D3DRS_STENCILENABLE, true);
	g_pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	g_pDevice->SetRenderState(D3DRS_STENCILREF, 0x1);
	g_pDevice->SetRenderState(D3DRS_STENCILMASK, 0xffffffff);
	g_pDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);
	g_pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
	g_pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	g_pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);

	// disable writes to the depth and back buffers
	g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
	g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
	g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	// draw the mirror to the stencil buffer
	g_pDevice->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
	g_pDevice->SetFVF(Vertex::FVF);
	g_pDevice->SetMaterial(&g_materialMirror);
	g_pDevice->SetTexture(0, g_pTextureMirror);
	D3DXMATRIX I;
	D3DXMatrixIdentity(&I);
	g_pDevice->SetTransform(D3DTS_WORLD, &I);
	g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 18, 2);

	// re-enable depth writes
	g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);

	// only draw reflected teapot to the pixels where the mirror
	// was drawn to.
	g_pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
	g_pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);

	// position reflection
	D3DXMATRIX W, T, R;
	D3DXPLANE plane(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	D3DXMatrixReflect(&R, &plane);

	D3DXMatrixTranslation(&T,
						  g_teapotPosition.x,
						  g_teapotPosition.y,
						  g_teapotPosition.z);

	W = T * R;

	// clear depth buffer and blend the reflected teapot with the mirror
	g_pDevice->Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
	g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
	g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

	// Finally, draw the reflected teapot
	g_pDevice->SetTransform(D3DTS_WORLD, &W);
	g_pDevice->SetMaterial(&g_materalTeapot);
	g_pDevice->SetTexture(0, 0);

	g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	g_pMeshTeapot->DrawSubset(0);

	// Restore render states.
	g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	g_pDevice->SetRenderState(D3DRS_STENCILENABLE, false);
	g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);//
}