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
static CGparameter g_cgWorldViewProj;
static CGparameter g_cgLightPos;
static CGparameter g_cgEyePos;
//pixel program
static CGparameter g_cgNormalTexture;
static CGparameter g_cgCubeTexture;
static CGparameter g_cgPower;

static LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = nullptr;
static IDirect3DTexture9 * g_pNormalTexture = nullptr;
static IDirect3DCubeTexture9 * g_pCubeTexture = nullptr;

static const WCHAR *g_strWindowTitleW = L"bump";	//程序名
static const char  *g_strWindowTitle = "bump";
static const char  *g_strVertexProgramFileName = "bump.cg";//着色程序文件名
static const char  *g_strVertexProgramName = "vs_main";			//着色程序入口函数名
static const char  *g_strPixelProgramFileName = "bump.cg";
static const char  *g_strPixelProgramName = "ps_main";

D3DXVECTOR3 g_lightPos;
D3DXVECTOR3 g_eyePos;
D3DXMATRIX g_worldViewProj;
float g_power = 8.f;

static const unsigned char
g_brickNormalMapImage[3 * (128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 + 1 * 1)] = {
/* RGB8 image data for a mipmapped 128x128 normal map for a brick pattern */
#include "brick_image.h"
};

static const unsigned char
g_cubeMapImage[6 * 3 * 32 * 32] = {
	/* RGB8 image data for a normalization vector cube map with 32x32 faces */
#include "normcm_image.h"
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


//定义灵活顶点结构
struct Vertex
{
	Vertex(float x, float y, float z, float u, float v)
	{
		_x = x; _y = y; _z = z;
		_u = u; _v = v;
	}

	float _x, _y, _z;
	float _u, _v;
	static const DWORD FVF;
};

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_TEX1;

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
	g_cgWorldViewProj = cgGetNamedParameter(g_cgVertexProgram, "worldViewProj");
	g_cgLightPos = cgGetNamedParameter(g_cgVertexProgram, "lightPos");
	g_cgEyePos = cgGetNamedParameter(g_cgVertexProgram, "eyePos");
	assert(g_cgWorldViewProj && g_cgLightPos && g_cgEyePos);

	//get pixel program parameter
	g_cgNormalTexture = cgGetNamedParameter(g_cgPixelProgram, "samplerTexture");
	g_cgCubeTexture = cgGetNamedParameter(g_cgPixelProgram, "samplerCubeTexture");
	g_cgPower = cgGetNamedParameter(g_cgPixelProgram, "power");
	assert(g_cgNormalTexture && g_cgCubeTexture && g_cgPower);
}

static HRESULT initVertexBuffer(IDirect3DDevice9* pDev)
{
	//create vertex buffer
	HRESULT hr;
	hr = pDev->CreateVertexBuffer(sizeof(Vertex)* 4,
								  D3DUSAGE_DYNAMIC,
								  Vertex::FVF,
								  D3DPOOL_DEFAULT,
								  &g_pVertexBuffer,
								  nullptr);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("创建顶点缓存失败!"), NULL, MB_OK);
		return hr;
	}
	//init vertex buffer
	Vertex * pVertex = nullptr;
	g_pVertexBuffer->Lock(0, 0, (void**)&pVertex, 0);
	// triangle sequence is 123 243
	*pVertex = Vertex(-1, 1, 0, 0, 0);				//1
	*(pVertex + 1) = Vertex(-1, -1, 0, 0, 2);		//2
	*(pVertex + 2) = Vertex(1, 1, 0, 2, 0);			//3
	*(pVertex + 3) = Vertex(1, -1, 0, 2, 2);		//4
	
	g_pVertexBuffer->Unlock();

	return S_OK;
}

HRESULT init(IDirect3DDevice9 * pDevice)
{
	HRESULT hr;
	//init light pos
	g_lightPos.x = -6.5f;			//灯光的位置影响高光，如果在正中央，则有很多砖块出现高光现象！
	g_lightPos.y = 0.f;
	g_lightPos.z = 5.f;

	//set view and projection
	D3DXMATRIX view, proj;

	D3DXVECTOR3 pos(0.f, 0.f, 1.1f);		//注意裁剪平面！！否则物体被裁剪不能显示
	D3DXVECTOR3 target(0.f, 0.f, 0.f);
	D3DXVECTOR3 up(0.f, 1.f, 0.f);

	D3DXMatrixLookAtLH(&view, &pos, &target, &up);
	pDevice->SetTransform(D3DTS_VIEW, &view);
	g_eyePos = pos;							//set eye position

	D3DXMatrixPerspectiveFovLH(&proj,
							   D3DX_PI / 4.f,
							   1.f,//400.f / 400.f,
							   1.f,
							   1000.f);
	pDevice->SetTransform(D3DTS_PROJECTION, &proj);

	g_worldViewProj = view * proj;		// set world view projction matirx

	//create normal texture
	//hr = D3DXCreateTextureFromFile(pDevice, _T("bumpmap.png"), &g_pNormalTexture);
	unsigned int size, level;
	int face;
	const unsigned char *image;
	D3DLOCKED_RECT lockedRect;

	hr = pDevice->CreateTexture(128, 128, 0,
								0, D3DFMT_X8R8G8B8,
								D3DPOOL_MANAGED,
								&g_pNormalTexture, NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("create normal texture faile"), NULL, MB_OK);
		return hr;
	}

	for (size = 128, level = 0, image = g_brickNormalMapImage;
		 size > 0;
		 image += 3 * size*size, size /= 2, level++) 
	{
		if (FAILED(g_pNormalTexture->LockRect(level, &lockedRect, 0, 0)))
			return E_FAIL;
		DWORD *texel = (DWORD*)lockedRect.pBits;

		const int bytes = size*size * 3;

		for (int i = 0; i < bytes; i += 3) 
		{
			*texel++ = image[i + 0] << 16 |
				image[i + 1] << 8 |
				image[i + 2];
		}
		g_pNormalTexture->UnlockRect(level);
	}
	//create cube map of normalize vector
	hr = pDevice->CreateCubeTexture(32, 1,
									0, D3DFMT_X8R8G8B8,
									D3DPOOL_MANAGED,
									&g_pCubeTexture, NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("Create cube map faile!"), NULL, MB_OK);
		return hr;
	}

	const int bytesPerFace = 32 * 32 * 3;
	for (face = D3DCUBEMAP_FACE_POSITIVE_X, image = g_cubeMapImage;
		 face <= D3DCUBEMAP_FACE_NEGATIVE_Z;
		 face += 1, image += bytesPerFace) 
	{
		if (FAILED(g_pCubeTexture->LockRect((D3DCUBEMAP_FACES)face, 0, &lockedRect, 0, 0)))
			return E_FAIL;

		DWORD *texel = (DWORD*)lockedRect.pBits;

		for (int i = 0; i < bytesPerFace; i += 3) {
			*texel++ = image[i + 0] << 16 |
				image[i + 1] << 8 |
				image[i + 2];
		}
		g_pCubeTexture->UnlockRect((D3DCUBEMAP_FACES)face, 0);
	}

	return initVertexBuffer(pDevice);
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

	if (SUCCEEDED(pDev->BeginScene())) {
#if 0
		pDev->SetRenderState(D3DRS_LIGHTING, FALSE);
		pDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
#else
		//pDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		//pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	//	pDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		//pDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
		//pDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);

		cgD3D9BindProgram(g_cgVertexProgram);
		checkForCgError("binding vertex program");
		cgD3D9BindProgram(g_cgPixelProgram);
		checkForCgError("binding pixel program");

		cgSetParameterValuefc(g_cgWorldViewProj, 16, g_worldViewProj.m[0]);
		cgSetParameter3f(g_cgLightPos, g_lightPos.x, g_lightPos.y, g_lightPos.z);
		cgSetParameter3f(g_cgEyePos, g_eyePos.x, g_eyePos.y, g_eyePos.z);

		cgD3D9SetTexture(g_cgNormalTexture, g_pNormalTexture);
		cgD3D9SetTexture(g_cgCubeTexture, g_pCubeTexture);
		cgSetParameter1f(g_cgPower, g_power);
#endif
		pDev->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(Vertex));
		pDev->SetFVF(Vertex::FVF);
		pDev->SetTexture(0, g_pNormalTexture);
		pDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

		pDev->EndScene();
	}
}

static void CALLBACK OnLostDevice(void* userContext)
{
	if (g_pVertexBuffer){ g_pVertexBuffer->Release(); g_pVertexBuffer = nullptr; }
	if (g_pNormalTexture){ g_pNormalTexture->Release(); g_pNormalTexture = nullptr; }
	if (g_pCubeTexture) { g_pCubeTexture->Release(); g_pCubeTexture = nullptr; }
	cgD3D9SetDevice(NULL);
}
