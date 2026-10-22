#include "NavigationDebugLineBuffer.h"

#ifdef _DEBUG

NS_BEGIN(GameCore)

CNavigationDebugLineBuffer::CNavigationDebugLineBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: Engine::CVIBuffer{ pDevice, pContext }
{
}

HRESULT CNavigationDebugLineBuffer::Initialize_Prototype(_uint iMaxVertexCount)
{
	if (0 == iMaxVertexCount)
		return E_FAIL;

	m_iNumVertexBuffers = 1;
	m_iNumVertices = iMaxVertexCount;
	m_iVertexStride = sizeof(Engine::VTXCOLOR);
	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	m_iLineVertexCount = 0;

	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
	VertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = 0;

	vector<Engine::VTXCOLOR> Vertices;
	Vertices.resize(m_iNumVertices);

	// 빈 버퍼라도 생성해 둬야 이후 Set_Lines에서 동적 갱신할 수 있다.
	D3D11_SUBRESOURCE_DATA VertexInitialData = {};
	VertexInitialData.pSysMem = Vertices.data();

	if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
		return E_FAIL;

	return S_OK;
}

HRESULT CNavigationDebugLineBuffer::Set_Lines(const vector<Engine::VTXCOLOR>& Lines)
{
	if (nullptr == m_pVB)
		return E_FAIL;

	m_iLineVertexCount = min(ETOUI(Lines.size()), m_iNumVertices);

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};

	if (FAILED(m_pContext->Map(m_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
		return E_FAIL;

	Engine::VTXCOLOR* pVertices = reinterpret_cast<Engine::VTXCOLOR*>(MappedResource.pData);

	if (0 < m_iLineVertexCount)
		memcpy(pVertices, Lines.data(), sizeof(Engine::VTXCOLOR) * m_iLineVertexCount);

	m_pContext->Unmap(m_pVB, 0);

	return S_OK;
}

HRESULT CNavigationDebugLineBuffer::Bind_Resources()
{
	if (nullptr == m_pVB)
		return E_FAIL;

	ID3D11Buffer* pVertexBuffers[] =
	{
		m_pVB
	};

	_uint iVertexStrides[] =
	{
		m_iVertexStride
	};

	_uint iOffsets[] =
	{
		0
	};

	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffers, iVertexStrides, iOffsets);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

	return S_OK;
}

HRESULT CNavigationDebugLineBuffer::Render()
{
	if (0 == m_iLineVertexCount)
		return S_OK;

	m_pContext->Draw(m_iLineVertexCount, 0);

	return S_OK;
}

CNavigationDebugLineBuffer* CNavigationDebugLineBuffer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iMaxVertexCount)
{
	CNavigationDebugLineBuffer* pInstance = new CNavigationDebugLineBuffer(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(iMaxVertexCount)))
	{
		MSG_BOX("Failed to Created : CNavigationDebugLineBuffer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNavigationDebugLineBuffer::Free()
{
	__super::Free();
}

NS_END

#endif