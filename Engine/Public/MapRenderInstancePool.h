#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CMapInstanceObject;
class CComputeShader;

class ENGINE_DLL CMapRenderInstancePool final : public CBase
{
public:
    struct MAP_RENDER_POOL_INSTANCE
    {
        CMapInstanceObject* pOwner = nullptr;
        _uint iSectorID = UINT_MAX;
        _uint iLocalInstanceIndex = UINT_MAX;
        _uint iGroupID = UINT_MAX;
        _float4x4 WorldMatrix = {};
        BoundingBox WorldBounds = {};
        _bool bActive = false;
        _bool bRenderable = false;
        _bool bUseGPUCull = false;
    };

    struct MAP_RENDER_POOL_GROUP
    {
        CMapInstanceObject* pOwner = nullptr;
        _uint iVisibleBase = 0;
        _uint iVisibleCapacity = 0;
        _uint iIndexCountPerInstance = 0;
        _bool bActive = false;
    };

    struct MAP_OWNER_GLOBAL_DRAW_DESC
    {
        ID3D11ShaderResourceView* pGlobalWorldSRV = nullptr;
        ID3D11ShaderResourceView* pGroupVisibleIndexSRV = nullptr;
        ID3D11Buffer* pIndirectArgsBuffer = nullptr;
        _uint iIndirectArgsOffset = 0;
        _uint iVisibleBase = 0;
    };

private:
    CMapRenderInstancePool(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CMapRenderInstancePool() = default;

public:
    HRESULT Initialize();
    void Clear();
    void Begin_Frame();
    HRESULT Prepare_GPUResources();

    void Release_SectorInstances(_uint iSectorID);
    _uint Register_Instance(_uint iSectorID, CMapInstanceObject* pOwner, _uint iLocalInstanceIndex, const _float4x4& WorldMatrix, const BoundingBox& WorldBounds, _bool bRenderable, _bool bUseGPUCull);
    void Add_Candidate(_uint iGlobalInstanceIndex);
    HRESULT Dispatch_GlobalCull_CS(const MAP_CULL_CS_DESC& CullDesc, CComputeShader* pCullCS, ID3D11Buffer* pCullCSConstantBuffer, ID3D11ShaderResourceView* pHiZSRV);

    const MAP_RENDER_POOL_INSTANCE* Get_Instance(_uint iGlobalInstanceIndex) const;
    const vector<_uint>& Get_Candidates() const { return m_CandidateIndices; }
    _uint Get_CandidateCount() const { return static_cast<_uint>(m_CandidateIndices.size()); }

    ID3D11ShaderResourceView* Get_GlobalWorldSRV() const { return m_pGlobalWorldSRV; }
    _uint Get_GlobalInstanceCount() const { return static_cast<_uint>(m_Instances.size()); }

    _bool Get_OwnerGlobalDrawDesc(CMapInstanceObject* pOwner, MAP_OWNER_GLOBAL_DRAW_DESC* pOutDesc) const;

private:
    enum
    {
        MAP_INSTANCE_BOUNDS_SRV_SLOT = 26,
        MAP_CANDIDATE_INDEX_SRV_SLOT = 27,
        MAP_INSTANCE_META_SRV_SLOT = 28
    };

    _uint Acquire_InstanceSlot();

    HRESULT Sync_GlobalInstanceBuffers();
    HRESULT Sync_GlobalBoundsBuffer();
    HRESULT Sync_GlobalWorldBuffer();
    HRESULT Ensure_CandidateIndexBuffer(_uint iRequiredCapacity);


    HRESULT Create_StructuredSRV(ID3D11Buffer* pBuffer, _uint iElementCount, ID3D11ShaderResourceView** ppOutSRV);
    HRESULT Create_StructuredUAV(ID3D11Buffer* pBuffer, _uint iElementCount, _bool bAppend, ID3D11UnorderedAccessView** ppOutUAV);
    void Unbind_GlobalCullCSResources(CComputeShader* pCullCS);
    void Release_GPUCullResources();

private:
    _uint Acquire_OwnerGroup(CMapInstanceObject* pOwner);
    HRESULT Sync_GlobalInstanceMetaBuffer();
    HRESULT Ensure_GroupVisibleIndexBuffer(_uint iCapacity);
    HRESULT Ensure_GroupIndirectArgsBuffer(_uint iGroupCount);
    HRESULT Sync_GroupIndirectArgsBuffer();
    HRESULT Create_RawUAV(ID3D11Buffer* pBuffer, _uint iByteWidth, ID3D11UnorderedAccessView** ppOutUAV);


private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;

    vector<MAP_RENDER_POOL_INSTANCE> m_Instances;
    vector<_uint> m_FreeInstanceIndices;
    vector<_uint> m_CandidateIndices;

    vector<_uint> m_CandidateFrameMarks;

    _uint m_iCandidateFrameStamp = 1;

    ID3D11Buffer* m_pGlobalBoundsBuffer = nullptr;
    ID3D11ShaderResourceView* m_pGlobalBoundsSRV = nullptr;
    _uint m_iGlobalBoundsCapacity = 0;

    ID3D11Buffer* m_pGlobalWorldBuffer = nullptr;
    ID3D11ShaderResourceView* m_pGlobalWorldSRV = nullptr;
    _uint m_iGlobalWorldCapacity = 0;

    ID3D11Buffer* m_pCandidateIndexBuffer = nullptr;
    ID3D11ShaderResourceView* m_pCandidateIndexSRV = nullptr;
    _uint m_iCandidateIndexCapacity = 0;

    _bool m_bGPUDataDirty = true;

private:
    vector<MAP_RENDER_POOL_GROUP> m_Groups;
    unordered_map<CMapInstanceObject*, _uint> m_OwnerGroupIndices;

    ID3D11Buffer* m_pGlobalInstanceMetaBuffer = nullptr;
    ID3D11ShaderResourceView* m_pGlobalInstanceMetaSRV = nullptr;
    _uint m_iGlobalInstanceMetaCapacity = 0;

    ID3D11Buffer* m_pGroupVisibleIndexBuffer = nullptr;
    ID3D11ShaderResourceView* m_pGroupVisibleIndexSRV = nullptr;
    ID3D11UnorderedAccessView* m_pGroupVisibleIndexUAV = nullptr;
    _uint m_iGroupVisibleIndexCapacity = 0;
    _uint m_iGroupVisibleIndexCount = 0;

    ID3D11Buffer* m_pGroupIndirectArgsBuffer = nullptr;
    ID3D11UnorderedAccessView* m_pGroupIndirectArgsUAV = nullptr;
    _uint m_iGroupIndirectArgsCapacity = 0;






public:
    static CMapRenderInstancePool* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END
