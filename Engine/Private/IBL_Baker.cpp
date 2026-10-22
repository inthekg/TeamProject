#include "IBL_Baker.h"
#include "ComputeShader.h"
#include "Texture_UAV.h"

CIBL_Baker::CIBL_Baker(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

ID3D11ShaderResourceView* CIBL_Baker::Get_Irradiance_SRV() const
{
    return m_pIrradiance ? m_pIrradiance->Get_SRV() : nullptr;
}

ID3D11ShaderResourceView* CIBL_Baker::Get_Prefiltered_SRV() const
{
    return m_pPrefiltered ? m_pPrefiltered->Get_SRV() : nullptr;
}

ID3D11ShaderResourceView* CIBL_Baker::Get_BRDFLUT_SRV() const
{
    return m_pBRDFLut ? m_pBRDFLut->Get_SRV() : nullptr;
}

HRESULT CIBL_Baker::Initialize()
{
    if (FAILED(Ready_ComputeShaders()))
        return E_FAIL;
    if (FAILED(Ready_OutputTextures()))
        return E_FAIL;
    if (FAILED(Ready_PrefilterCB()))
        return E_FAIL;
    if (FAILED(Ready_LinearSampler()))
        return E_FAIL;


    return S_OK;
}

HRESULT CIBL_Baker::Bake(ID3D11ShaderResourceView* pSkyCubeSRV)
{
    // SkyBox SRV 없으면 Return
    if(nullptr == pSkyCubeSRV)
        return E_FAIL;

    // Compute Stage 샘플러 바인드
    m_pContext->CSSetSamplers(0, 1, &m_pLinearSampler);

    /* 1) Irradiance ( Cube 32^2, 1Mip ) */
    m_pCSIrradiance->Bind_SRV(0, pSkyCubeSRV);
    m_pCSIrradiance->Bind_UAV(0, m_pIrradiance->Get_UAV(0));
    m_pCSIrradiance->Dispatch(ETOUI(IBL::IRR_SIZE) / 8, ETOUI(IBL::IRR_SIZE) / 8, 6);
    m_pCSIrradiance->Unbind_UAV(0);
    m_pCSIrradiance->Unbind_SRV(0);

    /* 2) Prefilter (cube 128^2, 5mip) 밉마다 roughness/ size 갱신 */
    for (_uint iMip = 0; iMip < ETOUI(IBL::PREF_MIPS); iMip++)
    {
        PerPrefilterData Desc{};
        Desc.fRoughness = static_cast<float>(iMip) / static_cast<float>(ETOUI(IBL::PREF_MIPS) - 1);
        Desc.iMipSize = ETOUI(IBL::PREF_SIZE) >> iMip;
        m_pContext->UpdateSubresource(m_pPrefilterCB, 0, nullptr, &Desc, 0, 0);

        m_pCSPrefilter->Bind_ConstantBuffer(0, m_pPrefilterCB);
        m_pCSPrefilter->Bind_SRV(0, pSkyCubeSRV);
        m_pCSPrefilter->Bind_UAV(0, m_pPrefiltered->Get_UAV(iMip));

        const _uint iGroups = max(Desc.iMipSize / 8u, 1u);
        m_pCSPrefilter->Dispatch(iGroups, iGroups, 6);
        m_pCSPrefilter->Unbind_UAV(0);
    }
    m_pCSPrefilter->Unbind_SRV(0);

    /* 3) BRDF LUT */
    m_pCSBRDFLut->Bind_UAV(0, m_pBRDFLut->Get_UAV(0));
    m_pCSBRDFLut->Dispatch(ETOUI(IBL::LUT_SIZE) / 8, ETOUI(IBL::LUT_SIZE) / 8, 1);
    m_pCSBRDFLut->Unbind_UAV(0);

    ID3D11SamplerState* pNullSampler = nullptr;
    m_pContext->CSSetSamplers(0, 1, &pNullSampler);

    m_bBaked = true;
    return S_OK;
}

HRESULT CIBL_Baker::Ready_ComputeShaders()
{
    const _wstring strDir = _wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/";

    /*Compute_IBL_Irradiance.hlsl*/
    {
        CComputeShader::COMPUTE_SHADER_DESC Desc{};
        Desc.strShaderName = L"CS_IBL_Irradiance";
        Desc.strCSFilePath = strDir + L"Compute_IBL_Irradiance.hlsl";
        Desc.strCSFunName = "CS_Main";
        m_pCSIrradiance = CComputeShader::Create(m_pDevice, m_pContext, Desc);
        if (nullptr == m_pCSIrradiance)
            return E_FAIL;
    }

    /*Compute_IBL_Prefilter.hlsl*/
    {
        CComputeShader::COMPUTE_SHADER_DESC Desc{};
        Desc.strShaderName = L"CS_IBL_Prefilter";
        Desc.strCSFilePath = strDir + L"Compute_IBL_Prefilter.hlsl";
        Desc.strCSFunName = "CS_Main";
        m_pCSPrefilter = CComputeShader::Create(m_pDevice, m_pContext, Desc);
        if (nullptr == m_pCSPrefilter)
            return E_FAIL;
    }

    /*Compute_IBL_BRDFLUT.hlsl*/
    {
        CComputeShader::COMPUTE_SHADER_DESC Desc{};
        Desc.strShaderName = L"CS_IBL_BRDFLUT";
        Desc.strCSFilePath = strDir + L"Compute_IBL_BRDFLUT.hlsl";
        Desc.strCSFunName = "CS_Main";
        m_pCSBRDFLut = CComputeShader::Create(m_pDevice, m_pContext, Desc);
        if (nullptr == m_pCSBRDFLut)
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CIBL_Baker::Ready_OutputTextures()
{
    /*Irradiance*/
    {
        CTexture_UAV::TEX_UAV_DESC Desc{};
        Desc.iWidth = Desc.iHeight = ETOUI(IBL::IRR_SIZE);
        Desc.eFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        Desc.iArraySize = 6;
        Desc.iMipLevels = 1;
        Desc.bCubeMap = true;
        m_pIrradiance = CTexture_UAV::Create(m_pDevice, m_pContext, Desc);
        if (nullptr == m_pIrradiance)
            return E_FAIL;
    }

    /*Prefiltered*/
    {
        CTexture_UAV::TEX_UAV_DESC Desc{};
        Desc.iWidth = Desc.iHeight = ETOUI(IBL::PREF_SIZE);
        Desc.eFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        Desc.iArraySize = 6;
        Desc.iMipLevels = ETOUI(IBL::PREF_MIPS);
        Desc.bCubeMap = true;
        m_pPrefiltered = CTexture_UAV::Create(m_pDevice, m_pContext, Desc);
        if (nullptr == m_pPrefiltered)
            return E_FAIL;
    }

    /*BRDF LUT*/
    {
        CTexture_UAV::TEX_UAV_DESC Desc{};
        Desc.iWidth = Desc.iHeight = ETOUI(IBL::LUT_SIZE);
        Desc.eFormat = DXGI_FORMAT_R16G16_FLOAT;
        Desc.iArraySize = 1;
        Desc.iMipLevels = 1;
        Desc.bCubeMap = false;
        m_pBRDFLut = CTexture_UAV::Create(m_pDevice, m_pContext, Desc);
        if (nullptr == m_pBRDFLut)
            return E_FAIL;
    }


    return S_OK;
}

HRESULT CIBL_Baker::Ready_PrefilterCB()
{
    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = sizeof(PerPrefilterData);
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    BufferDesc.CPUAccessFlags = 0;
    if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pPrefilterCB)))
        return E_FAIL;
    return S_OK;
}

HRESULT CIBL_Baker::Ready_LinearSampler()
{
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0.f;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(m_pDevice->CreateSamplerState(&sd, &m_pLinearSampler)))
        return E_FAIL;
    return S_OK;
}

CIBL_Baker* CIBL_Baker::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CIBL_Baker* pInstance = new CIBL_Baker(pDevice, pContext);
    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CIBL_Baker");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CIBL_Baker::Free()
{
    __super::Free();

    Safe_Release(m_pLinearSampler);
    Safe_Release(m_pPrefilterCB);

    Safe_Release(m_pBRDFLut);
    Safe_Release(m_pPrefiltered);
    Safe_Release(m_pIrradiance);

    Safe_Release(m_pCSBRDFLut);
    Safe_Release(m_pCSPrefilter);
    Safe_Release(m_pCSIrradiance);

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}
