#include "MapWaterManager.h"
#include "GameInstance.h"
#include "MapWorld.h"
#include "Map.h"
#include "MapObject.h"

static _wstring MakeChapterWaterPath(const _wstring& strChapterPath)
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

    return strFolderPath + strFileName + L".Water.json";
}

CMapWaterManager::CMapWaterManager()
    : m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CMapWaterManager::Initialize()
{
    if (nullptr == m_pGameInstance)
        return E_FAIL;

    m_pGameInstance->Subscribe(EVENT_TYPE::SECTOR_CHANGED, this);

    return S_OK;
}

HRESULT CMapWaterManager::Load_ChapterWaters(const _wstring& strChapterPath)
{
    if (nullptr == m_pGameInstance)
        return E_FAIL;

    const _wstring strWaterPath = MakeChapterWaterPath(strChapterPath);

    if (true == strWaterPath.empty())
        return E_FAIL;

    ifstream File(strWaterPath, ios::binary);

    // Water 파일은 선택 파일이므로 없으면 성공 처리한다.
    if (false == File.is_open())
        return S_OK;

    json Root = {};

    try
    {
        File >> Root;
    }
    catch (const json::exception&)
    {
        MSG_BOX("Chapter Water Read Failed");
        return E_FAIL;
    }

    m_WaterPlanes.clear();

    if (false == Root.contains("WaterPlanes") || false == Root["WaterPlanes"].is_array())
        return S_OK;

    for (const json& WaterJson : Root["WaterPlanes"])
    {
        if (false == WaterJson.is_object())
            continue;

        const _uint iSectorID = WaterJson.value("SectorID", UINT_MAX);

        if (UINT_MAX == iSectorID)
            continue;

        WATER_PLANE tPlane{};
        tPlane.iSectorID = iSectorID;
        tPlane.fHeight = WaterJson.value("Height", 0.f);
        tPlane.fMaxDistance = WaterJson.value("MaxDistance", 300.f);

        if (WaterJson.contains("ReflectSectors") && WaterJson["ReflectSectors"].is_array())
        {
            for (const json& SectorJson : WaterJson["ReflectSectors"])
            {
                if (true == SectorJson.is_number_unsigned())
                    tPlane.ReflectSectors.emplace_back(SectorJson.get<_uint>());
            }
        }

        // 지정이 없으면 자기 섹터만 반사한다.
        if (true == tPlane.ReflectSectors.empty())
            tPlane.ReflectSectors.emplace_back(iSectorID);

        m_WaterPlanes.emplace_back(tPlane);
    }

#ifdef _DEBUG
    {
        _tchar szLog[256]{};
        swprintf_s(szLog, TEXT("[Water] loaded planes = %u\n"), static_cast<_uint>(m_WaterPlanes.size()));
        OutputDebugStringW(szLog);
    }
#endif

    return S_OK;
}

const CMapWaterManager::WATER_PLANE* CMapWaterManager::Find_ActivePlane(const unordered_set<_uint>& ActiveSectorIDs) const
{
    // 물이 보이는 것과 플레이어가 그 섹터에 서 있는 것은 별개이므로
    // 현재 섹터가 아니라 렌더 활성 섹터 집합으로 판정한다.
    for (const auto& tPlane : m_WaterPlanes)
    {
        if (ActiveSectorIDs.end() != ActiveSectorIDs.find(tPlane.iSectorID))
            return &tPlane;
    }

    return nullptr;
}

void CMapWaterManager::Update_Reflection(CMapWorld* pMapWorld)
{
    if (nullptr == m_pGameInstance)
        return;

    if (nullptr == pMapWorld)
    {
        m_bReflectionActive = false;
        m_pGameInstance->Set_ReflectionPlane(0.f, false);
        return;
    }

    // 이벤트 구독보다 MapWorld 생성이 빨라 최초 SECTOR_CHANGED 를 놓치므로
    // 매 프레임 현재 섹터를 직접 읽는다.
    m_iCurrentSectorID = pMapWorld->Get_CurrentSectorID();

    const unordered_set<_uint>& ActiveSectorIDs = pMapWorld->Get_ActiveRenderSectorIDs();
    const WATER_PLANE* pPlane = Find_ActivePlane(ActiveSectorIDs);
    const _float4* pCamPosition = m_pGameInstance->Get_CamPosition();

    _float fCamAbove = 0.f;
    _uint  iRegistered = 0;
    _uint  iHLODMapFound = 0;
    _uint  iInactiveSkipped = 0;
    _uint  iReason = 0;

    if (nullptr == pPlane)
    {
        m_bReflectionActive = false;
        iReason = 1;
    }
    else if (nullptr == pCamPosition)
    {
        m_bReflectionActive = false;
        iReason = 2;
    }
    else
    {
        fCamAbove = pCamPosition->y - pPlane->fHeight;

        // 수면 높이에서 켜짐/꺼짐이 매 프레임 진동하지 않도록 진입/이탈 문턱을 벌린다.
        if (false == m_bReflectionActive)
        {
            if (fCamAbove > m_fEnterHeight && fCamAbove <= pPlane->fMaxDistance)
                m_bReflectionActive = true;
        }
        else
        {
            if (fCamAbove < m_fExitHeight || fCamAbove > pPlane->fMaxDistance * m_fExitDistanceScale)
                m_bReflectionActive = false;
        }

        if (false == m_bReflectionActive)
            iReason = (fCamAbove <= m_fEnterHeight) ? 3 : 4;
    }

    m_pGameInstance->Set_ReflectionPlane(nullptr != pPlane ? pPlane->fHeight : 0.f, m_bReflectionActive);

    if (true == m_bReflectionActive)
    {
        for (_uint iSectorID : pPlane->ReflectSectors)
        {
            Engine::CMap* pHLODMap = pMapWorld->Get_HLODMap(iSectorID);

            if (nullptr == pHLODMap)
                continue;

            ++iHLODMapFound;

            for (Engine::CMapObject* pObject : pHLODMap->Get_Objects())
            {
                if (nullptr == pObject)
                    continue;

                if (true == pObject->Is_InActive())
                {
                    ++iInactiveSkipped;
                    continue;
                }

                m_pGameInstance->Add_RenderGroup(RENDERID::REFLECTION, pObject);
                ++iRegistered;
            }
        }

        if (0 == iRegistered)
            iReason = 5;
    }

#ifdef _DEBUG
    {
        static const _tchar* s_szReason[] =
        {
            TEXT("OK"), TEXT("NO_PLANE"), TEXT("NO_CAM"), TEXT("BELOW"), TEXT("TOO_FAR"), TEXT("NO_OBJECT")
        };

        static _uint s_iPrevSector = UINT_MAX;
        static _uint s_iPrevRegistered = UINT_MAX;
        static _uint s_iPrevReason = UINT_MAX;
        static _bool s_bPrevEnable = false;

        if (s_iPrevSector != m_iCurrentSectorID ||
            s_iPrevRegistered != iRegistered ||
            s_iPrevReason != iReason ||
            s_bPrevEnable != m_bReflectionActive)
        {
            s_iPrevSector = m_iCurrentSectorID;
            s_iPrevRegistered = iRegistered;
            s_iPrevReason = iReason;
            s_bPrevEnable = m_bReflectionActive;

            _tchar szSectors[128]{};
            _int   iOffset = 0;

            for (_uint iSectorID : ActiveSectorIDs)
            {
                if (iOffset >= 110)
                    break;

                const _int iWritten = swprintf_s(szSectors + iOffset, 128 - iOffset, TEXT("%u "), iSectorID);

                if (iWritten <= 0)
                    break;

                iOffset += iWritten;
            }

            _tchar szLog[512]{};
            swprintf_s(szLog,
                TEXT("[Water] cur=%u  active=[%s]  plane=%s  planeY=%.2f  camY=%.2f  above=%.2f  enable=%d  reason=%s  hlod=%u  reg=%u  inact=%u\n"),
                m_iCurrentSectorID,
                szSectors,
                (nullptr != pPlane) ? TEXT("FOUND") : TEXT("NONE"),
                (nullptr != pPlane) ? pPlane->fHeight : 0.f,
                (nullptr != pCamPosition) ? pCamPosition->y : 0.f,
                fCamAbove,
                m_bReflectionActive ? 1 : 0,
                s_szReason[iReason],
                iHLODMapFound,
                iRegistered,
                iInactiveSkipped);
            OutputDebugStringW(szLog);
        }
    }
#endif
}

void CMapWaterManager::OnEvent(EVENT_TYPE etype, Event_Desc* pData)
{
    if (EVENT_TYPE::SECTOR_CHANGED != etype || nullptr == pData)
        return;

    SECTOR_CHANGED_EVENT_DESC* pSectorChangedDesc = static_cast<SECTOR_CHANGED_EVENT_DESC*>(pData);

    m_iCurrentSectorID = pSectorChangedDesc->iCurrentSectorID;
}

CMapWaterManager* CMapWaterManager::Create()
{
    CMapWaterManager* pInstance = new CMapWaterManager();

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CMapWaterManager");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMapWaterManager::Free()
{
    if (nullptr != m_pGameInstance)
        m_pGameInstance->UnSubscribe_AllEvent(this);

    m_WaterPlanes.clear();

    __super::Free();
}
