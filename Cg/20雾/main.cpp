#define _CRT_SECURE_NO_WARNINGS

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
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib,"miniDXUT.lib")		//DXUT


static CGcontext g_cgContext;
static CGprofile g_cgVertexProfile;
static CGprogram g_cgVertexProgram;
static CGprofile g_cgPixelProfile;
static CGprogram g_cgPixelProgram;

//vertex program
static CGparameter g_cgWorldView;
static CGparameter g_cgWorldViewProj;
//pixel program


static LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = nullptr;

static const WCHAR *g_strWindowTitleW = L"fog";	//程序名
static const char  *g_strWindowTitle = "fog";
static const char  *g_strVertexProgramFileName = "fog.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "vs_main";			//着色程序入口函数名
static const char  *g_strPixelProgramFileName = "fog.cg";
static const char  *g_strPixelProgramName = "ps_main";

D3DXMATRIX g_worldViewProj;

static const int g_numMesh = 10;

struct Cylinder
{
	ID3DXMesh * pMesh;						//物体网格数据
	D3DXMATRIX transformForWorldView;		//world && view
	D3DXMATRIX transformForWorldViewProj;	//world && view && proj
};
Cylinder g_cylinders[g_numMesh];

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
	checkForCgError("getting vertex latest profile");

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
	checkForCgError("getting pixel latest progile");

	profileOpts = cgD3D9GetOptimalOptions(g_cgPixelProfile);
	checkForCgError("getting pixel profile options");

	g_cgPixelProgram = cgCreateProgramFromFile(g_cgContext,
											   CG_SOURCE,
											   g_strPixelProgramFileName,
											   g_cgPixelProfile,
											   g_strPixelProgramName, profileOpts);
	checkForCgError("create pixel program from file");


	//get vertex program parameter
	g_cgWorldView = cgGetNamedParameter(g_cgVertexProgram, "worldView");
	g_cgWorldViewProj = cgGetNamedParameter(g_cgVertexProgram, "worldViewProj");
	assert(g_cgWorldView && g_cgWorldViewProj);

	//get pixel program parameter
	
	
}

HRESULT init(IDirect3DDevice9 * pDevice)
{
	HRESULT hr;
	//set view and projection
	D3DXMATRIX view, proj;

	D3DXVECTOR3 pos(0.f, 0.f, -15.f);		//注意裁剪平面！！否则物体被裁剪不能显示
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);

	D3DXMatrixLookAtLH(&view, &pos, &target, &up);
	pDevice->SetTransform(D3DTS_VIEW, &view);

	D3DXMatrixPerspectiveFovLH(&proj,
							   D3DX_PI / 4.f,
							   1.f,//400.f / 400.f,
							   1.f,
							   1000.f);
	pDevice->SetTransform(D3DTS_PROJECTION, &proj);

	g_worldViewProj = view * proj;		// set world view projction matirx

	//创建圆柱体
	float x, y, z;
	float deltaX, deltaZ;
	x = -4;
	y = 0;
	z = 0;
	deltaX = 1.f;
	deltaZ = 3.f;

	for (int i = 0; i < g_numMesh; ++i)
	{
		//创建网格数据
		hr = D3DXCreateCylinder(pDevice,
								.2f,
								.2f,
								4.f,
								20,
								10,
								&(g_cylinders[i].pMesh),
								nullptr);
		if (FAILED(hr))
		{
			MessageBox(NULL, _T("创建网格数据失败"), NULL, MB_OK);
			return false;
		}
		//设置世界变换矩阵
		D3DXMATRIX matrixRotation;
		D3DXMATRIX matrixTranslation;
		D3DXMatrixRotationX(&matrixRotation, D3DX_PI / 2);
		D3DXMatrixTranslation(&matrixTranslation,
							  x,
							  y,
							  z);
		g_cylinders[i].transformForWorldView = matrixRotation * matrixTranslation * view;		//world && view
		g_cylinders[i].transformForWorldViewProj = g_cylinders[i].transformForWorldView * proj;	//world && view && proj

		x += deltaX;
		z += deltaZ;
		if (i == g_numMesh / 2 - 1)
		{
			z -= deltaZ;
			deltaZ *= -1;
		}
	}
	
	return S_OK;
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

	return init(pDev);
}

static void CALLBACK OnFrameRender(IDirect3DDevice9* pDev,
								   double time,
								   float elapsedTime,
								   void* userContext)
{
	pDev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
				D3DXCOLOR(0.1f, 0.3f, 0.6f, 0.0f), 1.0f, 0);

	if (SUCCEEDED(pDev->BeginScene())) 
	{
		
		//cgD3D9BindProgram(g_cgVertexProgram);			//只能画出一个物体
		//checkForCgError("binding vertex program");
		//cgD3D9BindProgram(g_cgPixelProgram);
		//checkForCgError("binding pixel program");

		for (int i = 0; i < g_numMesh; ++i)
		{
			cgD3D9BindProgram(g_cgVertexProgram);				//此段代码如果放在外面的话，就只会画一个物体出来，所以每画一个物体，都需要重新绑定顶点程序和片段程序
			checkForCgError("binding vertex program");
			cgD3D9BindProgram(g_cgPixelProgram);
			checkForCgError("binding pixel program");

			cgSetParameterValuefc(g_cgWorldView, 16, &g_cylinders[i].transformForWorldView[0]);
			cgSetParameterValuefc(g_cgWorldViewProj, 16, &g_cylinders[i].transformForWorldViewProj[0]);
			g_cylinders[i].pMesh->DrawSubset(0);
		}

		pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	if (g_pVertexBuffer){ g_pVertexBuffer->Release(); g_pVertexBuffer = nullptr; }
	for (int i = 0; i < g_numMesh; ++i)
	{
		g_cylinders[i].pMesh->Release();
	}
	cgD3D9SetDevice(NULL);
}
