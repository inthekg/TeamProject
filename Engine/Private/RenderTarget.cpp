#include "RenderTarget.h"
#include "GameInstance.h"


CRenderTarget::CRenderTarget(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CRenderTarget::Initialize(_uint iWidth, _uint iHeight, DXGI_FORMAT ePixelFormat, const _float4& vClearColor)
{
    D3D11_TEXTURE2D_DESC	TextureDesc{};

    TextureDesc.Width = iWidth;
    TextureDesc.Height = iHeight;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = ePixelFormat;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;

    if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, nullptr, &m_pTexture2D)))
        return E_FAIL;

    if (FAILED(m_pDevice->CreateRenderTargetView(m_pTexture2D, nullptr, &m_pRTV)))
        return E_FAIL;

    if (FAILED(m_pDevice->CreateShaderResourceView(m_pTexture2D, nullptr, &m_pSRV)))
        return E_FAIL;

    m_vClearColor = vClearColor;

    return S_OK;
}

HRESULT CRenderTarget::Bind_ShaderResource(_uint iRegisterSlot)
{
    // CS_추가
    if (m_pContext == nullptr || m_pSRV == nullptr)
        return E_FAIL;

    m_pContext->PSSetShaderResources(iRegisterSlot, 1, &m_pSRV);
    return S_OK;
}

void CRenderTarget::Clear()
{
    m_pContext->ClearRenderTargetView(m_pRTV, reinterpret_cast<_float*>(&m_vClearColor));
}

HRESULT CRenderTarget::Copy_Resource(ID3D11Texture2D* pOut)
{
    m_pContext->CopyResource(pOut, m_pTexture2D);

    return S_OK;
}

#ifdef _DEBUG

HRESULT CRenderTarget::Ready_Debug(_float fX, _float fY, _float fSizeX, _float fSizeY)
{
    XMStoreFloat4x4(&m_WorldMatrix, XMMatrixScaling(fSizeX, fSizeY, 1.f));

    _float2		vViewportDesc = CGameInstance::Get_Instance()->Get_ViewportDesc();

    m_WorldMatrix._41 = fX - vViewportDesc.x * 0.5f;
    m_WorldMatrix._42 = -fY + vViewportDesc.y * 0.5f;

    return S_OK;
}

HRESULT CRenderTarget::Render_Debug(CShader* pShader, CVIBuffer_Rect* pVIBuffer)
{
    if (pShader == nullptr || pVIBuffer == nullptr) return E_FAIL;
    pVIBuffer->Bind_Resources();

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(m_strName, SRV_ALBEDO_SLOT)))
        return E_FAIL;

    m_pContext->VSSetShader(pShader->Get_VertexShader(), nullptr, 0);
    m_pContext->PSSetShader(pShader->Get_PixelShader(), nullptr, 0);
    m_pContext->IASetInputLayout(pShader->Get_InputLayout()); 

    if (FAILED(pVIBuffer->Render()))
        return E_FAIL;

    
    return S_OK;
}

#endif

CRenderTarget* CRenderTarget::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iWidth, _uint iHeight, DXGI_FORMAT ePixelFormat, const _float4& vClearColor)
{
    CRenderTarget* pInstance = new CRenderTarget(pDevice, pContext);

    if (FAILED(pInstance->Initialize(iWidth, iHeight, ePixelFormat, vClearColor)))
    {
        MSG_BOX("Failed to Created : CRenderTarget");
        Safe_Release(pInstance);
    }

    return pInstance;
}


void CRenderTarget::Free()
{
    __super::Free();

    Safe_Release(m_pSRV);
    Safe_Release(m_pRTV);
    Safe_Release(m_pTexture2D);

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}
