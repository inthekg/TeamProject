#include "ChapterMapManager.h"
#include "GameInstance.h"
#include "MapWorld.h"
#include "MonsterSpawnManager.h"
#include "MapGameplayObjectManager.h"
#include "ActorSectorManager.h"
#include "Map.h"
#include "MapController.h"
#include "MapLightManager.h"
#include "NavigationManager.h"
#include "MapTriggerManager.h"
#include "MapFogManager.h"
#include "MapEffectManager.h"
#include "StargazerManager.h"
#include "MapWaterManager.h"
#include "Character.h"
#include "Transform.h"


const vector<_wstring> CChapterMapManager::g_ChapterLayerTags =
{
	TEXT("Layer_Map"),
	TEXT("Layer_MapObject"),
	TEXT("Layer_Monster"),
	TEXT("Layer_Fog"),
	TEXT("Layer_Trigger"),
};

CChapterMapManager::CChapterMapManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_pGameInstance{ CGameInstance::Get_Instance() }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

//추가
HRESULT CChapterMapManager::Unload_Chapter()
{
	if (m_pMapWorld == nullptr) return E_FAIL;

	// 맵 피직스 비활성화
	unordered_set<_wstring> Maps{};

	for (const auto& Pair : m_ChapterDesc.Sectors)
	{
		const SECTOR_DESC& Sector = Pair.second;

		if (Maps.emplace(Sector.MapName).second)
			m_pGameInstance->Set_MapPhysicsActive(Sector.MapName, false);
	}

	//하위 매니저들 해제
	Release_SubManagers();

	// 챕터 소속 레이어 해제
	for (const _wstring& strLayerTag : g_ChapterLayerTags)
		m_pGameInstance->Clear_Layer(ETOUI(m_eLevel), strLayerTag);

	m_ChapterDesc = {};
	m_strCurrentChapterPath.clear();
	m_iCurrentChapterIndex = UINT_MAX;

	return S_OK;
}

HRESULT CChapterMapManager::Reset_Chapter()
{
	if (nullptr == m_pMapWorld || nullptr == m_pMonsterSpawnManager) return E_FAIL;
	
	// 몬스터 레이어만 클리어
	m_pGameInstance->Clear_Layer(ETOUI(m_eLevel), TEXT("Layer_Monster"));
	
	// Monster 전용 매니저로 기존 스폰 섹터의 Monster만 재생성한다.
	if (FAILED(m_pMonsterSpawnManager->Respawn_Monsters())) return E_FAIL;
	
	// 이웃 Render 섹터 몬스터도 미리 활성화해 섹터 경계에서 갑자기 나타나지 않게 한다.
	m_pActorSectorManager->Update_MonsterSleep(&m_pMapWorld->Get_ActiveRenderSectorIDs());
	
	//트리거 전부 재생성 
	if (m_pMapTriggerManager)
	{
		m_pGameInstance->Clear_Layer(ETOUI(m_eLevel), TEXT("Layer_Trigger"));
		if(FAILED(m_pMapTriggerManager->Initialize_Triggers(m_ChapterDesc.Triggers))) 
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CChapterMapManager::Get_ChapterPath(_uint iChapterIndex, _wstring& OutPath)
{
	if (iChapterIndex >= g_iMaxChapter) return E_FAIL;
	
	OutPath = CGameInstance::Get_Instance()->Get_ResourcePath() + L"\\Map\\Map_Data\\" + g_ChapterFiles[iChapterIndex];
	return S_OK;
}

void CChapterMapManager::Get_CutSceneAssetNames(vector<string>* pOutAssetNames) const
{
	if (nullptr == pOutAssetNames)
		return;

	pOutAssetNames->clear();
	unordered_set<string> UniqueAssetNames = {};

	for (const MAP_TRIGGER_DATA& Trigger : m_ChapterDesc.Triggers)
	{
		if (Trigger.strCutSceneAssetName.empty() || false == UniqueAssetNames.emplace(Trigger.strCutSceneAssetName).second)
			continue;

		pOutAssetNames->push_back(Trigger.strCutSceneAssetName);
	}
}

HRESULT CChapterMapManager::Refresh_CurrentSector(_fvector vPosition)
{
	if (nullptr == m_pMapWorld)
		return E_FAIL;

	return m_pMapWorld->Refresh_CurrentSectorByPosition(vPosition);
}


void CChapterMapManager::Release_SubManagers()
{
	Safe_Release(m_pMapEffectManager);
	Safe_Release(m_pMapFogManager);
	Safe_Release(m_pMapWaterManager);
	Safe_Release(m_pMapTriggerManager);
	Safe_Release(m_pMapGameplayObjectManager);
	Safe_Release(m_pMapLightManager);
	Safe_Release(m_pMonsterSpawnManager);
	Safe_Release(m_pActorSectorManager);
	Safe_Release(m_pMapWorld);
	Safe_Release(m_pNavigationManager);
}

// 추가 끝 

//일부 추가 함 
HRESULT CChapterMapManager::Load_Chapter(const _wstring& strChapterPath, LEVEL eLevel, LEVEL eProtoLevel)
{
	//추가
	if (m_pMapWorld != nullptr) Unload_Chapter();

	_wstring strError{};

	if (FAILED(CChapterData::Load(strChapterPath, &m_ChapterDesc, &strError)))
	{
		MSG_BOX("Chapter data load failed.");
		return E_FAIL;
	}

	_uint iChapterIndex = UINT_MAX;
	const _int iResolvedChapterIndex = ETOI(eProtoLevel) - ETOI(LEVEL::CHAP_01);

	// Editor와 Shader에서도 Boss Barrier가 현재 Chapter의 보스 타입을 판별할 수 있도록 공통으로 계산한다.
	if (0 > iResolvedChapterIndex || iResolvedChapterIndex >= static_cast<_int>(g_iMaxChapter))
		return E_FAIL;

	iChapterIndex = static_cast<_uint>(iResolvedChapterIndex);

	if (LEVEL::GAMEPLAY == eLevel)
	{
		if (nullptr == m_pStargazerManager)
		{
			m_pStargazerManager = CStargazerManager::Create();

			if (nullptr == m_pStargazerManager)
				return E_FAIL;
		}

		// Object 생성 전에 Chapter 전체 Stargazer 데이터를 등록한다.
		if (FAILED(m_pStargazerManager->Register_Chapter(iChapterIndex, m_ChapterDesc)))
			return E_FAIL;

		// 호텔 별바라기에서 3 Chapter의 첫 번째 별바라기를 목적지로 읽을 수 있도록 데이터를 미리 등록한다.
		constexpr _uint iHotelChapterIndex = { 1 };
		constexpr _uint iChapter03Index = { 2 };
		constexpr _uint iChapter03FirstStargazerSpawnID = { 33 };

		if (iHotelChapterIndex == iChapterIndex)
		{
			_wstring strChapter03Path = {};
			_wstring strChapter03Error = {};
			CHAPTER_DESC Chapter03Desc = {};

			if (FAILED(Get_ChapterPath(iChapter03Index, strChapter03Path)))
				return E_FAIL;

			if (FAILED(CChapterData::Load(strChapter03Path, &Chapter03Desc, &strChapter03Error)))
				return E_FAIL;

			if (FAILED(m_pStargazerManager->Register_Chapter(iChapter03Index, Chapter03Desc)))
				return E_FAIL;

			const _uint64 iChapter03FirstStargazerID = CStargazerManager::Make_StargazerID(iChapter03Index, iChapter03FirstStargazerSpawnID);
			Respawn_Point RegisteredRespawnPoint = {};

			// 실제 방문으로 저장된 위치가 있으면 호텔 재방문 시 상수 위치로 덮어쓰지 않는다.
			if (FAILED(m_pStargazerManager->Get_RespawnPoint(iChapter03FirstStargazerID, &RegisteredRespawnPoint)))
			{
				Respawn_Point Chapter03FirstStargazerPoint = {};
				Chapter03FirstStargazerPoint.iChapterIndex = iChapter03Index;
				Chapter03FirstStargazerPoint.vPosition = g_ChapterPlayer_StartPos[iChapter03Index];

				if (FAILED(m_pStargazerManager->Activate_Stargazer(iChapter03FirstStargazerID, Chapter03FirstStargazerPoint)))
					return E_FAIL;
			}
		}
	}

	m_pMapWorld = CMapWorld::Create(m_pDevice, m_pContext, m_ChapterDesc);

	if (nullptr == m_pMapWorld)
		return E_FAIL;

	m_pActorSectorManager = CActorSectorManager::Create();

	if (nullptr == m_pActorSectorManager)
		return E_FAIL;

	m_pNavigationManager = CNavigationManager::Create();

	if (nullptr == m_pNavigationManager)
		return E_FAIL;

	if (FAILED(m_pNavigationManager->Load_ChapterNavigation(strChapterPath, m_ChapterDesc)))
		return E_FAIL;

	m_pMonsterSpawnManager = CMonsterSpawnManager::Create(m_pDevice, m_pContext, m_ChapterDesc, m_pActorSectorManager, m_pNavigationManager, eLevel, eProtoLevel);

	if (nullptr == m_pMonsterSpawnManager)
		return E_FAIL;

	// 기존 Gameplay/Editor/Shader 객체 Prototype은 계속 GAMEPLAY 레벨을 사용한다.
	m_pMapGameplayObjectManager = CMapGameplayObjectManager::Create(m_pDevice, m_pContext, m_ChapterDesc, eLevel, LEVEL::GAMEPLAY, m_pStargazerManager, iChapterIndex);

	if (nullptr == m_pMapGameplayObjectManager)
		return E_FAIL;

	m_pMapLightManager = CMapLightManager::Create();

	if (nullptr == m_pMapLightManager)
		return E_FAIL;

	if (FAILED(m_pMapLightManager->Load_ChapterLights(strChapterPath)))
		return E_FAIL;

	m_pMapFogManager = CMapFogManager::Create(m_pDevice, m_pContext);

	if (nullptr == m_pMapFogManager)
		return E_FAIL;

	if (FAILED(m_pMapFogManager->Load_ChapterFogs(strChapterPath, ETOUI(LEVEL::GAMEPLAY), TEXT("Layer_Fog"))))
		return E_FAIL;

	// 최초 로드도 몬스터 Sleep 컬링과 같은 Render 활성 섹터 집합으로 Fog 상태를 맞춘다.
	if (FAILED(m_pMapFogManager->Apply_FogSectorCulling(&m_pMapWorld->Get_ActiveRenderSectorIDs())))
		return E_FAIL;

	// 최초 섹터 변경 이벤트 전에도 현재 섹터의 노출을 바로 적용한다.
	if (FAILED(m_pMapFogManager->Apply_SectorExposure(m_pMapWorld->Get_CurrentSectorID())))
		return E_FAIL;

	m_pMapWaterManager = CMapWaterManager::Create();

	if (nullptr == m_pMapWaterManager)
		return E_FAIL;

	if (FAILED(m_pMapWaterManager->Load_ChapterWaters(strChapterPath)))
		return E_FAIL;

	m_pMapTriggerManager = CMapTriggerManager::Create(m_pDevice, m_pContext, m_pMapGameplayObjectManager, eLevel);

	if (nullptr == m_pMapTriggerManager)
		return E_FAIL;

	// 실제 Trigger 생성은 MapGameplayObject 스폰이 완료된 Spawn_All에서 처리한다.

	unordered_set<_wstring> LoadedPhysicsMaps{};

	// 현재 코드의 동작을 유지하기 위해 챕터에 포함된 맵 피직스를 활성화한다.
	for (const auto& Pair : m_ChapterDesc.Sectors)
	{
		const SECTOR_DESC& Sector = Pair.second;

		if (false == LoadedPhysicsMaps.emplace(Sector.MapName).second)
			continue;

		m_pGameInstance->Set_MapPhysicsActive(Sector.MapName, true);
	}

	// 공유되는 ChapterMapManager에서 실제 Gameplay 챕터에만 환경 이펙트를 생성한다.
	if (LEVEL::GAMEPLAY == eLevel)
	{
		m_pMapEffectManager = CMapEffectManager::Create();

		if (nullptr == m_pMapEffectManager)
			return E_FAIL;

		if (FAILED(m_pMapEffectManager->Load_ChapterEffects(strChapterPath)))
			return E_FAIL;

		// 최초 섹터 변경 이벤트 전에도 현재 렌더 섹터의 환경 이펙트를 바로 활성화한다.
		if (FAILED(m_pMapEffectManager->Apply_EffectSectorCulling(&m_pMapWorld->Get_ActiveRenderSectorIDs())))
			return E_FAIL;
	}

	//추가
	m_strCurrentChapterPath = strChapterPath;
	m_eLevel = eLevel;
	m_iCurrentChapterIndex = iChapterIndex;

	return S_OK;
}

HRESULT CChapterMapManager::Load_LogoChapter(const _wstring& strChapterPath)
{
	if (nullptr != m_pMapWorld)
		Unload_Chapter();

	_wstring strError{};

	if (FAILED(CChapterData::Load(strChapterPath, &m_ChapterDesc, &strError)))
	{
		MSG_BOX("Logo chapter data load failed.");
		return E_FAIL;
	}

	if (false == m_ChapterDesc.MonsterSpawns.empty() || false == m_ChapterDesc.Triggers.empty())
		return E_FAIL;

	// Logo Chapter 데이터가 Gameplay 객체를 실수로 생성하지 못하게 입력 단계에서 차단한다.
	for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : m_ChapterDesc.MapGameplaySpawns)
	{
		if (L"MapGameplayObject" != SpawnData.strActorType)
			return E_FAIL;
	}

	unordered_set<_wstring> LoadedMaps{};

	for (const auto& Pair : m_ChapterDesc.Sectors)
	{
		const SECTOR_DESC& Sector = Pair.second;

		if (false == LoadedMaps.emplace(Sector.MapName).second)
			continue;

		Engine::CMap* pOriginalMap = nullptr;

		if (FAILED(m_pGameInstance->Load_Map(Sector.MapName, &pOriginalMap, Sector.MapScale, false)))
			return E_FAIL;

		Engine::CMap* pHLODMap = nullptr;

		if (FAILED(m_pGameInstance->Load_HLODMap(Sector.MapName, &pHLODMap)))
			return E_FAIL;
	}

	m_pMapWorld = CMapWorld::Create(m_pDevice, m_pContext, m_ChapterDesc);

	if (nullptr == m_pMapWorld)
		return E_FAIL;

	// Logo도 Chapter와 같은 이름의 Light JSON을 기존 MapLightManager로 불러온다.
	m_pMapLightManager = CMapLightManager::Create();

	if (nullptr == m_pMapLightManager)
		return E_FAIL;

	if (FAILED(m_pMapLightManager->Load_ChapterLights(strChapterPath)))
		return E_FAIL;

	m_pMapGameplayObjectManager = CMapGameplayObjectManager::Create(m_pDevice, m_pContext, m_ChapterDesc, LEVEL::LOGO, LEVEL::LOGO);

	if (nullptr == m_pMapGameplayObjectManager)
		return E_FAIL;

	m_strCurrentChapterPath = strChapterPath;
	m_eLevel = LEVEL::LOGO;
	m_iCurrentChapterIndex = UINT_MAX;
	return S_OK;
}


void CChapterMapManager::Update(_fvector vPlayerPosition, _fvector vCameraPosition)
{
	if (nullptr == m_pMapWorld)
		return;

	// Debug Free Camera에서는 vPlayerPosition 위치가 Camera일 수 있다.
	m_pMapWorld->Update(vPlayerPosition, vCameraPosition);

	if (LEVEL::GAMEPLAY == m_eLevel && nullptr != m_pStargazerManager && UINT_MAX != m_iCurrentChapterIndex)
	{
		CCharacter* pPlayer = m_pGameInstance->Get_Player();

		if (nullptr != pPlayer && nullptr != pPlayer->Get_Transform())
		{
			const _vector vActualPlayerPosition = pPlayer->Get_Transform()->Get_State(STATE::POSITION);
			const _uint iPlayerSectorID = m_pMapWorld->Find_PrimarySectorByPosition(vActualPlayerPosition);

			// 실제 Player가 포함된 Primary Sector만 방문 처리한다.
			if (UINT_MAX != iPlayerSectorID)
			{
				Respawn_Point SectorVisitPoint = {};
				SectorVisitPoint.iChapterIndex = m_iCurrentChapterIndex;
				XMStoreFloat4(&SectorVisitPoint.vPosition, XMVectorSetW(vActualPlayerPosition, 1.f));

				_vector vPlayerLook = XMVectorSetY(pPlayer->Get_Transform()->Get_State(STATE::LOOK), 0.f);

				if (XMVectorGetX(XMVector3LengthSq(vPlayerLook)) > 1e-8f)
				{
					vPlayerLook = XMVector3Normalize(vPlayerLook);
					SectorVisitPoint.fYaw = atan2f(XMVectorGetX(vPlayerLook), XMVectorGetZ(vPlayerLook));
				}

				m_pStargazerManager->Visit_Sector(m_iCurrentChapterIndex, iPlayerSectorID, SectorVisitPoint);
			}
		}
	}

	// 이전 프레임에 재생이 끝난 챕터 이펙트의 보관 참조를 정리한다.
	if (nullptr != m_pMapEffectManager)
		m_pMapEffectManager->Update();

	if (nullptr != m_pMapFogManager)
		m_pMapFogManager->Update_ExposureTuning();

	if (nullptr != m_pMapWaterManager)
		m_pMapWaterManager->Update_Reflection(m_pMapWorld);
}

HRESULT CChapterMapManager::Resolve_StargazerTravel(_uint64 iTargetStargazerID, _uint iCurrentChapterIndex, _uint iCurrentSectorID, Respawn_Point* pOutPoint) const
{
	if (nullptr == m_pStargazerManager)
		return E_FAIL;

	return m_pStargazerManager->Resolve_Travel(iTargetStargazerID, iCurrentChapterIndex, iCurrentSectorID, pOutPoint);
}


void CChapterMapManager::Late_Update(_float fTimeDelta)
{
	if (m_pMapWorld == nullptr)
		return;

	if (nullptr != m_pMapTriggerManager)
	{
		_float3 vElevatorTransferPosition{};

		// PhysX Callback 이후 Item/Chest 제거와 예약된 Elevator 순간이동을 처리한다.
		const HRESULT hPendingResult = m_pMapTriggerManager->Flush_PendingActions(fTimeDelta, &vElevatorTransferPosition);

		if (S_OK == hPendingResult)
		{
			// 순간이동 위치 기준으로 현재 Sector와 Simulation Sector를 즉시 갱신한다.
			Refresh_CurrentSector(XMLoadFloat3(&vElevatorTransferPosition));
		}
		else if (FAILED(hPendingResult))
		{
			// 컷신 JSON 누락 또는 재생 실패가 Chapter 중계 과정에서 조용히 사라지지 않게 남긴다.
			OutputDebugStringA("[Chapter Map Pending Action Error]\n");
		}
	}

	// 최신 카메라/PipeLine 갱신 이후 MapController 컬링을 실행한다.
	m_pMapWorld->Late_Update(fTimeDelta);

	if (nullptr != m_pMapLightManager)
	{
		// 같은 프레임의 MapController 캐시로 라이트 Bounds를 갱신한다.
		m_pMapLightManager->Update(m_pMapWorld->Get_MapController());
	}
}

HRESULT CChapterMapManager::Spawn_All()
{
	if (nullptr == m_pMapWorld || nullptr == m_pMonsterSpawnManager || nullptr == m_pMapGameplayObjectManager || nullptr == m_pMapTriggerManager || nullptr == m_pActorSectorManager)
		return E_FAIL;

	// Cage Dynamic Shape를 먼저 Scene에 등록한 뒤 Monster CCT를 생성해 첫 이동부터 바닥을 조회하게 한다.
	if (FAILED(m_pMapGameplayObjectManager->Spawn_All()))
		return E_FAIL;

	if (FAILED(m_pMonsterSpawnManager->Spawn_All()))
		return E_FAIL;

	// MapGameplayObject가 모두 등록된 뒤 Trigger를 생성하고 TargetSpawnID의 실제 오너를 연결한다.
	if (FAILED(m_pMapTriggerManager->Initialize_Triggers(m_ChapterDesc.Triggers)))
		return E_FAIL;

	// 전체 스폰 후 이웃을 포함한 Render 섹터 기준으로 몬스터 상태를 맞춘다.
	m_pActorSectorManager->Update_MonsterSleep(&m_pMapWorld->Get_ActiveRenderSectorIDs());


	return S_OK;
}

HRESULT CChapterMapManager::Spawn_StartSector()
{
	if (nullptr == m_pMapWorld || nullptr == m_pMonsterSpawnManager || nullptr == m_pMapGameplayObjectManager || nullptr == m_pActorSectorManager)
		return E_FAIL;

	const _uint iSectorID = m_pMapWorld->Get_CurrentSectorID();

	// 시작 섹터에서도 이웃을 포함한 Render 섹터 기준으로 몬스터 상태를 먼저 갱신한다.
	m_pActorSectorManager->Update_MonsterSleep(&m_pMapWorld->Get_ActiveRenderSectorIDs());

	// 섹터 진입 시에도 Cage Dynamic Shape를 Monster CCT보다 먼저 등록한다.
	if (FAILED(m_pMapGameplayObjectManager->Spawn_ForSector(iSectorID)))
		return E_FAIL;

	if (FAILED(m_pMonsterSpawnManager->Spawn_ForSector(iSectorID)))
		return E_FAIL;

	return S_OK;
}

HRESULT CChapterMapManager::Spawn_MapGameplayObjects()
{
	if (nullptr == m_pMapWorld || nullptr == m_pMapGameplayObjectManager)
		return E_FAIL;

	// Logo 경로는 Monster, Trigger, Navigation 매니저 없이 장식물만 생성한다.
	return m_pMapGameplayObjectManager->Spawn_All();
}

Engine::CMap* CChapterMapManager::Get_CurrentMap() const
{
	return nullptr == m_pMapWorld ? nullptr : m_pMapWorld->Get_CurrentMap();
}

Engine::CMapController* CChapterMapManager::Get_MapController() const
{
	return nullptr == m_pMapWorld ? nullptr : m_pMapWorld->Get_MapController();
}

Engine::CNavigation* CChapterMapManager::Find_Navigation(_uint iSectorID) const
{
	return nullptr == m_pNavigationManager ? nullptr : m_pNavigationManager->Find_Navigation(iSectorID);
}

_uint CChapterMapManager::Get_CurrentSectorID() const
{
	return nullptr == m_pMapWorld ? UINT_MAX : m_pMapWorld->Get_CurrentSectorID();
}

_uint CChapterMapManager::Find_PrimarySectorByPosition(_fvector vPosition) const
{
	return nullptr == m_pMapWorld ? UINT_MAX : m_pMapWorld->Find_PrimarySectorByPosition(vPosition);
}

CChapterMapManager* CChapterMapManager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CChapterMapManager* pInstance = new CChapterMapManager(pDevice, pContext);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CChapterMapManager");
		return nullptr;
	}

	return pInstance;
}

void CChapterMapManager::Free()
{
	Release_SubManagers();

	// Chapter Unload가 아니라 ChapterMapManager 자체가 종료될 때만 Runtime 기록을 제거한다.
	Safe_Release(m_pStargazerManager);

	m_ChapterDesc.Sectors.clear();
	m_ChapterDesc.Links.clear();
	m_ChapterDesc.MonsterSpawns.clear();
	m_ChapterDesc.MapGameplaySpawns.clear();
	m_ChapterDesc.Triggers.clear();

	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);

	__super::Free();
}
