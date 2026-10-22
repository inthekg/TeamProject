#include "MapRenderInstancePool.h"
#include "MapInstanceObject.h"
#include "ComputeShader.h"

CMapRenderInstancePool::CMapRenderInstancePool(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice(pDevice), m_pContext(pContext)
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CMapRenderInstancePool::Initialize()
{
    m_Instances.reserve(8192);
    m_FreeInstanceIndices.reserve(1024);
    m_CandidateIndices.reserve(8192);

    return S_OK;
}

void CMapRenderInstancePool::Clear()
{
    m_CandidateIndices.clear();
    m_CandidateFrameMarks.clear();
    m_iCandidateFrameStamp = 1;

    m_FreeInstanceIndices.clear();
    m_Instances.clear();
    m_Groups.clear();
    m_OwnerGroupIndices.clear();

    Release_GPUCullResources();

    m_iGlobalInstanceMetaCapacity = 0;
    m_iGroupVisibleIndexCapacity = 0;
    m_iGroupVisibleIndexCount = 0;
    m_iGroupIndirectArgsCapacity = 0;
    m_iGlobalBoundsCapacity = 0;
    m_iGlobalWorldCapacity = 0;
    m_iCandidateIndexCapacity = 0;
    m_bGPUDataDirty = true;

}

void CMapRenderInstancePool::Begin_Frame()
{
    m_CandidateIndices.clear();

    // candidate 중복 방지용 frame stamp를 매 프레임 갱신한다.
    ++m_iCandidateFrameStamp;

    if (0 == m_iCandidateFrameStamp)
    {
        for (_uint& iMark : m_CandidateFrameMarks)
            iMark = 0;

        m_iCandidateFrameStamp = 1;
    }
}

HRESULT CMapRenderInstancePool::Prepare_GPUResources()
{
    // 등록된 전체 인스턴스의 공용 GPU 데이터를 로딩 단계에서 한 번 생성한다.
    if (FAILED(Sync_GlobalInstanceBuffers()))
        return E_FAIL;

    if (FAILED(Ensure_GroupVisibleIndexBuffer(m_iGroupVisibleIndexCount)))
        return E_FAIL;

    if (FAILED(Sync_GroupIndirectArgsBuffer()))
        return E_FAIL;

    return S_OK;
}

void CMapRenderInstancePool::Release_SectorInstances(_uint iSectorID)
{
    for (_uint i = 0; i < static_cast<_uint>(m_Instances.size()); ++i)
    {
        MAP_RENDER_POOL_INSTANCE& Instance = m_Instances[i];

        if (false == Instance.bActive || Instance.iSectorID != iSectorID)
            continue;

        Instance = MAP_RENDER_POOL_INSTANCE{};
        m_FreeInstanceIndices.push_back(i);
        m_bGPUDataDirty = true;
    }
}

_uint CMapRenderInstancePool::Register_Instance(_uint iSectorID, CMapInstanceObject* pOwner, _uint iLocalInstanceIndex, const _float4x4& WorldMatrix, const BoundingBox& WorldBounds, _bool bRenderable, _bool bUseGPUCull)
{
    if (nullptr == pOwner)
        return UINT_MAX;

    const _uint iGlobalInstanceIndex = Acquire_InstanceSlot();
    const _uint iGroupID = Acquire_OwnerGroup(pOwner);

    if (UINT_MAX == iGroupID)
        return UINT_MAX;

    MAP_RENDER_POOL_INSTANCE& Instance = m_Instances[iGlobalInstanceIndex];
    Instance.pOwner = pOwner;
    Instance.iSectorID = iSectorID;
    Instance.iLocalInstanceIndex = iLocalInstanceIndex;
    Instance.iGroupID = iGroupID;
    Instance.WorldMatrix = WorldMatrix;
    Instance.WorldBounds = WorldBounds;
    Instance.bActive = true;
    Instance.bRenderable = bRenderable;
    Instance.bUseGPUCull = bUseGPUCull;

    // global instance index를 mark 배열 인덱스로 쓰기 위해 크기를 맞춘다.
    if (m_CandidateFrameMarks.size() <= iGlobalInstanceIndex)
        m_CandidateFrameMarks.resize(static_cast<size_t>(iGlobalInstanceIndex) + 1, 0);

    m_bGPUDataDirty = true;

    return iGlobalInstanceIndex;
}

void CMapRenderInstancePool::Add_Candidate(_uint iGlobalInstanceIndex)
{
    const MAP_RENDER_POOL_INSTANCE* pInstance = Get_Instance(iGlobalInstanceIndex);

    if (nullptr == pInstance || false == pInstance->bActive || false == pInstance->bRenderable || false == pInstance->bUseGPUCull)
        return;

    // 같은 global instance가 한 프레임에 여러 번 들어오면 CS output write 범위를 넘길 수 있으므로 막는다.
    if (m_CandidateFrameMarks.size() <= iGlobalInstanceIndex)
        m_CandidateFrameMarks.resize(static_cast<size_t>(iGlobalInstanceIndex) + 1, 0);

    if (m_CandidateFrameMarks[iGlobalInstanceIndex] == m_iCandidateFrameStamp)
        return;

    m_CandidateFrameMarks[iGlobalInstanceIndex] = m_iCandidateFrameStamp;
    m_CandidateIndices.push_back(iGlobalInstanceIndex);
}

HRESULT CMapRenderInstancePool::Dispatch_GlobalCull_CS(const MAP_CULL_CS_DESC& CullDesc, CComputeShader* pCullCS, ID3D11Buffer* pCullCSConstantBuffer, ID3D11ShaderResourceView* pHiZSRV) 
{
    if (nullptr == pCullCS || nullptr == pCullCSConstantBuffer || (0 != CullDesc.iUseHiZ && nullptr == pHiZSRV))
        return E_FAIL;

    const _uint iCandidateCount = static_cast<_uint>(m_CandidateIndices.size());

    if (0 == iCandidateCount)
        return S_OK;
    if (FAILED(Sync_GlobalInstanceBuffers()))
        return E_FAIL;

    // unique candidate 수가 전체 GPU cull instance capacity보다 크면 CS output overflow 위험이 있으므로 막는다.
    if (iCandidateCount > m_iGroupVisibleIndexCount)
        return E_FAIL;


    if (FAILED(Ensure_CandidateIndexBuffer(iCandidateCount)))
        return E_FAIL;

    if (FAILED(Ensure_GroupVisibleIndexBuffer(m_iGroupVisibleIndexCount)))
        return E_FAIL;

    if (FAILED(Sync_GroupIndirectArgsBuffer()))
        return E_FAIL;

    D3D11_BOX CandidateBox{};
    CandidateBox.left = 0;
    CandidateBox.right = iCandidateCount * sizeof(_uint);
    CandidateBox.top = 0;
    CandidateBox.bottom = 1;
    CandidateBox.front = 0;
    CandidateBox.back = 1;

    m_pContext->UpdateSubresource(m_pCandidateIndexBuffer, 0, &CandidateBox, m_CandidateIndices.data(), 0, 0);
    m_pContext->UpdateSubresource(pCullCSConstantBuffer, 0, nullptr, &CullDesc, 0, 0);

    Unbind_GlobalCullCSResources(pCullCS);

    constexpr _uint iVisibleIndexSRVSlot = 25;
    ID3D11ShaderResourceView* pNullVisibleIndexSRV = nullptr;
    m_pContext->VSSetShaderResources(iVisibleIndexSRVSlot, 1, &pNullVisibleIndexSRV);

    pCullCS->Bind_ConstantBuffer(0, pCullCSConstantBuffer);
    pCullCS->Bind_SRV(MAP_INSTANCE_BOUNDS_SRV_SLOT, m_pGlobalBoundsSRV);
    pCullCS->Bind_SRV(MAP_CANDIDATE_INDEX_SRV_SLOT, m_pCandidateIndexSRV);
    pCullCS->Bind_SRV(MAP_INSTANCE_META_SRV_SLOT, m_pGlobalInstanceMetaSRV);

    pCullCS->Bind_SRV(29, pHiZSRV);


    pCullCS->Bind_UAV(0, m_pGroupVisibleIndexUAV, UINT_MAX);
    pCullCS->Bind_UAV(1, m_pGroupIndirectArgsUAV, UINT_MAX);

    const _uint iDispatchGroupCount = (iCandidateCount + 63) / 64;
    pCullCS->Dispatch(iDispatchGroupCount, 1, 1);

    pCullCS->Unbind_UAV(0, 2);
    pCullCS->Unbind_SRV(MAP_INSTANCE_BOUNDS_SRV_SLOT, 3);
    pCullCS->Unbind_SRV(29);

    return S_OK;
}


HRESULT CMapRenderInstancePool::Sync_GlobalBoundsBuffer()
{
    if (false == m_bGPUDataDirty && nullptr != m_pGlobalBoundsBuffer && nullptr != m_pGlobalBoundsSRV)
        return S_OK;

    const _uint iInstanceCount = static_cast<_uint>(m_Instances.size());

    if (0 == iInstanceCount)
        return S_OK;

    if (nullptr == m_pGlobalBoundsBuffer || nullptr == m_pGlobalBoundsSRV || m_iGlobalBoundsCapacity < iInstanceCount)
    {
        Safe_Release(m_pGlobalBoundsSRV);
        Safe_Release(m_pGlobalBoundsBuffer);

        D3D11_BUFFER_DESC BufferDesc{};
        BufferDesc.ByteWidth = sizeof(MAP_INSTANCE_BOUNDS) * iInstanceCount;
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
        BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        BufferDesc.CPUAccessFlags = 0;
        BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        BufferDesc.StructureByteStride = sizeof(MAP_INSTANCE_BOUNDS);

        if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pGlobalBoundsBuffer)))
            return E_FAIL;

        if (FAILED(Create_StructuredSRV(m_pGlobalBoundsBuffer, iInstanceCount, &m_pGlobalBoundsSRV)))
            return E_FAIL;

        m_iGlobalBoundsCapacity = iInstanceCount;
    }

    vector<MAP_INSTANCE_BOUNDS> BoundsList;
    BoundsList.resize(iInstanceCount);

    for (_uint i = 0; i < iInstanceCount; ++i)
    {
        const MAP_RENDER_POOL_INSTANCE& Instance = m_Instances[i];

        BoundsList[i].vCenter = Instance.WorldBounds.Center;
        BoundsList[i].vExtents = Instance.WorldBounds.Extents;
    }

    D3D11_BOX Box{};
    Box.left = 0;
    Box.right = sizeof(MAP_INSTANCE_BOUNDS) * iInstanceCount;
    Box.top = 0;
    Box.bottom = 1;
    Box.front = 0;
    Box.back = 1;

    m_pContext->UpdateSubresource(m_pGlobalBoundsBuffer, 0, &Box, BoundsList.data(), 0, 0);

    return S_OK;
}

HRESULT CMapRenderInstancePool::Sync_GlobalWorldBuffer()
{
    const _uint iInstanceCount = static_cast<_uint>(m_Instances.size());

    if (0 == iInstanceCount)
        return S_OK;

    if (nullptr == m_pGlobalWorldBuffer || nullptr == m_pGlobalWorldSRV || m_iGlobalWorldCapacity < iInstanceCount)
    {
        Safe_Release(m_pGlobalWorldSRV);
        Safe_Release(m_pGlobalWorldBuffer);

        D3D11_BUFFER_DESC BufferDesc{};
        BufferDesc.ByteWidth = sizeof(MAP_INSTANCE_WORLD) * iInstanceCount;
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
        BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        BufferDesc.CPUAccessFlags = 0;
        BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        BufferDesc.StructureByteStride = sizeof(MAP_INSTANCE_WORLD);

        if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pGlobalWorldBuffer)))
            return E_FAIL;

        if (FAILED(Create_StructuredSRV(m_pGlobalWorldBuffer, iInstanceCount, &m_pGlobalWorldSRV)))
            return E_FAIL;

        m_iGlobalWorldCapacity = iInstanceCount;
    }

    vector<MAP_INSTANCE_WORLD> WorldList;
    WorldList.resize(iInstanceCount);

    for (_uint i = 0; i < iInstanceCount; ++i)
    {
        const MAP_RENDER_POOL_INSTANCE& Instance = m_Instances[i];
        const _float4x4& WorldMatrix = Instance.WorldMatrix;

        WorldList[i].vRight = XMFLOAT4(WorldMatrix._11, WorldMatrix._12, WorldMatrix._13, WorldMatrix._14);
        WorldList[i].vUp = XMFLOAT4(WorldMatrix._21, WorldMatrix._22, WorldMatrix._23, WorldMatrix._24);
        WorldList[i].vLook = XMFLOAT4(WorldMatrix._31, WorldMatrix._32, WorldMatrix._33, WorldMatrix._34);
        WorldList[i].vTranslation = XMFLOAT4(WorldMatrix._41, WorldMatrix._42, WorldMatrix._43, WorldMatrix._44);
    }

    D3D11_BOX Box{};
    Box.left = 0;
    Box.right = sizeof(MAP_INSTANCE_WORLD) * iInstanceCount;
    Box.top = 0;
    Box.bottom = 1;
    Box.front = 0;
    Box.back = 1;

    m_pContext->UpdateSubresource(m_pGlobalWorldBuffer, 0, &Box, WorldList.data(), 0, 0);

    return S_OK;
}

HRESULT CMapRenderInstancePool::Ensure_CandidateIndexBuffer(_uint iRequiredCapacity)
{
    if (0 == iRequiredCapacity)
        iRequiredCapacity = 1;

    if (nullptr != m_pCandidateIndexBuffer && nullptr != m_pCandidateIndexSRV && m_iCandidateIndexCapacity >= iRequiredCapacity)
        return S_OK;

    // Candidate가 늘 때마다 버퍼를 다시 만들지 않도록 용량을 배수 단위로 확장한다.
    _uint iNewCapacity = 1024;

    if (0 != m_iCandidateIndexCapacity)
        iNewCapacity = m_iCandidateIndexCapacity;

    while (iNewCapacity < iRequiredCapacity)
        iNewCapacity *= 2;

    Safe_Release(m_pCandidateIndexSRV);
    Safe_Release(m_pCandidateIndexBuffer);

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = sizeof(_uint) * iNewCapacity;
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = 0;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(_uint);

    if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pCandidateIndexBuffer)))
        return E_FAIL;

    if (FAILED(Create_StructuredSRV(m_pCandidateIndexBuffer, iNewCapacity, &m_pCandidateIndexSRV)))
        return E_FAIL;

    m_iCandidateIndexCapacity = iNewCapacity;

    return S_OK;
}

HRESULT CMapRenderInstancePool::Create_StructuredSRV(ID3D11Buffer* pBuffer, _uint iElementCount, ID3D11ShaderResourceView** ppOutSRV)
{
    if (nullptr == pBuffer || nullptr == ppOutSRV || 0 == iElementCount)
        return E_FAIL;

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.FirstElement = 0;
    SRVDesc.Buffer.NumElements = iElementCount;

    return m_pDevice->CreateShaderResourceView(pBuffer, &SRVDesc, ppOutSRV);
}

HRESULT CMapRenderInstancePool::Create_StructuredUAV(ID3D11Buffer* pBuffer, _uint iElementCount, _bool bAppend, ID3D11UnorderedAccessView** ppOutUAV)
{
    if (nullptr == pBuffer || nullptr == ppOutUAV || 0 == iElementCount)
        return E_FAIL;

    D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
    UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    UAVDesc.Buffer.FirstElement = 0;
    UAVDesc.Buffer.NumElements = iElementCount;
    UAVDesc.Buffer.Flags = bAppend ? D3D11_BUFFER_UAV_FLAG_APPEND : 0;

    return m_pDevice->CreateUnorderedAccessView(pBuffer, &UAVDesc, ppOutUAV);
}

void CMapRenderInstancePool::Unbind_GlobalCullCSResources(CComputeShader* pCullCS)
{
    if (nullptr == pCullCS)
        return;

    pCullCS->Unbind_UAV(0, 2);
    pCullCS->Unbind_SRV(MAP_INSTANCE_BOUNDS_SRV_SLOT, 3);
    pCullCS->Unbind_SRV(29);
}

void CMapRenderInstancePool::Release_GPUCullResources()
{
    Safe_Release(m_pGroupIndirectArgsUAV);
    Safe_Release(m_pGroupIndirectArgsBuffer);

    Safe_Release(m_pGroupVisibleIndexUAV);
    Safe_Release(m_pGroupVisibleIndexSRV);
    Safe_Release(m_pGroupVisibleIndexBuffer);

    Safe_Release(m_pGlobalInstanceMetaSRV);
    Safe_Release(m_pGlobalInstanceMetaBuffer);

    Safe_Release(m_pCandidateIndexSRV);
    Safe_Release(m_pCandidateIndexBuffer);

    Safe_Release(m_pGlobalWorldSRV);
    Safe_Release(m_pGlobalWorldBuffer);

    Safe_Release(m_pGlobalBoundsSRV);
    Safe_Release(m_pGlobalBoundsBuffer);

}

_uint CMapRenderInstancePool::Acquire_OwnerGroup(CMapInstanceObject* pOwner)
{
    if (nullptr == pOwner)
        return UINT_MAX;

    auto Iter = m_OwnerGroupIndices.find(pOwner);

    if (m_OwnerGroupIndices.end() != Iter)
        return Iter->second;

    MAP_RENDER_POOL_GROUP Group{};
    Group.pOwner = pOwner;
    Group.iIndexCountPerInstance = pOwner->Get_IndexCountPerInstance();

    const _uint iGroupID = static_cast<_uint>(m_Groups.size());
    m_Groups.push_back(Group);
    m_OwnerGroupIndices.emplace(pOwner, iGroupID);

    return iGroupID;
}

HRESULT CMapRenderInstancePool::Sync_GlobalInstanceBuffers()
{
    if (false == m_bGPUDataDirty &&
        nullptr != m_pGlobalBoundsBuffer && nullptr != m_pGlobalBoundsSRV &&
        nullptr != m_pGlobalWorldBuffer && nullptr != m_pGlobalWorldSRV &&
        nullptr != m_pGlobalInstanceMetaBuffer && nullptr != m_pGlobalInstanceMetaSRV)
        return S_OK;

    if (FAILED(Sync_GlobalBoundsBuffer()))
        return E_FAIL;

    if (FAILED(Sync_GlobalWorldBuffer()))
        return E_FAIL;

    if (FAILED(Sync_GlobalInstanceMetaBuffer()))
        return E_FAIL;

    const _uint iMaximumCandidateCount = static_cast<_uint>(m_Instances.size());

    if (FAILED(Ensure_CandidateIndexBuffer(iMaximumCandidateCount)))
        return E_FAIL;

    m_bGPUDataDirty = false;

    return S_OK;
}

HRESULT CMapRenderInstancePool::Sync_GlobalInstanceMetaBuffer()
{
    const _uint iInstanceCount = static_cast<_uint>(m_Instances.size());

    if (0 == iInstanceCount)
        return S_OK;

    for (MAP_RENDER_POOL_GROUP& Group : m_Groups)
    {
        Group.iVisibleBase = 0;
        Group.iVisibleCapacity = 0;
        Group.bActive = false;
    }

    for (const MAP_RENDER_POOL_INSTANCE& Instance : m_Instances)
    {
        if (false == Instance.bActive || false == Instance.bRenderable || false == Instance.bUseGPUCull || UINT_MAX == Instance.iGroupID)
            continue;

        MAP_RENDER_POOL_GROUP& Group = m_Groups[Instance.iGroupID];
        Group.bActive = true;
        ++Group.iVisibleCapacity;
    }

    _uint iVisibleBase = 0;

    for (MAP_RENDER_POOL_GROUP& Group : m_Groups)
    {
        Group.iVisibleBase = iVisibleBase;
        iVisibleBase += Group.iVisibleCapacity;
    }

    m_iGroupVisibleIndexCount = iVisibleBase;

    if (nullptr == m_pGlobalInstanceMetaBuffer || nullptr == m_pGlobalInstanceMetaSRV || m_iGlobalInstanceMetaCapacity < iInstanceCount)
    {
        Safe_Release(m_pGlobalInstanceMetaSRV);
        Safe_Release(m_pGlobalInstanceMetaBuffer);

        D3D11_BUFFER_DESC BufferDesc{};
        BufferDesc.ByteWidth = sizeof(MAP_INSTANCE_RENDER_META) * iInstanceCount;
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
        BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        BufferDesc.CPUAccessFlags = 0;
        BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        BufferDesc.StructureByteStride = sizeof(MAP_INSTANCE_RENDER_META);

        if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pGlobalInstanceMetaBuffer)))
            return E_FAIL;

        if (FAILED(Create_StructuredSRV(m_pGlobalInstanceMetaBuffer, iInstanceCount, &m_pGlobalInstanceMetaSRV)))
            return E_FAIL;

        m_iGlobalInstanceMetaCapacity = iInstanceCount;
    }

    vector<MAP_INSTANCE_RENDER_META> MetaList;
    MetaList.resize(iInstanceCount);

    for (_uint i = 0; i < iInstanceCount; ++i)
    {
        const MAP_RENDER_POOL_INSTANCE& Instance = m_Instances[i];

        MetaList[i].iGroupID = static_cast<int>(Instance.iGroupID);
        MetaList[i].iVisibleBase = 0;

        if (UINT_MAX != Instance.iGroupID && Instance.iGroupID < m_Groups.size())
            MetaList[i].iVisibleBase = static_cast<int>(m_Groups[Instance.iGroupID].iVisibleBase);
    }

    D3D11_BOX Box{};
    Box.left = 0;
    Box.right = sizeof(MAP_INSTANCE_RENDER_META) * iInstanceCount;
    Box.top = 0;
    Box.bottom = 1;
    Box.front = 0;
    Box.back = 1;

    m_pContext->UpdateSubresource(m_pGlobalInstanceMetaBuffer, 0, &Box, MetaList.data(), 0, 0);

    return S_OK;
}

HRESULT CMapRenderInstancePool::Ensure_GroupVisibleIndexBuffer(_uint iCapacity)
{
    if (0 == iCapacity)
        iCapacity = 1;

    if (nullptr != m_pGroupVisibleIndexBuffer && nullptr != m_pGroupVisibleIndexSRV && nullptr != m_pGroupVisibleIndexUAV && m_iGroupVisibleIndexCapacity >= iCapacity)
        return S_OK;

    Safe_Release(m_pGroupVisibleIndexUAV);
    Safe_Release(m_pGroupVisibleIndexSRV);
    Safe_Release(m_pGroupVisibleIndexBuffer);

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = sizeof(_uint) * iCapacity;
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    BufferDesc.CPUAccessFlags = 0;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(_uint);

    if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pGroupVisibleIndexBuffer)))
        return E_FAIL;

    if (FAILED(Create_StructuredSRV(m_pGroupVisibleIndexBuffer, iCapacity, &m_pGroupVisibleIndexSRV)))
        return E_FAIL;

    if (FAILED(Create_StructuredUAV(m_pGroupVisibleIndexBuffer, iCapacity, false, &m_pGroupVisibleIndexUAV)))
        return E_FAIL;

    m_iGroupVisibleIndexCapacity = iCapacity;

    return S_OK;
}

HRESULT CMapRenderInstancePool::Create_RawUAV(ID3D11Buffer* pBuffer, _uint iByteWidth, ID3D11UnorderedAccessView** ppOutUAV)
{
    if (nullptr == pBuffer || nullptr == ppOutUAV || 0 == iByteWidth)
        return E_FAIL;

    D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
    UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    UAVDesc.Buffer.FirstElement = 0;
    UAVDesc.Buffer.NumElements = iByteWidth / sizeof(_uint);
    UAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

    return m_pDevice->CreateUnorderedAccessView(pBuffer, &UAVDesc, ppOutUAV);
}

HRESULT CMapRenderInstancePool::Ensure_GroupIndirectArgsBuffer(_uint iGroupCount)
{
    if (0 == iGroupCount)
        iGroupCount = 1;

    if (nullptr != m_pGroupIndirectArgsBuffer && nullptr != m_pGroupIndirectArgsUAV && m_iGroupIndirectArgsCapacity >= iGroupCount)
        return S_OK;

    Safe_Release(m_pGroupIndirectArgsUAV);
    Safe_Release(m_pGroupIndirectArgsBuffer);

    const _uint iByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) * iGroupCount;

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = iByteWidth;
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    BufferDesc.CPUAccessFlags = 0;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    BufferDesc.StructureByteStride = 0;

    if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pGroupIndirectArgsBuffer)))
        return E_FAIL;

    if (FAILED(Create_RawUAV(m_pGroupIndirectArgsBuffer, iByteWidth, &m_pGroupIndirectArgsUAV)))
        return E_FAIL;

    m_iGroupIndirectArgsCapacity = iGroupCount;

    return S_OK;
}

HRESULT CMapRenderInstancePool::Sync_GroupIndirectArgsBuffer()
{
    const _uint iGroupCount = static_cast<_uint>(m_Groups.size());

    if (0 == iGroupCount)
        return S_OK;

    if (FAILED(Ensure_GroupIndirectArgsBuffer(iGroupCount)))
        return E_FAIL;

    vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS> ArgsList;
    ArgsList.resize(iGroupCount);

    for (_uint i = 0; i < iGroupCount; ++i)
    {
        ArgsList[i].IndexCountPerInstance = m_Groups[i].iIndexCountPerInstance;
        ArgsList[i].InstanceCount = 0;
        ArgsList[i].StartIndexLocation = 0;
        ArgsList[i].BaseVertexLocation = 0;
        ArgsList[i].StartInstanceLocation = 0;
    }

    D3D11_BOX Box{};
    Box.left = 0;
    Box.right = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) * iGroupCount;
    Box.top = 0;
    Box.bottom = 1;
    Box.front = 0;
    Box.back = 1;

    m_pContext->UpdateSubresource(m_pGroupIndirectArgsBuffer, 0, &Box, ArgsList.data(), 0, 0);

    return S_OK;
}



const CMapRenderInstancePool::MAP_RENDER_POOL_INSTANCE* CMapRenderInstancePool::Get_Instance(_uint iGlobalInstanceIndex) const
{
    if (iGlobalInstanceIndex >= m_Instances.size())
        return nullptr;

    return &m_Instances[iGlobalInstanceIndex];
}

_uint CMapRenderInstancePool::Acquire_InstanceSlot()
{
    if (false == m_FreeInstanceIndices.empty())
    {
        const _uint iGlobalInstanceIndex = m_FreeInstanceIndices.back();
        m_FreeInstanceIndices.pop_back();
        return iGlobalInstanceIndex;
    }

    const _uint iGlobalInstanceIndex = static_cast<_uint>(m_Instances.size());
    m_Instances.emplace_back();

    return iGlobalInstanceIndex;
}



_bool CMapRenderInstancePool::Get_OwnerGlobalDrawDesc(CMapInstanceObject* pOwner, MAP_OWNER_GLOBAL_DRAW_DESC* pOutDesc) const
{
    if (nullptr == pOwner || nullptr == pOutDesc)
        return false;

    auto Iter = m_OwnerGroupIndices.find(pOwner);

    if (m_OwnerGroupIndices.end() == Iter)
        return false;

    const _uint iGroupID = Iter->second;

    if (iGroupID >= m_Groups.size())
        return false;

    const MAP_RENDER_POOL_GROUP& Group = m_Groups[iGroupID];

    if (false == Group.bActive || 0 == Group.iVisibleCapacity)
        return false;

    if (nullptr == m_pGlobalWorldSRV || nullptr == m_pGroupVisibleIndexSRV || nullptr == m_pGroupIndirectArgsBuffer)
        return false;

    pOutDesc->pGlobalWorldSRV = m_pGlobalWorldSRV;
    pOutDesc->pGroupVisibleIndexSRV = m_pGroupVisibleIndexSRV;
    pOutDesc->pIndirectArgsBuffer = m_pGroupIndirectArgsBuffer;
    pOutDesc->iIndirectArgsOffset = iGroupID * sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
    pOutDesc->iVisibleBase = Group.iVisibleBase;

    return true;
}




CMapRenderInstancePool* CMapRenderInstancePool::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMapRenderInstancePool* pInstance = new CMapRenderInstancePool(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CMapRenderInstancePool");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMapRenderInstancePool::Free()
{
    Clear();

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);


    __super::Free();
}
