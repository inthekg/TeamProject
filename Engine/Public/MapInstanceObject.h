#pragma once

#include "MapObject.h"
#include "ModelDataStruct.h"

NS_BEGIN(Engine)

class CVIBuffer;
class CMap;
class CComputeShader;

class ENGINE_DLL CMapInstanceObject final : public CMapObject
{
public:
    struct RENDER_INSTANCE_CACHE
    {
        _float4x4 WorldMatrix = {};
        BoundingBox WorldBounds = {};
        _bool       bRenderable = {};
    };

private:
    CMapInstanceObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CMapInstanceObject() = default;


public:
    virtual HRESULT Render() override;
    virtual MapPickedRenderInfo Get_PickedRenderInfo() const override;
    HRESULT Update_VisibleInstanceBuffer(const vector<_float4x4>& VisibleWorldMatrices);

    HRESULT Update_VisibleInstanceBuffer(const vector<_uint>& VisibleInstanceIndices);
    _bool Use_IndexedVisiblePath() const;

public:
    virtual BoundingBox Get_WorldBounds() const override;

    HRESULT Set_InstanceBuffer(const _wstring& strInstanceBufferName);
    HRESULT Initialize_InstanceData(const vector<MapInstanceData>& Instances);

    HRESULT Add_RealObject(CMapObject* pMapObject);
    HRESULT Rebuild_InstanceBuffer();

    _uint Get_InstanceCount() const { return static_cast<_uint>(m_RenderInstanceCaches.size()); }
    CMapObject* Get_InstanceObject(_uint iInstanceIndex) const;

    HRESULT Rebuild_RenderInstanceCache();
    const RENDER_INSTANCE_CACHE* Get_RenderInstanceCache(_uint iInstanceIndex) const;

    void Set_OwnerMap(CMap* pOwnerMap) { m_pOwnerMap = pOwnerMap; }

    _uint Get_IndexCountPerInstance() const;
    HRESULT Prepare_RenderResources();
    HRESULT Set_GlobalIndirectRenderResources(ID3D11ShaderResourceView* pGlobalWorldSRV, ID3D11ShaderResourceView* pVisibleIndexSRV, ID3D11Buffer* pIndirectArgsBuffer, _uint iIndirectArgsOffset, _uint iVisibleBase);
    void Clear_GlobalIndirectRenderResources();

#ifdef _DEBUG
public:
    HRESULT Update_InstanceBuffer(_uint iInstanceIndex);
    HRESULT Remove_RealObject(_uint iInstanceIndex);
    _int Find_InstanceIndex(const CMapObject* pInstanceObject) const;
    const _float4x4* Get_InstanceWorldMatrix(_uint iInstanceIndex) const;
    _bool Get_InstanceWorldBounds(_uint iInstanceIndex, BoundingBox& OutBounds) const;
    virtual MapPickedRenderInfo Get_PickedRenderInfo(_uint iMaterialIndex) const override;
#endif


protected:
    virtual HRESULT Ready_Components() override;

private:
    HRESULT Update_RenderInstanceCache(_uint iInstanceIndex);


private:
    CVIBuffer*                      m_pInstanceBufferCom    = { nullptr };    
    CMap*                           m_pOwnerMap             = { nullptr };
    vector<CMapObject*>             m_RealObjects;
    vector<RENDER_INSTANCE_CACHE>   m_RenderInstanceCaches;



public:
    static CMapInstanceObject* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END
