#include <windows.h>
#include <stdio.h>
#include <d3d9.h>     /* Can't include this?  Is DirectX SDK installed? */
#include <Cg/cg.h>    /* Can't include this?  Is Cg Toolkit installed! */
#include <Cg/cgD3D9.h>
#include <tchar.h>
#include <cassert>

#include "DXUT.h"  /* DirectX Utility Toolkit (part of the DirectX SDK) */

#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgD3D9.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib,"miniDXUT.lib")		//DXUT


static CGcontext g_cgContext;
static CGprofile g_cgVertexProfile;
static CGprogram g_cgVertexProgram;



static LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL;

static const WCHAR *g_strWindowTitleW = L"unifor";	//程序名
static const char  *g_strWindowTitle = "uniform";
static const char  *g_strVertexProgramFileName = "uniform.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "VS_main";			//着色程序入口函数名

static const int g_numDivi = 5;		//三角形分割的次数  即层数

static CGparameter g_cgParamTwisting;		//三角形扭曲程序  uniform类型
static CGparameter g_cgParamOrigin;			//扭曲的坐标原点
static float g_x = 0.f;
static float g_y = -0.8f;
static float g_twisting = 0.f;
static bool g_bStartChange = true;


//定义顶点结构
struct Vertex
{
	Vertex(float x = 0.f, float y = 0.f, float z = 0.f,DWORD color = 0)
	{
		_x = x;
		_y = y;
		_z = z;
		_color = color;
	}
	float _x, _y, _z;
	DWORD _color;
	static const DWORD FVF;
};

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;

static void checkForCgError(const char *situation)
{
	char buffer[4096];
	CGerror error;
	const char *string = cgGetLastErrorString(&error);

	if (error != CG_NO_ERROR) {
		if (error == CG_COMPILER_ERROR) {
			sprintf(buffer,
					"Program: %s\n"
					"Situation: %s\n"
					"Error: %s\n\n"
					"Cg compiler output...\n",
					g_strWindowTitle, situation, string);
			OutputDebugStringA(buffer);
			OutputDebugStringA(cgGetLastListing(g_cgContext));
			sprintf(buffer,
					"Program: %s\n"
					"Situation: %s\n"
					"Error: %s\n\n"
					"Check debug output for Cg compiler output...",
					g_strWindowTitle, situation, string);
			MessageBoxA(0, buffer,
						"Cg compilation error", MB_OK | MB_ICONSTOP | MB_TASKMODAL);
		}
		else {
			sprintf(buffer,
					"Program: %s\n"
					"Situation: %s\n"
					"Error: %s",
					g_strWindowTitle, situation, string);
			MessageBoxA(0, buffer,
						"Cg runtime error", MB_OK | MB_ICONSTOP | MB_TASKMODAL);
		}
		exit(1);
	}
}

/* Forward declared DXUT callbacks registered by WinMain. */
static HRESULT CALLBACK OnResetDevice(IDirect3DDevice9*, const D3DSURFACE_DESC*, void*);
static void CALLBACK OnFrameRender(IDirect3DDevice9*, double, float, void*);
static void CALLBACK OnLostDevice(void*);
static void CALLBACK OnFrameMove(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);
static void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	g_cgContext = cgCreateContext();
	checkForCgError("dont create cg context");
	cgSetParameterSettingMode(g_cgContext, CG_DEFERRED_PARAMETER_SETTING);

	DXUTSetCallbackDeviceReset(OnResetDevice);
	DXUTSetCallbackDeviceLost(OnLostDevice);
	DXUTSetCallbackFrameRender(OnFrameRender);
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackKeyboard(OnKeyboard);

	/* Parse  command line, handle  default hotkeys, and show messages. */
	DXUTInit();

	DXUTCreateWindow(g_strWindowTitleW);

	/* Display 400x400 window. */
	DXUTCreateDevice(D3DADAPTER_DEFAULT, true, 400, 400);

	DXUTMainLoop();

	cgDestroyProgram(g_cgVertexProgram);
	cgDestroyContext(g_cgContext);

	return DXUTGetExitCode();
}

int getNumOfSubTriAngle(int numDivi)		//根据分隔层数获取子三角形的个数
{
	assert(numDivi >= 0 && "numDivi >=0");
	int num = 1;
	for (int i = 0; i < numDivi; ++i)
		num *= 4;
	return num;
}

Vertex getMidVertex(Vertex& va, Vertex & vb)//获取两个顶点的中点数据
{
	Vertex v;
	v._x = (va._x + vb._x) * 0.5f;
	v._y = (va._y + vb._y) * 0.5f;
	v._z = (va._z + vb._z) * 0.5f;
	v._color = (D3DXCOLOR(va._color) + D3DXCOLOR(vb._color)) / 2.f;
	return v;
}

void initVertexDataOfTriAngle(Vertex * pData,
							   int& index,
							   int depth,
							   Vertex & va,
							   Vertex & vb,
							   Vertex & vc)		//初始化三角形的顶点数据
{
	if (depth == 0)
	{
		pData[index] = va;
		pData[index + 1] = vb;
		pData[index + 2] = vc;
		index += 3;
	}
	else
	{
		Vertex vd, ve, vf;
		vd = getMidVertex(va, vb);
		ve = getMidVertex(vb, vc);
		vf = getMidVertex(vc, va);
		initVertexDataOfTriAngle(pData, index, depth-1, va, vd, vf);
		initVertexDataOfTriAngle(pData, index, depth-1, vd, vb, ve);
		initVertexDataOfTriAngle(pData, index, depth-1, vf, vd, ve);
		initVertexDataOfTriAngle(pData, index, depth-1, vf, ve, vc);
	}
}

static HRESULT CALLBACK OnResetDevice(IDirect3DDevice9* pDev,
									  const D3DSURFACE_DESC* backBuf,
									  void* userContext)
{
	//init d3d
	HRESULT hr;
	int numSubAngle = getNumOfSubTriAngle(g_numDivi);
	hr = pDev->CreateVertexBuffer(numSubAngle * sizeof(Vertex)* 3,	//所有顶点数
								  D3DUSAGE_DYNAMIC,
								  Vertex::FVF,
								  D3DPOOL_DEFAULT,
								  &g_pVertexBuffer,
								  nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点缓存失败"), NULL, MB_OK);
		return false;
	}
	//inint vertex buffer
	Vertex * pVertexs;
	g_pVertexBuffer->Lock(0, 0, (void**)&pVertexs, 0);
	Vertex va(-0.8f, 0.8f, 0, D3DCOLOR_XRGB(0, 0,255));
	Vertex vb(0.8f, 0.8f, 0, D3DCOLOR_XRGB(0, 0,255));
	Vertex vc(0.f, -0.8f, 0, D3DCOLOR_XRGB(int(255 * 0.7), int(255 * 0.7), 255));
	int index = 0;
	initVertexDataOfTriAngle(pVertexs, index, g_numDivi, va, vb, vc);
	g_pVertexBuffer->Unlock();
	//end init d3d

	//init cg
	cgD3D9SetDevice(pDev);
	static bool bFirst = true;
	if (bFirst)
	{
		//init 
		bFirst = false;

		g_cgVertexProfile = cgD3D9GetLatestVertexProfile();		//获取profile
		const char ** optimalOptions = cgD3D9GetOptimalOptions(g_cgVertexProfile);	//获取优化选项
		checkForCgError("get optimalOptions failed");

		//create cg program
		//1 create vertex program
		g_cgVertexProgram = cgCreateProgramFromFile(g_cgContext,
													CG_SOURCE,
													g_strVertexProgramFileName,
													g_cgVertexProfile,
													g_strVertexProgramName,
													optimalOptions);
		checkForCgError("create cg program failed");

		//get handle of uniform parameter  color;
		g_cgParamTwisting = cgGetNamedParameter(g_cgVertexProgram, "twisting");
		checkForCgError("get unifor param twisting fail");

		g_cgParamOrigin = cgGetNamedParameter(g_cgVertexProgram, "origin");
		if (g_cgParamOrigin == nullptr)
			MessageBox(NULL, NULL, NULL, MB_OK);
		checkForCgError("get unifor param origin fail");
	}
	cgD3D9LoadProgram(g_cgVertexProgram, FALSE, 0);
	checkForCgError("load program failed");

	return S_OK;
}

static void CALLBACK OnFrameRender(IDirect3DDevice9* pDev,
								   double time,
								   float elapsedTime,
								   void* userContext)
{
	HRESULT hr;
	pDev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
				D3DXCOLOR(1, 1, 1, 0.0f), 1.0f, 0);

	if (SUCCEEDED(pDev->BeginScene()))
	{
		hr = cgD3D9BindProgram(g_cgVertexProgram);		//设置顶点程序
		cgSetParameter1f(g_cgParamTwisting,g_twisting);	//设置uniform参数
		cgSetParameter2f(g_cgParamOrigin, g_x, g_y);
		hr = pDev->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
		hr = pDev->SetFVF(Vertex::FVF);
		hr = pDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, getNumOfSubTriAngle(g_numDivi) * 3);
		hr = pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	if (g_pVertexBuffer) { g_pVertexBuffer->Release(); g_pVertexBuffer = nullptr; }
	cgD3D9SetDevice(NULL);
}

static void CALLBACK OnFrameMove(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
	if (g_bStartChange)
	{
		static float delta = 0.05f;
		g_twisting += delta;
		if (g_twisting > 3.f || g_twisting < -3.f)
			delta *= -1;

	}
	
}

static void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	static bool bSolid = true;
	if (bKeyDown)
	{
		switch (nChar)
		{
		case ' ':
			g_bStartChange = !g_bStartChange;		//是否自动变化
			break;
		case 'W':
			if (bSolid)
				cgD3D9GetDevice()->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			else
				cgD3D9GetDevice()->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
			bSolid = !bSolid;
			break;
		}
	}
	
}