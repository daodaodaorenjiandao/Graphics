#include "BoundingBox.h"
#include <cassert>
#include "Vertex.h"
#include <tchar.h>

#define SAFE_RELEASE(v) if(v){v->Release();v = nullptr;}

BoundingBox::BoundingBox(IDirect3DDevice9 * pDevice,
						 D3DXVECTOR3& minPos,
						 D3DXVECTOR3& maxPos)
						 :m_minPos(minPos),
						 m_maxPos(maxPos),
						 m_pDevice(pDevice),
						 m_pVertexBuffer(nullptr),
						 m_pIndexBuffer(nullptr)
{
	assert(pDevice != nullptr);
	//assert(m_minPos <= m_maxPos);
	m_length = m_maxPos.x - m_minPos.x;
	m_width = m_maxPos.y - m_minPos.y;
	m_height = m_maxPos.z - m_minPos.z;
	assert(m_length >= 0.f && m_width >= 0.f && m_height >= 0.f);
	if (!InitVertexBuffer() || !InitIndexBuffer())
		Cleanup();
}


BoundingBox::~BoundingBox()
{
	Cleanup();
}

bool BoundingBox::InitVertexBuffer()
{
	HRESULT hr;
	hr = m_pDevice->CreateVertexBuffer(sizeof(Vertex)* 24,
									 0,
									 Vertex::FVF,
									 D3DPOOL_MANAGED,
									 &m_pVertexBuffer,
									 NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("´´½¨¶¥µã»º´æÊ§°Ü£¡"), NULL, MB_OK);
		return false;
	}
	Vertex * pVertex = nullptr;
	m_pVertexBuffer->Lock(0, 0, (void**)&pVertex, 0);
	// fill in the front face vertex data
	pVertex[0] = Vertex(m_minPos.x, m_minPos.y, m_minPos.z, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	pVertex[1] = Vertex(m_minPos.x, m_maxPos.y, m_minPos.z, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	pVertex[2] = Vertex(m_maxPos.x, m_maxPos.y, m_minPos.z, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
	pVertex[3] = Vertex(m_maxPos.x, m_minPos.y, m_minPos.z, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	// fill in the back face vertex data
	pVertex[4] = Vertex(m_minPos.x, m_minPos.y, m_minPos.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	pVertex[5] = Vertex(m_maxPos.x, m_minPos.y, m_maxPos.z, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	pVertex[6] = Vertex(m_maxPos.x, m_maxPos.y, m_maxPos.z, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	pVertex[7] = Vertex(m_minPos.x, m_maxPos.y, m_maxPos.z, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	// fill in the top face vertex data
	pVertex[8] = Vertex(m_minPos.x, m_maxPos.y, m_minPos.z, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	pVertex[9] = Vertex(m_minPos.x, m_maxPos.y, m_maxPos.z, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	pVertex[10] = Vertex(m_maxPos.x, m_maxPos.y, m_maxPos.z, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
	pVertex[11] = Vertex(m_maxPos.x, m_maxPos.y, m_minPos.z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);

	// fill in the bottom face vertex data
	pVertex[12] = Vertex(m_minPos.x, m_minPos.y, m_minPos.z, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	pVertex[13] = Vertex(m_maxPos.x, m_minPos.y, m_minPos.z, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	pVertex[14] = Vertex(m_maxPos.x, m_minPos.y, m_maxPos.z, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
	pVertex[15] = Vertex(m_minPos.x, m_minPos.y, m_maxPos.z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	// fill in the left face vertex data
	pVertex[16] = Vertex(m_minPos.x, m_minPos.y, m_maxPos.z, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	pVertex[17] = Vertex(m_minPos.x, m_maxPos.y, m_maxPos.z, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	pVertex[18] = Vertex(m_minPos.x, m_maxPos.y, m_minPos.z, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	pVertex[19] = Vertex(m_minPos.x, m_minPos.y, m_minPos.z, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// fill in the right face vertex data
	pVertex[20] = Vertex(m_maxPos.x, m_minPos.y, m_minPos.z, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	pVertex[21] = Vertex(m_maxPos.x, m_maxPos.y, m_minPos.z, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	pVertex[22] = Vertex(m_maxPos.x, m_maxPos.y, m_maxPos.z, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	pVertex[23] = Vertex(m_maxPos.x, m_minPos.y, m_maxPos.z, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	m_pVertexBuffer->Unlock();
	return true;
}

bool BoundingBox::InitIndexBuffer()
{
	HRESULT hr;
	hr = m_pDevice->CreateIndexBuffer(sizeof(WORD)* 36,
									  0, D3DFMT_INDEX16,
									  D3DPOOL_MANAGED,
									  &m_pIndexBuffer,
									  NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("´´½¨Ë÷Òý»º´æÊ§°Ü£¡"),NULL,MB_OK);
		return false;
	}
	WORD * pIndex = nullptr;
	m_pIndexBuffer->Lock(0, 0, (void**)&pIndex, 0);

	// fill in the front face index data
	pIndex[0] = 0; pIndex[1] = 1; pIndex[2] = 2;
	pIndex[3] = 0; pIndex[4] = 2; pIndex[5] = 3;

	// fill in the back face index data
	pIndex[6] = 4; pIndex[7] = 5; pIndex[8] = 6;
	pIndex[9] = 4; pIndex[10] = 6; pIndex[11] = 7;

	// fill in the top face index data
	pIndex[12] = 8; pIndex[13] = 9; pIndex[14] = 10;
	pIndex[15] = 8; pIndex[16] = 10; pIndex[17] = 11;

	// fill in the bottom face index data
	pIndex[18] = 12; pIndex[19] = 13; pIndex[20] = 14;
	pIndex[21] = 12; pIndex[22] = 14; pIndex[23] = 15;

	// fill in the left face index data
	pIndex[24] = 16; pIndex[25] = 17; pIndex[26] = 18;
	pIndex[27] = 16; pIndex[28] = 18; pIndex[29] = 19;

	// fill in the right face index data
	pIndex[30] = 20; pIndex[31] = 21; pIndex[32] = 22;
	pIndex[33] = 20; pIndex[34] = 22; pIndex[35] = 23;

	m_pIndexBuffer->Unlock();

	return true;
}

void BoundingBox::Cleanup()
{
	m_pDevice = nullptr;
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
}

void BoundingBox::Draw()
{
	if (m_pDevice)
	{
		m_pDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(Vertex));
		m_pDevice->SetIndices(m_pIndexBuffer);
		m_pDevice->SetFVF(Vertex::FVF);
		m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
										0,
										0,
										24,
										0,
										12);
	}
}