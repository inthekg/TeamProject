#include "StructuredBuffer.h"
#include "EditableStructuredBuffer.h"

CStructuredBuffer::CStructuredBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice { pDevice }, m_pContext { pContext }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CStructuredBuffer::Initialize(const SB_DESC& Desc)
{
	if (Desc.iElementStride == 0 || Desc.iElementCount == 0)
		return E_FAIL;

	m_iElementCount = Desc.iElementCount;
	m_iElementStride = Desc.iElementStride;
	m_iByteWidth = m_iElementCount * m_iElementStride;

	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = m_iByteWidth;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = Desc.iElementStride;

	D3D11_SUBRESOURCE_DATA init{};
	init.pSysMem = Desc.pInitialData;

	if (FAILED(m_pDevice->CreateBuffer(&bd, Desc.pInitialData ? &init : nullptr, &m_pBuffer)))
		return E_FAIL;

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavd{};
	uavd.Format = DXGI_FORMAT_UNKNOWN;
	uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavd.Buffer.FirstElement = 0;
	uavd.Buffer.NumElements = Desc.iElementCount;
	uavd.Buffer.Flags = Desc.bAppend ? D3D11_BUFFER_UAV_FLAG_APPEND : 0;
	if (FAILED(m_pDevice->CreateUnorderedAccessView(m_pBuffer, &uavd, &m_pUAV)))
		return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
	srvd.Format = DXGI_FORMAT_UNKNOWN;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvd.BufferEx.FirstElement = 0;
	srvd.BufferEx.NumElements = Desc.iElementCount;
	if (FAILED(m_pDevice->CreateShaderResourceView(m_pBuffer, &srvd, &m_pSRV)))
		return E_FAIL;

	return S_OK;
}

void CStructuredBuffer::Update_Buffer(const void* pData, _uint iElementCount)
{
	if (pData == nullptr || m_pBuffer == nullptr || iElementCount == 0)
		return;

	const _uint iUpdateByteSize = iElementCount * m_iElementStride;

	if (iUpdateByteSize > m_iByteWidth)
		return;

	m_pContext->UpdateSubresource(m_pBuffer, 0, nullptr, pData, 0, 0);
}

void CStructuredBuffer::Update_Buffer_Region(const void* pData, _uint iStartElement, _uint iElementCount)
{
	if (pData == nullptr || m_pBuffer == nullptr || iElementCount == 0)
		return;

	if (iStartElement >= m_iElementCount)
		return;

	if (iElementCount > m_iElementCount - iStartElement)
		return;

	D3D11_BOX Box = {};
	Box.left = iStartElement * m_iElementStride;
	Box.right = Box.left + iElementCount * m_iElementStride;
	Box.top = 0;
	Box.bottom = 1;
	Box.front = 0;
	Box.back = 1;

	m_pContext->UpdateSubresource(m_pBuffer, 0, &Box, pData, 0, 0);
}

HRESULT CStructuredBuffer::Readback(void* pDst, _uint iByteSize)
{
	if (m_pStaging == nullptr)
	{
		D3D11_BUFFER_DESC sd{};
		sd.ByteWidth = m_iByteWidth;
		sd.Usage = D3D11_USAGE_STAGING;
		sd.BindFlags = 0;
		sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		sd.MiscFlags = 0;
		sd.StructureByteStride = 0;
		if (FAILED(m_pDevice->CreateBuffer(&sd, nullptr, &m_pStaging)))
			return E_FAIL;
	}

	m_pContext->CopyResource(m_pStaging, m_pBuffer);

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(m_pContext->Map(m_pStaging, 0, D3D11_MAP_READ, 0, &mapped)))
		return E_FAIL;

	memcpy(pDst, mapped.pData, iByteSize);
	m_pContext->Unmap(m_pStaging, 0);
	
	return S_OK;
}

void CStructuredBuffer::Copy_Count(ID3D11Buffer* pDst, _uint iDstByteOffset)
{
	m_pContext->CopyStructureCount(pDst, iDstByteOffset, m_pUAV);
}

CStructuredBuffer* CStructuredBuffer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const SB_DESC& Desc)
{
	CStructuredBuffer* pInstance = new CStructuredBuffer(pDevice, pContext);
	if (FAILED(pInstance->Initialize(Desc)))
	{
		MSG_BOX("Failed to Created : CStructuredBuffer");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CStructuredBuffer::Free()
{
	__super::Free();

	Safe_Release(m_pStaging);
	Safe_Release(m_pSRV);
	Safe_Release(m_pUAV);
	Safe_Release(m_pBuffer);
	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
}