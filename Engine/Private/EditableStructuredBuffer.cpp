#include "EditableStructuredBuffer.h"


CEditableStructuredBuffer::CEditableStructuredBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CStructuredBuffer{ pDevice, pContext }
{
}


HRESULT	CEditableStructuredBuffer::Initialize(const SB_DESC& Desc)
{
	if (Desc.iElementStride == 0 || Desc.iElementCount == 0)
		return E_FAIL;

	m_iElementStride = Desc.iElementStride;
	m_iMaxElementCount = Desc.iElementCount;
	m_iByteWidth = m_iElementStride * m_iMaxElementCount;

	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = m_iByteWidth;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = m_iElementStride;

	if (FAILED(m_pDevice->CreateBuffer(&bd, nullptr, &m_pBuffer)))
		return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
	srvd.Format = DXGI_FORMAT_UNKNOWN;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvd.BufferEx.FirstElement = 0;
	srvd.BufferEx.NumElements = m_iMaxElementCount;

	if (FAILED(m_pDevice->CreateShaderResourceView(m_pBuffer, &srvd, &m_pSRV)))
		return E_FAIL;

	return S_OK;
}

void CEditableStructuredBuffer::Update_Buffer(void* pData, UINT dataCount)
{
	if (pData == nullptr || dataCount == 0 || dataCount > m_iMaxElementCount)
	{
		MSG_BOX("CEditableStructuredBuffer UpdateBuffer Failed!");
		return;
	}

	//부분 복사를 위한 영역Box 설정 
	D3D11_BOX destBox{};

	// 시작 바이트 offset
	destBox.left = 0;	
	// 끝 바이트 offset
	destBox.right = m_iElementStride * dataCount;

	//1D 버퍼이므로 Y,Z축은 0~1로 설정
	destBox.top = 0;
	destBox.bottom = 1;
	destBox.front = 0;
	destBox.back = 1;

	m_pContext->UpdateSubresource(m_pBuffer, 0, &destBox, pData, 0, 0);

}

void CEditableStructuredBuffer::BindVS(_uint iSlot)
{
	m_pContext->VSSetShaderResources(iSlot, 1, &m_pSRV);
}

void CEditableStructuredBuffer::BindPS(_uint iSlot)
{
	m_pContext->PSSetShaderResources(iSlot, 1, &m_pSRV);
}

CEditableStructuredBuffer* CEditableStructuredBuffer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const SB_DESC& Desc)
{
	CEditableStructuredBuffer* pBuffer = new CEditableStructuredBuffer(pDevice, pContext);
	if (FAILED(pBuffer->Initialize(Desc)))
	{
		MSG_BOX("CEditableStructuredBuffer Create Failed!");
		Safe_Release(pBuffer);
		return nullptr;
	}
	return pBuffer;
}

void CEditableStructuredBuffer::Free()
{
	__super::Free();
}

