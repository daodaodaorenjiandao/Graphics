/*
重影效果的原理是：每个像素的颜色值不断发生改变，颜色值的来源是根据纹理坐标获取的，而获取纹理坐标的
偏移量却是在动态的改变，这样就达到了颜色值的改变。注意这个配合多个纹理坐标才达到目的的，最后根据每
个纹理进行采样，然后对颜色值进行合成，就达到了重影效果！
*/

#include <windows.h>
#include <stdio.h>
#include <d3d9.h>     /* Can't include this?  Is DirectX SDK installed? */
#include <d3dx9.h>
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
#pragma comment(lib,"d3dx9.lib")


static CGcontext g_cgContext;
static CGprofile g_cgVertexProfile;
static CGprogram g_cgVertexProgram;

static CGprofile g_cgPixelProfile;
static CGprogram g_cgPixelProgram;

//vertex program
static CGparameter g_cgParamSeparation0;	//
static CGparameter g_cgParamSeparation1;

//fragment program
static CGparameter g_cgParamTexture;		//纹理采样器句柄

float g_separation = 0.f;

static LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL;
static IDirect3DTexture9 * g_pTexture = nullptr;		//纹理

bool g_bMove = true;

//unsigned 
const unsigned char g_textureData[3 * (128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 + 1 * 1)] =
{
#include "imageData.h"
};

static const WCHAR *g_strWindowTitleW = L"texture sampler";	//程序名
static const char  *g_strWindowTitle = "texture sampler";
static const char  *g_strVertexProgramFileName = "doubleTexture.cg";//着色程序文件名
static const char  *g_strPixelProgramFileName = "doubleTexture.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "VS_main";			//着色程序入口函数名
static const char  *g_strPiexlProgramName = "PS_main";			//片段程序

//定义顶点结构
struct Vertex
{
	Vertex(float x, float y, float z, float u, float v)
	{
		_x = x;
		_y = y;
		_z = z;
		_u = u;
		_v = v;
	}
	float _x, _y, _z;
	float _u, _v;
	static const DWORD FVF;
};

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_TEX1;	//使用纹理标记

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

static HRESULT CALLBACK OnResetDevice(IDirect3DDevice9* pDev,
									  const D3DSURFACE_DESC* backBuf,
									  void* userContext)
{
	//init d3d
	HRESULT hr;
	hr = pDev->CreateVertexBuffer(sizeof(Vertex)* 3,	//创建3个顶点
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
	pVertexs[0] = Vertex(-0.8f, 0.8f, 0.f, 0.f, 0.f);
	pVertexs[1] = Vertex(0.8f, 0.8f, 0.f, 1.f, 0.f);
	pVertexs[2] = Vertex(0.f, -0.8f, 0, .5f, 1.f);
	g_pVertexBuffer->Unlock();

	//创建纹理数据
	/*hr = D3DXCreateTexture(pDev,
						   128,
						   128,
						   0,
						   D3DUSAGE_AUTOGENMIPMAP,
						   D3DFMT_X8R8G8B8,
						   D3DPOOL_MANAGED,
						   &g_pTexture);*/
	hr = pDev->CreateTexture(128, 128, 0, D3DUSAGE_AUTOGENMIPMAP,
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
	//end init d3d

	//init cg
	cgD3D9SetDevice(pDev);
	static bool bFirst = true;
	if (bFirst)
	{
		bFirst = false;
		//init 

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

		g_cgParamSeparation0 = cgGetNamedParameter(g_cgVertexProgram, "separation0");
		g_cgParamSeparation1 = cgGetNamedParameter(g_cgVertexProgram, "separation1");

		//创建片段程序
		g_cgPixelProfile = cgD3D9GetLatestPixelProfile();
		optimalOptions = cgD3D9GetOptimalOptions(g_cgPixelProfile);
		checkForCgError("get pixel optimalOptions fail");

		g_cgPixelProgram = cgCreateProgramFromFile(g_cgContext,
												   CG_SOURCE,
												   g_strPixelProgramFileName,
												   g_cgPixelProfile,
												   g_strPiexlProgramName,
												   optimalOptions);
		checkForCgError("create pixel program fail");

		//get handle of uniform parameter  color;
		g_cgParamTexture = cgGetNamedParameter(g_cgPixelProgram, "textureSampler");
	}
	cgD3D9LoadProgram(g_cgVertexProgram, FALSE, 0);
	checkForCgError("load program failed");

	cgD3D9LoadProgram(g_cgPixelProgram, FALSE, 0);
	checkForCgError("load pixel program fail");
	return S_OK;
}

static void CALLBACK OnFrameMove(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
	static float delta = 0.005f;
	if (g_bMove)
	{
		if (g_separation > .5f || g_separation < -0.5f)
			delta *= -1;
		g_separation += delta;
	}
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
		cgD3D9BindProgram(g_cgVertexProgram);		//设置顶点程序
		cgSetParameter2f(g_cgParamSeparation0, -g_separation, 0);
		cgSetParameter2f(g_cgParamSeparation1, g_separation, 0);
		//checkForCgError("set cg texture fail");

		cgD3D9BindProgram(g_cgPixelProgram);
		cgD3D9SetTexture(g_cgParamTexture, g_pTexture);	//通过句柄设置纹理参数
		checkForCgError("set cg texture fail");

		pDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		pDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		hr = pDev->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
		hr = pDev->SetFVF(Vertex::FVF);
		//pDev->SetTexture(0, g_pTexture);
		//pDev->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = pDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
		hr = pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	if (g_pVertexBuffer) { g_pVertexBuffer->Release(); g_pVertexBuffer = nullptr; }
	if (g_pTexture) { g_pTexture->Release(); g_pTexture = nullptr; }
	cgD3D9SetDevice(NULL);
}

static void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	if (bKeyDown)
	{
		switch (nChar)
		{
		case ' ':
			g_bMove = !g_bMove;
			break;
		}
	}
}