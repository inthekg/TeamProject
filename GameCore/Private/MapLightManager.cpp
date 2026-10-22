#include "MapLightManager.h"
#include "GameInstance.h"
#include "MapController.h"

static _wstring MakeChapterLightPath(const _wstring& strChapterPath)
{
    if (strChapterPath.empty())
        return L"";

    const size_t iSlash = strChapterPath.find_last_of(L"\\/");

    const _wstring strFolderPath = _wstring::npos == iSlash ? L"" : strChapterPath.substr(0, iSlash + 1);
    _wstring strFileName = _wstring::npos == iSlash ? strChapterPath : strChapterPath.substr(iSlash + 1);

    const _wstring strChapterSuffix = L".chapter.json";
    const size_t iChapterSuffixPos = strFileName.rfind(strChapterSuffix);

    if (_wstring::npos != iChapterSuffixPos)
    {
        strFileName = strFileName.substr(0, iChapterSuffixPos);
    }
    else
    {
        const size_t iDot = strFileName.find_last_of(L'.');

        if (_wstring::npos != iDot)
            strFileName = strFileName.substr(0, iDot);
    }

    // Chapter01.chapter.json -> Chapter01_Light.json
    return strFolderPath + strFileName + L".Light.json";
}

CMapLightManager::CMapLightManager()
    : m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CMapLightManager::Initialize()
{
    // 매 프레임 Bounds 방식이므로 SECTOR_CHANGED 이벤트는 구독하지 않는다.
    return nullptr == m_pGameInstance ? E_FAIL : S_OK;
}

HRESULT CMapLightManager::Load_ChapterLights(const _wstring& strChapterPath)
{
    if (nullptr == m_pGameInstance)
        return E_FAIL;

    const _wstring strLightPath = MakeChapterLightPath(strChapterPath);

    if (strLightPath.empty())
        return E_FAIL;

    ifstream File(strLightPath, ios::binary);

    // Chapter light file is optional.
    if (false == File.is_open())
        return S_OK;

    json Root = {};

    try
    {
        File >> Root;
    }
    catch (const json::exception&)
    {
        MSG_BOX("Chapter Light Read Failed");
        return E_FAIL;
    }

    if (false == Root.contains("Lights") || false == Root["Lights"].is_array())
        return S_OK;

    for (const json& LightJson : Root["Lights"])
    {
        if (false == LightJson.is_object())
            continue;

        const string strType = LightJson.value("Type", string("POINT"));

        LIGHT eLightType = LIGHT::POINT;

        if ("DIRECTIONAL" == strType)
            eLightType = LIGHT::DIRECTIONAL;
        else if ("SPOT" == strType)
            eLightType = LIGHT::SPOT;
        else if ("POINT" == strType)
            eLightType = LIGHT::POINT;
        else
            continue;

        const _uint iOwnerSectorID = LightJson.value("SectorID", UINT_MAX);

        LIGHT_DESC LightDesc = {};
        LightDesc.eType = eLightType;
        const _float fIntensity = LightJson.value("Intensity", 1.f);
        LightDesc.fRange = LightJson.value("Range", 10.f);
        LightDesc.fInnerCos = cosf(XMConvertToRadians(LightJson.value("InnerDegree", 20.f)));
        LightDesc.fOuterCos = cosf(XMConvertToRadians(LightJson.value("OuterDegree", 30.f)));
        LightDesc.bShadow = LightJson.value("Shadow", true);
        LightDesc.bGodRay = LightJson.value("GodRay", false);
        LightDesc.fGodRayIntensity = LightJson.value("GodRayIntensity", 1.f);

        if (LightJson.contains("Diffuse") && LightJson["Diffuse"].is_array() && LightJson["Diffuse"].size() >= 4)
            LightDesc.vDiffuse = _float4(LightJson["Diffuse"][0].get<_float>() * fIntensity, LightJson["Diffuse"][1].get<_float>() * fIntensity, LightJson["Diffuse"][2].get<_float>() * fIntensity, LightJson["Diffuse"][3].get<_float>());
        else
            LightDesc.vDiffuse = _float4(1.f * fIntensity, 1.f * fIntensity, 1.f * fIntensity, 1.f);

        if (LightJson.contains("Ambient") && LightJson["Ambient"].is_array() && LightJson["Ambient"].size() >= 4)
            LightDesc.vAmbient = _float4(LightJson["Ambient"][0].get<_float>(), LightJson["Ambient"][1].get<_float>(), LightJson["Ambient"][2].get<_float>(), LightJson["Ambient"][3].get<_float>());
        else
            LightDesc.vAmbient = _float4(0.f, 0.f, 0.f, 0.2f);

        if (LightJson.contains("Specular") && LightJson["Specular"].is_array() && LightJson["Specular"].size() >= 4)
            LightDesc.vSpecular = _float4(LightJson["Specular"][0].get<_float>() * fIntensity, LightJson["Specular"][1].get<_float>() * fIntensity, LightJson["Specular"][2].get<_float>() * fIntensity, LightJson["Specular"][3].get<_float>());
        else
            LightDesc.vSpecular = _float4(1.f * fIntensity, 1.f * fIntensity, 1.f * fIntensity, 1.f);

        if (LightJson.contains("Position") && LightJson["Position"].is_array() && LightJson["Position"].size() >= 3)
            LightDesc.vPosition = _float4(LightJson["Position"][0].get<_float>(), LightJson["Position"][1].get<_float>(), LightJson["Position"][2].get<_float>(), 1.f);
        else
            continue;

        if (LightJson.contains("Direction") && LightJson["Direction"].is_array() && LightJson["Direction"].size() >= 3)
            LightDesc.vDirection = _float4(LightJson["Direction"][0].get<_float>(), LightJson["Direction"][1].get<_float>(), LightJson["Direction"][2].get<_float>(), 0.f);
        else
            LightDesc.vDirection = _float4(0.f, -1.f, 0.f, 0.f);

        const _uint iLightIndex = m_pGameInstance->Get_LightCount(eLightType);

        if (FAILED(m_pGameInstance->Add_Light(eLightType, LightDesc)))
            return E_FAIL;

        BoundingBox LightBounds{};
        LightBounds.Center = _float3(LightDesc.vPosition.x, LightDesc.vPosition.y, LightDesc.vPosition.z);

        const _float fRange = max(0.f, LightDesc.fRange);
        LightBounds.Extents = _float3(fRange, fRange, fRange);

        // Directional은 공간 Bounds를 사용하지 않는다.
        const _bool bUseBounds = LIGHT::DIRECTIONAL != eLightType;

        if (FAILED(Register_Light(eLightType, iLightIndex, iOwnerSectorID, LightBounds, bUseBounds)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CMapLightManager::Register_Light(LIGHT eLightType, _uint iLightIndex, _uint iOwnerSectorID, const BoundingBox& WorldBounds, _bool bUseBounds)
{
    if (nullptr == m_pGameInstance || eLightType >= LIGHT::END || UINT_MAX == iLightIndex)
        return E_FAIL;

    MAP_SECTOR_LIGHT_DATA LightData{};
    LightData.eLightType = eLightType;
    LightData.iLightIndex = iLightIndex;
    LightData.iOwnerSectorID = iOwnerSectorID;
    LightData.WorldBounds = WorldBounds;
    LightData.bUseBounds = bUseBounds;
    LightData.bEnabled = m_pGameInstance->Is_LightEnabled(eLightType, iLightIndex);

    m_SectorLights.push_back(LightData);

    return S_OK;
}

HRESULT CMapLightManager::Update(Engine::CMapController* pMapController)
{
    if (nullptr == m_pGameInstance || nullptr == pMapController)
        return E_FAIL;

    for (MAP_SECTOR_LIGHT_DATA& LightData : m_SectorLights)
    {
        if (LightData.eLightType >= LIGHT::END || UINT_MAX == LightData.iLightIndex)
            continue;

        // Directional은 항상 활성, POINT/SPOT은 캐싱된 Bounds로 검사한다.
        const _bool bDesiredEnable = false == LightData.bUseBounds || pMapController->Is_BoundsVisible(LightData.WorldBounds);
        const _bool bCurrentEnable = m_pGameInstance->Is_LightEnabled(LightData.eLightType, LightData.iLightIndex);

        LightData.bEnabled = bCurrentEnable;

        // 실제 상태가 같으면 Set_LightEnable을 호출하지 않는다.
        if (bCurrentEnable == bDesiredEnable)
            continue;

        if (FAILED(m_pGameInstance->Set_LightEnable(LightData.eLightType, LightData.iLightIndex, bDesiredEnable)))
            return E_FAIL;

        LightData.bEnabled = bDesiredEnable;
    }

    return S_OK;
}



void CMapLightManager::Release_RegisteredLights()
{
    if (nullptr == m_pGameInstance)
        return;

    for (_uint iType = 0; iType < ETOUI(LIGHT::END); ++iType)
    {
        vector<_uint> LightIndices = {};

        for (const MAP_SECTOR_LIGHT_DATA& LightData : m_SectorLights)
        {
            if (ETOUI(LightData.eLightType) == iType && UINT_MAX != LightData.iLightIndex)
                LightIndices.push_back(LightData.iLightIndex);
        }

        sort(LightIndices.begin(), LightIndices.end(), [](_uint iLeft, _uint iRight) { return iLeft > iRight; });

        for (_uint iLightIndex : LightIndices)
            m_pGameInstance->Remove_Light(static_cast<LIGHT>(iType), iLightIndex);
    }

    m_SectorLights.clear();
}



CMapLightManager* CMapLightManager::Create()
{
    CMapLightManager* pInstance = new CMapLightManager();

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CMapLightManager");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMapLightManager::Free()
{
    // 이벤트를 구독하지 않으므로 UnSubscribe가 필요 없다.
    Release_RegisteredLights();
    __super::Free();
}