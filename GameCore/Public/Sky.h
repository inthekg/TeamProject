#pragma once

#include "Core_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CTexture;
class CVIBuffer;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CSky final : public CGameObject
{
private:
    CSky(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CSky(const CSky& Prototype);
    virtual ~CSky() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void Priority_Update(_float fTimeDelta) override;
    virtual void Update(_float fTimeDelta) override;
    virtual void Physical_Update(_float fFixedTime) override;
    virtual void Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT Ready_Components();

private:
    CShader* m_pShaderCom = { nullptr };
    CTexture* m_pTextureCom = { nullptr };
    CVIBuffer* m_pVIBufferCom = { nullptr };

public:
    static CSky* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END