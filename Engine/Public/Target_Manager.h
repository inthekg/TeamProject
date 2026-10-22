#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CTarget_Manager final : public CBase
{
private:
    CTarget_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CTarget_Manager() = default;

public:

    HRESULT                                             Add_RenderTarget(const _wstring& strTargetTag, _uint iWidth, _uint iHeight, DXGI_FORMAT ePixelFormat, const _float4& vClearColor);
    void                                                Clear_RenderTarget(const _wstring& strTargetTag);

    ID3D11ShaderResourceView*                           Get_RenderTargetSRV(const _wstring strTargetTag);

    HRESULT                                             Add_MRT(const _wstring& strMRTTag, const _wstring& strTargetTag);
    HRESULT                                             Begin_MRT(const _wstring& strMRTTag, ID3D11DepthStencilView* pDSV = nullptr, _bool bUseDSV = true, _bool bClear = true);
    HRESULT                                             Bind_MRT(const _wstring& strMRTTag, ID3D11DepthStencilView* pDSV = nullptr);
    HRESULT                                             End_MRT();

    HRESULT                                             Bind_ShaderResource(const _wstring& strTargetTag, _uint iRegisterSlot);
    HRESULT                                             Copy_Resource(const _wstring& strTargetTag, ID3D11Texture2D* pOut);

    HRESULT                                             PopRenderTarget_InMRT(const _wstring& strMRTTag, const _wstring& strTargetTag);
    //위의 함수로 MRT에서 빼고 Remove하기 
    HRESULT                                             Remove_RenderTarget(const _wstring& strTargetTag);

public:
    class CRenderTarget*                                Find_RenderTarget(const _wstring& strTargetTag);
    list<class CRenderTarget*>*                         Find_MRT(const _wstring& strMRTTag);

#ifdef _DEBUG
public:
    HRESULT                                             Ready_Debug(const _wstring& strTargetTag, _float fX, _float fY, _float fSizeX, _float fSizeY);
    HRESULT                                             Render_Debug(const _wstring& strMRTTag, class CShader* pShader, class CVIBuffer_Rect* pVIBuffer, class CConstantBuffer* pObjectBuffer);
#endif

protected:
    HRESULT                                             Clear_RTVBind(class CRenderTarget* pRenderTarget);
    HRESULT                                             Clear_SRVBind(class CRenderTarget* pRenderTarget);

private:
    ID3D11Device*                                       m_pDevice           = { nullptr };
    ID3D11DeviceContext*                                m_pContext          = { nullptr };

    ID3D11RenderTargetView*                             m_pBackBufferRTV    = { nullptr };
    ID3D11DepthStencilView*                             m_pOriginalDSV      = { nullptr };


    unordered_map<_wstring, class CRenderTarget*>		m_RenderTargets;
    unordered_map<_wstring, list<class CRenderTarget*>>	m_MRTs;





public:
    static CTarget_Manager*                             Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                                        Free();
};

NS_END