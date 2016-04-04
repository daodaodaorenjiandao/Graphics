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
ID3DXMesh * g_pMesh = nullptr;
ID3DXPMesh * g_pPMesh = nullptr;		//渐进网格

ID3DXFont * g_pFont = nullptr;

int g_countFrames = 0;	//记录一秒中渲染帧的个数
float g_fLastTime = 0;
float g_fCurTime = 0;
float g_fElapseTime = 0;
float g_FPS = 0;
float ComputFPS();

std::vector<D3DMATERIAL9> g_vecMaterial;
std::vector<IDirect3DTexture9 *> g_vecTextures;

D3DMATERIAL9 YELLOW_MATERIAL = { D3DXCOLOR(1.f, 1.f, 0.f, 1.f),
								D3DXCOLOR(1.f, 1.f, 0.f, 1.f),
								D3DXCOLOR(1.f, 1.f, 0.f, 1.f),
								D3DXCOLOR(0.f, 0.f, 0.f, 0.f),
								2.f };

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
	ID3DXBuffer * pMaterialBuffer = nullptr;
	DWORD numMaterial = 0;
	hr = D3DXLoadMeshFromX(_T("bigship1.x"),
						   D3DXMESH_MANAGED,
						   g_pDevice,
						   NULL,
						   &pMaterialBuffer,
						   NULL,
						   &numMaterial,
						   &g_pMesh);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("从X文件导入数据失败！"), NULL, MB_OK);
		return false;
	}
	if (pMaterialBuffer && numMaterial)
	{
		D3DXMATERIAL * pMaterial = nullptr;
		pMaterial = static_cast<D3DXMATERIAL*>(pMaterialBuffer->GetBufferPointer());
		assert(pMaterial != nullptr);
		for (int count = 0; count < numMaterial; ++count)	//导入材质属性
		{
			g_vecMaterial.push_back(pMaterial[count].MatD3D);
			g_vecMaterial[count].Ambient = g_vecMaterial[count].Diffuse;
			if (pMaterial->pTextureFilename)
			{
				IDirect3DTexture9 * pTexture = nullptr;
				D3DXCreateTextureFromFileA(g_pDevice, pMaterial->pTextureFilename, &pTexture);
				g_vecTextures.push_back(pTexture);
			}
		}
		pMaterialBuffer->Release();
	}
	//网格优化
	std::vector<DWORD> vecAdjacencyInfo(g_pMesh->GetNumFaces() * 3);
	g_pMesh->GenerateAdjacency(0.001f, &vecAdjacencyInfo[0]);
	hr = g_pMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_STRIPREORDER,
								  &vecAdjacencyInfo[0],
								  NULL,
								  NULL,
								  NULL);
	g_pMesh->GenerateAdjacency(0.001f, &vecAdjacencyInfo[0]);
	hr = D3DXGeneratePMesh(g_pMesh,
						   &vecAdjacencyInfo[0],
						   NULL,
						   NULL,
						   1,
						   D3DXMESHSIMP_FACE,
						   &g_pPMesh);
	if (g_pMesh) g_pMesh->Release();
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("生成渐进网格失败！"), NULL, MB_OK);
		return false;
	}
	int numFaces;
	numFaces = g_pPMesh->GetMaxFaces();
	g_pPMesh->SetNumFaces(numFaces);		//设置面数为最大

	//g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	g_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

	D3DLIGHT9 light;
	memset(&light, 0, sizeof(light));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Ambient = D3DXCOLOR(0.4f, 0.4f, 0.4f, 0.4f);
	light.Diffuse = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);
	light.Specular = D3DXCOLOR(1.f, 1.f, 1.f, 1.f) * 0.6f;
	light.Direction = D3DXVECTOR3(1.f, -1.f, 1.f);

	g_pDevice->SetLight(0, &light);
	g_pDevice->LightEnable(0, TRUE);
	g_pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
	g_pDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);

	D3DXFONT_DESC fontDesc;
	fontDesc.Height = 25;
	fontDesc.Width = 12;
	fontDesc.Weight = 500;
	fontDesc.MipLevels = D3DX_DEFAULT;
	fontDesc.Italic = false;
	fontDesc.CharSet = DEFAULT_CHARSET;
	fontDesc.OutputPrecision = 0;
	fontDesc.Quality = 0;
	fontDesc.PitchAndFamily = 0;
	_tcscpy(fontDesc.FaceName, _T("Times New Roman"));
	hr = D3DXCreateFontIndirect(g_pDevice, &fontDesc, &g_pFont);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建D3DXFont接口失败！"), NULL, MB_OK);
		return false;
	}

	//取景
	//D3DXVECTOR3 pos(0.f, 0.f, -20.f);
	D3DXVECTOR3 pos(-8.0f, 4.0f, -12.0f);
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
	HRESULT hr;
	DWORD curNumFaces = g_pPMesh->GetNumFaces();
	DWORD curNumVertex = g_pPMesh->GetNumVertices();
	if (::GetAsyncKeyState('A') & 0x8000f)
	{
		
		/*hr= g_pPMesh->SetNumFaces(curNumFaces + 1);
		if (curNumFaces == g_pPMesh->GetNumFaces())
			g_pPMesh->SetNumFaces(curNumFaces + 3);*/
		int count = 1;
		do
		{
			g_pPMesh->SetNumVertices(curNumVertex + count++);
			
		} while (curNumVertex + count < g_pPMesh->GetMaxVertices() && //防止死循环
				 curNumVertex == g_pPMesh->GetNumVertices());
		
	}
	if (::GetAsyncKeyState('S') & 0x8000f)
	{
		/*g_pPMesh->SetNumFaces(curNumFaces - 1);*/
		g_pPMesh->SetNumVertices(curNumVertex - 1);
	}

	g_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
	g_pDevice->BeginScene();

	//Draw FPS
	TCHAR buffer[128] = { 0 };
	float FPS = ComputFPS();
	_stprintf(buffer, _T("FPS: %f"), FPS);

	RECT rect;
	rect.left = 100;
	rect.top = 100;
	rect.right = 400;
	rect.bottom = 300;
	
	g_pFont->DrawText(NULL, buffer, -1, &rect, DT_CENTER, D3DCOLOR_XRGB(0, 255, 0));
	//Draw Modal
	for (int count = 0; count < g_vecMaterial.size(); ++count)
	{
		g_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		g_pDevice->SetMaterial(&g_vecMaterial[count]);
		if (count < g_vecTextures.size())
			g_pDevice->SetTexture(0, g_vecTextures[count]);
		g_pPMesh->DrawSubset(count);

		g_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		g_pDevice->SetMaterial(&YELLOW_MATERIAL);
		if (count < g_vecTextures.size())
			g_pDevice->SetTexture(0, g_vecTextures[count]);
		g_pPMesh->DrawSubset(count);

	}
	g_pDevice->EndScene();
	g_pDevice->Present(0, 0, 0, 0);
}

void Cleanup()
{
	if (g_pDevice) g_pDevice->Release();
	//	if (g_pMesh) g_pMesh->Release();
	for (auto e : g_vecTextures)	//释放纹理数据
	{
		e->Release();
	}
	if (g_pPMesh) g_pPMesh->Release();
}

float ComputFPS()
{
	float deltaTime;

	g_fCurTime = timeGetTime();
	deltaTime = (g_fCurTime - g_fLastTime) * 0.001;


	g_fElapseTime += deltaTime;

	++g_countFrames;
	if (g_fElapseTime >= 1.0f)
	{
		g_FPS = (float)g_countFrames / g_fElapseTime;
		g_countFrames = 0;
		g_fElapseTime = 0.f;
	}
	g_fLastTime = g_fCurTime;
	return g_FPS;
}