#define _CRT_SECURE_NO_WARNINGS
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

CGcontext g_cgContext;
CGprofile g_cgVertexProfile;
CGprofile g_cgPixelProfile;


static LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = nullptr;
static IDirect3DTexture9 * g_pTexture = nullptr;		//纹理
static IDirect3DTexture9 * g_pShadowTexture = nullptr;	//深度纹理

static const WCHAR *g_strWindowTitleW = L"ProjfTex";	//程序名
static const char  *g_strWindowTitle = "ProjfTex";

static const char  *g_strVertexProgramFileName = "projTex.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "main_vs";			//着色程序入口函数名

static const char *g_strPixelProgramFileName = "projTex.cg";
static const char *g_strPixelProgramName = "main_ps";


struct ObjectInfo
{
	ID3DXMesh * pMesh;
	D3DXMATRIX matrixWrold;					//世界变换矩阵
	D3DXMATRIX matrixWorldViewProj;			//投影矩阵
	D3DXMATRIX matrixWorldViewProjTex;		//纹理投影矩阵

	CGprogram g_cgVertexProgram;
	CGprogram g_cgPixelProgram;

	//shader handle
	CGparameter g_cgWorldMatrix;
	CGparameter g_cgWorldViewProjMatrix;
	CGparameter g_cgWorldViewProjMatrixTex;
	CGparameter g_cgLightPos;
	CGparameter g_cgTexture;
	CGparameter g_cgShadowMapTexture;
};


ObjectInfo g_objectsInfo[2];

//for palne
CGprogram g_cgVertexProgram;
CGprogram g_cgPixelProgram;

//shader handle
CGparameter g_cgWorldMatrix;
CGparameter g_cgWorldViewProjMatrix;
CGparameter g_cgWorldViewProjMatrixTex;
CGparameter g_cgLightPos;
CGparameter g_cgTexture;
CGparameter g_cgShadowMapTexture;

struct Vertex
{
	Vertex(float x, float y, float z, float nx, float ny, float nz)
	{
		_x = x; _y = y; _z = z;
		_nx = nx; _ny = ny; _nz = nz;
	}
	float _x, _y, _z;
	float _nx, _ny, _nz;
	const static DWORD FVF;
};

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL;

//for vertex buffer
D3DXMATRIX g_matrixWorld;
D3DXMATRIX g_matrixWorldViewProj;
D3DXMATRIX g_matrixWorldViewProjTex;

D3DXMATRIX g_matrixLightView;
D3DXMATRIX g_matrixLightProj;

//matrix

D3DXVECTOR3 g_eyePos;
D3DXVECTOR3 g_lightPos;



//unsigned 
const unsigned char g_textureData[3 * (128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 + 1 * 1)] =
{
#include "imageData.h"
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
 	cgDestroyProgram(g_cgPixelProgram);
 	checkForCgError("destroying pixel program");

	for (int i = 0; i < 2; ++i)
	{
		cgDestroyProgram(g_objectsInfo[i].g_cgVertexProgram);
		checkForCgError("destroying vertex program");
		cgDestroyProgram(g_objectsInfo[i].g_cgPixelProgram);
		checkForCgError("destroying pixel program");
	}
	
	cgDestroyContext(g_cgContext);

	return DXUTGetExitCode();
}

static void createCgPrograms()
{
	const char **profileOptsVertex;
	const char **profileOptsPixel;

	/* Determine the best profile once a device to be set. */
	g_cgVertexProfile = cgD3D9GetLatestVertexProfile();
	checkForCgError("getting latest profile");

	g_cgPixelProfile = cgD3D9GetLatestPixelProfile();
	checkForCgError("getting latest profile");

	profileOptsVertex = cgD3D9GetOptimalOptions(g_cgVertexProfile);
	checkForCgError("getting latest profile options");

	profileOptsPixel = cgD3D9GetOptimalOptions(g_cgPixelProfile);
	checkForCgError("getting latest profile options");

	//for plane
 	g_cgVertexProgram =
 		cgCreateProgramFromFile(
 		g_cgContext,              /* Cg runtime context */
 		CG_SOURCE,                /* Program in human-readable form */
 		g_strVertexProgramFileName,/* Name of file containing program */
 		g_cgVertexProfile,        /* Profile: OpenGL ARB vertex program */
 		g_strVertexProgramName,   /* Entry function name */
 		profileOptsVertex);             /* Pass optimal compiler options */
 	checkForCgError("creating vertex program from file");
 
 
 	g_cgPixelProgram = cgCreateProgramFromFile(g_cgContext,
 											   CG_SOURCE,
 											   g_strPixelProgramFileName,
 											   g_cgPixelProfile,
 											   g_strPixelProgramName,
 											   profileOptsPixel);
 
 	//get variable handle
 	g_cgWorldMatrix = cgGetNamedParameter(g_cgVertexProgram, "worldMatrix");
 	g_cgWorldViewProjMatrix = cgGetNamedParameter(g_cgVertexProgram, "worldViewProj");
 	g_cgWorldViewProjMatrixTex = cgGetNamedParameter(g_cgVertexProgram, "worldViewProjTex");
 	g_cgLightPos = cgGetNamedParameter(g_cgVertexProgram, "lightPos");
 	assert(g_cgWorldViewProjMatrix && g_cgWorldViewProjMatrixTex && g_cgWorldMatrix && g_cgLightPos);
 
 	g_cgTexture = cgGetNamedParameter(g_cgPixelProgram, "samplerTex");
 	g_cgShadowMapTexture = cgGetNamedParameter(g_cgPixelProgram, "samplerShadowMap");
 	assert(g_cgTexture && g_cgShadowMapTexture);

	//for object  : sphere and teapot
	for (int i = 0; i < 2; ++i)
	{
		g_objectsInfo[i].g_cgVertexProgram =
			cgCreateProgramFromFile(
			g_cgContext,              /* Cg runtime context */
			CG_SOURCE,                /* Program in human-readable form */
			g_strVertexProgramFileName,/* Name of file containing program */
			g_cgVertexProfile,        /* Profile: OpenGL ARB vertex program */
			g_strVertexProgramName,   /* Entry function name */
			profileOptsVertex);             /* Pass optimal compiler options */
		checkForCgError("creating vertex program from file");


		g_objectsInfo[i].g_cgPixelProgram = cgCreateProgramFromFile(g_cgContext,
												   CG_SOURCE,
												   g_strPixelProgramFileName,
												   g_cgPixelProfile,
												   g_strPixelProgramName,
												   profileOptsPixel);

		//get variable handle
		g_objectsInfo[i].g_cgWorldMatrix = cgGetNamedParameter(g_objectsInfo[i].g_cgVertexProgram, "worldMatrix");
		g_objectsInfo[i].g_cgWorldViewProjMatrix = cgGetNamedParameter(g_objectsInfo[i].g_cgVertexProgram, "worldViewProj");
		g_objectsInfo[i].g_cgWorldViewProjMatrixTex = cgGetNamedParameter(g_objectsInfo[i].g_cgVertexProgram, "worldViewProjTex");
		g_objectsInfo[i].g_cgLightPos = cgGetNamedParameter(g_objectsInfo[i].g_cgVertexProgram, "lightPos");
		assert(g_objectsInfo[i].g_cgWorldViewProjMatrix && 
			   g_objectsInfo[i].g_cgWorldViewProjMatrixTex && 
			   g_objectsInfo[i].g_cgWorldMatrix && 
			   g_objectsInfo[i].g_cgLightPos);

		g_objectsInfo[i].g_cgTexture = cgGetNamedParameter(g_objectsInfo[i].g_cgPixelProgram, "samplerTex");
		g_objectsInfo[i].g_cgShadowMapTexture = cgGetNamedParameter(g_objectsInfo[i].g_cgPixelProgram, "samplerShadowMap");
		assert(g_objectsInfo[i].g_cgTexture && 
			   g_objectsInfo[i].g_cgShadowMapTexture);
	}
	

}

bool init(IDirect3DDevice9 * pDevice)
{
	HRESULT hr;

	//set matrix//
	//set view --camera
	D3DXVECTOR3 pos(0.f, 3.f, -15.f);
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);
	D3DXMATRIX matrixView;
	D3DXMatrixLookAtLH(&matrixView, &pos, &target, &up);
	g_eyePos = pos;

	//set projection
	D3DXMATRIX matrixProj;
	D3DXMatrixPerspectiveFovLH(&matrixProj,
							   D3DX_PI / 4.f,
							   1.f,/* 400.f / 400.f */
							   1.f,
							   1000.f);
	
	//init light pos
	g_lightPos = D3DXVECTOR3(21.5f,25.f, 0.0f);		//将光的位置作为投影仪的位置
	//g_lightPos = D3DXVECTOR3(1.5f, 25.f, 0.0f);		//将光的位置作为投影仪的位置

	D3DXMATRIX matrixLightView;
	D3DXMATRIX matrixLightProj;

	D3DXVECTOR3 target1(-5.f, 0.f, 8.f);
	up = D3DXVECTOR3(0, 1, 0);
	D3DXMatrixLookAtLH(&matrixLightView, &g_lightPos, &target1, &up);
	D3DXMatrixPerspectiveFovLH(&matrixLightProj, D3DX_PI / 4.f, 1.f, 1.f, 1000.f);
	g_matrixLightView = matrixLightView;
	g_matrixLightProj = matrixLightProj;

	//创建对象的网格数据
	hr = D3DXCreateSphere(pDevice, 1.f, 30, 20, &(g_objectsInfo[0].pMesh), nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建球网格数据失败"), NULL, MB_OK);
		return false;
	}
	hr = D3DXCreateTeapot(pDevice, &(g_objectsInfo[1].pMesh), nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建茶壶网格数据失败"), NULL, MB_OK);
		return false;
	}
	D3DXMATRIX matrixHalf;
	D3DXMatrixIdentity(&matrixHalf);
	matrixHalf.m[0][0] = matrixHalf.m[1][1] = matrixHalf.m[2][2] = matrixHalf.m[3][3] = 0.5f;
	matrixHalf.m[3][0] = matrixHalf.m[3][1] = matrixHalf.m[3][2] = matrixHalf.m[3][2] = 0.5f;

	//set world matrix
	D3DXMATRIX temp;

	//sphere
	D3DXMatrixIdentity(&temp);
	D3DXMatrixTranslation(&temp, 1.5f, 3.0f, 2.f);
	g_objectsInfo[0].matrixWrold = temp;
	g_objectsInfo[0].matrixWorldViewProj = temp * matrixView * matrixProj;
	//g_objectsInfo[0].matrixWorldViewProjTex = temp * matrixLightView * matrixLightProj;	//之前未乘以1/2，导致投影的纹理坐标在[-1,1]范围内
	g_objectsInfo[0].matrixWorldViewProjTex = temp * matrixLightView * matrixLightProj * matrixLightProj;

	//teapot
	D3DXMatrixIdentity(&temp);
	D3DXMatrixTranslation(&temp, -2.5f, 1.3f, -1.f);
	g_objectsInfo[1].matrixWrold = temp;
	g_objectsInfo[1].matrixWorldViewProj = temp * matrixView * matrixProj;
	//g_objectsInfo[1].matrixWorldViewProjTex = temp * matrixLightView * matrixLightProj;
	g_objectsInfo[1].matrixWorldViewProjTex = temp * matrixLightView * matrixLightProj * matrixHalf;
	
	//create vertex buffer
	hr = pDevice->CreateVertexBuffer(sizeof(Vertex) * 4,
								  D3DUSAGE_DYNAMIC,
								  Vertex::FVF,
								  D3DPOOL_DEFAULT,
								  &g_pVertexBuffer,
								  nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点缓存失败!"), NULL, MB_OK);
		return false;
	}
	//init vertex buffer
	Vertex * pVertex = nullptr;
	g_pVertexBuffer->Lock(0, 0, (void**)&pVertex, 0);
	// triangle sequence is 123 243
	*pVertex = Vertex(-5, 0, 5, 0, 1, 0);
	*(pVertex + 1) = Vertex(5, 0, 5, 0, 1, 0);
	*(pVertex + 2) = Vertex(-5, 0, -5, 0, 1, 0);
	*(pVertex + 3) = Vertex(5, 0, -5, 0, 1, 0);
	g_pVertexBuffer->Unlock();

	D3DXMatrixIdentity(&temp);
	g_matrixWorld = temp;
	g_matrixWorldViewProj = temp * matrixView * matrixProj;
	//g_matrixWorldViewProjTex = temp * matrixLightView * matrixLightProj;
	g_matrixWorldViewProjTex = temp * matrixLightView * matrixLightProj * matrixHalf;

	//创建纹理数据
	hr = pDevice->CreateTexture(128, 128, 0, D3DUSAGE_AUTOGENMIPMAP,
							 D3DFMT_X8R8G8B8,
							 D3DPOOL_MANAGED,
							 &g_pTexture,
							 nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建纹理数据失败"), NULL, MB_OK);
		return false;
	}
	//初始化纹理数据
	D3DLOCKED_RECT lockedRect;
	hr = g_pTexture->LockRect(0, &lockedRect, 0, 0);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("lock rect fail"), NULL, MB_OK);
		return false;
	}
	DWORD *pData = (DWORD*)lockedRect.pBits;
	for (int i = 0; i < 128 * 128 * 3; i += 3)
	{
		*pData++ = g_textureData[i] << 16 |
			g_textureData[i + 1] << 8 |
			g_textureData[i + 2];
	}
	g_pTexture->UnlockRect(0);
	
	//创建深度纹理
	//1 获取后台缓存信息
	IDirect3DSurface9 * pSurface = nullptr;
	hr = pDevice->GetRenderTarget(0, &pSurface);		//ref count +1
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("获取RenderTarget failed"), NULL, MB_OK);
		return false;
	}
	D3DSURFACE_DESC desc;
	pSurface->GetDesc(&desc);
	pSurface->Release();								//ref count -1
	
	int texWidth, texHeight;
	texWidth = desc.Width;
	texHeight = desc.Height;
	//2 根据后台缓存大小创建深度纹理
	hr = pDevice->CreateTexture(texWidth,
								texHeight,
								1,
								D3DUSAGE_DEPTHSTENCIL,
								D3DFMT_D16,
								D3DPOOL_DEFAULT,
								&g_pShadowTexture,
								nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建深度纹理失败！"), NULL, MB_OK);
		return false;
	}
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
	checkForCgError("loading vertex program");

	for (int i = 0; i < 2; ++i)
	{
		cgD3D9LoadProgram(g_objectsInfo[i].g_cgVertexProgram, false, 0);
		checkForCgError("loading vertex program");
		cgD3D9LoadProgram(g_objectsInfo[i].g_cgPixelProgram, false, 0);
		checkForCgError("loading vertex program");
	}
	
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
	HRESULT hr;
	pDev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
				D3DXCOLOR(0.1f, 0.3f, 0.6f, 0.0f), 1.0f, 0);

	if (SUCCEEDED(pDev->BeginScene())) 
	{
		//write depth buffer
		pDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);	//在写入深度值时，可以关闭颜色的写入，提高效率,但后面要开启
		D3DXMATRIX identiey;
		D3DXMatrixIdentity(&identiey);

		pDev->SetTransform(D3DTS_WORLD, &identiey);
		pDev->SetTransform(D3DTS_WORLD, &g_matrixWorld);
		pDev->SetTransform(D3DTS_VIEW, &g_matrixLightView);
		pDev->SetTransform(D3DTS_PROJECTION, &g_matrixLightProj);

		IDirect3DSurface9 * pOldDepthSurface = nullptr;
		IDirect3DSurface9 * pNewDepthSurface = nullptr;
		hr = pDev->GetDepthStencilSurface(&pOldDepthSurface);
		assert(SUCCEEDED(hr));
		hr = g_pShadowTexture->GetSurfaceLevel(0, &pNewDepthSurface);		//顶层纹理是0，先前这里犯了个错误，设置了1，导致失败
		assert(SUCCEEDED(hr));
		hr = pDev->SetDepthStencilSurface(pNewDepthSurface);				//设置新的深度缓存
		assert(SUCCEEDED(hr));
		pDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.f, 0);			//***	//重置深度缓存的值，以免影响正确的结果

		pDev->SetTransform(D3DTS_WORLD, &g_matrixWorld);
		pDev->SetFVF(Vertex::FVF);
		pDev->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
		//pDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

		int i = 1;
		for (int i = 0; i < sizeof(g_objectsInfo) / sizeof(g_objectsInfo[0]); ++i)
		{
			pDev->SetTransform(D3DTS_WORLD, &g_objectsInfo[i].matrixWrold);
			g_objectsInfo[i].pMesh->DrawSubset(0);
		}
		
		pDev->SetDepthStencilSurface(pOldDepthSurface);					//设置旧的深度模版缓存
		pOldDepthSurface->Release();
		pNewDepthSurface->Release();

		//pDev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,		//** 由于没进制颜色的写入，所以需要禁止
		//			D3DXCOLOR(0.1f, 0.3f, 0.6f, 0.0f), 1.0f, 0);
		pDev->SetRenderState(D3DRS_COLORWRITEENABLE, 					//开启颜色写
							 D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);		
//////////////////////////////////////////////////////////////////////////////////////////////////
		//for vertex buffer
		cgD3D9BindProgram(g_cgVertexProgram);				//不同物体使用同一个顶点程序时，需要用不同的句柄绑定，不能使用同一个句柄？
		checkForCgError("binding vertex program");
		cgD3D9BindProgram(g_cgPixelProgram);
		checkForCgError("binding pixel program");

		cgSetParameterValuefc(g_cgWorldMatrix, 16, g_matrixWorld.m[0]);
		cgSetParameterValuefc(g_cgWorldViewProjMatrix, 16, g_matrixWorldViewProj.m[0]);
		cgSetParameterValuefc(g_cgWorldViewProjMatrixTex, 16, g_matrixWorldViewProjTex.m[0]);
		cgSetParameter3f(g_cgLightPos, g_lightPos.x, g_lightPos.y, g_lightPos.z);
		cgD3D9SetTexture(g_cgTexture, g_pTexture);
		cgD3D9SetTexture(g_cgShadowMapTexture, g_pShadowTexture);

		pDev->SetFVF(Vertex::FVF);
		pDev->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
		pDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		
		//for mesh
		for (int i = 0; i < sizeof(g_objectsInfo) / sizeof(g_objectsInfo[0]); ++i)
		{
			cgD3D9BindProgram(g_objectsInfo[i].g_cgVertexProgram);				//不同物体使用同一个顶点程序时，需要用不同的句柄绑定，不能使用同一个句柄？
			checkForCgError("binding vertex program");
			cgD3D9BindProgram(g_objectsInfo[i].g_cgPixelProgram);
			checkForCgError("binding pixel program");

			cgSetParameterValuefc(g_objectsInfo[i].g_cgWorldMatrix, 16, g_objectsInfo[i].matrixWrold.m[0]);
			cgSetParameterValuefc(g_objectsInfo[i].g_cgWorldViewProjMatrix, 16, g_objectsInfo[i].matrixWorldViewProj.m[0]);
			cgSetParameterValuefc(g_objectsInfo[i].g_cgWorldViewProjMatrixTex, 16, g_objectsInfo[i].matrixWorldViewProjTex.m[0]);
			cgSetParameter3f(g_objectsInfo[i].g_cgLightPos, g_lightPos.x, g_lightPos.y, g_lightPos.z);
			cgD3D9SetTexture(g_objectsInfo[i].g_cgTexture, g_pTexture);
			cgD3D9SetTexture(g_objectsInfo[i].g_cgShadowMapTexture, g_pShadowTexture);

			g_objectsInfo[i].pMesh->DrawSubset(0);
		}

		//!!!!!!!!!!!!
		//渲染出来的结果不正确：光的位置和影子的位置不合理

		pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	for (int i = 0; i < sizeof(g_objectsInfo) / sizeof(g_objectsInfo[0]); ++i)
	{
		if (g_objectsInfo[i].pMesh)
		{
			g_objectsInfo[i].pMesh->Release();
			g_objectsInfo[i].pMesh = nullptr;
		}
	}		
	g_pTexture->Release();
	g_pVertexBuffer->Release();
	g_pShadowTexture->Release();
	cgD3D9SetDevice(NULL);
}
