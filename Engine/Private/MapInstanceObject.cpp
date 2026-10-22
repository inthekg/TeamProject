#include "MapInstanceObject.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "VIBuffer_MapInstance.h"
#include "Material.h"
#include "Map.h"
#include "Shader.h"
#include "Shader_Resolver.h"
#include "ComputeShader.h"

CMapInstanceObject::CMapInstanceObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapObject {pDevice, pContext}
{
}

HRESULT CMapInstanceObject::Render()
{
    if (!m_pMaterialcom || !m_pInstanceBufferCom)
        return E_FAIL;

    StaticCommand command{};
    command.header.etype = RENDER_TYPE::STATIC;
    command.header.iCommandSize = sizeof(StaticCommand);

    command.pVIBuffer = m_pInstanceBufferCom;
    command.pMaterial = m_pMaterialcom;
    XMStoreFloat4x4(&command.worldMatrix, XMMatrixIdentity());
    command.iAlphaClip = m_pMaterialcom->Get_OpacityMaskSource();
    m_pGameInstance->Submit_Static_Resolved(command, true);

    return S_OK;
}



CMapObject::MapPickedRenderInfo CMapInstanceObject::Get_PickedRenderInfo() const
{
    CMapObject::MapPickedRenderInfo Info = __super::Get_PickedRenderInfo();

    if (nullptr != m_pMaterialcom)
    {
        CShader_Resolver* pResolver = m_pGameInstance->Get_ShaderResolver();
        CShader* pShader = nullptr;

        // 실제 Render()가 Submit_Static_Resolved(command, true)를 쓰므로 출력도 bInstanced=true로 맞춘다.
        if (nullptr != pResolver)
            pShader = pResolver->Resolve(m_pMaterialcom->Get_Meta_Desc(), false, true);

        if (nullptr != pShader)
            Info.strShaderName = pShader->Get_Name();
    }

    return Info;
}


HRESULT CMapInstanceObject::Update_VisibleInstanceBuffer(const vector<_float4x4>& VisibleWorldMatrices)
{
    CVIBuffer_MapInstance* pInstanceBuffer =dynamic_cast<CVIBuffer_MapInstance*>(m_pInstanceBufferCom);

    if (nullptr == pInstanceBuffer)
        return E_FAIL;

    return pInstanceBuffer->Update_VisibleInstances(VisibleWorldMatrices);
}

HRESULT CMapInstanceObject::Update_VisibleInstanceBuffer(const vector<_uint>& VisibleInstanceIndices)
{
    CVIBuffer_MapInstance* pInstanceBuffer = dynamic_cast<CVIBuffer_MapInstance*>(m_pInstanceBufferCom);

    if (nullptr == pInstanceBuffer)
        return E_FAIL;

    pInstanceBuffer->Clear_GlobalIndirectResources();

    if (Use_IndexedVisiblePath())
        return pInstanceBuffer->Update_VisibleInstanceIndices(VisibleInstanceIndices);

    vector<_float4x4> VisibleWorldMatrices;
    VisibleWorldMatrices.reserve(VisibleInstanceIndices.size());

    // Glass/Unlit은 기존 WORLD0~3 shader input을 쓰므로 matrix fallback을 만든다.
    for (_uint iInstanceIndex : VisibleInstanceIndices)
    {
        const RENDER_INSTANCE_CACHE* pCache = Get_RenderInstanceCache(iInstanceIndex);

        if (nullptr == pCache || false == pCache->bRenderable)
            continue;

        VisibleWorldMatrices.push_back(pCache->WorldMatrix);
    }

    return pInstanceBuffer->Update_VisibleInstances(VisibleWorldMatrices);
}


_bool CMapInstanceObject::Use_IndexedVisiblePath() const
{
    if (nullptr == m_pMaterialcom)
        return false;

    Material_Meta Meta = m_pMaterialcom->Get_Meta_Desc();

    if (Meta.eShadingModel != SHADING_MODEL::DEFAULT_LIT)
        return false;

    // Shader_MapInstance는 DEFAULT_LIT opaque/masked 인스턴스만 사용한다.
    return Meta.eBlendMode == BLEND_MODE::BLEND_OPAQUE || Meta.eBlendMode == BLEND_MODE::BLEND_MASKED;
}


HRESULT CMapInstanceObject::Update_RenderInstanceCache(_uint iInstanceIndex)
{
    if (iInstanceIndex >= m_RenderInstanceCaches.size())
        return E_FAIL;

    RENDER_INSTANCE_CACHE& Cache = m_RenderInstanceCaches[iInstanceIndex];

    // 에디터 실객체가 존재하면 편집된 Transform을 캐시에 반영한다.
    if (iInstanceIndex < m_RealObjects.size())
    {
        Cache = RENDER_INSTANCE_CACHE{};

        CMapObject* pInstanceObject = m_RealObjects[iInstanceIndex];

        if (nullptr == pInstanceObject || pInstanceObject->Is_InActive())
            return S_OK;

        const _float4x4* pWorldMatrix = pInstanceObject->Get_WorldTransform();

        if (nullptr == pWorldMatrix)
            return S_OK;

        Cache.WorldMatrix = *pWorldMatrix;
        Cache.WorldBounds = pInstanceObject->Get_WorldBounds();
        Cache.bRenderable = true;

        return S_OK;
    }

    if (false == Cache.bRenderable)
        return S_OK;

    // 런타임은 저장된 행렬과 인스턴스 오너의 로컬 바운드만 사용한다.
    m_LocalBounds.Transform(Cache.WorldBounds, XMLoadFloat4x4(&Cache.WorldMatrix));

    return S_OK;
}

BoundingBox CMapInstanceObject::Get_WorldBounds() const
{
    BoundingBox MergedBounds{};
    _bool bHasBounds = false;

    // 실객체 유무와 관계없이 런타임 인스턴스 캐시를 기준으로 그룹 바운드를 계산한다.
    for (const RENDER_INSTANCE_CACHE& Cache : m_RenderInstanceCaches)
    {
        if (false == Cache.bRenderable)
            continue;

        if (false == bHasBounds)
        {
            MergedBounds = Cache.WorldBounds;
            bHasBounds = true;
            continue;
        }

        BoundingBox::CreateMerged(MergedBounds, MergedBounds, Cache.WorldBounds);
    }

    return MergedBounds;
}
HRESULT CMapInstanceObject::Set_InstanceBuffer(const _wstring& strInstanceBufferName)
{
    return m_pGameInstance->Get_VIBuffer(strInstanceBufferName, &m_pInstanceBufferCom);
}

HRESULT CMapInstanceObject::Initialize_InstanceData(const vector<MapInstanceData>& Instances)
{
    if (Instances.empty() || false == m_RealObjects.empty())
        return E_FAIL;

    m_RenderInstanceCaches.clear();
    m_RenderInstanceCaches.reserve(Instances.size());

    for (const MapInstanceData& Instance : Instances)
    {
        RENDER_INSTANCE_CACHE Cache{};

        // 맵 파일의 월드 행렬을 런타임 인스턴스 원본으로 저장한다.
        Cache.WorldMatrix = Instance.worldMatrix;
        Cache.bRenderable = true;

        // 실객체 없이 로컬 바운드와 월드 행렬로 컬링용 월드 바운드를 생성한다.
        m_LocalBounds.Transform(Cache.WorldBounds, XMLoadFloat4x4(&Cache.WorldMatrix));

        m_RenderInstanceCaches.push_back(Cache);
    }

    if (nullptr != m_pOwnerMap)
        m_pOwnerMap->Set_SpatialDirty(true);

    return S_OK;
}

HRESULT CMapInstanceObject::Add_RealObject(CMapObject* pMapObject)
{
    if (nullptr == pMapObject)
        return E_FAIL;

    m_RealObjects.push_back(pMapObject);

    _bool bAddedRenderCache = false;

    // 에디터에서 새 Floor 인스턴스를 추가하는 경우에만 캐시 크기도 확장한다.
    if (m_RenderInstanceCaches.size() < m_RealObjects.size())
    {
        m_RenderInstanceCaches.emplace_back();
        bAddedRenderCache = true;
    }

    const _uint iInstanceIndex = static_cast<_uint>(m_RealObjects.size() - 1);
    const HRESULT hr = Update_RenderInstanceCache(iInstanceIndex);

    if (FAILED(hr))
    {
        m_RealObjects.pop_back();

        if (true == bAddedRenderCache)
            m_RenderInstanceCaches.pop_back();

        return hr;
    }

    if (nullptr != m_pOwnerMap)
        m_pOwnerMap->Set_SpatialDirty(true);

    return S_OK;
}

HRESULT CMapInstanceObject::Rebuild_InstanceBuffer()
{
    CVIBuffer_MapInstance* pInstanceBuffer = dynamic_cast<CVIBuffer_MapInstance*>(m_pInstanceBufferCom);

    if (nullptr == pInstanceBuffer)
        return E_FAIL;

    vector<_float4x4> WorldMatrices;
    WorldMatrices.reserve(m_RenderInstanceCaches.size());

    // GPU 인스턴스 버퍼도 런타임 캐시에 저장된 행렬을 기준으로 재생성한다.
    for (const RENDER_INSTANCE_CACHE& Cache : m_RenderInstanceCaches)
        WorldMatrices.push_back(Cache.WorldMatrix);

    const HRESULT hr = pInstanceBuffer->Rebuild_InstanceBuffer(WorldMatrices);

    if (FAILED(hr))
        return hr;

    if (FAILED(Rebuild_RenderInstanceCache()))
        return E_FAIL;

    if (nullptr != m_pOwnerMap)
        m_pOwnerMap->Set_SpatialDirty(true);

    return S_OK;
}

CMapObject* CMapInstanceObject::Get_InstanceObject(_uint iInstanceIndex) const
{
    if (iInstanceIndex >= m_RealObjects.size())
        return nullptr;

    return m_RealObjects[iInstanceIndex];
}

HRESULT CMapInstanceObject::Rebuild_RenderInstanceCache()
{
    // 기존 월드 행렬을 유지하면서 각 인스턴스의 월드 바운드를 갱신한다.
    for (_uint i = 0; i < static_cast<_uint>(m_RenderInstanceCaches.size()); ++i)
    {
        if (FAILED(Update_RenderInstanceCache(i)))
            return E_FAIL;
    }

    return S_OK;
}

const CMapInstanceObject::RENDER_INSTANCE_CACHE* CMapInstanceObject::Get_RenderInstanceCache(_uint iInstanceIndex) const
{
    if (iInstanceIndex >= m_RenderInstanceCaches.size())
        return nullptr;

    return &m_RenderInstanceCaches[iInstanceIndex];
}




#ifdef _DEBUG

const _float4x4* CMapInstanceObject::Get_InstanceWorldMatrix(_uint iInstanceIndex) const
{
    CMapObject* pInstanceObject = Get_InstanceObject(iInstanceIndex);

    if (nullptr == pInstanceObject)
        return nullptr;

    return pInstanceObject->Get_WorldTransform();
}

_bool CMapInstanceObject::Get_InstanceWorldBounds(_uint iInstanceIndex, BoundingBox& OutBounds) const
{
    CMapObject* pInstanceObject = Get_InstanceObject(iInstanceIndex);

    if (nullptr == pInstanceObject)
        return false;

    OutBounds = pInstanceObject->Get_WorldBounds();
    return true;
}


HRESULT CMapInstanceObject::Update_InstanceBuffer(_uint iInstanceIndex)
{
    const _float4x4* pWorldMatrix = Get_InstanceWorldMatrix(iInstanceIndex);

    if (nullptr == pWorldMatrix)
        return E_FAIL;

    CVIBuffer_MapInstance* pInstanceBuffer = dynamic_cast<CVIBuffer_MapInstance*>(m_pInstanceBufferCom);

    if (nullptr == pInstanceBuffer)
        return E_FAIL;

    HRESULT hr = pInstanceBuffer->Update_InstanceWorldMatrix(iInstanceIndex, *pWorldMatrix);

    if (FAILED(hr))
        return hr;

    // 에디터에서 transform을 바꾼 인스턴스의 렌더 캐시도 갱신한다.
    if (FAILED(Update_RenderInstanceCache(iInstanceIndex)))
        return E_FAIL;

    if (nullptr != m_pOwnerMap)
        m_pOwnerMap->Set_SpatialDirty(true);

    return S_OK;
}

HRESULT CMapInstanceObject::Remove_RealObject(_uint iInstanceIndex)
{
    if (iInstanceIndex >= m_RealObjects.size())
        return E_FAIL;

    Safe_Release(m_RealObjects[iInstanceIndex]);
    m_RealObjects.erase(m_RealObjects.begin() + iInstanceIndex);

    if (iInstanceIndex < m_RenderInstanceCaches.size())
        m_RenderInstanceCaches.erase(m_RenderInstanceCaches.begin() + iInstanceIndex);

    if (nullptr != m_pOwnerMap)
        m_pOwnerMap->Set_SpatialDirty(true);

    return Rebuild_InstanceBuffer();
}

_int CMapInstanceObject::Find_InstanceIndex(const CMapObject* pInstanceObject) const
{
    if (nullptr == pInstanceObject)
        return -1;

    for (_uint i = 0; i < m_RealObjects.size(); ++i)
    {
        if (m_RealObjects[i] == pInstanceObject)
            return static_cast<_int>(i);
    }

    return -1;
}


CMapObject::MapPickedRenderInfo CMapInstanceObject::Get_PickedRenderInfo(_uint iMaterialIndex) const
{
    CMapObject::MapPickedRenderInfo Info = __super::Get_PickedRenderInfo(iMaterialIndex);
    CMaterial* pMaterial = Get_DebugMaterial(iMaterialIndex);

    if (nullptr != pMaterial)
    {
        CShader_Resolver* pResolver = m_pGameInstance->Get_ShaderResolver();
        CShader* pShader = nullptr;

        // 실제 인스턴스 렌더 경로와 동일하게 bInstanced=true로 Resolve한다.
        if (nullptr != pResolver)
            pShader = pResolver->Resolve(pMaterial->Get_Meta_Desc(), false, true);

        if (nullptr != pShader)
            Info.strShaderName = pShader->Get_Name();
    }

    return Info;
}
#endif

_uint CMapInstanceObject::Get_IndexCountPerInstance() const
{
    CVIBuffer_MapInstance* pInstanceBuffer = dynamic_cast<CVIBuffer_MapInstance*>(m_pInstanceBufferCom);

    if (nullptr == pInstanceBuffer)
        return 0;

    return pInstanceBuffer->Get_IndexCountPerInstance();
}

HRESULT CMapInstanceObject::Prepare_RenderResources()
{
    CVIBuffer_MapInstance* pInstanceBuffer = dynamic_cast<CVIBuffer_MapInstance*>(m_pInstanceBufferCom);

    if (nullptr == pInstanceBuffer)
        return E_FAIL;

    // 오브젝트가 보이기 전 인스턴스 렌더 전용 GPU 버퍼를 생성한다.
    return pInstanceBuffer->Prepare_RenderResources();
}

HRESULT CMapInstanceObject::Set_GlobalIndirectRenderResources(ID3D11ShaderResourceView* pGlobalWorldSRV, ID3D11ShaderResourceView* pVisibleIndexSRV, ID3D11Buffer* pIndirectArgsBuffer, _uint iIndirectArgsOffset, _uint iVisibleBase)
{
    CVIBuffer_MapInstance* pInstanceBuffer = dynamic_cast<CVIBuffer_MapInstance*>(m_pInstanceBufferCom);

    if (nullptr == pInstanceBuffer)
        return E_FAIL;

    return pInstanceBuffer->Set_GlobalIndirectResources(pGlobalWorldSRV, pVisibleIndexSRV, pIndirectArgsBuffer, iIndirectArgsOffset, iVisibleBase);
}

void CMapInstanceObject::Clear_GlobalIndirectRenderResources()
{
    CVIBuffer_MapInstance* pInstanceBuffer = dynamic_cast<CVIBuffer_MapInstance*>(m_pInstanceBufferCom);

    if (nullptr == pInstanceBuffer)
        return;

    pInstanceBuffer->Clear_GlobalIndirectResources();
}

HRESULT CMapInstanceObject::Ready_Components()
{
    return S_OK;
}

CMapInstanceObject* CMapInstanceObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMapInstanceObject* pInstance = new CMapInstanceObject(pDevice, pContext);

    if (FAILED(pInstance->Initialize(nullptr)))
    {
        MSG_BOX("Failed to Created : CMapInstanceObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMapInstanceObject::Free()
{
    for (auto* pObject : m_RealObjects)
        Safe_Release(pObject);

    m_RealObjects.clear();
    m_RenderInstanceCaches.clear();
    
    Safe_Release(m_pInstanceBufferCom);

    __super::Free();
}
