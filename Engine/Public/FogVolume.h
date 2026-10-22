#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CFogVolume final : public CGameObject
{
public:
    typedef struct tagFogVolumeDesc : public CGameObject::GAMEOBJECT_DESC
    {
        _uint   iShapeType = { 0 };
        _float  fRadius = { 45.f };
        _float3 vExtents = { 10.f, 5.f, 10.f };
        _float3 vRotation = {};
        _float3 vColor = { 0.3f , 0.3f, 0.3f };
        _float  fDestiny = { 0.005f };
        _float  fHeightFalloff = { 0.f };
        _bool   bHeightFalloffBottomToTop = { true };
    } FOGVOLUME_DESC;

#ifdef _DEBUG
public:
    DECLARE_REFLECTION(CFogVolume)
#endif // _DEBUG


private:
    CFogVolume(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CFogVolume(const CFogVolume& Prototype);
    virtual ~CFogVolume() = default;

public:
    void Set_FogEnable(_bool bEnable) { m_bFogEnable = bEnable; }
    _bool Is_FogEnable() const { return m_bFogEnable; }

public:
    virtual HRESULT      Initialize_Prototype() override;
    virtual HRESULT      Initialize(void* pArg) override;
    virtual void         Update(_float fTimeDelta) override;
    virtual void         Physical_Update(_float fFixedTime) override;
    virtual void         Late_Update(_float fTimeDelta) override;
    virtual HRESULT      Render() override;

private:
    void Bake_RotationAxis(const _float3& vRotation);

private:
    FogVolume m_tVolume = {};
    _bool m_bFogEnable = true;

#ifdef _DEBUG
    _bool     m_bAdd = { true };
#endif // _DEBUG




public:
    static  CFogVolume* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void         Free() override;
};

NS_END
