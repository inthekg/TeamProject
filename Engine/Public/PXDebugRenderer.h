#pragma once
#include "GameObject.h"
NS_BEGIN(Engine)

class CVIBuffer_PXDebugLine;
class CShader;
class CGameInstance;
class ENGINE_DLL CPXDebugRenderer : public CGameObject
{
private:
    CPXDebugRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CPXDebugRenderer(const CPXDebugRenderer& Prototype) = delete;
    CPXDebugRenderer& operator=(const CPXDebugRenderer& other) = delete;
    virtual ~CPXDebugRenderer() = default;

public:
    virtual HRESULT			    Initialize_Prototype() override;
    virtual void			    Update();
    virtual HRESULT			    Render() override;

protected:
    HRESULT                     Ready_Components();

protected:
    CVIBuffer_PXDebugLine*      m_pVIBufferCom  = { nullptr };
    CShader*                    m_pShaderCom    = { nullptr }; 
    CGameInstance*              m_pGameInstance = { nullptr };
    
public:
    static CPXDebugRenderer*    Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*        Clone(void* pArg) { return nullptr; };
    virtual void			    Free();
};


NS_END  
