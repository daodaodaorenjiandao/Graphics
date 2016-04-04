#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>
#include <D3DX10Math.h>
#include <memory.h>
#include <d3dx9Mesh.h>
#include <vector>
#include <fstream>

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
IDirect3DTexture9 * g_pTextures[] = { NULL, NULL, NULL };

void dumpVertices(std::ofstream& outFile, ID3DXMesh * pMesh);
void dumpIndices(std::ofstream& outFile, ID3DXMesh * pMesh);
void dumpAttributeTable(std::ofstream& outFile, ID3DXMesh * pMesh);
void dumpAttributeBuffer(std::ofstream& outFile, ID3DXMesh * pMesh);
void dumpAdjacencyBuffer(std::ofstream& outFile, ID3DXMesh * pMesh);

struct Vertex
{
	Vertex(){}
	Vertex(float x, float y, float z,float nx,float ny,float nz,float u,float v)
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
	hr = D3DXCreateMeshFVF(12,
						   24,
						   D3DXMESH_MANAGED,
						   Vertex::FVF,
						   g_pDevice,
						   &g_pMesh);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("D3DXCreateMeshFVF Failed!"), NULL, MB_OK);
		return false;
	}
	Vertex * v = nullptr;
	g_pMesh->LockVertexBuffer(0, (void**)&v);
	v[0] = Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[1] = Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[2] = Vertex(1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
	v[3] = Vertex(1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	// fill in the back face vertex data
	v[4] = Vertex(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[5] = Vertex(1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[6] = Vertex(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	v[7] = Vertex(-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	// fill in the top face vertex data
	v[8] = Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[9] = Vertex(-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	v[10] = Vertex(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
	v[11] = Vertex(1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);

	// fill in the bottom face vertex data
	v[12] = Vertex(-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	v[13] = Vertex(1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
	v[15] = Vertex(-1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	// fill in the left face vertex data
	v[16] = Vertex(-1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[17] = Vertex(-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[18] = Vertex(-1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[19] = Vertex(-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// fill in the right face vertex data
	v[20] = Vertex(1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[21] = Vertex(1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[22] = Vertex(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[23] = Vertex(1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	g_pMesh->UnlockVertexBuffer();

	WORD * i = nullptr;
	g_pMesh->LockIndexBuffer(0, (void**)&i);

	// fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	g_pMesh->UnlockIndexBuffer();

	DWORD * attributeBuffer = nullptr;
	g_pMesh->LockAttributeBuffer(0, &attributeBuffer);

	for (int a = 0; a < 4; a++)
		attributeBuffer[a] = 0;

	for (int b = 4; b < 8; b++)
		attributeBuffer[b] = 1;

	for (int c = 8; c < 12; c++)
		attributeBuffer[c] = 2;

	g_pMesh->UnlockAttributeBuffer();

	std::vector<DWORD> vecAdjcentInfo(g_pMesh->GetNumFaces() * 3);
	g_pMesh->GenerateAdjacency(0.001, &vecAdjcentInfo[0]);
	hr = g_pMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_STRIPREORDER,
								  &vecAdjcentInfo[0],
								  0,
								  0,
								  0);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("OptimizeInplace Failed !"), NULL, MB_OK);
	}

	std::ofstream  outFile;
	outFile.open("log.txt");

	dumpVertices(outFile, g_pMesh);
	dumpIndices(outFile, g_pMesh);
	dumpAttributeTable(outFile, g_pMesh);
	dumpAttributeBuffer(outFile, g_pMesh);
	dumpAdjacencyBuffer(outFile, g_pMesh);

	outFile.close();

	hr = D3DXCreateTextureFromFile(g_pDevice, _T("brick0.jpg"), g_pTextures);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建纹理1失败"), NULL, MB_OK);
		return false;
	}

	hr = D3DXCreateTextureFromFile(g_pDevice, _T("brick1.jpg"), g_pTextures + 1);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建纹理2失败"), NULL, MB_OK);
		return false;
	}

	hr = D3DXCreateTextureFromFile(g_pDevice, _T("checker.jpg"), g_pTextures + 2);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建纹理3失败"), NULL, MB_OK);
		return false;
	}

	g_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

	g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	//取景
	D3DXVECTOR3 pos(0.f, 0.f, -4.f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);

	D3DXMATRIX V;
	D3DXMatrixLookAtLH(&V, &pos, &target, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &V);

	//投影
	D3DXMATRIX projection;
	D3DXMatrixPerspectiveFovLH(&projection,
							   D3DX_PI * 0.5,
							   (float)g_cxWindow / (float)g_cyWindow,
							   1.f,
							   1000.f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &projection);

	return TRUE;
}

void Render()
{
	g_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
	g_pDevice->BeginScene();

	D3DXMATRIX xRot;
	D3DXMatrixRotationX(&xRot, D3DX_PI * 0.2f);

	float timeDelta = 0.01;
	static float y = 0.f;
	y += timeDelta;
	if (y > D3DX_PI * 2)
		y = 0.f;

	D3DXMATRIX yRot;
	D3DXMatrixRotationY(&yRot, y);

	D3DXMATRIX m = xRot * yRot;
	g_pDevice->SetTransform(D3DTS_WORLD, &m);

	for (int count = 0; count < 3; ++count)
	{
		g_pDevice->SetTexture(0, g_pTextures[count]);
		g_pMesh->DrawSubset(count);
	}

	g_pDevice->EndScene();
	g_pDevice->Present(0, 0, 0, 0);
}

void Cleanup()
{
	if (g_pDevice) g_pDevice->Release();
	if (g_pMesh) g_pMesh->Release();
}

void dumpVertices(std::ofstream& outFile, ID3DXMesh * pMesh)
{
	outFile << "Vertex begin" << std::endl;
	outFile << "----------" << std::endl;
	Vertex * pVertex = nullptr;
	pMesh->LockVertexBuffer(0, (void**)&pVertex);
	for (int count = 0; count < g_pMesh->GetNumVertices(); ++count)
	{
		outFile << pVertex[count]._x << " " << pVertex[count]._y << " " << pVertex[count]._z << " --- "
			<< pVertex[count]._nx << " " << pVertex[count]._ny << " " << pVertex[count]._nz << " --- "
			<< pVertex[count]._u << " " << pVertex[count]._v << std::endl;
	}
	pMesh->UnlockVertexBuffer();
	outFile << "----------" << std::endl;
	outFile << "Vertex end" << std::endl << std::endl;
}
void dumpIndices(std::ofstream& outFile, ID3DXMesh * pMesh)
{
	outFile << "Index begin" << std::endl;
	outFile << "----------" << std::endl;
	WORD * pIndex = nullptr;
	pMesh->LockIndexBuffer(0, (void**)&pIndex);
	for (int count = 0; count < pMesh->GetNumFaces() * 3; ++count)
		outFile << pIndex[count] << " ";
	outFile << std::endl;
	pMesh->UnlockIndexBuffer();
	outFile << "----------" << std::endl;
	outFile << "index end" << std::endl << std::endl;
}
void dumpAttributeTable(std::ofstream& outFile, ID3DXMesh * pMesh)
{
	outFile << "AttributeTable begin" << std::endl;
	outFile << "----------" << std::endl;
	DWORD attributeSize = 0;
	pMesh->GetAttributeTable(NULL, &attributeSize);
	std::vector<D3DXATTRIBUTERANGE> vecAttributeTable(attributeSize);
	if (attributeSize == 0)		//如果没有进行优化，那么网格的属性表的大小为0
		return;
	pMesh->GetAttributeTable(&vecAttributeTable[0], NULL);

	for (auto attributeRange : vecAttributeTable)
	{
		outFile << attributeRange.AttribId << " "
			<< attributeRange.FaceStart << " "
			<< attributeRange.FaceCount << " "
			<< attributeRange.VertexStart << " "
			<< attributeRange.VertexCount << std::endl;
	}
	outFile << "----------" << std::endl;
	outFile << "AttributeTable end" << std::endl << std::endl;
}
void dumpAttributeBuffer(std::ofstream& outFile, ID3DXMesh * pMesh)
{
	outFile << "AttributeBuffer begin" << std::endl;
	outFile << "----------" << std::endl;
	DWORD * pAttributeBuffer = nullptr;
	pMesh->LockAttributeBuffer(0, &pAttributeBuffer);
	for (int count = 0; count < pMesh->GetNumFaces(); ++count)
		outFile << pAttributeBuffer[count] << " ";
	outFile << std::endl;
	pMesh->UnlockAttributeBuffer();
	outFile << "----------" << std::endl;
	outFile << "AttributeBuffer end" << std::endl << std::endl;

}
void dumpAdjacencyBuffer(std::ofstream& outFile, ID3DXMesh * pMesh)
{
	outFile << "AdjacencyBuffer begin" << std::endl;
	outFile << "----------" << std::endl;
	std::vector<DWORD> vecAdjcentInfo(g_pMesh->GetNumFaces() * 3);
	g_pMesh->GenerateAdjacency(0.001, &vecAdjcentInfo[0]);
	
	for (int count = 0; count < pMesh->GetNumFaces() * 3; ++count)
		outFile << vecAdjcentInfo[count] << " ";
	outFile << std::endl;
	outFile << "----------" << std::endl;
	outFile << "AdjacencyBuffer end" << std::endl << std::endl;
}