#pragma once
#include "VIBuffer.h"

NS_BEGIN(Engine)

class CGameInstance;
class ENGINE_DLL CVIBuffer_PXDebugLine : public CVIBuffer
{
private:
    CVIBuffer_PXDebugLine(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CVIBuffer_PXDebugLine(const CVIBuffer_PXDebugLine& Prototype);
    virtual ~CVIBuffer_PXDebugLine() = default;

public:
    virtual HRESULT     Initialize_Prototype();
    virtual HRESULT     Initialize(void* pArg);
    
    HRESULT             Update();

    virtual HRESULT     Bind_Resources();
    virtual HRESULT     Render();

protected:
    CGameInstance*      m_pGameInstance = { nullptr };
    _uint               m_iLineCount = { };
public:
    static CVIBuffer_PXDebugLine* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CComponent* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END