#include <windows.h>
#include <stdio.h>
#include <d3d9.h>     /* Can't include this?  Is DirectX SDK installed? */
#include <d3dx9.h>
#include <Cg/cg.h>    /* Can't include this?  Is Cg Toolkit installed! */
#include <Cg/cgD3D9.h>
#include <tchar.h>
#include <memory.h>
#include <cassert>
#include <vector>

#include "DXUT.h"  /* DirectX Utility Toolkit (part of the DirectX SDK) */

using std::vector;

#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgD3D9.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib,"miniDXUT.lib")		//DXUT


static CGcontext g_cgContext;
static CGprofile g_cgVertexProfile;
static CGprogram g_cgVertexProgram;


static const WCHAR *g_strWindowTitleW = L"particle";	//程序名
static const char  *g_strWindowTitle = "particle";
static const char  *g_strVertexProgramFileName = "particle.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "main_vs";			//着色程序入口函数名

ID3DXMesh * g_pMeshSphere = nullptr;
IDirect3DVertexBuffer9 * g_pVertexBuffer = nullptr;
IDirect3DVertexDeclaration9 * g_pDecl = nullptr;

//matrix
D3DXMATRIX g_matrixWorld;
D3DXMATRIX g_matrixView;
D3DXMATRIX g_matrixProjection;
D3DXMATRIX g_matrixWorldViewProj;

//shader handle
CGparameter g_cgWorldMatrix;
CGparameter g_cgWorldViewProjMatrix;
CGparameter g_cgAcceleration;		//加速度
CGparameter g_cgGlobalTime;			//总共流逝的时间


const int g_numMaxParticle = 100;			//顶点缓存容纳的最大粒子个数 
int g_curNumParticle = 0;

//定义顶点结构
struct Vertex
{
	Vertex(D3DXVECTOR4 position, D3DXVECTOR4 v,float time)
	{
		_initPosition = position;
		_initV = v;
		_time = time;
	}
	D3DXVECTOR4 _initPosition;		//初始位置
	D3DXVECTOR4 _initV;				//初始速度
	float _time;					//创建时间
};


//使用顶点声明
D3DVERTEXELEMENT9 decl[] =
{
	{0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
	{ 0, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0, 32, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
	D3DDECL_END()
};



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

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	g_cgContext = cgCreateContext();
	checkForCgError("creating context");
	cgSetParameterSettingMode(g_cgContext, CG_DEFERRED_PARAMETER_SETTING);

	DXUTSetCallbackDeviceReset(OnResetDevice);
	DXUTSetCallbackDeviceLost(OnLostDevice);
	DXUTSetCallbackFrameRender(OnFrameRender);

	/* Parse  command line, handle  default hotkeys, and show messages. */
	DXUTInit();

	DXUTCreateWindow(g_strWindowTitleW);

	/* Display 400x400 window. */
	DXUTCreateDevice(D3DADAPTER_DEFAULT, true, 400, 400);

	DXUTMainLoop();

	cgDestroyProgram(g_cgVertexProgram);
	checkForCgError("destroying vertex program");
	cgDestroyContext(g_cgContext);

	return DXUTGetExitCode();
}

static void createCgPrograms()
{
	const char **profileOpts;

	/* Determine the best profile once a device to be set. */
	g_cgVertexProfile = cgD3D9GetLatestVertexProfile();
	checkForCgError("getting latest profile");

	profileOpts = cgD3D9GetOptimalOptions(g_cgVertexProfile);
	checkForCgError("getting latest profile options");

	g_cgVertexProgram =
		cgCreateProgramFromFile(
		g_cgContext,              /* Cg runtime context */
		CG_SOURCE,                /* Program in human-readable form */
		g_strVertexProgramFileName,/* Name of file containing program */
		g_cgVertexProfile,        /* Profile: OpenGL ARB vertex program */
		g_strVertexProgramName,   /* Entry function name */
		profileOpts);             /* Pass optimal compiler options */
	checkForCgError("creating vertex program from file");

	//get variable handle
	g_cgWorldMatrix = cgGetNamedParameter(g_cgVertexProgram, "worldMatrix");
	g_cgWorldViewProjMatrix = cgGetNamedParameter(g_cgVertexProgram, "worldViewProjMatrix");
	g_cgAcceleration = cgGetNamedParameter(g_cgVertexProgram, "acceleration");
	g_cgGlobalTime = cgGetNamedParameter(g_cgVertexProgram, "globalTime");

	assert(g_cgWorldMatrix && g_cgWorldViewProjMatrix && g_cgAcceleration && g_cgGlobalTime);

}

bool init(IDirect3DDevice9 * pDevice)
{
	HRESULT hr;
	/*hr = D3DXCreateTeapot(pDevice, &g_pMeshSphere, nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建球网格数据失败"), NULL, MB_OK);
		return false;
	}*/
	hr = pDevice->CreateVertexBuffer(g_numMaxParticle * sizeof(Vertex),
									 D3DUSAGE_DYNAMIC | D3DUSAGE_POINTS | D3DUSAGE_WRITEONLY,
									 0,
									 D3DPOOL_DEFAULT,
									 &g_pVertexBuffer,
									 nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点缓存失败"), NULL, MB_OK);
		return false;
	}

	//create vertex declaration
	hr = pDevice->CreateVertexDeclaration(decl, &g_pDecl);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点声明失败"), NULL, MB_OK);
		return false;
	}

	//set matrix//
	//set view 
	D3DXVECTOR3 pos(-1.f, 0.f, -3.f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMatrixLookAtLH(&g_matrixView, &pos, &target, &up);

	//set projection
	D3DXMatrixPerspectiveFovLH(&g_matrixProjection,
							   D3DX_PI / 4.f,
							   1.f,/* 400.f / 400.f */
							   1.f,
							   1000.f);
	//set world matrix
	D3DXMATRIX temp;
	D3DXMatrixIdentity(&g_matrixWorld);
	/*D3DXMatrixRotationX(&temp, D3DX_PI / 8.f);
	g_matrixWorld = temp;
	D3DXMatrixRotationY(&temp, D3DX_PI / 5.f);
	g_matrixWorld *= temp;
	D3DXMatrixRotationZ(&temp, D3DX_PI / 4.f);
	g_matrixWorld *= temp;
	D3DXMatrixTranslation(&temp, 0.5f, 0.3f, 2.f);
	g_matrixWorld *= temp;*/

	//set worldViewProj matrix
	g_matrixWorldViewProj = g_matrixWorld * g_matrixView * g_matrixProjection;

	return true;
}


static HRESULT CALLBACK OnResetDevice(IDirect3DDevice9* pDev,
									  const D3DSURFACE_DESC* backBuf,
									  void* userContext)
{
	cgD3D9SetDevice(pDev);
	checkForCgError("setting Direct3D device");

	
	static int firstTime = 1;
	if (firstTime) {
		/* Cg runtime resources such as CGprogram and CGparameter handles
		survive a device reset so we just need to compile a Cg program
		just once.  We do however need to unload Cg programs with
		cgD3DUnloadProgram upon when a Direct3D device is lost and load
		Cg programs every Direct3D device reset with cgD3D9UnloadProgram. */
		createCgPrograms();
		firstTime = 0;
	}

	/* false below means "no parameter shadowing" */
	cgD3D9LoadProgram(g_cgVertexProgram, false, 0);
	checkForCgError("loading vertex program");

	bool bRet;
	bRet = init(pDev);
	if (bRet)
		return S_OK;
	else
		return S_FALSE;
}

void addParticle(double time)
{
	if (g_curNumParticle < g_numMaxParticle)
	{
		Vertex * pVertex;
		HRESULT hr;
		hr = g_pVertexBuffer->Lock(sizeof(Vertex)* g_curNumParticle,
								   sizeof(Vertex),
								   (void**)&pVertex,
								   0);
		if (FAILED(hr))
		{
			MessageBox(NULL, _T("Lock fail"), NULL, MB_OK);
			return;
		}
		//pVertex += g_curNumParticle;
		//for (int i = 0; i < 50; ++i)
		{
			pVertex->_initPosition = D3DXVECTOR4(0.2f, 0.f, 0.f, 1.f);
			pVertex->_initV = D3DXVECTOR4( 0.f, 1.1 , 0.f, 0.f);
			pVertex->_time = time ;
			++g_curNumParticle;
		}
		g_pVertexBuffer->Unlock();
	}
	else
		assert(0 && "g_curNumParticle < g_numMaxParticle");
}

static void CALLBACK OnFrameRender(IDirect3DDevice9* pDev,
								   double time,
								   float elapsedTime,
								   void* userContext)
{
	pDev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
				D3DXCOLOR(0.1f, 0.3f, 0.6f, 0.0f), 1.0f, 0);
	static double lastTime = 0;
	static bool flag = true;
	if (flag && time - lastTime > 1.f)
	{
		addParticle(time);
		lastTime = time;
	//	flag = false;
	}
	if (SUCCEEDED(pDev->BeginScene())) 
	{

		cgD3D9BindProgram(g_cgVertexProgram);
		checkForCgError("binding vertex program");

		cgSetParameterValuefc(g_cgWorldMatrix, 16, g_matrixWorld.m[0]);
		cgSetParameterValuefc(g_cgWorldViewProjMatrix, 16, g_matrixWorldViewProj.m[0]);
		cgSetParameter4f(g_cgAcceleration, 0.f, -9.8f, 0.f, 0.f);
		cgSetParameter1f(g_cgGlobalTime, time);

		pDev->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
		pDev->SetVertexDeclaration(g_pDecl);
		pDev->DrawPrimitive(D3DPT_POINTLIST, 0, g_curNumParticle);

		pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	g_pVertexBuffer->Release();
	g_pDecl->Release();
	cgD3D9SetDevice(NULL);
}
