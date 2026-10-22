#include "ChapterMapData.h"
#include "Engine_Utility.h"


NS_BEGIN(GameCore)

namespace
{
    // JSON 저장용으로 wstring을 UTF-8 문자열로 바꿈
    string ToUtf8(const _wstring& strValue)
    {
        if (strValue.empty())
            return {};

        const _int iSize =
            WideCharToMultiByte(CP_UTF8, 0, strValue.c_str(), static_cast<_int>(strValue.size()), nullptr, 0, nullptr, nullptr);

        if (iSize <= 0)
            return {};

        string strResult(iSize, 0);

        WideCharToMultiByte(CP_UTF8, 0, strValue.c_str(), static_cast<_int>(strValue.size()), &strResult[0], iSize, nullptr,
            nullptr);

        return strResult;
    }

    // 실패 메시지를 선택적으로 채우고 E_FAIL 반환함
    HRESULT Fail(_wstring* pError, const _wstring& strMessage)
    {
        if (nullptr != pError)
            *pError = strMessage;

        return E_FAIL;
    }

    // float3를 JSON 배열 [x, y, z] 형태로 씀
    json WriteVec3(const _float3& vValue)
    {
        return json::array({ vValue.x,vValue.y,vValue.z });
    }

    // JSON 배열 [x, y, z]를 float3로 읽음
    _bool ReadVec3(const json& Data, _float3* pValue)
    {
        if (nullptr == pValue || false == Data.is_array() || 3 != Data.size())
            return false;

        pValue->x = Data[0].get<_float>();
        pValue->y = Data[1].get<_float>();
        pValue->z = Data[2].get<_float>();

        return true;
    }

    // BoundingBox를 Center/Extents 구조로 저장함
    json WriteBounds(const BoundingBox& Bounds)
    {
        json Data;
        Data["Center"] = WriteVec3(Bounds.Center);
        Data["Extents"] = WriteVec3(Bounds.Extents);
        return Data;
    }

    // Center/Extents가 있는 JSON을 BoundingBox로 복원함
    _bool ReadBounds(const json& Data, BoundingBox* pBounds)
    {
        if (nullptr == pBounds || false == Data.contains("Center") || false == Data.contains("Extents"))
            return false;

        return ReadVec3(Data["Center"], &pBounds->Center) && ReadVec3(Data["Extents"], &pBounds->Extents);
    }

    // NaN/Inf 값이 섞였는지 확인함
    _bool IsFinite(const _float3& vValue)
    {
        return std::isfinite(vValue.x) && std::isfinite(vValue.y) && std::isfinite(vValue.z);
    }

    // 쳅터 파일 경로 기준으로 간튼 폴더의 스폰 파일 경로를 만듬
    _wstring MakeSpawnPath(const _wstring& strChapterPath, const _wstring& strChapterName)
    {
        const size_t iSlash = strChapterPath.find_last_of(L"\\/");
        if (_wstring::npos == iSlash)
            return strChapterName + L".spawn.json";

        return strChapterPath.substr(0, iSlash + 1) + strChapterName + L".spawn.json";
    }

    _wstring MakeMapGameplaySpawnPath(const _wstring& strChapterPath, const _wstring& strChapterName)
    {
        const size_t iSlash = strChapterPath.find_last_of(L"\\/");
        if (_wstring::npos == iSlash)
            return strChapterName + L".MapGameplaySpawn.json";

        return strChapterPath.substr(0, iSlash + 1) + strChapterName + L".MapGameplaySpawn.json";
    }

    _wstring MakeLegacyMapSkinnedSpawnPath(const _wstring& strChapterPath, const _wstring& strChapterName)
    {
        const size_t iSlash = strChapterPath.find_last_of(L"\\/");
        if (_wstring::npos == iSlash)
            return strChapterName + L".MapSkinnedSpawn.json";

        return strChapterPath.substr(0, iSlash + 1) + strChapterName + L".MapSkinnedSpawn.json";
    }

    _wstring MakeTriggerPath(const _wstring& strChapterPath, const _wstring& strChapterName)
    {
        const size_t iSlash = strChapterPath.find_last_of(L"\\/");

        if (_wstring::npos == iSlash)
            return strChapterName + L".Trigger.json";

        return strChapterPath.substr(0, iSlash + 1) + strChapterName + L".Trigger.json";
    }

    HRESULT LoadMonsterSpawns(const _wstring& strSpawnPath, CHAPTER_DESC* pChapter, _wstring* pError)
    {
        if (nullptr == pChapter)
            return Fail(pError, L"Invalid chapter output.");

        pChapter->MonsterSpawns.clear();

        ifstream File(strSpawnPath, ios::binary);

        // 스폰 파일은 없어도 정상 챕터로 처리한다.
        if (false == File.is_open())
            return S_OK;

        try
        {
            json Root{};
            File >> Root;

            if (false == Root.contains("MonsterSpawns") || false == Root["MonsterSpawns"].is_array())
                return S_OK;

            _uint iSerializedSpawnIndex = 0;

            for (const json& SpawnJson : Root["MonsterSpawns"])
            {
                const _uint iFallbackSpawnID = iSerializedSpawnIndex++;

                if (false == SpawnJson.is_object())
                    continue;

                MONSTER_SPAWN_DATA SpawnData{};
                // 기존 파일은 SpawnID가 없으므로 배열 순번을 Cage TargetSpawnID와 동일한 ID로 복원한다.
                SpawnData.iSpawnID = SpawnJson.value("SpawnID", iFallbackSpawnID);
                SpawnData.iSectorID = SpawnJson.value("SectorID", UINT_MAX);
                SpawnData.iSpawnNumber = SpawnJson.value("SpawnNumber", 0);
                SpawnData.fYawDegree = SpawnJson.value("YawDegree", 0.f);

                SpawnData.eMonsterType = To_MonsterType(SpawnJson.value("MonsterType", string("BUTTLER_BASE")));
                SpawnData.eWeaponType = To_WeaponType(SpawnJson.value("WeaponType", string("AUTO")));

                if (false == SpawnJson.contains("Position") || false == ReadVec3(SpawnJson["Position"], &SpawnData.vPosition))
                    continue;

                if (UINT_MAX == SpawnData.iSectorID)
                    continue;

                pChapter->MonsterSpawns.push_back(SpawnData);
            }
        }
        catch (const std::exception& Exception)
        {
            return Fail(pError, Engine::StringToWString(Exception.what()));
        }

        return S_OK;
    }


    HRESULT LoadMapGameplaySpawns(const _wstring& strSpawnPath, const _wstring& strLegacySpawnPath, CHAPTER_DESC* pChapter, _wstring* pError)
    {
        if (nullptr == pChapter)
            return Fail(pError, L"Invalid chapter output.");

        pChapter->MapGameplaySpawns.clear();

        ifstream File(strSpawnPath, ios::binary);

        // 신규 파일이 없으면 기존 MapSkinnedSpawn 파일을 그대로 읽는다.
        if (false == File.is_open())
        {
            File.clear();
            File.open(strLegacySpawnPath, ios::binary);
        }

        if (false == File.is_open())
            return S_OK;

        try
        {
            json Root{};
            File >> Root;

            const char* pSpawnKey = nullptr;

            if (Root.contains("MapGameplaySpawns") && Root["MapGameplaySpawns"].is_array())
                pSpawnKey = "MapGameplaySpawns";
            else if (Root.contains("MapSkinnedSpawns") && Root["MapSkinnedSpawns"].is_array())
                pSpawnKey = "MapSkinnedSpawns";

            if (nullptr == pSpawnKey)
                return S_OK;

            for (const json& SpawnJson : Root[pSpawnKey])
            {
                if (false == SpawnJson.is_object())
                    continue;

                MAP_GAMEPLAY_SPAWN_DATA SpawnData{};
                SpawnData.iSpawnID = SpawnJson.value("SpawnID", UINT_MAX);
                SpawnData.iSectorID = SpawnJson.value("SectorID", UINT_MAX);
                SpawnData.iTargetSpawnID = SpawnJson.value("TargetSpawnID", UINT_MAX);
                SpawnData.strActorType = Engine::StringToWString(SpawnJson.value("ActorType", string("Stargazer")));
                SpawnData.strObjectPrototypeTag = Engine::StringToWString(SpawnJson.value("ObjectPrototypeTag", string{}));
                SpawnData.bActivated = SpawnJson.value("Activated", false);
                SpawnData.fInteractionRange = SpawnJson.value("InteractionRange", 3.f);
                SpawnData.bUseHotelModel = SpawnJson.value("UseHotelModel", false);
                
                // 별바라기 텔레포트 UI에 사용할 이름과 이미지 경로를 읽는다.
                SpawnData.strStargazerLocationName = Engine::StringToWString(SpawnJson.value("StargazerLocationName", string{}));
                SpawnData.strStargazerPreviewTexturePath = SpawnJson.value("StargazerPreviewTexturePath", string{});

                SpawnData.bStartOpen = SpawnJson.value("StartOpen", false);
				SpawnData.eItemType = To_ItemType(SpawnJson.value("ItemType", string("ERGO")));
				SpawnData.strItemName = Engine::StringToWString(SpawnJson.value("ItemName", string("Ergo")));
				SpawnData.iItemCount = SpawnJson.value("ItemCount", 1u);
				SpawnData.iErgoMinAmount = SpawnJson.value("ErgoMinAmount", 100u);
				SpawnData.iErgoMaxAmount = SpawnJson.value("ErgoMaxAmount", 500u);
                SpawnData.fMapTrapDamage = SpawnJson.value("MapTrapDamage", 10.f);
                SpawnData.fMapTrapPushDistance = SpawnJson.value("MapTrapPushDistance", 0.5f);

                if (false == SpawnJson.contains("Position") || false == ReadVec3(SpawnJson["Position"], &SpawnData.vPosition))
                    continue;

                if (SpawnJson.contains("Rotation"))
                    ReadVec3(SpawnJson["Rotation"], &SpawnData.vRotation);

                if (SpawnJson.contains("Scale"))
                    ReadVec3(SpawnJson["Scale"], &SpawnData.vScale);

                if (SpawnJson.contains("MapTrapHitboxHalfExtents") && false == ReadVec3(SpawnJson["MapTrapHitboxHalfExtents"], &SpawnData.vMapTrapHitboxHalfExtents))
                    continue;

                if (SpawnJson.contains("MapTrapHitboxLocalOffset") && false == ReadVec3(SpawnJson["MapTrapHitboxLocalOffset"], &SpawnData.vMapTrapHitboxLocalOffset))
                    continue;

				SpawnData.fMapTrapBlockingRadius = SpawnJson.value("MapTrapBlockingRadius", 0.9f);
				SpawnData.fMapTrapBlockingHalfHeight = SpawnJson.value("MapTrapBlockingHalfHeight", 2.44f);

				if (SpawnJson.contains("MapTrapBlockingLocalOffset") && false == ReadVec3(SpawnJson["MapTrapBlockingLocalOffset"], &SpawnData.vMapTrapBlockingLocalOffset))
                    continue;

                if (UINT_MAX == SpawnData.iSectorID || UINT_MAX == SpawnData.iSpawnID || true == SpawnData.strActorType.empty())
                    continue;

				// 호텔 별바라기는 복구 상태와 애니메이션을 사용하지 않고 항상 Normal로 시작한다.
				if (L"Stargazer" == SpawnData.strActorType && true == SpawnData.bUseHotelModel)
					SpawnData.bActivated = true;

                pChapter->MapGameplaySpawns.push_back(SpawnData);
            }
        }
        catch (const std::exception& Exception)
        {
            return Fail(pError, Engine::StringToWString(Exception.what()));
        }

        return S_OK;
    }

    HRESULT LoadTriggers(const _wstring& strTriggerPath, CHAPTER_DESC* pChapter, _wstring* pError)
    {
        if (nullptr == pChapter)
            return Fail(pError, L"Invalid chapter output.");

        pChapter->Triggers.clear();

        ifstream File(strTriggerPath, ios::binary);

        if (false == File.is_open())
            return S_OK;

        try
        {
            json Root{};
            File >> Root;

            if (false == Root.contains("Triggers") || false == Root["Triggers"].is_array())
                return S_OK;

            for (const json& TriggerJson : Root["Triggers"])
            {
                if (false == TriggerJson.is_object())
                    continue;

                MAP_TRIGGER_DATA TriggerData{};
                TriggerData.iSectorID = TriggerJson.value("SectorID", UINT_MAX);
                TriggerData.iTriggerID = TriggerJson.value("TriggerID", UINT_MAX);
                TriggerData.iTargetSpawnID = TriggerJson.value("TargetSpawnID", UINT_MAX);
                TriggerData.iTargetFilter = TriggerJson.value("TargetFilter", ETOUI(Engine::FILTER_GROUP::PLAYER));
                TriggerData.bOnceTrigger = TriggerJson.value("OnceTrigger", false);
                TriggerData.strName = TriggerJson.value("Name", string("Trigger"));
                TriggerData.strEventKey = TriggerJson.value("EventKey", string(""));
                TriggerData.strCutSceneAssetName = TriggerJson.value("CutSceneAssetName", string(""));
                TriggerData.bCutScenePlayOnce = TriggerJson.value("CutScenePlayOnce", false);

                if (false == TriggerJson.contains("Position") || false == ReadVec3(TriggerJson["Position"], &TriggerData.vPosition))
                    continue;

                if (TriggerJson.contains("Rotation"))
                    ReadVec3(TriggerJson["Rotation"], &TriggerData.vRotation);

                if (TriggerJson.contains("Scale"))
                    ReadVec3(TriggerJson["Scale"], &TriggerData.vScale);

                if (TriggerJson.contains("Extents"))
                    ReadVec3(TriggerJson["Extents"], &TriggerData.vExtents);

                const _bool bHasEventKey = false == TriggerData.strEventKey.empty();
                const _bool bHasTargetSpawn = UINT_MAX != TriggerData.iTargetSpawnID;
                const _bool bHasCutScene = false == TriggerData.strCutSceneAssetName.empty();

                if (UINT_MAX == TriggerData.iSectorID || (false == bHasEventKey && false == bHasTargetSpawn && false == bHasCutScene))
                    continue;

                pChapter->Triggers.push_back(TriggerData);
            }
        }
        catch (const std::exception& Exception)
        {
            return Fail(pError, Engine::StringToWString(Exception.what()));
        }

        return S_OK;
    }

}

CChapterData::CChapterData()
{
}

HRESULT CChapterData::Save(const _wstring& strPath, const CHAPTER_DESC& Chapter, _wstring* pError)
{
    // 저장 전 데이터 무결성 먼저 확인함
    if (FAILED(Check(Chapter, pError)))
        return E_FAIL;

    json Root;
    Root["Version"] = Chapter.Version;
    Root["Name"] = ToUtf8(Chapter.Name);
    Root["StartSectorID"] = Chapter.StartSectorID;
    Root["Sectors"] = json::array();
    Root["Links"] = json::array();

    vector<_uint> SectorIDs;
    SectorIDs.reserve(Chapter.Sectors.size());

    for (const auto& Pair : Chapter.Sectors)
        SectorIDs.push_back(Pair.first);

    // 저장 결과가 매번 같은 순서가 되도록 ID 기준 정렬함
    sort(SectorIDs.begin(), SectorIDs.end());

    for (_uint iSectorID : SectorIDs)
    {
        const SECTOR_DESC& Sector = Chapter.Sectors.at(iSectorID);

        json Data;
        Data["ID"] = Sector.ID;
        Data["MapName"] = ToUtf8(Sector.MapName);
        Data["MapScale"] = Sector.MapScale;
        Data["Bounds"] = WriteBounds(Sector.Bounds);
        Data["Role"] = ETOUI(Sector.eRole);
        Data["OwnerSectorIDs"] = Sector.OwnerSectorIDs;

        Root["Sectors"].push_back(Data);
    }

    for (const SECTOR_LINK_DESC& Link : Chapter.Links)
    {
        json Data;
        Data["ID"] = Link.ID;
        Data["Name"] = ToUtf8(Link.Name);
        Data["SourceID"] = Link.SourceID;
        Data["TargetID"] = Link.TargetID;
        Data["Bounds"] = WriteBounds(Link.Bounds);
        Data["Rotation"] = WriteVec3(Link.Rotation);
        Data["Forward"] = WriteVec3(Link.Forward);
        Data["TwoWay"] = Link.TwoWay;
        Data["Type"] = ETOUI(Link.eType);
        Data["RenderDepth"] = Link.RenderDepth;
        Data["RenderRange"] = json::array({ Link.RenderEnter,Link.RenderExit });
        Data["UpdateRange"] = json::array({ Link.UpdateEnter,Link.UpdateExit });

        Root["Links"].push_back(Data);
    }

    ofstream File(strPath, ios::binary | ios::trunc);

    if (false == File.is_open())
        return Fail(pError, L"Chapter file open failed.");

    File << Root.dump(4);

    if (false == File.good())
        return Fail(pError, L"Chapter file write failed.");

    if (nullptr != pError)
        pError->clear();

    return S_OK;
}

HRESULT CChapterData::Load(const _wstring& strPath, CHAPTER_DESC* pChapter, _wstring* pError)
{
    // 결과를 받을 포인터가 없으면 로드 불가함
    if (nullptr == pChapter)
        return Fail(pError, L"Invalid chapter output.");

    ifstream File(strPath, ios::binary);

    if (false == File.is_open())
        return Fail(pError, L"Chapter file open failed.");

    try
    {
        json Root;
        File >> Root;

        CHAPTER_DESC Chapter{};
        Chapter.Version = Root.value("Version", VERSION);
        Chapter.Name = Engine::StringToWString(Root.value("Name", string{}));
        Chapter.StartSectorID = Root.value("StartSectorID", UINT_MAX);

        if (false == Root.contains("Sectors") || false == Root["Sectors"].is_array())
            return Fail(pError, L"Sectors array is missing.");

        // 섹터는 ID 중복을 막기 위해 map에 바로 넣음
        for (const json& Data : Root["Sectors"])
        {
            SECTOR_DESC Sector{};
            Sector.ID = Data.value("ID", UINT_MAX);
            Sector.MapName = Engine::StringToWString(Data.value("MapName", string{}));
            Sector.MapScale = Data.value("MapScale", 1.f);
            Sector.eRole = static_cast<SECTOR_ROLE>(Data.value("Role", ETOUI(SECTOR_ROLE::PRIMARY)));

            if (Data.contains("OwnerSectorIDs") && Data["OwnerSectorIDs"].is_array())
                Sector.OwnerSectorIDs = Data["OwnerSectorIDs"].get<vector<_uint>>();

            if (false == Data.contains("Bounds") || false == ReadBounds(Data["Bounds"], &Sector.Bounds))
                return Fail(pError, L"Invalid sector bounds.");

            if (false == Chapter.Sectors.emplace(Sector.ID, Sector).second)
                return Fail(pError, L"Duplicated sector ID.");
        }

        if (Root.contains("Links"))
        {
            if (false == Root["Links"].is_array())
                return Fail(pError, L"Links must be an array.");

            // 링크는 입력 순서가 의미 있을 수 있어서 vector에 그대로 보관함
            for (const json& Data : Root["Links"])
            {
                SECTOR_LINK_DESC Link{};
                Link.ID = Data.value("ID", UINT_MAX);
                Link.Name = Engine::StringToWString(Data.value("Name", string{}));
                Link.SourceID = Data.value("SourceID", UINT_MAX);
                Link.TargetID = Data.value("TargetID", UINT_MAX);
                Link.TwoWay = Data.value("TwoWay", false);
                Link.eType = static_cast<SECTOR_LINK_TYPE>(Data.value("Type", static_cast<_uint>(SECTOR_LINK_TYPE::TRANSITION_AND_RENDER))); 
                Link.RenderDepth = Data.value("RenderDepth", -1.f); 

                if (false == Data.contains("Bounds") || false == ReadBounds(Data["Bounds"], &Link.Bounds))
                    return Fail(pError, L"Invalid link bounds.");

                // Old chapter files do not have Rotation, so missing data keeps the default zero rotation.
                if (Data.contains("Rotation") && false == ReadVec3(Data["Rotation"], &Link.Rotation))
                    return Fail(pError, L"Invalid link rotation.");

                if (false == Data.contains("Forward") || false == ReadVec3(Data["Forward"], &Link.Forward))
                    return Fail(pError, L"Invalid link forward.");

                if (Data.contains("RenderRange") && Data["RenderRange"].is_array() && 2 == Data["RenderRange"].size())
                {
                    Link.RenderEnter = Data["RenderRange"][0].get<_float>();
                    Link.RenderExit = Data["RenderRange"][1].get<_float>();
                }

                if (Data.contains("UpdateRange") && Data["UpdateRange"].is_array() && 2 == Data["UpdateRange"].size())
                {
                    Link.UpdateEnter = Data["UpdateRange"][0].get<_float>();
                    Link.UpdateExit = Data["UpdateRange"][1].get<_float>();
                }

                Chapter.Links.push_back(Link);
            }
        }

        if (FAILED(Check(Chapter, pError)))
            return E_FAIL;

        if (FAILED(LoadMonsterSpawns(MakeSpawnPath(strPath, Chapter.Name), &Chapter, pError)))
            return E_FAIL;

        if (FAILED(LoadMapGameplaySpawns(MakeMapGameplaySpawnPath(strPath, Chapter.Name), MakeLegacyMapSkinnedSpawnPath(strPath, Chapter.Name), &Chapter, pError)))
            return E_FAIL;

        if (FAILED(LoadTriggers(MakeTriggerPath(strPath, Chapter.Name), &Chapter, pError)))
            return E_FAIL;

        *pChapter = move(Chapter);
    }
    catch (const std::exception& Exception)
    {
        return Fail(pError, Engine::StringToWString(Exception.what()));
    }

    if (nullptr != pError)
        pError->clear();

    return S_OK;
}

HRESULT CChapterData::Check(const CHAPTER_DESC& Chapter, _wstring* pError)
{
    // 챕터 기본 정보와 시작 섹터 먼저 검사함
    if (Chapter.Name.empty())
        return Fail(pError, L"Chapter name is empty.");

    if (Chapter.Sectors.empty())
        return Fail(pError, L"Chapter has no sectors.");

    if (Chapter.Sectors.end() == Chapter.Sectors.find(Chapter.StartSectorID))
        return Fail(pError, L"Invalid start sector.");

    for (const auto& Pair : Chapter.Sectors)
    {
        const SECTOR_DESC& Sector = Pair.second;

        // map key와 구조체 ID가 어긋나면 이후 조회가 꼬임
        if (Pair.first != Sector.ID)
            return Fail(pError, L"Sector key and ID do not match.");

        if (Sector.MapName.empty())
            return Fail(pError, L"Sector map name is empty.");

        if (false == std::isfinite(Sector.MapScale) || Sector.MapScale <= 0.f)
            return Fail(pError, L"Sector map scale must be positive.");

        if (Sector.eRole >= SECTOR_ROLE::END)
            return Fail(pError, L"Invalid sector role.");

        if (SECTOR_ROLE::SUPPORT == Sector.eRole)
        {
            if (Sector.OwnerSectorIDs.empty())
                return Fail(pError, L"Support sector has no owner sector.");

            unordered_set<_uint> OwnerIDs;

            for (_uint iOwnerSectorID : Sector.OwnerSectorIDs)
            {
                if (Sector.ID == iOwnerSectorID)
                    return Fail(pError, L"Support sector cannot own itself.");

                auto OwnerIter = Chapter.Sectors.find(iOwnerSectorID);

                if (Chapter.Sectors.end() == OwnerIter)
                    return Fail(pError, L"Support owner sector does not exist.");

                if (SECTOR_ROLE::PRIMARY != OwnerIter->second.eRole)
                    return Fail(pError, L"Support owner must be a primary sector.");

                if (false == OwnerIDs.emplace(iOwnerSectorID).second)
                    return Fail(pError, L"Duplicated support owner sector.");
            }
        }
    }

    unordered_set<_uint> LinkIDs;

    for (const SECTOR_LINK_DESC& Link : Chapter.Links)
    {
        // 링크 ID는 유효해야 하고 챕터 안에서 중복되면 안 됨
        if (UINT_MAX == Link.ID || false == LinkIDs.emplace(Link.ID).second)
            return Fail(pError, L"Invalid or duplicated link ID.");

        if (Link.SourceID == Link.TargetID)
            return Fail(pError, L"Link source and target are equal.");

        if (Chapter.Sectors.end() == Chapter.Sectors.find(Link.SourceID) || Chapter.Sectors.end() ==
            Chapter.Sectors.find(Link.TargetID))
            return Fail(pError, L"Link sector does not exist.");

        const SECTOR_DESC& SourceSector = Chapter.Sectors.at(Link.SourceID);
        const SECTOR_DESC& TargetSector = Chapter.Sectors.at(Link.TargetID);

        if (SECTOR_ROLE::SUPPORT == SourceSector.eRole || SECTOR_ROLE::SUPPORT == TargetSector.eRole)
            return Fail(pError, L"Support sector cannot be used by a sector link.");

        // 바운드 검사
        if (false == IsFinite(Link.Bounds.Center) || false == IsFinite(Link.Bounds.Extents) || false ==
            IsFinite(Link.Rotation) || false == IsFinite(Link.Forward))
            return Fail(pError, L"Link contains a non-finite value.");

        // 박스/방향 값이 계산 가능한 실수인지 확인함
        if (false == IsFinite(Link.Bounds.Center) || false == IsFinite(Link.Bounds.Extents) || false ==
            IsFinite(Link.Rotation) || false == IsFinite(Link.Forward))
            return Fail(pError, L"Link contains a non-finite value.");

        if (Link.Bounds.Extents.x <= 0.f || Link.Bounds.Extents.y <= 0.f || Link.Bounds.Extents.z <= 0.f)
            return Fail(pError, L"Link bounds extents must be positive.");

        if (XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&Link.Forward))) <= FLT_EPSILON)
            return Fail(pError, L"Link forward is zero.");

        if (Link.eType != SECTOR_LINK_TYPE::TRANSITION_ONLY && Link.eType != SECTOR_LINK_TYPE::RENDER_ONLY && Link.eType != SECTOR_LINK_TYPE::TRANSITION_AND_RENDER) // 변경 줄
            return Fail(pError, L"Invalid link type.");

        if (Link.RenderDepth < -1.f)
            return Fail(pError, L"Invalid render depth.");

        if (Link.RenderEnter < 0.f || Link.RenderExit < Link.RenderEnter)
            return Fail(pError, L"Invalid render range.");

        if (Link.UpdateEnter < 0.f || Link.UpdateExit < Link.UpdateEnter)
            return Fail(pError, L"Invalid update range.");
    }

    if (nullptr != pError)
        pError->clear();

    return S_OK;
}

NS_END
