#include "VIBuffer_PXDebugLine.h"
#include "GameInstance.h"


CVIBuffer_PXDebugLine::CVIBuffer_PXDebugLine(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CVIBuffer{ pDevice, pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

CVIBuffer_PXDebugLine::CVIBuffer_PXDebugLine(const CVIBuffer_PXDebugLine& Prototype)
    : CVIBuffer{ Prototype }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CVIBuffer_PXDebugLine::Initialize_Prototype()
{
    Set_Name(TEXT("VIBUFFER_PXDEBUGLINE"));

    m_iNumVertexBuffers = 1;
    m_iNumVertices = 1 << 20; //65536. TOOD: 지형메시 일부 안그려지면 더 늘리기
    m_iVertexStride = sizeof(VTXCOLOR);

    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    
    D3D11_BUFFER_DESC           VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
    VertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    VertexBufferDesc.MiscFlags = 0;
    VertexBufferDesc.StructureByteStride = 0;

    VTXCOLOR* pVertices = new VTXCOLOR[m_iNumVertices];
    ZeroMemory(pVertices, sizeof(VTXCOLOR) * m_iNumVertices);

    pVertices[0].vPosition = _float3(-0.5f, 0.f, 0.f);
    pVertices[0].vColor = _float4(1.f, 0.f, 0.f, 1.f);

    pVertices[1].vPosition = _float3(+0.5f, 0.f, 0.f);
    pVertices[1].vColor = _float4(1.f, 0.f, 0.f, 1.f);

    D3D11_SUBRESOURCE_DATA      VertexInitialData{};
    VertexInitialData.pSysMem = pVertices;

    if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
        return E_FAIL;

    Safe_Delete_Array(pVertices);

    return S_OK;
}


HRESULT CVIBuffer_PXDebugLine::Initialize(void* pArg)
{
    return S_OK;
}

HRESULT CVIBuffer_PXDebugLine::Bind_Resources()
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
    m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);
    
    return S_OK;
}

HRESULT CVIBuffer_PXDebugLine::Update()
{
#ifdef _DEBUG

    if (m_pGameInstance->Get_CurrentPxScene() == nullptr)         
        return E_FAIL;

    const PxRenderBuffer& DebugBuffer = m_pGameInstance->Get_RenderBuffer();
    const PxDebugLine* pxLines = DebugBuffer.getLines();
    m_iLineCount = DebugBuffer.getNbLines();
    m_iLineCount = (m_iLineCount * 2 > m_iNumVertices) ? (_uint)(m_iNumVertices / 2) : m_iLineCount;
    D3D11_MAPPED_SUBRESOURCE MappedResource{};
    if (FAILED(m_pContext->Map(m_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
    {
        MSG_BOX("PhysicX Debug Line Buffer Map Failed");
        return E_FAIL;
    }

    VTXCOLOR* pVertices = reinterpret_cast<VTXCOLOR*>(MappedResource.pData);
    for (_uint i = 0; i < m_iLineCount; i++)
    {
        const PxDebugLine& line = pxLines[i];
        _uint idx = i * 2;
        pVertices[idx].vPosition = _float3(line.pos0.x, line.pos0.y, line.pos0.z);
        pVertices[idx].vColor = _float4(
            static_cast<float>((line.color0 >> 16) & 0xFF) / 255.f,
            static_cast<float>((line.color0 >> 8) & 0xFF) / 255.f,
            static_cast<float>((line.color0 >> 0) & 0xFF) / 255.f,
            static_cast<float>((line.color0 >> 24) & 0xFF) / 255.f);

        idx++;

        pVertices[idx].vPosition = _float3(line.pos1.x, line.pos1.y, line.pos1.z);
        pVertices[idx].vColor = _float4(
            static_cast<float>((line.color1 >> 16) & 0xFF) / 255.f,
            static_cast<float>((line.color1 >> 8) & 0xFF) / 255.f,
            static_cast<float>((line.color1 >> 0) & 0xFF) / 255.f,
            static_cast<float>((line.color1 >> 24) & 0xFF) / 255.f);
    }
    m_pContext->Unmap(m_pVB, 0);

#endif
    return S_OK;
}

HRESULT CVIBuffer_PXDebugLine::Render()
{
    m_pContext->Draw(m_iLineCount * 2 , 0);

    return S_OK;
}


CVIBuffer_PXDebugLine* CVIBuffer_PXDebugLine::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CVIBuffer_PXDebugLine* pInstance = new CVIBuffer_PXDebugLine(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CVIBuffer_PXDebugLine");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CVIBuffer_PXDebugLine::Clone(void* pArg)
{
    CVIBuffer_PXDebugLine* pInstance = new CVIBuffer_PXDebugLine(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CVIBuffer_PXDebugLine");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CVIBuffer_PXDebugLine::Free()
{
    __super::Free();
}
