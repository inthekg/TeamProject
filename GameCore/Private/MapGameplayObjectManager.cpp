#include "MapGameplayObjectManager.h"

#include "GameInstance.h"
#include "MapGameplayObject.h"
#include "BossArenaBarrier.h"
#include "HotelDoor.h"
#include "Stargazer.h"
#include "Ladder.h"
#include "NPC.h"
#include "Elevator.h"
#include "MapActivator.h"
#include "MapTrap.h"
#include "RollingBallTrap.h"
#include "PressurePlate.h"
#include "NeedleLauncher.h"
#include "SteppingStone.h"
#include "MonasteryBridge.h"
#include "Item.h"
#include "TreasureChest.h"
#include "Player.h"
#include "SpringArm.h"
#include "StargazerManager.h"

CMapGameplayObjectManager::CMapGameplayObjectManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLevel, LEVEL ePrototypeLevel, CStargazerManager* pStargazerManager, _uint iChapterIndex)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_pGameInstance{ CGameInstance::Get_Instance() }
	, m_pStargazerManager{ pStargazerManager }
	, m_iChapterIndex{ iChapterIndex }
	, m_eCurrentLevel{ eLevel }
	, m_ePrototypeLevel{ ePrototypeLevel }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CMapGameplayObjectManager::Initialize(const CHAPTER_DESC& ChapterDesc)
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : ChapterDesc.MapGameplaySpawns)
	{
		if (UINT_MAX == SpawnData.iSectorID || UINT_MAX == SpawnData.iSpawnID || SpawnData.strActorType.empty())
			continue;

		// Logo Chapter에서는 입력과 Gameplay 시스템이 없는 장식용 객체만 허용한다.
		if (LEVEL::LOGO == m_eCurrentLevel && L"MapGameplayObject" != SpawnData.strActorType)
			return E_FAIL;

		m_MapGameplaySpawnsBySector[SpawnData.iSectorID].push_back(SpawnData);
	}

	// Logo는 Elevator/Trap 설정과 Gameplay 이벤트 구독을 사용하지 않는다.
	if (LEVEL::LOGO == m_eCurrentLevel)
		return S_OK;

	if (FAILED(Load_ElevatorSettings(ChapterDesc.Name)))
		return E_FAIL;

	if (FAILED(Validate_ElevatorData()))
		return E_FAIL;

	if (FAILED(Load_RollingBallSettings(ChapterDesc.Name)))
		return E_FAIL;

	if (FAILED(Validate_RollingBallData()))
		return E_FAIL;

	if (FAILED(Validate_SteppingStoneBridgeData()))
		return E_FAIL;

	// LadderActivater가 보낸 SpawnID를 공통 GameplayObject Hook으로 전달한다.
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::LADDER_OPEN, this);
	m_bLadderOpenSubscribed = true;

	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::ELEVATOR_START, this);
	m_bElevatorStartSubscribed = true;

	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::TRAP_OPEN, this);
	m_bTrapOpenSubscribed = true;

	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::STEPPING_STONE, this);
	m_bSteppingStoneSubscribed = true;

	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::DOOR_ACTIVATE, this);
	m_bDoorActivateSubscribed = true;

	return S_OK;
}

HRESULT CMapGameplayObjectManager::Spawn_All()
{
	for (const auto& Pair : m_MapGameplaySpawnsBySector)
	{
		if (FAILED(Spawn_ForSector(Pair.first)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CMapGameplayObjectManager::Spawn_ForSector(_uint iSectorID)
{
	if (UINT_MAX == iSectorID)
		return S_OK;

	if (m_SpawnedSectorIDs.end() != m_SpawnedSectorIDs.find(iSectorID))
		return S_OK;

	auto SpawnIter = m_MapGameplaySpawnsBySector.find(iSectorID);

	if (m_MapGameplaySpawnsBySector.end() == SpawnIter)
	{
		// SpawnData가 없는 섹터도 처리 완료로 기록한다.
		m_SpawnedSectorIDs.emplace(iSectorID);
		return S_OK;
	}

	for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : SpawnIter->second)
	{
		if (FAILED(Spawn_MapGameplayObject(SpawnData)))
			return E_FAIL;
	}

	// Gameplay Chapter만 상호작용 객체의 후속 연결을 수행한다.
	if (LEVEL::LOGO != m_eCurrentLevel)
	{
		if (FAILED(Resolve_ElevatorLinks(iSectorID)))
			return E_FAIL;

		if (FAILED(Resolve_BossBarrierLinks(iSectorID)))
			return E_FAIL;
	}

	m_SpawnedSectorIDs.emplace(iSectorID);

	return S_OK;
}
HRESULT CMapGameplayObjectManager::Spawn_MapGameplayObject(const MAP_GAMEPLAY_SPAWN_DATA& SpawnData)
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	if (L"MapGameplayObject" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(m_ePrototypeLevel);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Steelheart");
		Desc.strAnimFolderTag = TEXT("Steelheart");
		Desc.strAnimTag = TEXT("Steelheart");
		Desc.strDefaultClip = TEXT("SM_steelheart_V02|AS_steelheart_pump_V02");
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bStaticAnimation = true;
		Desc.bControllerEnabled = false;
		Desc.bPlayDefault = true;
		Desc.bApplyRootMotion = false;
		Desc.bLoopDefaultClip = true;
		Desc.bInteractionEnabled = false;
		Desc.bUsePhysics = false;
		Desc.bPhysicsStartRegistered = false;

		// 범용 CMapGameplayObject에 Steelheart Idle 설정만 전달한다.
		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_MapGameplayObject") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (0 == SpawnData.strActorType.find(L"Ladder_"))
		return Spawn_Ladder(SpawnData);

	if (0 == SpawnData.strActorType.find(L"NPC_"))
		return Spawn_NPC(SpawnData);

	if (L"Item_Lily" == SpawnData.strActorType)
	{
		CItem::ITEM_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::STATIC;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Item");
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.eItemType = SpawnData.eItemType;
		Desc.strItemName = SpawnData.strItemName;
		Desc.iItemCount = SpawnData.iItemCount;
		Desc.iErgoMinAmount = SpawnData.iErgoMinAmount;
		Desc.iErgoMaxAmount = SpawnData.iErgoMaxAmount;

		// 독립 배치 Item은 생성 즉시 연결된 외부 Map Trigger 상호작용을 허용한다.
		Desc.bPickupEnabled = true;

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_Item") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"TreasureChest" == SpawnData.strActorType)
	{
		CTreasureChest::TREASURE_CHEST_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_TreasureChest");
		Desc.strAnimFolderTag = TEXT("TreasureChest");
		Desc.strAnimTag = TEXT("TreasureChest");
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bUseInteractPosition = true;
		Desc.vInteractLocalOffsetXZ = _float2(0.88f, 0.f);
		Desc.fInteractLocalYaw = XMConvertToRadians(-90.f);
		// TreasureChest가 내부 Item을 직접 생성해서 같은 Runtime Layer에 추가한다.
		Desc.iItemPrototypeLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.iItemLayerLevel = ETOUI(m_eCurrentLevel);
		Desc.strItemPrototypeTag = TEXT("Prototype_GameObject_Item");
		Desc.strItemLayerTag = TEXT("Layer_MapObject");
		Desc.eItemType = SpawnData.eItemType;
		Desc.strItemName = SpawnData.strItemName;
		Desc.iItemCount = SpawnData.iItemCount;
		Desc.iErgoMinAmount = SpawnData.iErgoMinAmount;
		Desc.iErgoMaxAmount = SpawnData.iErgoMaxAmount;

		// 상자 본체의 Static Rigidbody 충돌은 그대로 유지하고 상호작용만 외부 Map Trigger가 담당한다.
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = true;
		Desc.vPhysicsHalfExtents = _float3(0.5f, 0.4f, 0.5f);
		Desc.vPhysicsLocalOffset = _float3(0.f, 0.6f, 0.f);
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_TreasureChest") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Stargazer" == SpawnData.strActorType)
	{
		CStargazer::STARGAZER_DESC Desc{};

		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.fInteractionRange = SpawnData.fInteractionRange;
		Desc.bActivated = SpawnData.bActivated;
		Desc.bUseHotelModel = SpawnData.bUseHotelModel;

		if (nullptr != m_pStargazerManager && UINT_MAX != m_iChapterIndex)
		{
			Desc.iStargazerID = CStargazerManager::Make_StargazerID(m_iChapterIndex, SpawnData.iSpawnID);
			Desc.pStargazerManager = m_pStargazerManager;
			Desc.bActivated = m_pStargazerManager->Is_Activated(Desc.iStargazerID);
		}

		// 기존 Stargazer Physics 설정을 유지한다.
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = Desc.bActivated;

		if (true == SpawnData.bUseHotelModel)
		{
			const _float fHotelModelPreScale = 0.01f;
			const _float fHotelPhysicsScale = 0.75f;

			// 호텔 시계 모델 바운드의 75% 크기를 Spawn Scale까지 반영한다.
			Desc.vPhysicsHalfExtents = _float3(3.61f * fHotelModelPreScale * fHotelPhysicsScale * fabsf(SpawnData.vScale.x), 9.95f * fHotelModelPreScale * fHotelPhysicsScale * fabsf(SpawnData.vScale.y), 3.47f * fHotelModelPreScale * fHotelPhysicsScale * fabsf(SpawnData.vScale.z));
			Desc.vPhysicsLocalOffset = _float3(0.f, 9.943f * fHotelModelPreScale * SpawnData.vScale.y, -0.249f * fHotelModelPreScale * SpawnData.vScale.z);
		}
		else
		{
			// 일반 별바라기는 기존 Physics 크기를 유지한다.
			Desc.vPhysicsHalfExtents = _float3(0.3f, 0.6f, 0.3f);
			Desc.vPhysicsLocalOffset = _float3(0.f, 0.3f, 0.f);
		}

		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_Stargazer") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"HotelDoor" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bUseInteractPosition = true;
		// 문 표면과 Player CCT 반경이 겹치지 않도록 정면 중앙에서 0.8만큼 떨어진다.
		Desc.vInteractLocalOffsetXZ = _float2(0.8f, 0.f);
		Desc.fInteractLocalYaw = XMConvertToRadians(-90.f);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_HotelDoor");
		Desc.strAnimFolderTag = TEXT("Hotel_Door");
		Desc.strAnimTag = TEXT("Hotel_Door");
		Desc.bControllerEnabled = true;
		Desc.bPlayDefault = false;
		Desc.bApplyRootMotion = false;

		// Door_R, Door_L Bone을 따라가는 Kinematic Dynamic Collider가 Player와 Monster를 막는다.
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = true;
		// HotelDoor 전용 Dynamic Physics에서 Width, Height, Depth 순서로 해석한다.
		Desc.vPhysicsHalfExtents = _float3(0.8f * fabsf(SpawnData.vScale.x), 1.5f * fabsf(SpawnData.vScale.y), 0.15f * fabsf(SpawnData.vScale.z));
		Desc.vPhysicsLocalOffset = _float3(0.f, 0.f, 0.f);
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_HotelDoor") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Hotel_InDoor" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bUseInteractPosition = true;
		Desc.vInteractLocalOffsetXZ = _float2(0.8f, 0.f);
		Desc.fInteractLocalYaw = XMConvertToRadians(-90.f);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Hotel_InDoor");
		Desc.strAnimFolderTag = TEXT("Hotel_InDoor");
		Desc.strAnimTag = TEXT("Hotel_InDoor");
		Desc.bControllerEnabled = true;
		Desc.bPlayDefault = false;
		Desc.bApplyRootMotion = false;
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = true;
		// HotelDoor 전용 Dynamic Physics에서 Width, Height, Depth 순서로 해석한다.
		Desc.vPhysicsHalfExtents = _float3(0.8f * fabsf(SpawnData.vScale.x), 1.5f * fabsf(SpawnData.vScale.y), 0.15f * fabsf(SpawnData.vScale.z));
		Desc.vPhysicsLocalOffset = _float3(0.f, 0.f, 0.f);
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_HotelDoor") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Monastery_Door" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bUseInteractPosition = true;
		Desc.vInteractLocalOffsetXZ = _float2(0.8f, 0.f);
		Desc.fInteractLocalYaw = XMConvertToRadians(-90.f);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Monastery_Door");
		Desc.strAnimFolderTag = TEXT("Monastery_Door");
		Desc.strAnimTag = TEXT("Monastery_Door");
		Desc.bControllerEnabled = true;
		Desc.bPlayDefault = false;
		Desc.bApplyRootMotion = false;
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = true;
		// Door_R/Door_L 가중 정점 범위와 기존 HotelDoor 충돌 비율을 적용한 Width, Height, Depth 반경이다.
		Desc.vPhysicsHalfExtents = _float3(1.1f * fabsf(SpawnData.vScale.x), 3.1f * fabsf(SpawnData.vScale.y), 0.55f * fabsf(SpawnData.vScale.z));
		Desc.vPhysicsLocalOffset = _float3(0.f, 0.f, 0.f);
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_HotelDoor") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Train_Door" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bUseInteractPosition = true;
		// Train_Door의 손잡이 위치에 맞춰 Player를 문 앞에서 로컬 우측으로 보정한다.
		Desc.vInteractLocalOffsetXZ = _float2(0.9f, -1.65f);
		Desc.fInteractLocalYaw = XMConvertToRadians(-90.f);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Train_Door");
		Desc.strAnimFolderTag = TEXT("Train_Door");
		Desc.strAnimTag = TEXT("Train_Door");
		Desc.bControllerEnabled = true;
		Desc.bPlayDefault = false;
		Desc.bApplyRootMotion = false;
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = true;
		// Door 가중 정점을 본 로컬 공간으로 변환한 X/Y/Z 반경과 중심점이다.
		Desc.vPhysicsHalfExtents = _float3(1.3f * fabsf(SpawnData.vScale.x), 1.25f * fabsf(SpawnData.vScale.y), 0.23f * fabsf(SpawnData.vScale.z));
		Desc.vPhysicsLocalOffset = _float3(1.48f * SpawnData.vScale.x, 0.83f * SpawnData.vScale.y, 0.26f * SpawnData.vScale.z);
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_HotelDoor") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Boss_Barrier_Entrance" == SpawnData.strActorType || L"Boss_Barrier_Exit" == SpawnData.strActorType)
	{
		CBossArenaBarrier::BOSS_ARENA_BARRIER_DESC Desc{};
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;

		// ActorType만으로 입구와 출구를 구분하여 별도 저장 필드를 추가하지 않는다.
		Desc.eRole = L"Boss_Barrier_Entrance" == SpawnData.strActorType ? CBossArenaBarrier::BARRIER_ROLE::ENTRANCE : CBossArenaBarrier::BARRIER_ROLE::EXIT;

		// 현재 Chapter가 실제 사용하는 보스만 클리어 대상으로 허용한다.
		Desc.eClearBossType = Resolve_BossBarrierClearType();
		Desc.eStartBossType = Resolve_BossBarrierStartType();

		if (SPAWN_MONSTER_TYPE::END == Desc.eClearBossType || SPAWN_MONSTER_TYPE::END == Desc.eStartBossType)
			return E_FAIL;

		Desc.fDissolveDuration = 0.8f;
		Desc.vDissolveColor = _float4(0.f, 0.f, 0.f, 0.f);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_BossArenaBarrier") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Boss_Door" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bUseInteractPosition = true;
		Desc.vInteractLocalOffsetXZ = _float2(0.8f, 0.f);
		Desc.fInteractLocalYaw = XMConvertToRadians(-90.f);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Boss_Door");
		Desc.strAnimFolderTag = TEXT("Boss_Door");
		Desc.strAnimTag = TEXT("Boss_Door");
		Desc.bControllerEnabled = true;
		Desc.bPlayDefault = false;
		Desc.bApplyRootMotion = false;
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = true;
		// 전용 좌우 문 본의 가중 정점 범위를 본 로컬 공간으로 변환한 X/Y/Z 반경과 중심점이다.
		Desc.vPhysicsHalfExtents = _float3(0.23f * fabsf(SpawnData.vScale.x), 1.8f * fabsf(SpawnData.vScale.y), 1.17f * fabsf(SpawnData.vScale.z));
		Desc.vPhysicsLocalOffset = _float3(-0.014f * SpawnData.vScale.x, 1.8f * SpawnData.vScale.y, 1.03f * SpawnData.vScale.z);
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_HotelDoor") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Cage" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Cage");
		Desc.strAnimFolderTag = TEXT("Cage");
		Desc.strAnimTag = TEXT("Cage");
		Desc.eAnimTimeUnit = ANIMATION_TIME_UNIT::MILLISEC;
		Desc.bStaticAnimation = true;
		Desc.bControllerEnabled = true;
		Desc.bPlayDefault = false;
		Desc.bApplyRootMotion = false;
		// Cage가 열릴 때 깨울 Monster SpawnID를 그대로 전달한다.
		Desc.iTargetSpawnID = SpawnData.iTargetSpawnID;
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = true;
		// CageHatch_Ctrl 가중 정점의 본 로컬 범위로 바닥에만 충돌체를 배치한다.
		Desc.vPhysicsHalfExtents = _float3(1.06f * fabsf(SpawnData.vScale.x), 0.22f * fabsf(SpawnData.vScale.y), 1.f * fabsf(SpawnData.vScale.z));
		Desc.vPhysicsLocalOffset = _float3(0.97f * SpawnData.vScale.x, -0.13f * SpawnData.vScale.y, 0.f);
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::GROUND);
		// Cage 바닥은 내부 Monster를 지지하는 용도로만 충돌한다.
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_Cage") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Elevator" == SpawnData.strActorType)
	{
		auto SettingIter = m_ElevatorSettingsBySpawnID.find(SpawnData.iSpawnID);

		if (m_ElevatorSettingsBySpawnID.end() == SettingIter)
			return E_FAIL;

		const ELEVATOR_SETTING_DATA& Setting = SettingIter->second;

		CElevator::ELEVATOR_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::STATIC;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Elevator");

		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;

		Desc.vMoveOffset = Setting.vMoveOffset;
		Desc.fMoveSpeed = Setting.fMoveSpeed;
		Desc.bStartAtEnd = Setting.bStartAtEnd;

		// Runtime에서만 Dynamic Kinematic Actor를 생성한다.
		Desc.bPhysicsStartRegistered = true;

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_Elevator") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Elevator_Controller00" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.iTargetSpawnID = SpawnData.iTargetSpawnID;

		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bUseInteractPosition = true;
		Desc.vInteractLocalOffsetXZ = _float2(0.f, -1.38f);
		Desc.fInteractLocalYaw = 0.f;

		// 위/아래 외부 Map Trigger 두 개가 같은 단일 Controller SpawnID를 가리킨다.

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_Elevator_Controller00") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"LadderActivater" == SpawnData.strActorType)
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.iTargetSpawnID = SpawnData.iTargetSpawnID;
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bUseInteractPosition = true;
		Desc.vInteractLocalOffsetXZ = _float2(0.f, -1.f);
		Desc.fInteractLocalYaw = 0.f;

		// 외부 상호작용 Trigger와 별도로 Activater 본체의 일반 Collider를 생성한다.
		Desc.bUsePhysics = true;
		Desc.bPhysicsStartRegistered = true;
		Desc.vPhysicsHalfExtents = _float3(0.6f * fabsf(SpawnData.vScale.x), 0.75f * fabsf(SpawnData.vScale.y), 0.4f * fabsf(SpawnData.vScale.z));
		Desc.vPhysicsLocalOffset = _float3(0.f, 0.75f * fabsf(SpawnData.vScale.y), 0.f);
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_LadderActivater") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"RollingBallTrap" == SpawnData.strActorType)
	{
		auto SettingIter = m_RollingBallSettingsBySpawnID.find(SpawnData.iSpawnID);

		if (m_RollingBallSettingsBySpawnID.end() == SettingIter)
			return E_FAIL;

		const ROLLING_BALL_SETTING_DATA& Setting = SettingIter->second;

		CRollingBallTrap::ROLLING_BALL_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::STATIC;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.strModelPrototypeTag = TEXT("Prototype_Component_Model_RollingBall");
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;

		// 저장된 절대 월드 위치를 변환 없이 공의 경로로 전달한다.
		Desc.Waypoints = Setting.WaypointPositions;

		Desc.vResetPosition = SpawnData.vPosition;
		Desc.fRadius = Setting.fRadius;
		Desc.fTriggerRadius = Setting.fTriggerRadius;
		Desc.fMass = Setting.fMass;
		Desc.fMoveSpeed = Setting.fMoveSpeed;
		Desc.fSteerGain = Setting.fSteerGain;
		Desc.fMaxSteerAcceleration = Setting.fMaxSteerAcceleration;
		Desc.fLookAheadDistance = Setting.fLookAheadDistance;
		Desc.fWaypointRadius = Setting.fWaypointRadius;
		Desc.fFinishRadius = Setting.fFinishRadius;
		Desc.fHiddenTime = Setting.fHiddenTime;
		Desc.fDamage = Setting.fDamage;
		Desc.fKillY = Setting.fKillY;
		Desc.fStuckSpeed = Setting.fStuckSpeed;
		Desc.fStuckTime = Setting.fStuckTime;
		Desc.bPhysicsStartRegistered = true;

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_RollingBallTrap") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"PressurePlate" == SpawnData.strActorType)
	{
		CPressurePlate::PRESSURE_PLATE_DESC Desc{};

		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

		// 발판 하나는 발사구 3개를 관리하는 NeedleLauncher 하나만 가리킨다.
		Desc.iTargetSpawnID = SpawnData.iTargetSpawnID;

		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;

		// 즉시 1회, 1초 후 1회, 2초 후 1회 발사한 뒤 3초에 발판 복귀를 시작한다.
		Desc.fResetDelay = 3.f;

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_PressurePlate") : SpawnData.strObjectPrototypeTag;

		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"NeedleLauncher" == SpawnData.strActorType)
	{
		CNeedleLauncher::NEEDLE_LAUNCHER_DESC Desc{};

		// 모델은 GAMEPLAY Prototype을 사용한다.
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::STATIC;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

		// Projectile은 현재 Chapter Layer에 추가한다.
		Desc.eLayerLevel = m_eCurrentLevel;

		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;

		// 한 번 활성화되면 1초 간격으로 총 3회 발사한다.
		Desc.iBurstCount = 3;
		Desc.fShotInterval = 1.f;

		Desc.fNeedleSpeed = 30.f;
		Desc.fNeedleRange = 20.f;
		Desc.fNeedleRadius = 0.08f;
		Desc.fNeedleDamage = 15.f;

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_NeedleLauncher") : SpawnData.strObjectPrototypeTag;

		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"SteppingStone" == SpawnData.strActorType)
	{
		CSteppingStone::STEPPING_STONE_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::STATIC;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.iTargetSpawnID = SpawnData.iTargetSpawnID;
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bPhysicsStartRegistered = true;

		// 콜라이더 크기와 이동량은 SteppingStone 전용 Desc 기본값을 사용한다.
		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_SteppingStone") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Monastery_Bridge" == SpawnData.strActorType)
	{
		CMonasteryBridge::MONASTERY_BRIDGE_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::STATIC;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;
		Desc.bPhysicsStartRegistered = true;

		// Bridge 전용 Desc 기본값으로 Y축 8, 5초, Dynamic Kinematic 이동을 사용한다.
		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_MonasteryBridge") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}

	if (L"Monastery_WallDeco01" == SpawnData.strActorType)
	{
		CMapTrap::MAP_TRAP_DESC Desc{};
		Desc.eVisualType = CMapGameplayObject::VISUAL_TYPE::SKINNED;
		Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
		Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
		Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
		Desc.vScale = SpawnData.vScale;

		// SD_WallTrap Bone의 끝 피해 Trigger와 원통 본체 Blocking Capsule 설정이다.
		Desc.bPhysicsStartRegistered = true;
		Desc.vPhysicsHalfExtents = _float3(SpawnData.vMapTrapHitboxHalfExtents.x * fabsf(SpawnData.vScale.x), SpawnData.vMapTrapHitboxHalfExtents.y * fabsf(SpawnData.vScale.y), SpawnData.vMapTrapHitboxHalfExtents.z * fabsf(SpawnData.vScale.z));
		Desc.vPhysicsLocalOffset = _float3(SpawnData.vMapTrapHitboxLocalOffset.x * fabsf(SpawnData.vScale.x), SpawnData.vMapTrapHitboxLocalOffset.y * fabsf(SpawnData.vScale.y), SpawnData.vMapTrapHitboxLocalOffset.z * fabsf(SpawnData.vScale.z));
		Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::TRIGGER);
		Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER);
		Desc.fDamage = SpawnData.fMapTrapDamage;
		Desc.fPushDistance = SpawnData.fMapTrapPushDistance;
		// Capsule의 길이축은 로컬 X축이며 원형 단면은 Y/Z 중 큰 스케일을 적용한다.
		Desc.fBlockingRadius = SpawnData.fMapTrapBlockingRadius * max(fabsf(SpawnData.vScale.y), fabsf(SpawnData.vScale.z));
		Desc.fBlockingHalfHeight = SpawnData.fMapTrapBlockingHalfHeight * fabsf(SpawnData.vScale.x);
		Desc.vBlockingLocalOffset = _float3(SpawnData.vMapTrapBlockingLocalOffset.x * fabsf(SpawnData.vScale.x), SpawnData.vMapTrapBlockingLocalOffset.y * fabsf(SpawnData.vScale.y), SpawnData.vMapTrapBlockingLocalOffset.z * fabsf(SpawnData.vScale.z));

		const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_MapTrap") : SpawnData.strObjectPrototypeTag;
		return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
	}




	// 등록하지 않은 ActorType은 기존 흐름처럼 건너뛴다.
	return S_OK;
}

HRESULT CMapGameplayObjectManager::Spawn_Ladder(const MAP_GAMEPLAY_SPAWN_DATA& SpawnData)
{
	CLadder::LADDER_DESC Desc{};

	// ActorType을 Ladder 동작 타입과 길이별 모델 태그로 변환한다.
	if (FAILED(CLadder::Resolve_ActorType(SpawnData.strActorType, &Desc)))
		return E_FAIL;

	Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
	Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
	Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
	Desc.vScale = SpawnData.vScale;
	Desc.bStartOpen = SpawnData.bStartOpen;

	// Ladder 본체의 실제 Local 축에 맞춰 X는 앞뒤 두께, Z는 좌우 폭으로 사용한다.
	_float fPhysicsHalfDepth = 0.06f;
	_float fPhysicsHalfHeight = 2.5f;
	_float fPhysicsHalfWidth = 0.44f;
	_float fPhysicsCenterX = -0.161f;
	_float fPhysicsCenterZ = -0.01f;

	if (L"Ladder_Static_3_5m" == SpawnData.strActorType)
		fPhysicsHalfHeight = 1.75f;
	else if (L"Ladder_Static_7m" == SpawnData.strActorType)
		fPhysicsHalfHeight = 3.5f;
	else if (L"Ladder_Static_7_5m" == SpawnData.strActorType)
		fPhysicsHalfHeight = 3.75f;
	else if (L"Ladder_MetalWood" == SpawnData.strActorType)
	{
		// MetalWood 모델의 닫힌 Pose 본체 Bounds를 사용한다.
		fPhysicsHalfDepth = 0.24f;
		fPhysicsHalfWidth = 0.5f;
		fPhysicsCenterX = -0.057f;
		fPhysicsCenterZ = 0.f;
	}
	else if (L"Ladder_MetalWood6M" == SpawnData.strActorType)
	{
		// MetalWood6M 모델의 닫힌 Pose 본체 Bounds를 사용한다.
		fPhysicsHalfHeight = 3.f;
		fPhysicsHalfDepth = 0.27f;
		fPhysicsHalfWidth = 0.44f;
		fPhysicsCenterX = 0.031f;
		fPhysicsCenterZ = 0.f;
	}

	Desc.bUsePhysics = true;
	Desc.bPhysicsStartRegistered = true;
	Desc.vPhysicsHalfExtents = _float3(fPhysicsHalfDepth * fabsf(SpawnData.vScale.x), fPhysicsHalfHeight * fabsf(SpawnData.vScale.y), fPhysicsHalfWidth * fabsf(SpawnData.vScale.z));
	Desc.vPhysicsLocalOffset = _float3(fPhysicsCenterX * fabsf(SpawnData.vScale.x), fPhysicsHalfHeight * fabsf(SpawnData.vScale.y), fPhysicsCenterZ * fabsf(SpawnData.vScale.z));
	Desc.iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
	Desc.iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

	// Static과 Skinned 모두 하나의 Ladder GameObject Prototype을 사용한다.
	return Add_SpawnedObject(SpawnData, TEXT("Prototype_GameObject_Ladder"), &Desc);
}

HRESULT CMapGameplayObjectManager::Spawn_NPC(const MAP_GAMEPLAY_SPAWN_DATA& SpawnData)
{
	CNPC::NPC_DESC Desc{};

	if (FAILED(CNPC::Resolve_ActorType(SpawnData.strActorType, &Desc)))
		return E_FAIL;

	Desc.iSpawnID = SpawnData.iSpawnID;
	Desc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
	Desc.vTranslate = _float4(SpawnData.vPosition.x, SpawnData.vPosition.y, SpawnData.vPosition.z, 1.f);
	Desc.vRotationDegree = _float3(XMConvertToRadians(SpawnData.vRotation.x), XMConvertToRadians(SpawnData.vRotation.y), XMConvertToRadians(SpawnData.vRotation.z));
	Desc.vScale = SpawnData.vScale;

	// fInteractionRange는 사용하지 않고 SpawnID로 연결된 외부 Map Trigger Box를 사용한다.
	Desc.bApplyRootMotion = false;

	const _wstring strPrototypeTag = SpawnData.strObjectPrototypeTag.empty() ? TEXT("Prototype_GameObject_NPC") : SpawnData.strObjectPrototypeTag;

	return Add_SpawnedObject(SpawnData, strPrototypeTag, &Desc);
}

HRESULT CMapGameplayObjectManager::Add_SpawnedObject(const MAP_GAMEPLAY_SPAWN_DATA& SpawnData, const _wstring& strPrototypeTag, void* pDesc)
{
	if (nullptr == m_pGameInstance || nullptr == pDesc || strPrototypeTag.empty())
		return E_FAIL;

	if (UINT_MAX == SpawnData.iSpawnID || m_SpawnedObjectsByID.end() != m_SpawnedObjectsByID.find(SpawnData.iSpawnID))
		return E_FAIL;

	CMapGameplayObject* pObject = m_pGameInstance->Clone_Prototype<CMapGameplayObject>(ETOUI(m_ePrototypeLevel), strPrototypeTag, pDesc);

	if (nullptr == pObject)
		return E_FAIL;

	// Player 상호작용 이벤트가 Trigger의 TargetSpawnID와 동일한 객체를 식별할 수 있도록 보관한다.
	pObject->Set_SpawnID(SpawnData.iSpawnID);

	if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eCurrentLevel), TEXT("Layer_MapObject"), pObject)))
	{
		Safe_Release(pObject);
		return E_FAIL;
	}

	// Layer와 별도로 Manager가 SpawnID 조회용 참조를 보유한다.
	m_SpawnedObjectsByID.emplace(SpawnData.iSpawnID, pObject);
	Safe_AddRef(pObject);

	return S_OK;
}

CMapGameplayObject* CMapGameplayObjectManager::Find_SpawnedObject(_uint iSpawnID) const
{
	if (UINT_MAX == iSpawnID)
		return nullptr;

	auto Iter = m_SpawnedObjectsByID.find(iSpawnID);

	if (m_SpawnedObjectsByID.end() == Iter || nullptr == Iter->second || Iter->second->Is_InActive())
		return nullptr;

	return Iter->second;
}

HRESULT CMapGameplayObjectManager::Transfer_PlayerToElevator(_uint iElevatorSpawnID, Engine::CGameObject* pInstigator, _float3* pOutLandingPosition)
{
	if (UINT_MAX == iElevatorSpawnID || nullptr == pInstigator || nullptr == pOutLandingPosition)
		return E_FAIL;

	CPlayer* pPlayer = dynamic_cast<CPlayer*>(pInstigator);
	CElevator* pTargetElevator = dynamic_cast<CElevator*>(Find_SpawnedObject(iElevatorSpawnID));

	if (nullptr == pPlayer || nullptr == pTargetElevator)
		return E_FAIL;

	_float3 vLandingPosition{};
	_float fLandingYaw = 0.f;

	if (FAILED(pTargetElevator->Get_TransferLandingPose(&vLandingPosition, &fLandingYaw)))
		return E_FAIL;

	// Player를 이동하기 전에 Elevator가 정상적으로 출발할 수 있는지 먼저 확인한다.
	if (FAILED(pTargetElevator->Start_MovingFromHeight(vLandingPosition.y)))
		return E_FAIL;

	for (const auto& Pair : m_SpawnedObjectsByID)
	{
		CMapGameplayObject* pMapObject = Pair.second;

		if (nullptr == pMapObject)
			continue;

		// 순간이동 전 위치에 있던 상호작용 후보를 Player에서 제거한다.
		pPlayer->End_Overlap(pMapObject);

		CElevator* pElevator = dynamic_cast<CElevator*>(pMapObject);

		if (nullptr != pElevator)
		{
			// 기존 Elevator가 다음 프레임에도 Player를 운반하지 않도록 한다.
			pElevator->End_Overlap(pPlayer);
		}
	}

	Respawn_Point TransferPoint{};
	TransferPoint.vPosition = _float4(vLandingPosition.x, vLandingPosition.y, vLandingPosition.z, 1.f);
	TransferPoint.fYaw = fLandingYaw;

	// 기존 공개 함수가 CCT FootPosition과 Player Transform을 함께 이동시킨다.
	pPlayer->Teleport(TransferPoint);

	Engine::CSpringArm* pSpringArm = pPlayer->Get_SpringArmComponent();

	if (nullptr != pSpringArm)
	{
		// 이전 위치에서 보간하지 않고 변경된 Player 위치와 Yaw를 기준으로 다시 계산한다.
		pSpringArm->Clear();
	}

	// 다음 PhysX Begin을 기다리지 않고 목적지 Elevator의 Passenger로 즉시 등록한다.
	pTargetElevator->Begin_Overlap(pPlayer);

	*pOutLandingPosition = vLandingPosition;

	return S_OK;
}

void CMapGameplayObjectManager::Clear_SpawnedObjects()
{
	for (auto& Pair : m_SpawnedObjectsByID)
		Safe_Release(Pair.second);

	m_SpawnedObjectsByID.clear();
}

void CMapGameplayObjectManager::Dispatch_LinkedTrigger(_uint iSpawnID, Engine::CGameObject* pInstigator, _bool bBegin)
{
	if (nullptr == pInstigator)
		return;

	CMapGameplayObject* pTarget = Find_SpawnedObject(iSpawnID);

	if (nullptr == pTarget)
		return;

	// Static/Skinned 여부를 확인하지 않고 공통 Hook을 호출한다.
	if (true == bBegin)
		pTarget->On_LinkedTriggerBegin(pInstigator);
	else
		pTarget->On_LinkedTriggerEnd(pInstigator);
}

void CMapGameplayObjectManager::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{

}

void CMapGameplayObjectManager::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
	const _bool bLinkedActivateEvent =
		EVENT_TYPE_MAP_OBJECT::LADDER_OPEN == eType ||
		EVENT_TYPE_MAP_OBJECT::ELEVATOR_START == eType ||
		EVENT_TYPE_MAP_OBJECT::TRAP_OPEN == eType ||
		EVENT_TYPE_MAP_OBJECT::STEPPING_STONE == eType ||
		EVENT_TYPE_MAP_OBJECT::DOOR_ACTIVATE == eType;

	if (false == bLinkedActivateEvent || nullptr == pData)
		return;

	MAP_GAMEPLAY_ACTIVATE_EVENT_DESC* pEventDesc = static_cast<MAP_GAMEPLAY_ACTIVATE_EVENT_DESC*>(pData);

	if (UINT_MAX == pEventDesc->iTargetSpawnID)
		return;

	CMapGameplayObject* pTarget = Find_SpawnedObject(pEventDesc->iTargetSpawnID);

	if (nullptr == pTarget)
		return;

	if (EVENT_TYPE_MAP_OBJECT::ELEVATOR_START == eType)
	{
		CElevator* pElevator = dynamic_cast<CElevator*>(pTarget);

		if (nullptr != pElevator)
			pElevator->Start_MovingFromHeight(pEventDesc->fActivatorHeight);

		return;
	}

	pTarget->On_LinkedActivate();
}

HRESULT CMapGameplayObjectManager::Load_ElevatorSettings(const _wstring& strChapterName)
{
	if (nullptr == m_pGameInstance || strChapterName.empty())
		return E_FAIL;

	m_ElevatorSettingsBySpawnID.clear();

	const _wstring strFilePath = m_pGameInstance->Get_ResourcePath() + L"\\Map\\Map_Data\\" + strChapterName + L".ElevatorSetting.json";
	ifstream File(strFilePath);

	if (false == File.is_open())
		return S_OK;

	json Root = {};

	try
	{
		File >> Root;

		if (false == Root.contains("ElevatorSettings") || false == Root["ElevatorSettings"].is_array())
			return E_FAIL;

		for (const json& SettingJson : Root["ElevatorSettings"])
		{
			if (false == SettingJson.is_object())
				return E_FAIL;

			ELEVATOR_SETTING_DATA Setting{};
			Setting.iElevatorSpawnID = SettingJson.value("SpawnID", UINT_MAX);
			Setting.fMoveSpeed = SettingJson.value("MoveSpeed", 2.f);
			Setting.bStartAtEnd = SettingJson.value("StartAtEnd", false);

			if (false == SettingJson.contains("MoveOffset"))
				return E_FAIL;

			const json& MoveOffset = SettingJson["MoveOffset"];

			if (false == MoveOffset.is_array() || 3 != MoveOffset.size())
				return E_FAIL;

			Setting.vMoveOffset = _float3(MoveOffset[0].get<_float>(), MoveOffset[1].get<_float>(), MoveOffset[2].get<_float>());

			if (UINT_MAX == Setting.iElevatorSpawnID)
				return E_FAIL;

			// 같은 Elevator SpawnID의 설정이 두 번 들어오면 실패한다.
			if (false == m_ElevatorSettingsBySpawnID.emplace(Setting.iElevatorSpawnID, Setting).second)
				return E_FAIL;
		}
	}
	catch (const json::exception&)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CMapGameplayObjectManager::Validate_ElevatorData() const
{
	// 검증 중에만 사용하는 임시 컨테이너다.
	unordered_set<_uint> SpawnIDs = {};
	unordered_set<_uint> AttachedElevatorIDs = {};
	_uint iElevatorCount = 0;

	for (const auto& SectorPair : m_MapGameplaySpawnsBySector)
	{
		const _uint iSectorID = SectorPair.first;
		const vector<MAP_GAMEPLAY_SPAWN_DATA>& SectorSpawns = SectorPair.second;

		for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : SectorSpawns)
		{
			if (SpawnData.iSectorID != iSectorID || UINT_MAX == SpawnData.iSpawnID)
				return E_FAIL;

			// m_SpawnedObjectsByID가 SpawnID를 전역 키로 사용하므로 중복을 허용하지 않는다.
			if (false == SpawnIDs.emplace(SpawnData.iSpawnID).second)
				return E_FAIL;

			if (L"Elevator" == SpawnData.strActorType)
			{
				++iElevatorCount;

				auto SettingIter = m_ElevatorSettingsBySpawnID.find(SpawnData.iSpawnID);

				if (m_ElevatorSettingsBySpawnID.end() == SettingIter)
					return E_FAIL;

				const ELEVATOR_SETTING_DATA& Setting = SettingIter->second;
				const _float fMoveLengthSq = Setting.vMoveOffset.x * Setting.vMoveOffset.x + Setting.vMoveOffset.y * Setting.vMoveOffset.y + Setting.vMoveOffset.z * Setting.vMoveOffset.z;

				if (false == std::isfinite(Setting.vMoveOffset.x) || false == std::isfinite(Setting.vMoveOffset.y) || false == std::isfinite(Setting.vMoveOffset.z))
					return E_FAIL;

				if (false == std::isfinite(Setting.fMoveSpeed) || Setting.fMoveSpeed <= 0.f || fMoveLengthSq <= FLT_EPSILON || fabsf(Setting.vMoveOffset.y) <= FLT_EPSILON)
					return E_FAIL;

				continue;
			}

			if (L"Elevator_Controller00" != SpawnData.strActorType)
				continue;

			if (UINT_MAX == SpawnData.iTargetSpawnID)
				return E_FAIL;

			const MAP_GAMEPLAY_SPAWN_DATA* pTargetSpawn = nullptr;

			// Controller와 같은 Sector의 SpawnData만 확인한다.
			for (const MAP_GAMEPLAY_SPAWN_DATA& Candidate : SectorSpawns)
			{
				if (Candidate.iSpawnID == SpawnData.iTargetSpawnID)
				{
					pTargetSpawn = &Candidate;
					break;
				}
			}

			if (nullptr == pTargetSpawn || L"Elevator" != pTargetSpawn->strActorType)
				return E_FAIL;

			// Controller는 하나만 두고 위/아래 외부 Trigger 두 개가 같은 Controller SpawnID를 가리킨다.
			if (false == AttachedElevatorIDs.emplace(SpawnData.iTargetSpawnID).second)
				return E_FAIL;
		}
	}

	// 모든 Elevator는 설정 하나를 가져야 하며 남는 설정도 허용하지 않는다.
	if (iElevatorCount != static_cast<_uint>(m_ElevatorSettingsBySpawnID.size()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMapGameplayObjectManager::Load_RollingBallSettings(const _wstring& strChapterName)
{
	if (nullptr == m_pGameInstance || strChapterName.empty())
		return E_FAIL;

	m_RollingBallSettingsBySpawnID.clear();

	const _wstring strFilePath = m_pGameInstance->Get_ResourcePath() + L"\\Map\\Map_Data\\" + strChapterName + L".RollingBallSetting.json";
	ifstream File(strFilePath);

	if (false == File.is_open())
		return S_OK;

	try
	{
		json Root{};
		File >> Root;

		if (false == Root.contains("RollingBallSettings") || false == Root["RollingBallSettings"].is_array())
			return E_FAIL;

		for (const json& SettingJson : Root["RollingBallSettings"])
		{
			if (false == SettingJson.is_object() || false == SettingJson.contains("Waypoints") || false == SettingJson["Waypoints"].is_array())
				return E_FAIL;

			ROLLING_BALL_SETTING_DATA Setting{};
			Setting.iRollingBallSpawnID = SettingJson.value("SpawnID", UINT_MAX);
			Setting.fRadius = SettingJson.value("Radius", 0.5f);
			Setting.fTriggerRadius = SettingJson.value("TriggerRadius", 0.65f);
			Setting.fMass = SettingJson.value("Mass", 50.f);
			Setting.fMoveSpeed = SettingJson.value("MoveSpeed", 6.f);
			Setting.fSteerGain = SettingJson.value("SteerGain", 4.f);
			Setting.fMaxSteerAcceleration = SettingJson.value("MaxSteerAcceleration", 12.f);
			Setting.fLookAheadDistance = SettingJson.value("LookAheadDistance", 2.f);
			Setting.fWaypointRadius = SettingJson.value("WaypointRadius", 0.75f);
			Setting.fFinishRadius = SettingJson.value("FinishRadius", 1.f);
			Setting.fHiddenTime = SettingJson.value("HiddenTime", 1.f);
			Setting.fDamage = SettingJson.value("Damage", 10.f);
			Setting.fKillY = SettingJson.value("KillY", -1000.f);
			Setting.fStuckSpeed = SettingJson.value("StuckSpeed", 0.1f);
			Setting.fStuckTime = SettingJson.value("StuckTime", 3.f);
			Setting.WaypointPositions.clear();

			for (const json& WaypointJson : SettingJson["Waypoints"])
			{
				if (false == WaypointJson.is_array() || 3 != WaypointJson.size())
					return E_FAIL;

				Setting.WaypointPositions.push_back(_float3(WaypointJson[0].get<_float>(), WaypointJson[1].get<_float>(), WaypointJson[2].get<_float>()));
			}

			if (UINT_MAX == Setting.iRollingBallSpawnID || false == m_RollingBallSettingsBySpawnID.emplace(Setting.iRollingBallSpawnID, Setting).second)
				return E_FAIL;
		}
	}
	catch (const json::exception&)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CMapGameplayObjectManager::Validate_RollingBallData() const
{
	_uint iRollingBallCount = 0;

	for (const auto& SectorPair : m_MapGameplaySpawnsBySector)
	{
		for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : SectorPair.second)
		{
			if (L"RollingBallTrap" != SpawnData.strActorType)
				continue;

			++iRollingBallCount;

			auto SettingIter = m_RollingBallSettingsBySpawnID.find(SpawnData.iSpawnID);

			if (m_RollingBallSettingsBySpawnID.end() == SettingIter || false == Is_ValidRollingBallSettingData(SettingIter->second))
				return E_FAIL;
		}
	}

	// RollingBall Spawn과 설정은 SpawnID 기준으로 정확히 하나씩 대응해야 한다.
	return iRollingBallCount == static_cast<_uint>(m_RollingBallSettingsBySpawnID.size()) ? S_OK : E_FAIL;
}

HRESULT CMapGameplayObjectManager::Validate_SteppingStoneBridgeData() const
{
	for (const auto& SectorPair : m_MapGameplaySpawnsBySector)
	{
		const vector<MAP_GAMEPLAY_SPAWN_DATA>& SectorSpawns = SectorPair.second;

		for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : SectorSpawns)
		{
			if (L"Monastery_Bridge" == SpawnData.strActorType)
			{
				// Bridge는 이벤트 대상이며 다른 Spawn을 다시 가리키지 않는다.
				if (UINT_MAX != SpawnData.iTargetSpawnID)
					return E_FAIL;

				continue;
			}

			if (L"SteppingStone" != SpawnData.strActorType)
				continue;

			if (UINT_MAX == SpawnData.iTargetSpawnID)
				return E_FAIL;

			const MAP_GAMEPLAY_SPAWN_DATA* pTargetBridge = nullptr;

			for (const MAP_GAMEPLAY_SPAWN_DATA& Candidate : SectorSpawns)
			{
				if (Candidate.iSpawnID == SpawnData.iTargetSpawnID)
				{
					pTargetBridge = &Candidate;
					break;
				}
			}

			// 섹터 단위 스폰 때문에 연결 Bridge도 반드시 같은 섹터에 있어야 한다.
			if (nullptr == pTargetBridge || L"Monastery_Bridge" != pTargetBridge->strActorType)
				return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT CMapGameplayObjectManager::Resolve_BossBarrierLinks(_uint iSectorID)
{
	(void)iSectorID;

	const _wstring strExpectedDoorActorType = Resolve_BossBarrierLegacyDoorActorType();

	for (const auto& BarrierSectorPair : m_MapGameplaySpawnsBySector)
	{
		for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : BarrierSectorPair.second)
		{
			if (L"Boss_Barrier_Exit" == SpawnData.strActorType)
			{
				// 출구 장벽은 기존 입구 문을 대체하지 않으므로 외부 Spawn을 연결하지 않는다.
				if (UINT_MAX != SpawnData.iTargetSpawnID)
					return E_FAIL;

				continue;
			}

			if (L"Boss_Barrier_Entrance" != SpawnData.strActorType)
				continue;

			if (UINT_MAX == SpawnData.iTargetSpawnID || strExpectedDoorActorType.empty())
				return E_FAIL;

			const MAP_GAMEPLAY_SPAWN_DATA* pTargetDoorSpawn = nullptr;

			for (const auto& DoorSectorPair : m_MapGameplaySpawnsBySector)
			{
				for (const MAP_GAMEPLAY_SPAWN_DATA& Candidate : DoorSectorPair.second)
				{
					if (Candidate.iSpawnID == SpawnData.iTargetSpawnID)
					{
						pTargetDoorSpawn = &Candidate;
						break;
					}
				}

				if (nullptr != pTargetDoorSpawn)
					break;
			}

			// Chapter01은 Boss_Door, 락사시아 Chapter는 Monastery_Door만 연결할 수 있다.
			if (nullptr == pTargetDoorSpawn || strExpectedDoorActorType != pTargetDoorSpawn->strActorType)
				return E_FAIL;

			CBossArenaBarrier* pBarrier = dynamic_cast<CBossArenaBarrier*>(Find_SpawnedObject(SpawnData.iSpawnID));
			CHotelDoor* pLegacyDoor = dynamic_cast<CHotelDoor*>(Find_SpawnedObject(SpawnData.iTargetSpawnID));

			// 서로 다른 섹터라면 나중에 스폰되는 쪽의 Resolve 호출에서 다시 연결한다.
			if (nullptr == pBarrier || nullptr == pLegacyDoor)
				continue;

			pBarrier->Attach_LegacyDoor(pLegacyDoor);
		}
	}

	return S_OK;
}

SPAWN_MONSTER_TYPE CMapGameplayObjectManager::Resolve_BossBarrierClearType() const
{
	switch (m_iChapterIndex)
	{
	case 0:
		// Chapter01의 축제 인도자 보스다.
		return SPAWN_MONSTER_TYPE::BOSS_FESTIVAL;

	case 2:
	case 5:
		// Chapter02 데이터와 전용 BossMap은 모두 락사시아 최종 페이즈를 사용한다.
		return SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2;

	default:
		// Hotel의 미친 당나귀를 포함한 나머지 Chapter는 BossArenaBarrier를 사용하지 않는다.
		return SPAWN_MONSTER_TYPE::END;
	}
}

SPAWN_MONSTER_TYPE CMapGameplayObjectManager::Resolve_BossBarrierStartType() const
{
	switch (m_iChapterIndex)
	{
	case 0:
		return SPAWN_MONSTER_TYPE::BOSS_FESTIVAL;

	case 2:
	case 5:
		// 클리어 기준은 최종 페이즈지만 전투를 시작하는 쪽은 1페이즈다.
		return SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P1;

	default:
		return SPAWN_MONSTER_TYPE::END;
	}
}

_wstring CMapGameplayObjectManager::Resolve_BossBarrierLegacyDoorActorType() const
{
	switch (m_iChapterIndex)
	{
	case 0:
		return L"Boss_Door";

	case 2:
	case 5:
		return L"Monastery_Door";

	default:
		return {};
	}
}

HRESULT CMapGameplayObjectManager::Resolve_ElevatorLinks(_uint iSectorID)
{
	auto SectorIter = m_MapGameplaySpawnsBySector.find(iSectorID);

	if (m_MapGameplaySpawnsBySector.end() == SectorIter)
		return S_OK;

	for (const MAP_GAMEPLAY_SPAWN_DATA& SpawnData : SectorIter->second)
	{
		if (L"Elevator_Controller00" != SpawnData.strActorType)
			continue;

		CMapActivator* pController = dynamic_cast<CMapActivator*>(Find_SpawnedObject(SpawnData.iSpawnID));
		CElevator* pElevator = dynamic_cast<CElevator*>(Find_SpawnedObject(SpawnData.iTargetSpawnID));

		if (nullptr == pController || nullptr == pElevator)
			return E_FAIL;

		if (CMapActivator::ACTIVATOR_TYPE::ELEVATOR_CONTROLLER != pController->Get_ActivatorType())
			return E_FAIL;

		// 단일 Controller 모델은 Elevator와 함께 움직이고 외부 Trigger 두 개만 위/아래에 고정한다.
		pElevator->Attach_Controller(pController);
	}

	return S_OK;
}

CMapGameplayObjectManager* CMapGameplayObjectManager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CHAPTER_DESC& ChapterDesc, LEVEL eLevel, LEVEL ePrototypeLevel, CStargazerManager* pStargazerManager, _uint iChapterIndex)
{
	CMapGameplayObjectManager* pInstance = new CMapGameplayObjectManager(pDevice, pContext, eLevel, ePrototypeLevel, pStargazerManager, iChapterIndex);

	if (FAILED(pInstance->Initialize(ChapterDesc)))
	{
		MSG_BOX("Failed to Created : CMapGameplayObjectManager");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMapGameplayObjectManager::Free()
{
	if (true == m_bLadderOpenSubscribed && nullptr != m_pGameInstance)
	{
		m_pGameInstance->UnSubscribe(EVENT_TYPE_MAP_OBJECT::LADDER_OPEN, this);
		m_bLadderOpenSubscribed = false;
	}

	if (true == m_bElevatorStartSubscribed && nullptr != m_pGameInstance)
	{
		m_pGameInstance->UnSubscribe(EVENT_TYPE_MAP_OBJECT::ELEVATOR_START, this);
		m_bElevatorStartSubscribed = false;
	}

	if (true == m_bTrapOpenSubscribed && nullptr != m_pGameInstance)
	{
		m_pGameInstance->UnSubscribe(EVENT_TYPE_MAP_OBJECT::TRAP_OPEN, this);
		m_bTrapOpenSubscribed = false;
	}

	if (true == m_bSteppingStoneSubscribed && nullptr != m_pGameInstance)
	{
		m_pGameInstance->UnSubscribe(EVENT_TYPE_MAP_OBJECT::STEPPING_STONE, this);
		m_bSteppingStoneSubscribed = false;
	}

	if (true == m_bDoorActivateSubscribed && nullptr != m_pGameInstance)
	{
		m_pGameInstance->UnSubscribe(EVENT_TYPE_MAP_OBJECT::DOOR_ACTIVATE, this);
		m_bDoorActivateSubscribed = false;
	}

	Clear_SpawnedObjects();

	m_ElevatorSettingsBySpawnID.clear();
	m_RollingBallSettingsBySpawnID.clear();
	m_SpawnedSectorIDs.clear();
	m_MapGameplaySpawnsBySector.clear();

	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);

	__super::Free();
}
