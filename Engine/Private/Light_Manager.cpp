#include "Light_Manager.h"
#include "Light.h"

CLight_Manager::CLight_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

const LIGHT_DESC* CLight_Manager::Get_LightDesc(LIGHT eLightType, _uint iIndex)
{
    if (eLightType >= LIGHT::END)
        return nullptr;

    const vector<CLight*>& Lights = m_Lights[ETOUI(eLightType)];

    if (iIndex >= Lights.size() || nullptr == Lights[iIndex])
        return nullptr;

    return Lights[iIndex]->Get_LightDesc();
}

_uint CLight_Manager::Get_LightCount(LIGHT eLightType) const
{
    if (eLightType >= LIGHT::END)
        return 0;
    return ETOUI(m_Lights[ETOUI(eLightType)].size());
}

_bool CLight_Manager::Is_LightEnabled(LIGHT eLightType, _uint iIndex) const
{
    if (eLightType >= LIGHT::END)
        return false;

    const vector<CLight*>& Lights = m_Lights[ETOUI(eLightType)];

    if (iIndex >= Lights.size() || nullptr == Lights[iIndex])
        return false;

    return Lights[iIndex]->Is_Enable();
}

HRESULT CLight_Manager::Add_Light(LIGHT eLightType, const LIGHT_DESC& LightDesc)
{
    if (ETOUI(eLightType) < 0 || eLightType >= LIGHT::END)
        return E_FAIL;

    CLight* pLight = CLight::Create(m_pDevice, m_pContext, LightDesc);
    if (nullptr == pLight)
        return E_FAIL;

    m_Lights[ETOUI(eLightType)].push_back(pLight);

    return S_OK;
}

HRESULT CLight_Manager::Fill_LightData(LIGHT eLightType, _uint iIndex, PerLightData& perLightData)
{
    const LIGHT_DESC* lightDesc = Get_LightDesc(eLightType, iIndex);
    if (lightDesc == nullptr) return E_FAIL;

    perLightData.vAmbient = lightDesc->vAmbient;
    perLightData.vDiffuse = lightDesc->vDiffuse;
    perLightData.vDirection = lightDesc->vDirection;
    perLightData.vSpecular = lightDesc->vSpecular;
    perLightData.vLight_Pos = lightDesc->vPosition;
    perLightData.fLight_Range = lightDesc->fRange;
    //추가
    perLightData.fLight_InnerCos = lightDesc->fInnerCos;
    perLightData.fLight_OuterCos = lightDesc->fOuterCos;
    perLightData.iShadowIndex = -1;

    return S_OK;
}

HRESULT CLight_Manager::Set_LightEnable(LIGHT eLightType, _uint iIndex, _bool bEnable)
{
    if (eLightType >= LIGHT::END)
        return E_FAIL;

    vector<CLight*>& Lights = m_Lights[ETOUI(eLightType)];

    if (iIndex >= Lights.size() || nullptr == Lights[iIndex])
        return E_FAIL;

    if (Lights[iIndex]->Is_Enable() == bEnable)
        return S_OK;

    Lights[iIndex]->Set_Enable(bEnable);

    return S_OK;
}

HRESULT CLight_Manager::Remove_Light(LIGHT eLightType, _uint iIndex)
{
    if (eLightType >= LIGHT::END)
        return E_FAIL;

    vector<CLight*>& Lights = m_Lights[ETOUI(eLightType)];

    if (iIndex >= Lights.size() || nullptr == Lights[iIndex])
        return E_FAIL;

    Safe_Release(Lights[iIndex]);

    while (false == Lights.empty() && nullptr == Lights.back())
        Lights.pop_back();

    return S_OK;
}

HRESULT CLight_Manager::Set_LightDesc(LIGHT eLightType, _uint iIndex, const LIGHT_DESC& LightDesc)
{
    if (eLightType >= LIGHT::END)
        return E_FAIL;

    vector<CLight*>& Lights = m_Lights[ETOUI(eLightType)];

    if (iIndex >= Lights.size() || nullptr == Lights[iIndex])
        return E_FAIL;

    Lights[iIndex]->Set_LightDesc(LightDesc);

    return S_OK;
}


CLight_Manager* CLight_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    return new CLight_Manager(pDevice, pContext);
}

void CLight_Manager::Free()
{
    __super::Free();

    for (_uint i = 0; i < ETOUI(LIGHT::END); i++)
    {
        for (auto& pLight : m_Lights[i])
            Safe_Release(pLight);

        m_Lights[i].clear();
    }
    

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);


}
