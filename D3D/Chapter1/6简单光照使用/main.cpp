#include <windows.h>
#include <tchar.h>
#include <d3dx9.h>

#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx10.lib")

//������ر���
const TCHAR g_szClassName[] = _T("DirextClass");	//��������
const TCHAR g_szWindowTitle[] = _T("Application");	//������
const int g_xPos = 400;								//������ʾx����
const int g_yPos = 200;								//������ʾy����
const int g_cxWindow = 600;							//���ڳ�
const int g_cyWindow = 480;							//���ڿ�

//DirectX��ر���
IDirect3DDevice9 * g_pDevice;
IDirect3DVertexBuffer9 * g_pVertexBuffer;

struct Vertex			
{
	Vertex(float x, float y, float z,float n1,float n2,float n3)
	{
		_x = x;
		_y = y;
		_z = z;
		_n1 = n1;
		_n2 = n2;
		_n3 = n3;
	}
	float _x, _y, _z;
	float _n1, _n2, _n3;
	static const DWORD FVF;
};

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL;


//��������
BOOL InitD3D(HINSTANCE hInstance);
BOOL Setup();
void Render();
void Cleanup();

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI _tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPreInstance,
					 LPTSTR lpCmdLine,
					 int nCmdShow)
{
	if (!InitD3D(hInstance))			//��ʼ��D3D
	{
		MessageBox(NULL, _T("InitD3D Failed"), NULL, MB_OK);
		return FALSE;
	}

	MSG msg;					
	while (true)						//��ʼ��Ϣѭ��
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

	//DirectX��ʼ�� 1 ��ȡDirect3D9�ӿ�ָ��
	IDirect3D9 * pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D)
	{
		MessageBox(NULL, _T("Direct3DCreate9 Failed"), NULL, MB_OK);
		return FALSE;
	}

	//DirectX��ʼ�� 2 ����豸����
	D3DCAPS9 caps;
	pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT,
						D3DDEVTYPE_HAL,
						&caps);
	int vp;		//���㴦������
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	//DirectX��ʼ�� 3 ��ʼ��D3DPRESENT_PARAMETERS�ṹ��
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

	//DirectX��ʼ�� 4 ����IDirect3DDevice����
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

	//����
	if (!Setup())
	{
		MessageBox(NULL, _T("Setup Failed"), NULL, MB_OK);
		return FALSE;
	}
	return TRUE;
}

BOOL Setup()
{
	if (g_pDevice)
	{
		D3DLIGHT9 light;					//����ʹ�� 1 ������Դ
		memset(&light, 0, sizeof(D3DLIGHT9));
		light.Type = D3DLIGHT_DIRECTIONAL;
		light.Diffuse = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255));
		light.Specular = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255)) * 0.3f;
		light.Ambient = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255)) * 0.6f;
		light.Direction = D3DXVECTOR3(1.f, 0, 0);

		g_pDevice->SetLight(0, &light);	//���ù�Դ
		g_pDevice->LightEnable(0, TRUE);//������Ӧ��Դ


		D3DMATERIAL9 material;				//����ʹ�� 2 �����������
		material.Diffuse = D3DXCOLOR(D3DCOLOR_XRGB(255,255,255));
		material.Specular = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255));
		material.Ambient = D3DXCOLOR(D3DCOLOR_XRGB(255, 255, 255));
		material.Emissive = D3DXCOLOR(D3DCOLOR_XRGB(0, 0, 0));
		material.Power = 5.f;
		
		g_pDevice->SetMaterial(&material);

		
		g_pDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);		//����ʹ�� 3 ������������Ĺ���״̬
		g_pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);	//�ٴα�׼������״̬��ʹ�õķ���������ֹ�ڱ任�Ĺ����з����ı�
		g_pDevice->SetRenderState(D3DRS_LIGHTING, true);			//����ʹ�� 4 ���ù���

		g_pDevice->CreateVertexBuffer(12 * sizeof(Vertex),
									  0,
									  Vertex::FVF,
									  D3DPOOL_MANAGED,
									  &g_pVertexBuffer,
									  NULL);
		Vertex * pVertexs;
		g_pVertexBuffer->Lock(0, 0, (void **)&pVertexs, 0);
		// front face
		pVertexs[0] = Vertex(-1.0f, 0.0f, -1.0f, 0.0f, 0.707f, -0.707f);
		pVertexs[1] = Vertex(0.0f, 1.0f, 0.0f, 0.0f, 0.707f, -0.707f);
		pVertexs[2] = Vertex(1.0f, 0.0f, -1.0f, 0.0f, 0.707f, -0.707f);
		
			// left face
		pVertexs[3] = Vertex(-1.0f, 0.0f, 1.0f, -0.707f, 0.707f, 0.0f);
		pVertexs[4] = Vertex(0.0f, 1.0f, 0.0f, -0.707f, 0.707f, 0.0f);
		pVertexs[5] = Vertex(-1.0f, 0.0f, -1.0f, -0.707f, 0.707f, 0.0f);
		
			// right face
		pVertexs[6] = Vertex(1.0f, 0.0f, -1.0f, 0.707f, 0.707f, 0.0f);
		pVertexs[7] = Vertex(0.0f, 1.0f, 0.0f, 0.707f, 0.707f, 0.0f);
		pVertexs[8] = Vertex(1.0f, 0.0f, 1.0f, 0.707f, 0.707f, 0.0f);
		
			// back face
		pVertexs[9] = Vertex(1.0f, 0.0f, 1.0f, 0.0f, 0.707f, 0.707f);
		pVertexs[10] = Vertex(0.0f, 1.0f, 0.0f, 0.0f, 0.707f, 0.707f);
		pVertexs[11] = Vertex(-1.0f, 0.0f, 1.0f, 0.0f, 0.707f, 0.707f);
		g_pVertexBuffer->Unlock();
		
		//g_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);	//��Ҫ���߿�ģʽ������ΪĬ�ϣ�Ϊ
		//g_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_POINT);		//��
		//g_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);		//Ĭ�ϣ�ʵ�ĵ�


		//
		// Position and aim the camera.
		//
		D3DXVECTOR3 pos(0.0f, 1.0f, -3.0f);
		D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
		D3DXMATRIX V;
		D3DXMatrixLookAtLH(&V, &pos, &target, &up);
		g_pDevice->SetTransform(D3DTS_VIEW, &V);

		//
		// Set the projection matrix.
		//

		D3DXMATRIX proj;
		D3DXMatrixPerspectiveFovLH(
			&proj,
			D3DX_PI * 0.5f, // 90 - degree
			(float)g_cxWindow / (float)g_cyWindow,
			1.0f,
			1000.0f);
		g_pDevice->SetTransform(D3DTS_PROJECTION, &proj);
	}
	return TRUE;
}

void Render()
{
	D3DXMATRIX yRot;

	static float y = 0.0f;

	D3DXMatrixRotationY(&yRot, y);
	y += 0.01;

	if (y >= 6.28f)
		y = 0.0f;

	g_pDevice->SetTransform(D3DTS_WORLD, &yRot);

	g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1, 0);
	g_pDevice->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
	g_pDevice->SetFVF(Vertex::FVF);

	g_pDevice->BeginScene();
	g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 4);
	g_pDevice->EndScene();
	g_pDevice->Present(NULL, NULL, NULL, NULL);
}

void Cleanup()
{
	if (g_pDevice) g_pDevice->Release();
}