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
static CGprofile g_cgPixelProfile;
static CGprogram g_cgPixelProgram;

static LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL;
static IDirect3DCubeTexture9 * g_pCubeTexture = NULL;

static const WCHAR *g_strWindowTitleW = L"CubeMap";	//程序名
static const char  *g_strWindowTitle = "CubeMap";
static const char  *g_strVertexProgramFileName = "cubeMap.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "main_vs";			//着色程序入口函数名
static const char  *g_strPixelProgramFileName = "cubeMap.cg";
static const char  *g_strPixelProgramName = "main_ps";

static const float g_bias = 0.f;
static const float g_scale = 1.f;
static const float g_power = 2.f;
static const float g_etaR = 1.f / 1.8f;
static const float g_etaG = 1.f / 1.7f;
static const float g_etaB = 1.f / 1.5f;

ID3DXMesh * g_pMeshSphere = nullptr;

//matrix
D3DXMATRIX g_matrixWorld;
D3DXMATRIX g_matrixView;
D3DXMATRIX g_matrixProjection;
D3DXMATRIX g_matrixWorldViewProj;
D3DXVECTOR3 g_eyePos;


//shader handle
CGparameter g_cgWorldMatrix;
CGparameter g_cgWorldViewProjMatrix;
CGparameter g_cgEyePos;
CGparameter g_cgBias;
CGparameter g_cgScale;
CGparameter g_cgPower;
CGparameter g_cgEtaRatio;		// r g b channle

CGparameter g_cgCubeMapSampler;



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
	g_cgEyePos = cgGetNamedParameter(g_cgVertexProgram, "eyePosition");
	g_cgBias = cgGetNamedParameter(g_cgVertexProgram, "bias");
	g_cgScale = cgGetNamedParameter(g_cgVertexProgram, "scale");
	g_cgPower = cgGetNamedParameter(g_cgVertexProgram, "power");
	g_cgEtaRatio = cgGetNamedParameter(g_cgVertexProgram, "etaRatio");

	
	assert(g_cgWorldMatrix && g_cgWorldViewProjMatrix && g_cgEyePos &&
		   g_cgBias && g_cgScale && g_cgPower && g_cgEtaRatio);


	//create pixel program
	g_cgPixelProfile = cgD3D9GetLatestPixelProfile();
	checkForCgError("get lastest pixel profile faile");

	profileOpts = cgD3D9GetOptimalOptions(g_cgPixelProfile);
	checkForCgError("get lastest pixel profile options faile");

	g_cgPixelProgram = cgCreateProgramFromFile(g_cgContext,
											   CG_SOURCE,
											   g_strPixelProgramFileName,
											   g_cgPixelProfile,
											   g_strPixelProgramName,
											   profileOpts);
	g_cgCubeMapSampler = cgGetNamedParameter(g_cgPixelProgram, "environmentMap");
	assert(g_cgCubeMapSampler);

}

bool init(IDirect3DDevice9 * pDevice)
{
	HRESULT hr;
	hr = D3DXCreateSphere(pDevice, 1.f, 30, 20, &g_pMeshSphere, nullptr);
	//hr = D3DXCreateTeapot(pDevice, &g_pMeshSphere, nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建球网格数据失败"), NULL, MB_OK);
		return false;
	}

	//create cube texture
	hr = D3DXCreateCubeTextureFromFile(pDevice, _T("cubeMap.dds"), &g_pCubeTexture);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建立方体纹理失败！"), NULL, MB_OK);
		return false;
	}

	//set matrix//
	//set view 
	D3DXVECTOR3 pos(0.f, 0.f, -2.f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMatrixLookAtLH(&g_matrixView, &pos, &target, &up);
	g_eyePos = pos;

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
	g_matrixWorld *= temp;*/
	D3DXMatrixTranslation(&temp, 0.f, 0.3f, 2.f);
	g_matrixWorld *= temp;

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

	cgD3D9LoadProgram(g_cgPixelProgram, false, 0);
	checkForCgError("load pixel program");

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

		cgD3D9BindProgram(g_cgPixelProgram);
		checkForCgError("binding pixel program");

		cgSetParameterValuefc(g_cgWorldMatrix, 16, g_matrixWorld.m[0]);
		cgSetParameterValuefc(g_cgWorldViewProjMatrix, 16, g_matrixWorldViewProj.m[0]);
		cgSetParameter3f(g_cgEyePos, g_eyePos.x, g_eyePos.y, g_eyePos.z);
		cgSetParameter1f(g_cgBias, g_bias);
		cgSetParameter1f(g_cgScale, g_scale);
		cgSetParameter1f(g_cgPower, g_power);
		cgSetParameter3f(g_cgEtaRatio, g_etaR, g_etaG, g_etaB);

		cgD3D9SetTexture(g_cgCubeMapSampler, g_pCubeTexture);

		g_pMeshSphere->DrawSubset(0);
		pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	g_pMeshSphere->Release();
	g_pCubeTexture->Release();
	cgD3D9SetDevice(NULL);
}
