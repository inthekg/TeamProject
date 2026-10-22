#include "Light.h"

CLight::CLight(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CLight::Initialize(const LIGHT_DESC& LightDesc)
{
    m_LightDesc = LightDesc;

    return S_OK;
}

CLight* CLight::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const LIGHT_DESC& LightDesc)
{
    CLight* pInstance = new CLight(pDevice, pContext);

    if (FAILED(pInstance->Initialize(LightDesc)))
    {
        MSG_BOX("Failed to Created : CLight");
        Safe_Release(pInstance);
    }

    return pInstance;
}


void CLight::Free()
{
    __super::Free();

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}
