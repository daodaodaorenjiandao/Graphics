#include <windows.h>
#include <stdio.h>
#include <d3d9.h>     /* Can't include this?  Is DirectX SDK installed? */
#include <d3dx9.h>
#include <Cg/cg.h>    /* Can't include this?  Is Cg Toolkit installed! */
#include <Cg/cgD3D9.h>
#include <tchar.h>
#include <memory.h>
#include <cassert>

#include "DXUT.h"  /* DirectX Utility Toolkit (part of the DirectX SDK) */

#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgD3D9.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib,"miniDXUT.lib")		//DXUT


static CGcontext g_cgContext;
static CGprofile g_cgVertexProfile;
static CGprogram g_cgVertexProgram;

static LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL;

static const WCHAR *g_strWindowTitleW = L"KeyFrame";	//程序名
static const char  *g_strWindowTitle = "KeyFrame";
static const char  *g_strVertexProgramFileName = "KeyFrame.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "main_vs";			//着色程序入口函数名

//vertex
struct Vertex
{
	Vertex(float x1, float y1, float z1,float x2,float y2,float z2)
	{
		_x1 = x1; _y1 = y1; _z1 = z1;
		_x2 = x2; _y2 = y2; _z2 = z2;
	}
	float _x1, _y1, _z1;
	float _x2, _y2, _z2;
	static const DWORD FVF;
};

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL;

CGparameter g_cgTime;


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
	
	g_cgTime = cgGetNamedParameter(g_cgVertexProgram, "time");
	

	assert(g_cgTime );

}

bool init(IDirect3DDevice9 * pDevice)
{
	HRESULT hr;
	hr = pDevice->CreateVertexBuffer(sizeof(Vertex)* 3,
									 D3DUSAGE_DYNAMIC,
									 Vertex::FVF,
									 D3DPOOL_DEFAULT,
									 &g_pVertexBuffer,
									 nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点缓存失败！"), NULL, MB_OK);
		return false;
	}

	//init vertex buffer
	//			|\   (向左旋转90度)        /|
	//			| \       --->            / |     
	//			---                       ---
	//
	Vertex *pVertexs;
	g_pVertexBuffer->Lock(0, 0, (void**)&pVertexs, 0);
	*pVertexs = Vertex(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
	*(pVertexs+1) = Vertex(0.f, 0.5f, 0.f, -.5f, 0.f, 0.f);
	*(pVertexs + 2) = Vertex(0.2f, 0.f, 0.f, 0.f, 0.2f, 0.f);
	g_pVertexBuffer->Unlock();

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

static void CALLBACK OnFrameRender(IDirect3DDevice9* pDev,
								   double time,
								   float elapsedTime,
								   void* userContext)
{
	pDev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
				D3DXCOLOR(0.1f, 0.3f, 0.6f, 0.0f), 1.0f, 0);

	if (SUCCEEDED(pDev->BeginScene())) {

		cgD3D9BindProgram(g_cgVertexProgram);
		checkForCgError("binding vertex program");

		cgSetParameter1f(g_cgTime, time);
		pDev->SetFVF(Vertex::FVF);
		pDev->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
		pDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
		pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	g_pVertexBuffer->Release();
	cgD3D9SetDevice(NULL);
}
