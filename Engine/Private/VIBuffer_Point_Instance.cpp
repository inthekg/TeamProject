#include "VIBuffer_Point_Instance.h"
#include "GameInstance.h"
#include "Engine_Utility.h"

CVIBuffer_Point_Instance::CVIBuffer_Point_Instance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer_Instance { pDevice, pContext }
{

}

CVIBuffer_Point_Instance::CVIBuffer_Point_Instance(const CVIBuffer_Point_Instance& Prototype)
	: CVIBuffer_Instance{ Prototype }
    , m_pSpeeds { Prototype.m_pSpeeds }
    , m_isLoop { Prototype.m_isLoop }
    , m_iVertexCountPerInstance { Prototype.m_iVertexCountPerInstance }    
    , m_vPivot { Prototype.m_vPivot }
{
	
}

HRESULT CVIBuffer_Point_Instance::Initialize_Prototype(void* pInitialDesc)
{
    Set_Name(TEXT("VIBUFFER_POINT_INSTANCE"));

    m_iNumVertexBuffers = 2;
    m_iNumVertices = 1;
    m_iVertexStride = sizeof(VTXPOS);

    m_iNumIndices = 0;
    m_iIndexStride = 0;
    m_eIndexFormat = DXGI_FORMAT_UNKNOWN;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

    auto        pDesc = static_cast<CVIBuffer_Point_Instance::POINT_INSTANCE_DESC*>(pInitialDesc);

    m_iInstanceStride = sizeof(VTXPARTICLE_INSTANCE);
    m_iNumInstances = pDesc->iNumInstance;
    m_iVertexCountPerInstance = 1;


    /* 네모를 표현해주기 위한 정점, 인덱스를 생성한다. */
    D3D11_BUFFER_DESC           VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.CPUAccessFlags = 0;
    VertexBufferDesc.MiscFlags = 0;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;

    VTXPOS* pVertices = new VTXPOS[m_iNumVertices];
    ZeroMemory(pVertices, sizeof(VTXPOS) * m_iNumVertices);

    D3D11_SUBRESOURCE_DATA      VertexInitialData{};
    VertexInitialData.pSysMem = pVertices;

    if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
        return E_FAIL;

    Safe_Delete_Array(pVertices);

    m_InstanceBufferDesc.ByteWidth = m_iNumInstances * m_iInstanceStride;
    m_InstanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    m_InstanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    m_InstanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_InstanceBufferDesc.MiscFlags = 0;
    m_InstanceBufferDesc.StructureByteStride = m_iInstanceStride;

    m_pInstanceVertices = new VTXPARTICLE_INSTANCE[m_iNumInstances];
    ZeroMemory(m_pInstanceVertices, sizeof(VTXPARTICLE_INSTANCE) * m_iNumInstances);

    m_pSpeeds = new _float[m_iNumInstances];

    m_isLoop = pDesc->isLoop;

    m_vPivot = pDesc->vPivot;

    for (size_t i = 0; i < m_iNumInstances; i++)
    {
        _float      fSize = Random(pDesc->vSize.x, pDesc->vSize.y);

        m_pSpeeds[i] = Random(pDesc->vSpeed.x, pDesc->vSpeed.y);

        m_pInstanceVertices[i].vRight = _float4(fSize, 0.f, 0.f, 0.f);
        m_pInstanceVertices[i].vUp = _float4(0.f, fSize, 0.f, 0.f);
        m_pInstanceVertices[i].vLook = _float4(0.f, 0.f, fSize, 0.f);
        m_pInstanceVertices[i].vTranslation = _float4(
            Random(pDesc->vCenter.x - pDesc->vRange.x * 0.5f, pDesc->vCenter.x + pDesc->vRange.x * 0.5f),
            Random(pDesc->vCenter.y - pDesc->vRange.y * 0.5f, pDesc->vCenter.y + pDesc->vRange.y * 0.5f),
            Random(pDesc->vCenter.z - pDesc->vRange.z * 0.5f, pDesc->vCenter.z + pDesc->vRange.z * 0.5f),
            1.f
        );

        m_pInstanceVertices[i].vLifeTime = _float2( Random(pDesc->vLifeTime.x, pDesc->vLifeTime.y),  0.f );
    }

  
   

	return S_OK;
}

HRESULT CVIBuffer_Point_Instance::Initialize(void* pArg)
{
    D3D11_SUBRESOURCE_DATA      InstanceInitialData{};
    InstanceInitialData.pSysMem = m_pInstanceVertices;

    if (FAILED(m_pDevice->CreateBuffer(&m_InstanceBufferDesc, &InstanceInitialData, &m_pVBInstance)))
        return E_FAIL;

	return S_OK;
}

HRESULT CVIBuffer_Point_Instance::Bind_Resources()
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
    m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

    return S_OK;
}

HRESULT CVIBuffer_Point_Instance::Render()
{
    m_pContext->DrawInstanced(m_iVertexCountPerInstance, m_iNumInstances, 0, 0);

    return S_OK;
}

CVIBuffer_Point_Instance* CVIBuffer_Point_Instance::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pInitialDesc)
{
    CVIBuffer_Point_Instance* pInstance = new CVIBuffer_Point_Instance(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(pInitialDesc)))
    {
        MSG_BOX("Failed to Created : CVIBuffer_Point_Instance");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CVIBuffer_Point_Instance::Clone(void* pArg)
{
    CVIBuffer_Point_Instance* pInstance = new CVIBuffer_Point_Instance(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CVIBuffer_Point_Instance");
        Safe_Release(pInstance);
    }

    return pInstance;
}
void CVIBuffer_Point_Instance::Free()
{
	__super::Free();

	if (false == m_bClone)
		Safe_Delete_Array(m_pSpeeds);
}
