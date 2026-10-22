#include "StargazerManager.h"

_uint64 CStargazerManager::Make_StargazerID(_uint iChapterIndex, _uint iSpawnID)
{
	if (UINT_MAX == iChapterIndex || UINT_MAX == iSpawnID)
		return INVALID_STARGAZER_ID;

	return (static_cast<_uint64>(iChapterIndex) << 32) | static_cast<_uint64>(iSpawnID);
}

STARGAZER_RUNTIME_DATA* CStargazerManager::Find_RuntimeData(_uint64 iStargazerID)
{
	for (STARGAZER_RUNTIME_DATA& RuntimeData : m_RuntimeData)
	{
		if (RuntimeData.iStargazerID == iStargazerID)
			return &RuntimeData;
	}

	return nullptr;
}

const STARGAZER_RUNTIME_DATA* CStargazerManager::Find_RuntimeData(_uint64 iStargazerID) const
{
	for (const STARGAZER_RUNTIME_DATA& RuntimeData : m_RuntimeData)
	{
		if (RuntimeData.iStargazerID == iStargazerID)
			return &RuntimeData;
	}

	return nullptr;
}

HRESULT CStargazerManager::Register_Chapter(_uint iChapterIndex, const CHAPTER_DESC& ChapterDesc)
{
	if (iChapterIndex >= g_iMaxChapter || ChapterDesc.Name.empty())
		return E_FAIL;

	// 이미 등록한 Chapter는 기존 Runtime 활성화와 방문 기록을 유지한다.
	if (m_RegisteredChapterIndices.end() != m_RegisteredChapterIndices.find(iChapterIndex))
		return S_OK;

	unordered_set<_uint> StargazerSpawnIDs = {};
	vector<STARGAZER_RUNTIME_DATA> NewRuntimeData = {};

	for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : ChapterDesc.MapGameplaySpawns)
	{
		if (L"Stargazer" != SpawnData.strActorType)
			continue;

		if (UINT_MAX == SpawnData.iSpawnID || UINT_MAX == SpawnData.iSectorID)
			return E_FAIL;

		if (false == StargazerSpawnIDs.emplace(SpawnData.iSpawnID).second)
			return E_FAIL;

		const _uint64 iStargazerID = Make_StargazerID(iChapterIndex, SpawnData.iSpawnID);

		if (INVALID_STARGAZER_ID == iStargazerID || nullptr != Find_RuntimeData(iStargazerID))
			return E_FAIL;

		STARGAZER_RUNTIME_DATA RuntimeData = {};
		RuntimeData.iStargazerID = iStargazerID;
		RuntimeData.iChapterIndex = iChapterIndex;
		RuntimeData.iSpawnID = SpawnData.iSpawnID;
		RuntimeData.iSectorID = SpawnData.iSectorID;
		RuntimeData.strChapterName = ChapterDesc.Name;

		// 추가
		// Spawn JSON에서 읽은 별바라기 UI 정보를 Runtime 데이터에 보관
		RuntimeData.strLocationName = SpawnData.strStargazerLocationName;
		RuntimeData.strPreviewTexturePath = SpawnData.strStargazerPreviewTexturePath;
		RuntimeData.bIsHotel = SpawnData.bUseHotelModel;

		// 호텔 모델은 복구 애니메이션이 없는 Normal 전용 Stargazer다.
		RuntimeData.bActivated = true == SpawnData.bUseHotelModel ? true : SpawnData.bActivated;

		NewRuntimeData.push_back(RuntimeData);
	}

	// Chapter 데이터 검증이 끝난 뒤 Runtime Vector에 반영한다.
	m_RuntimeData.insert(m_RuntimeData.end(), NewRuntimeData.begin(), NewRuntimeData.end());
	m_RegisteredChapterIndices.emplace(iChapterIndex);

	return S_OK;
}

HRESULT CStargazerManager::Activate_Stargazer(_uint64 iStargazerID, const Respawn_Point& RespawnPoint)
{
	STARGAZER_RUNTIME_DATA* pRuntimeData = Find_RuntimeData(iStargazerID);

	if (nullptr == pRuntimeData)
		return E_FAIL;

	pRuntimeData->bActivated = true;
	pRuntimeData->bSectorVisited = true;
	pRuntimeData->bHasRespawnPoint = true;
	pRuntimeData->RespawnPoint = RespawnPoint;
	pRuntimeData->RespawnPoint.iChapterIndex = pRuntimeData->iChapterIndex;

	return S_OK;
}

HRESULT CStargazerManager::Update_RespawnPoint(_uint64 iStargazerID, const Respawn_Point& RespawnPoint)
{
	STARGAZER_RUNTIME_DATA* pRuntimeData = Find_RuntimeData(iStargazerID);

	if (nullptr == pRuntimeData || false == pRuntimeData->bActivated)
		return E_FAIL;

	pRuntimeData->bSectorVisited = true;
	pRuntimeData->bHasRespawnPoint = true;
	pRuntimeData->RespawnPoint = RespawnPoint;
	pRuntimeData->RespawnPoint.iChapterIndex = pRuntimeData->iChapterIndex;

	return S_OK;
}

void CStargazerManager::Visit_Sector(_uint iChapterIndex, _uint iSectorID, const Respawn_Point& RespawnPoint)
{
	if (UINT_MAX == iChapterIndex || UINT_MAX == iSectorID)
		return;

	for (STARGAZER_RUNTIME_DATA& RuntimeData : m_RuntimeData)
	{
		if (RuntimeData.iChapterIndex != iChapterIndex || RuntimeData.iSectorID != iSectorID)
			continue;

		RuntimeData.bSectorVisited = true;

		// 처음부터 Normal인 Stargazer는 해당 Sector에 실제 진입한 Player 위치를 최초 안전 위치로 사용한다.
		if (true == RuntimeData.bActivated && false == RuntimeData.bHasRespawnPoint)
		{
			RuntimeData.RespawnPoint = RespawnPoint;
			RuntimeData.RespawnPoint.iChapterIndex = iChapterIndex;
			RuntimeData.bHasRespawnPoint = true;
		}
	}
}

_bool CStargazerManager::Is_Activated(_uint64 iStargazerID) const
{
	const STARGAZER_RUNTIME_DATA* pRuntimeData = Find_RuntimeData(iStargazerID);
	return nullptr != pRuntimeData ? pRuntimeData->bActivated : false;
}


HRESULT CStargazerManager::Build_UIOpenDesc(_uint64 iSourceStargazerID, STARGAZER_UI_OPEN_DESC* pOutDesc) const
{
	if (nullptr == pOutDesc)
		return E_FAIL;

	const STARGAZER_RUNTIME_DATA* pSourceData = Find_RuntimeData(iSourceStargazerID);

	if (nullptr == pSourceData || false == pSourceData->bActivated)
		return E_FAIL;

	pOutDesc->iSourceStargazerID = iSourceStargazerID;
	pOutDesc->TravelEntries.clear();

	pOutDesc->bHotelShortcutAvailable = false;
	pOutDesc->iHotelStargazerID = INVALID_STARGAZER_ID;
	pOutDesc->strHotelPreviewTexturePath.clear();

	for (const STARGAZER_RUNTIME_DATA& RuntimeData : m_RuntimeData)
	{
		if (false == RuntimeData.bActivated || false == RuntimeData.bSectorVisited || false == RuntimeData.bHasRespawnPoint)
			continue;

		// 호텔은 일반 챕터 및 지역 목록에 넣지 않고 하단 고정 버튼으로 분리
		if (RuntimeData.bIsHotel)
		{
			pOutDesc->bHotelShortcutAvailable = RuntimeData.iStargazerID != iSourceStargazerID;
			pOutDesc->iHotelStargazerID = RuntimeData.iStargazerID;
			pOutDesc->strHotelPreviewTexturePath = RuntimeData.strPreviewTexturePath;
			continue;
		}

		STARGAZER_TRAVEL_ENTRY Entry = {};
		Entry.iStargazerID = RuntimeData.iStargazerID;
		Entry.iChapterIndex = RuntimeData.iChapterIndex;
		Entry.iSpawnID = RuntimeData.iSpawnID;
		Entry.iSectorID = RuntimeData.iSectorID;

		Entry.strChapterName = RuntimeData.strChapterName;
		Entry.strLocationName = RuntimeData.strLocationName;
		Entry.strPreviewTexturePath = RuntimeData.strPreviewTexturePath;

		pOutDesc->TravelEntries.push_back(Entry);
	}

	// Runtime Vector 등록 순서와 무관하게 Chapter, SpawnID 순서로 UI에 제공한다.
	sort(pOutDesc->TravelEntries.begin(), pOutDesc->TravelEntries.end(), [](const STARGAZER_TRAVEL_ENTRY& Left, const STARGAZER_TRAVEL_ENTRY& Right)
		{
			if (Left.iChapterIndex != Right.iChapterIndex)
				return Left.iChapterIndex < Right.iChapterIndex;

			return Left.iSpawnID < Right.iSpawnID;
		});

	return S_OK;
}

HRESULT CStargazerManager::Get_RespawnPoint(_uint64 iStargazerID, Respawn_Point* pOutPoint) const
{
	if (nullptr == pOutPoint)
		return E_FAIL;

	const STARGAZER_RUNTIME_DATA* pRuntimeData = Find_RuntimeData(iStargazerID);

	if (nullptr == pRuntimeData || false == pRuntimeData->bHasRespawnPoint)
		return E_FAIL;

	*pOutPoint = pRuntimeData->RespawnPoint;
	return S_OK;
}

HRESULT CStargazerManager::Resolve_Travel(_uint64 iTargetStargazerID, _uint iCurrentChapterIndex, _uint iCurrentSectorID, Respawn_Point* pOutPoint) const
{
	if (nullptr == pOutPoint)
		return E_FAIL;

	const STARGAZER_RUNTIME_DATA* pRuntimeData = Find_RuntimeData(iTargetStargazerID);

	if (nullptr == pRuntimeData || false == pRuntimeData->bActivated || false == pRuntimeData->bSectorVisited || false == pRuntimeData->bHasRespawnPoint)
		return E_FAIL;

	// 같은 Chapter 이동은 현재 Sector 안에서만 허용한다.
	if (pRuntimeData->iChapterIndex == iCurrentChapterIndex && pRuntimeData->iSectorID != iCurrentSectorID)
		return E_FAIL;

	*pOutPoint = pRuntimeData->RespawnPoint;
	return S_OK;
}

CStargazerManager* CStargazerManager::Create()
{
	return new CStargazerManager();
}

void CStargazerManager::Free()
{
	m_RegisteredChapterIndices.clear();
	m_RuntimeData.clear();

	__super::Free();
}
