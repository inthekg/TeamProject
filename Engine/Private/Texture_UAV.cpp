#include "Texture_UAV.h"

CTexture_UAV::CTexture_UAV(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CTexture_UAV::Initialize(const TEX_UAV_DESC& Desc)
{
    if(Desc.iWidth == 0 || Desc.iHeight == 0 || Desc.iMipLevels == 0  || Desc.iArraySize == 0)
        return E_FAIL;

    if (Desc.bCubeMap && Desc.iArraySize != 6)
        return E_FAIL;

    m_tDesc = Desc;

    D3D11_TEXTURE2D_DESC TextureDesc{};
    TextureDesc.Width = Desc.iWidth;
    TextureDesc.Height = Desc.iHeight;
    TextureDesc.MipLevels = Desc.iMipLevels;
    TextureDesc.ArraySize = Desc.iArraySize;
    TextureDesc.Format = Desc.eFormat;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = Desc.bCubeMap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

    if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, nullptr, &m_pTexture2D)))
        return E_FAIL;

    D3D11_SHADER_RESOURCE_VIEW_DESC  SRVDesc{};
    SRVDesc.Format = Desc.eFormat;
    if (Desc.bCubeMap)
    {
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        SRVDesc.TextureCube.MostDetailedMip = 0;
        SRVDesc.TextureCube.MipLevels = Desc.iMipLevels;

    }
    else
    {
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MostDetailedMip = 0;
        SRVDesc.Texture2D.MipLevels = Desc.iMipLevels;
    }

    if (FAILED(m_pDevice->CreateShaderResourceView(m_pTexture2D, &SRVDesc, &m_pSRV)))
        return E_FAIL;

    for (_uint iMip = 0; iMip < Desc.iMipLevels; iMip++)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
        UAVDesc.Format = Desc.eFormat;

        if (Desc.iArraySize > 1)
        {
            UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            UAVDesc.Texture2DArray.MipSlice = iMip;
            UAVDesc.Texture2DArray.FirstArraySlice = 0;
            UAVDesc.Texture2DArray.ArraySize = Desc.iArraySize;
        }
        else
        {
            UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            UAVDesc.Texture2D.MipSlice = iMip;
        }

        ID3D11UnorderedAccessView* pUAV = { nullptr };
        if (FAILED(m_pDevice->CreateUnorderedAccessView(m_pTexture2D, &UAVDesc, &pUAV)))
            return E_FAIL;

        m_UAVs.push_back(pUAV);
    }

    return S_OK;
}

CTexture_UAV* CTexture_UAV::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const TEX_UAV_DESC& Desc)
{    
    CTexture_UAV* pInstance = new CTexture_UAV(pDevice, pContext);
    if (FAILED(pInstance->Initialize(Desc)))
    {
        MSG_BOX("Failed to Created : CTexture_UAV");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CTexture_UAV::Free()
{
    __super::Free();

    for (auto& pUAV : m_UAVs)
        Safe_Release(pUAV);
    m_UAVs.clear();

    Safe_Release(m_pSRV);
    Safe_Release(m_pTexture2D);
    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}
