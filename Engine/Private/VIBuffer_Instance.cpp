#include "VIBuffer_Instance.h"

CVIBuffer_Instance::CVIBuffer_Instance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer { pDevice, pContext }
{

}

CVIBuffer_Instance::CVIBuffer_Instance(const CVIBuffer_Instance& Prototype)
	: CVIBuffer{ Prototype }	
	, m_iNumInstances { Prototype.m_iNumInstances }
	, m_iInstanceStride{ Prototype.m_iInstanceStride }	
	, m_iIndexCountPerInstance { Prototype.m_iIndexCountPerInstance }
	, m_InstanceBufferDesc { Prototype.m_InstanceBufferDesc }
	, m_pInstanceVertices { Prototype.m_pInstanceVertices }
{
	

}

HRESULT CVIBuffer_Instance::Initialize_Prototype()
{
	Set_Name(TEXT("VIBUFFER_INSTANCE"));
	return S_OK;
}

HRESULT CVIBuffer_Instance::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CVIBuffer_Instance::Bind_Resources()
{
	ID3D11Buffer* pVertexBuffers[] = {
		m_pVB, 
		m_pVBInstance
	};

	_uint		iVertexStrides[] = {
		m_iVertexStride, 
		m_iInstanceStride,
	};

	_uint		iOffsets[] = {
		0, 0
	};

	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffers, iVertexStrides, iOffsets);
	m_pContext->IASetIndexBuffer(m_pIB, m_eIndexFormat, 0);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);



	return S_OK;
}

HRESULT CVIBuffer_Instance::Render()
{
	
	m_pContext->DrawIndexedInstanced(m_iIndexCountPerInstance, m_iNumInstances, 0, 0, 0);

	return S_OK;
}

void CVIBuffer_Instance::Free()
{
	__super::Free();

	if(false == m_bClone)
		Safe_Delete_Array(m_pInstanceVertices);

	Safe_Release(m_pVBInstance);

}
