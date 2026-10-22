#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CRenderTarget final : public CBase
{
private:
    CRenderTarget(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CRenderTarget() = default;

public:
    ID3D11RenderTargetView*     Get_RTV() const
    {
        return m_pRTV;
    }

    ID3D11ShaderResourceView*   Get_SRV() const
    {
        return m_pSRV;
    }

    ID3D11Texture2D*            Get_Texture2D() const
    {
        return m_pTexture2D;
    }

#ifdef _DEBUG
    void                        Set_Name(_wstring strName)
    {
        m_strName = strName;
    }
    _float4x4                   Get_WorldMatrix() const
    {
        return m_WorldMatrix;
    }
#endif // _DEBUG


public:
    HRESULT                     Initialize(_uint iWidth, _uint iHeight, DXGI_FORMAT ePixelFormat, const _float4& vClearColor);
    HRESULT                     Bind_ShaderResource(_uint iRegisterSlot);
    void                        Clear();
    HRESULT                     Copy_Resource(ID3D11Texture2D* pOut);

#ifdef _DEBUG
public:
    HRESULT                     Ready_Debug(_float fX, _float fY, _float fSizeX, _float fSizeY);
    HRESULT                     Render_Debug(class CShader* pShader, class CVIBuffer_Rect* pVIBuffer);
#endif
    
private:
    class CGameInstance*        m_pGameInstance = { nullptr };
    ID3D11Device*               m_pDevice = { nullptr };
    ID3D11DeviceContext*        m_pContext = { nullptr };

    ID3D11Texture2D*            m_pTexture2D = { nullptr };
    ID3D11RenderTargetView*     m_pRTV = { nullptr };
    ID3D11ShaderResourceView*   m_pSRV = { nullptr };

    _float4				        m_vClearColor = {};

#ifdef _DEBUG
    _float4x4			        m_WorldMatrix = {};
    _wstring                    m_strName = { };
#endif

public:
    static CRenderTarget*       Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iWidth, _uint iHeight, DXGI_FORMAT ePixelFormat, const _float4& vClearColor);
    virtual void                Free() override;
};

NS_END