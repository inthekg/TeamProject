#include "ConstantBuffer.h"

CConstantBuffer::CConstantBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CConstantBuffer::Initialize(void* pData, _uint iDataSize)
{
	D3D11_BUFFER_DESC BufferDesc = {};
	
	m_iDataSize = iDataSize;
	BufferDesc.ByteWidth = m_iDataSize;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;


	HRESULT hr = {};
	if( pData == nullptr ) hr = m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pConstantBuffer);
	else
	{
		D3D11_SUBRESOURCE_DATA BufferInitData = {};
		BufferInitData.pSysMem = pData;
		hr = m_pDevice->CreateBuffer(&BufferDesc, &BufferInitData, &m_pConstantBuffer);
	}

	return hr;
}

HRESULT CConstantBuffer::Update_Data(void* pData, _uint iDataSize)
{
	if (iDataSize > m_iDataSize) return E_FAIL;

	D3D11_MAPPED_SUBRESOURCE SubResource;

	if (FAILED(m_pContext->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubResource))) return E_FAIL;
	memcpy(SubResource.pData, pData, iDataSize);
	m_pContext->Unmap(m_pConstantBuffer, 0);
	return S_OK;
}

HRESULT CConstantBuffer::Update_Data_CallBack(function<void(void*)> writeCallBack)
{
	if (writeCallBack == nullptr) return E_FAIL;

	D3D11_MAPPED_SUBRESOURCE SubResource;

	if (FAILED(m_pContext->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubResource))) return E_FAIL;

	writeCallBack(SubResource.pData);

	m_pContext->Unmap(m_pConstantBuffer, 0);
	return S_OK;
}

void CConstantBuffer::BindVS(_uint iSlot)
{
	m_pContext->VSSetConstantBuffers(iSlot, 1, &m_pConstantBuffer);
}

void CConstantBuffer::BindPS(_uint iSlot)
{
	m_pContext->PSSetConstantBuffers(iSlot, 1, &m_pConstantBuffer);

}

void CConstantBuffer::BindGS(_uint iSlot)
{
	m_pContext->GSSetConstantBuffers(iSlot, 1, &m_pConstantBuffer);
}

CConstantBuffer* CConstantBuffer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pInitData, _uint iDataSize)
{
	CConstantBuffer* pInstance = new CConstantBuffer(pDevice, pContext);

	if (FAILED(pInstance->Initialize(pInitData, iDataSize)))
	{
		MSG_BOX("Failed to Created : CConstantBuffer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CConstantBuffer::Free()
{
	__super::Free();

	Safe_Release(m_pConstantBuffer);
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
