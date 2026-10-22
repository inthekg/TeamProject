#include "VIBuffer.h"

CVIBuffer::CVIBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CResource{ pDevice, pContext }
{

}

//리소스 타입이지만, 인스턴싱을 대비하여 복사생성자 정의 
CVIBuffer::CVIBuffer(const CVIBuffer& Prototype)
	: CResource{ Prototype }
	, m_pVB{ Prototype.m_pVB }
	, m_pIB{ Prototype.m_pIB }
	, m_iNumVertexBuffers{ Prototype.m_iNumVertexBuffers }
	, m_iNumVertices{ Prototype.m_iNumVertices }
	, m_iVertexStride{ Prototype.m_iVertexStride }
	, m_iNumIndices{ Prototype.m_iNumIndices }
	, m_iIndexStride{ Prototype.m_iIndexStride }
	, m_eIndexFormat{ Prototype.m_eIndexFormat }
	, m_ePrimitiveType{ Prototype.m_ePrimitiveType }
{
	Safe_AddRef(m_pVB);
	Safe_AddRef(m_pIB);
}

HRESULT CVIBuffer::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CVIBuffer::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CVIBuffer::Bind_Resources()
{
	ID3D11Buffer* pVertexBuffers[] = {
		m_pVB,
	};

	_uint		iVertexStrides[] = {
		m_iVertexStride,
	};

	_uint		iOffsets[] = {
		0,
	};

	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffers, iVertexStrides, iOffsets);
	m_pContext->IASetIndexBuffer(m_pIB, m_eIndexFormat, 0);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

	return S_OK;
}

HRESULT CVIBuffer::Render()
{
	m_pContext->DrawIndexed(m_iNumIndices, 0, 0);

	return S_OK;
}

void CVIBuffer::Free()
{
	__super::Free();

	Safe_Release(m_pVB);
	Safe_Release(m_pIB);
}
