#include "MapFogManager.h"
#include "GameInstance.h"
#include "FogVolume.h"

static _wstring MakeChapterFogPath(const _wstring& strChapterPath)
{
    if (true == strChapterPath.empty())
        return L"";

    const size_t iSlash = strChapterPath.find_last_of(L"\\/");
    const _wstring strFolderPath = _wstring::npos == iSlash ? L"" : strChapterPath.substr(0, iSlash + 1);
    _wstring strFileName = _wstring::npos == iSlash ? strChapterPath : strChapterPath.substr(iSlash + 1);

    const _wstring strChapterSuffix = L".chapter.json";
    const size_t iChapterSuffixPos = strFileName.rfind(strChapterSuffix);

    if (_wstring::npos != iChapterSuffixPos)
        strFileName = strFileName.substr(0, iChapterSuffixPos);

    return strFolderPath + strFileName + L".Fog.json";
}

CMapFogManager::CMapFogManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CMapFogManager::Initialize()
{
    if (nullptr == m_pGameInstance)
        return E_FAIL;

    m_pGameInstance->Subscribe(EVENT_TYPE::SECTOR_CHANGED, this);

    return S_OK;
}

HRESULT CMapFogManager::Load_ChapterFogs(const _wstring& strChapterPath, _uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    if (nullptr == m_pGameInstance || true == strLayerTag.empty())
        return E_FAIL;

    const _wstring strFogPath = MakeChapterFogPath(strChapterPath);

    if (true == strFogPath.empty())
        return E_FAIL;

    ifstream File(strFogPath, ios::binary);

    // Fog 파일은 선택 파일이므로 없으면 성공 처리한다.
    if (false == File.is_open())
        return S_OK;

    json Root = {};

    try
    {
        File >> Root;
    }
    catch (const json::exception&)
    {
        MSG_BOX("Chapter Fog Read Failed");
        return E_FAIL;
    }

    m_SectorExposures.clear();

    if (true == Root.contains("SectorExposures") && true == Root["SectorExposures"].is_array())
    {
        for (const json& ExposureJson : Root["SectorExposures"])
        {
            if (false == ExposureJson.is_object())
                continue;

            const _uint iSectorID = ExposureJson.value("SectorID", UINT_MAX);

            if (UINT_MAX == iSectorID)
                continue;

            SECTOR_EXPOSURE tExposure{};
            tExposure.iSectorID = iSectorID;
            tExposure.fScale = ExposureJson.value("Scale", 1.f);
            tExposure.fBlendTime = ExposureJson.value("BlendTime", m_fDefaultBlendTime);

            m_SectorExposures.emplace_back(tExposure);
        }
    }

    if (false == Root.contains("FogVolumes") || false == Root["FogVolumes"].is_array())
        return S_OK;

    for (const json& FogJson : Root["FogVolumes"])
    {
        if (false == FogJson.is_object())
            continue;

        if (false == FogJson.value("Enable", true))
            continue;

        CFogVolume::FOGVOLUME_DESC Desc{};
        Desc.iShapeType = FogJson.value("ShapeType", 0u);
        Desc.fRadius = FogJson.value("Radius", 45.f);
        Desc.fDestiny = FogJson.value("Density", 0.005f);
        Desc.fHeightFalloff = FogJson.value("HeightFalloff", 0.f);
        Desc.bHeightFalloffBottomToTop = FogJson.value("HeightFalloffBottomToTop", true); // 기존 Fog 파일은 현재 아래→위 감쇠를 유지한다.

        if (FogJson.contains("Position") && FogJson["Position"].is_array() && FogJson["Position"].size() >= 3)
            Desc.vTranslate = _float4(FogJson["Position"][0].get<_float>(), FogJson["Position"][1].get<_float>(), FogJson["Position"][2].get<_float>(), 1.f);
        else
            continue;

        if (FogJson.contains("Extents") && FogJson["Extents"].is_array() && FogJson["Extents"].size() >= 3)
            Desc.vExtents = _float3(FogJson["Extents"][0].get<_float>(), FogJson["Extents"][1].get<_float>(), FogJson["Extents"][2].get<_float>());

        if (FogJson.contains("Rotation") && FogJson["Rotation"].is_array() && FogJson["Rotation"].size() >= 3)
            Desc.vRotation = _float3(FogJson["Rotation"][0].get<_float>(), FogJson["Rotation"][1].get<_float>(), FogJson["Rotation"][2].get<_float>());

        if (FogJson.contains("Color") && FogJson["Color"].is_array() && FogJson["Color"].size() >= 3)
            Desc.vColor = _float3(FogJson["Color"][0].get<_float>(), FogJson["Color"][1].get<_float>(), FogJson["Color"][2].get<_float>());

        const _uint iSectorID = FogJson.value("SectorID", UINT_MAX);

        CFogVolume* pFogVolume = m_pGameInstance->Clone_Prototype<CFogVolume>(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_FogVolume"), &Desc);

        if (nullptr == pFogVolume)
            return E_FAIL;

        // 생성된 Fog 객체를 Layer에 넣고, 매니저는 섹터 제어용 포인터만 보관한다.
        if (FAILED(m_pGameInstance->Add_GameObject(iLayerLevelIndex, strLayerTag, pFogVolume)))
        {
            Safe_Release(pFogVolume);
            return E_FAIL;
        }

        m_FogVolumes.emplace_back(iSectorID, pFogVolume);
    }

    return S_OK;
}

HRESULT CMapFogManager::Apply_FogSectorCulling(const unordered_set<_uint>* pActiveRenderSectorIDs)
{
    if (nullptr == pActiveRenderSectorIDs)
        return E_FAIL;

    for (auto& Pair : m_FogVolumes)
    {
        const _uint iFogSectorID = Pair.first;
        CFogVolume* pFogVolume = Pair.second;

        if (nullptr == pFogVolume)
            continue;

        // 몬스터 Sleep 컬링과 동일하게 이웃을 포함한 Render 활성 섹터의 Fog만 활성화한다.
        const _bool bEnable = UINT_MAX != iFogSectorID && pActiveRenderSectorIDs->end() != pActiveRenderSectorIDs->find(iFogSectorID);
        pFogVolume->Set_FogEnable(bEnable);
    }

    return S_OK;
}

HRESULT CMapFogManager::Apply_SectorExposure(_uint iCurrentSectorID)
{
    if (nullptr == m_pGameInstance)
        return E_FAIL;

    m_iCurrentSectorID = iCurrentSectorID;

    for (const auto& tExposure : m_SectorExposures)
    {
        if (tExposure.iSectorID != iCurrentSectorID)
            continue;

        m_pGameInstance->Set_ExposureScale(tExposure.fScale, tExposure.fBlendTime);
        return S_OK;
    }

    m_pGameInstance->Set_ExposureScale(1.f, m_fDefaultBlendTime);

    return S_OK;
}

void CMapFogManager::Update_ExposureTuning()
{
#ifdef _DEBUG
    if (nullptr == m_pGameInstance || UINT_MAX == m_iCurrentSectorID)
        return;

    const _bool bStepDown = m_pGameInstance->Key_Down(DIK_LBRACKET);
    const _bool bStepUp = m_pGameInstance->Key_Down(DIK_RBRACKET);

    if (false == bStepDown && false == bStepUp)
        return;

    SECTOR_EXPOSURE* pExposure = nullptr;

    for (auto& tExposure : m_SectorExposures)
    {
        if (tExposure.iSectorID != m_iCurrentSectorID)
            continue;

        pExposure = &tExposure;
        break;
    }

    if (nullptr == pExposure)
    {
        SECTOR_EXPOSURE tNewExposure{};
        tNewExposure.iSectorID = m_iCurrentSectorID;
        tNewExposure.fScale = 1.f;
        tNewExposure.fBlendTime = m_fDefaultBlendTime;

        m_SectorExposures.emplace_back(tNewExposure);
        pExposure = &m_SectorExposures.back();
    }

    pExposure->fScale += bStepUp ? m_fExposureTuneStep : -m_fExposureTuneStep;

    if (pExposure->fScale < 0.05f)
        pExposure->fScale = 0.05f;
    if (pExposure->fScale > 8.f)
        pExposure->fScale = 8.f;

    m_pGameInstance->Set_ExposureScale(pExposure->fScale, 0.f);

    _tchar szLog[256] = {};
    swprintf_s(szLog, TEXT("[Exposure] { \"SectorID\": %u, \"Scale\": %.2f, \"BlendTime\": %.2f },\n"),
        pExposure->iSectorID, pExposure->fScale, pExposure->fBlendTime);
    OutputDebugStringW(szLog);
#endif
}


void CMapFogManager::OnEvent(EVENT_TYPE etype, Event_Desc* pData)
{
    if (EVENT_TYPE::SECTOR_CHANGED != etype || nullptr == pData)
        return;

    SECTOR_CHANGED_EVENT_DESC* pSectorChangedDesc = static_cast<SECTOR_CHANGED_EVENT_DESC*>(pData);

    // ActorSectorManager의 몬스터 Sleep 컬링과 같은 Render 활성 섹터 집합을 사용한다.
    Apply_FogSectorCulling(pSectorChangedDesc->pActiveRenderSectorIDs);

    Apply_SectorExposure(pSectorChangedDesc->iCurrentSectorID);
}


CMapFogManager* CMapFogManager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMapFogManager* pInstance = new CMapFogManager(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CMapFogManager");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMapFogManager::Free()
{
    if (nullptr != m_pGameInstance)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    m_FogVolumes.clear();
    m_SectorExposures.clear();

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);

    __super::Free();
}
