#pragma once
#include <D3dx9.h>
#include <climits>



class BoundingBox
{
public:
	BoundingBox(IDirect3DDevice9 * pDevice,
				D3DXVECTOR3& minPos = D3DXVECTOR3(-FLT_MAX,-FLT_MAX,-FLT_MAX),
				D3DXVECTOR3& maxPos = D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX));
	~BoundingBox();
	
	void Draw();
private:
	bool InitVertexBuffer();
	bool InitIndexBuffer();

	void Cleanup();

private:
	D3DXVECTOR3 m_minPos;
	D3DXVECTOR3 m_maxPos;
	int m_length;
	int m_width;
	int m_height;

	IDirect3DDevice9 * m_pDevice;
	IDirect3DVertexBuffer9 * m_pVertexBuffer;
	IDirect3DIndexBuffer9 * m_pIndexBuffer;
};

