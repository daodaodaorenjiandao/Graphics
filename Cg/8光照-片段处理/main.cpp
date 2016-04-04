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

static const WCHAR *g_strWindowTitleW = L"illumination";	//程序名
static const char  *g_strWindowTitle = "illumination";
static const char  *g_strVertexProgramFileName = "illumination.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "main_vs";			//着色程序入口函数名
static const char  *g_strPixelProgramFileName = "illumination.cg";
static const char  *g_strPixelProgramName = "main_ps";

ID3DXMesh * g_pMeshSphere = nullptr;

//matrix
D3DXMATRIX g_matrixWorld;
D3DXMATRIX g_matrixView;
D3DXMATRIX g_matrixProjection;
D3DXMATRIX g_matrixWorldViewProj;
D3DXVECTOR3 g_eyePos;

D3DLIGHT9 g_light;
D3DMATERIAL9 g_material;

//shader handle
CGparameter g_cgWorldMatrix;
CGparameter g_cgWorldViewProjMatrix;
CGparameter g_cgAmbient;
CGparameter g_cgLightColor;
CGparameter g_cgLightPos;
CGparameter g_cgLightAttenuation;
CGparameter g_cgEyePos;
CGparameter g_cgKe;
CGparameter g_cgKa;
CGparameter g_cgKd;
CGparameter g_cgKs;
CGparameter g_cgShininess;		//power


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
	cgDestroyProgram(g_cgPixelProgram);
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

	g_cgPixelProfile = cgD3D9GetLatestPixelProfile();
	checkForCgError("getting lastest profile pixel");

	profileOpts = cgD3D9GetOptimalOptions(g_cgPixelProfile);
	checkForCgError("getting lastest profile options");

	g_cgPixelProgram = cgCreateProgramFromFile(g_cgContext,
											   CG_SOURCE,
											   g_strPixelProgramFileName,
											   g_cgPixelProfile,
											   g_strPixelProgramName,
											   profileOpts);
	checkForCgError("Createing pixel program form file");

	//get variable handle
	g_cgWorldMatrix = cgGetNamedParameter(g_cgVertexProgram, "worldMatrix");
	g_cgWorldViewProjMatrix = cgGetNamedParameter(g_cgVertexProgram, "worldViewProjMatrix");

	g_cgAmbient = cgGetNamedParameter(g_cgPixelProgram, "ambient");
	g_cgLightColor = cgGetNamedParameter(g_cgPixelProgram, "lightColor");
	g_cgLightPos = cgGetNamedParameter(g_cgPixelProgram, "lightPos");
	g_cgLightAttenuation = cgGetNamedParameter(g_cgPixelProgram, "lightAttenuation");
	g_cgEyePos = cgGetNamedParameter(g_cgPixelProgram, "eyePos");
	g_cgKe = cgGetNamedParameter(g_cgPixelProgram, "ke");
	g_cgKa = cgGetNamedParameter(g_cgPixelProgram, "ka");
	g_cgKd = cgGetNamedParameter(g_cgPixelProgram, "kd");
	g_cgKs = cgGetNamedParameter(g_cgPixelProgram, "ks");
	g_cgShininess = cgGetNamedParameter(g_cgPixelProgram, "shininess");

	assert(g_cgWorldMatrix && g_cgWorldViewProjMatrix && g_cgAmbient &&
		   g_cgLightColor && g_cgLightPos && g_cgEyePos &&
		   g_cgKe && g_cgKa && g_cgKd && g_cgKs && g_cgShininess);

}

bool init(IDirect3DDevice9 * pDevice)
{
	HRESULT hr;
	//hr = D3DXCreateSphere(pDevice, 1.f, 30, 20, &g_pMeshSphere, nullptr);
	hr = D3DXCreateTeapot(pDevice, &g_pMeshSphere, nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建球网格数据失败"), NULL, MB_OK);
		return false;
	}

	//set matrix//
	//set view 
	D3DXVECTOR3 pos(-1.f, 0.f, -3.f);
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
	D3DXMatrixRotationX(&temp, D3DX_PI / 8.f);
	g_matrixWorld = temp;
	D3DXMatrixRotationY(&temp, D3DX_PI / 5.f);
	g_matrixWorld *= temp;
	D3DXMatrixRotationZ(&temp, D3DX_PI / 4.f);
	g_matrixWorld *= temp;
	D3DXMatrixTranslation(&temp, 0.5f, 0.3f, 2.f);
	g_matrixWorld *= temp;

	//set worldViewProj matrix
	g_matrixWorldViewProj = g_matrixWorld * g_matrixView * g_matrixProjection;



	/*  init light */
	memset(&g_light, 0, sizeof(g_light));
	g_light.Type = D3DLIGHT_POINT;
	g_light.Ambient = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);
	g_light.Diffuse = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);
	g_light.Specular = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);
	g_light.Position = D3DXVECTOR3(-3.f, -1.f, -1.f);
	g_light.Range = 10000.f;
	g_light.Attenuation0 = 1.f;
	g_light.Attenuation1 = .4f;

	//inint material
	memset(&g_material, 0, sizeof(g_material));
	g_material.Ambient = D3DXCOLOR(0.2f, .2f, .2f, 1.f);
	g_material.Diffuse = D3DXCOLOR(0.4f, .2f, .7f, 1.f);
	g_material.Specular = D3DXCOLOR(0.2f, .7f, .2f, 1.f);
	g_material.Power = 10.f;

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
	checkForCgError("loading pixel program");

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
#if 0
		pDev->SetTransform(D3DTS_WORLD, &g_matrixWorld);
		pDev->SetTransform(D3DTS_VIEW, &g_matrixView);
		pDev->SetTransform(D3DTS_PROJECTION, &g_matrixProjection);
		pDev->SetLight(0, &g_light);
		pDev->LightEnable(0, TRUE);
		pDev->SetMaterial(&g_material);
		//pDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		pDev->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
#else
		cgD3D9BindProgram(g_cgVertexProgram);
		checkForCgError("binding vertex program");
		cgD3D9BindProgram(g_cgPixelProgram);
		checkForCgError("binding pixel program");

		cgSetParameterValuefc(g_cgWorldMatrix, 16, g_matrixWorld.m[0]);
		cgSetParameterValuefc(g_cgWorldViewProjMatrix, 16, g_matrixWorldViewProj.m[0]);
		cgSetParameterValuefc(g_cgAmbient, 3, (float*)&g_light.Ambient);
		cgSetParameterValuefc(g_cgLightColor, 3, (float*)&g_light.Diffuse);
		cgSetParameterValuefc(g_cgLightPos, 3, (float*)&g_light.Position);
		cgSetParameter3f(g_cgLightAttenuation, g_light.Attenuation0, g_light.Attenuation1, g_light.Attenuation2);
		cgSetParameterValuefc(g_cgEyePos, 3, (float*)&g_eyePos);
		cgSetParameterValuefc(g_cgKe, 3, (float*)&g_material.Emissive);
		cgSetParameterValuefc(g_cgKa, 3, (float*)&g_material.Ambient);
		cgSetParameterValuefc(g_cgKd, 3, (float*)&g_material.Diffuse);
		cgSetParameterValuefc(g_cgKs, 3, (float*)&g_material.Specular);
		cgSetParameterValuefc(g_cgShininess, 3, &g_material.Power);
#endif

		g_pMeshSphere->DrawSubset(0);
		pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	g_pMeshSphere->Release();
	cgD3D9SetDevice(NULL);
}
