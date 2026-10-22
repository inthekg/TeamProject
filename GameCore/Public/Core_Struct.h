#pragma once

namespace GameCore
{

	typedef struct tagActionData
	{
		ActionCategory	eCategory;
		StatusProfile	profile;
		_wstring		strAnimTrigger;
		float			fStaminaCost = 0.0f;
		_float			fDamage = 0.0f;
		HIT_TYPE		eHitType = HIT_TYPE::End;	// 이 공격을 받은 상대가 피격될 때의 피격 타입
		ATTACK_STYLE	eAttackStyle = ATTACK_STYLE::End; // 공격스타일 찌르기 / 베기 (이펙트용)
		_float			fGroggy = 10.f;
	} ActionData;

	typedef struct tagWeaponData
	{
		WeaponType		eType;
		unordered_map<AttackType, vector<ActionData>>	AttackSteps;

	} WeaponData;


	// 챕터 스폰 json에서 읽어온 몬스터 배치 데이터
	typedef struct tagMonsterSpawnData
	{
		_uint iSpawnID = UINT_MAX;
		_uint iSectorID = UINT_MAX;
		SPAWN_MONSTER_TYPE eMonsterType = SPAWN_MONSTER_TYPE::END;
		SPAWN_WEAPON_TYPE  eWeaponType = SPAWN_WEAPON_TYPE::AUTO;
		_int	iSpawnNumber = { 0 };
		_float3 vPosition = {};
		_float fYawDegree = 0.f;
	} MONSTER_SPAWN_DATA;

	// 섹터 변경 이벤트에 전달할 데이터
	typedef struct tagSectorChangedEventDesc : public Engine::Event_Desc
	{
		_uint iPreviousSectorID = UINT_MAX;
		_uint iCurrentSectorID = UINT_MAX;

		// fog 용 
		const unordered_set<_uint>* pActiveRenderSectorIDs = nullptr;
		// 몬스터 + 게임 로직용
		const unordered_set<_uint>* pActiveSimulationSectorIDs = nullptr;
	} SECTOR_CHANGED_EVENT_DESC;

	// 맵 라이트가 소속된 섹터를 관리하는 데이터
	typedef struct tagMapSectorLightData
	{
		LIGHT eLightType = LIGHT::END;
		_uint iLightIndex = UINT_MAX;
		_uint iOwnerSectorID = UINT_MAX;

		BoundingBox WorldBounds{};
		_bool bUseBounds = false;
		_bool bEnabled = true;
	} MAP_SECTOR_LIGHT_DATA;


	typedef struct tagLightPropFileHeader
	{
		_uint iVersion = 1;
		_uint iLightPropCount = 0;
	} LIGHT_PROP_FILE_HEADER;

	typedef struct tagLightPropFileRecord
	{
		_uint iSectorID = UINT_MAX;
		_float3 vPosition = {};
		_float3 vRotation = {};
		_float3 vScale = { 1.f, 1.f, 1.f };
	} LIGHT_PROP_FILE_RECORD;

	inline const char* To_String(SPAWN_MONSTER_TYPE eType)
	{
		switch (eType)
		{
		case SPAWN_MONSTER_TYPE::BUTTLER_BASE:    return "BUTTLER_BASE";
		case SPAWN_MONSTER_TYPE::BUTTLER_TRAIN:   return "BUTTLER_TRAIN";
		case SPAWN_MONSTER_TYPE::BUTTLER_NOHEAD:  return "BUTTLER_NOHEAD";
		case SPAWN_MONSTER_TYPE::BUTTLER_RANGE:   return "BUTTLER_RANGE";
		case SPAWN_MONSTER_TYPE::MONSTER_ELITE:   return "MONSTER_ELITE";
		case SPAWN_MONSTER_TYPE::TRAINING:        return "TRAINING";
		case SPAWN_MONSTER_TYPE::WATCH_DOG:       return "WATCH_DOG";
		case SPAWN_MONSTER_TYPE::BOSS_FESTIVAL:   return "BOSS_FESTIVAL";
		case SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P1: return "BOSS_RAXASIA_P1";
		case SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2: return "BOSS_RAXASIA_P2";
		case SPAWN_MONSTER_TYPE::BALISTAR:        return "BALISTAR";
		case SPAWN_MONSTER_TYPE::CANNON:          return "CANNON";
		case SPAWN_MONSTER_TYPE::BUTTERFLY:       return "BUTTERFLY";
		case SPAWN_MONSTER_TYPE::BIG_DOG:         return "BIG_DOG";
		case SPAWN_MONSTER_TYPE::ZOMBIE:          return "ZOMBIE";
		case SPAWN_MONSTER_TYPE::PRIEST:          return "PRIEST";
		case SPAWN_MONSTER_TYPE::BOSS_DONKEY:     return "BOSS_DONKEY";
		default:                                  return "BUTTLER_BASE";
		}
	}

	inline SPAWN_MONSTER_TYPE To_MonsterType(const string& strType)
	{
		if ("BUTTLER_BASE" == strType)   return SPAWN_MONSTER_TYPE::BUTTLER_BASE;
		if ("BUTTLER_TRAIN" == strType)  return SPAWN_MONSTER_TYPE::BUTTLER_TRAIN;
		if ("BUTTLER_NOHEAD" == strType) return SPAWN_MONSTER_TYPE::BUTTLER_NOHEAD;
		if ("BUTTLER_RANGE" == strType)  return SPAWN_MONSTER_TYPE::BUTTLER_RANGE;
		if ("MONSTER_ELITE" == strType)  return SPAWN_MONSTER_TYPE::MONSTER_ELITE;
		if ("TRAINING" == strType)       return SPAWN_MONSTER_TYPE::TRAINING;
		if ("WATCH_DOG" == strType)      return SPAWN_MONSTER_TYPE::WATCH_DOG;
		if ("BOSS_FESTIVAL" == strType)  return SPAWN_MONSTER_TYPE::BOSS_FESTIVAL;
		if ("BOSS_RAXASIA_P1" == strType) return SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P1;
		if ("BOSS_RAXASIA_P2" == strType) return SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2;
		if ("BALISTAR" == strType)        return SPAWN_MONSTER_TYPE::BALISTAR;
		if ("CANNON" == strType)          return SPAWN_MONSTER_TYPE::CANNON;
		if ("BUTTERFLY" == strType)       return SPAWN_MONSTER_TYPE::BUTTERFLY;
		if ("BIG_DOG" == strType)         return SPAWN_MONSTER_TYPE::BIG_DOG;
		if ("ZOMBIE" == strType)          return SPAWN_MONSTER_TYPE::ZOMBIE;
		if ("PRIEST" == strType)          return SPAWN_MONSTER_TYPE::PRIEST;
		if ("BOSS_DONKEY" == strType)     return SPAWN_MONSTER_TYPE::BOSS_DONKEY;
		if ("BUTTLER" == strType)         return SPAWN_MONSTER_TYPE::BUTTLER_BASE;
		return SPAWN_MONSTER_TYPE::BUTTLER_BASE;
	}

	inline const char* To_String(SPAWN_WEAPON_TYPE eType)
	{
		switch (eType)
		{
		case SPAWN_WEAPON_TYPE::CANDLE: return "CANDLE";
		case SPAWN_WEAPON_TYPE::SIGN:   return "SIGN";
		case SPAWN_WEAPON_TYPE::KNIFE:  return "KNIFE";
		case SPAWN_WEAPON_TYPE::RANGE:  return "RANGE";
		case SPAWN_WEAPON_TYPE::ELITE:  return "ELITE";
		case SPAWN_WEAPON_TYPE::BOSS:   return "BOSS";
		default:                        return "AUTO";
		}
	}

	inline SPAWN_WEAPON_TYPE To_WeaponType(const string& strType)
	{
		if ("CANDLE" == strType) return SPAWN_WEAPON_TYPE::CANDLE;
		if ("SIGN" == strType)   return SPAWN_WEAPON_TYPE::SIGN;
		if ("KNIFE" == strType)  return SPAWN_WEAPON_TYPE::KNIFE;
		if ("RANGE" == strType)  return SPAWN_WEAPON_TYPE::RANGE;
		if ("ELITE" == strType)  return SPAWN_WEAPON_TYPE::ELITE;
		if ("BOSS" == strType)   return SPAWN_WEAPON_TYPE::BOSS;
		return SPAWN_WEAPON_TYPE::AUTO;
	}

	inline const char* To_String(ITEM_TYPE eType)
	{
		switch (eType)
		{
		case ITEM_TYPE::WEAPON: return "WEAPON";
		default: return "ERGO";
		}
	}

	inline ITEM_TYPE To_ItemType(const string& strType)
	{
		if ("WEAPON" == strType) return ITEM_TYPE::WEAPON;
		return ITEM_TYPE::ERGO;
	}


	typedef struct tag_PlayerFxEvent
	{
		PLAYER_FX	eType = PLAYER_FX::End;
		_bool		bActive = false;
		_float3		vWorldPos = {};
		_int		iID = {};	// 식별값 
		_int		iSubID = {}; // 세부 식별값
	}PLAYER_FX_EVENT;

	typedef struct tagMapGameplayObjectSpawnData
	{
		_uint iSpawnID = UINT_MAX;
		_uint iSectorID = UINT_MAX;
		_uint iTargetSpawnID = UINT_MAX;
		_wstring strActorType = L"";
		_wstring strObjectPrototypeTag = L"";
		_float3 vPosition = {};
		_float3 vRotation = {}; // 에디터 저장값은 degree 기준이다.
		_float3 vScale = { 1.f, 1.f, 1.f };
		_bool bActivated = false;
		_float fInteractionRange = 3.f;

		// Stargazer ActorType을 유지하면서 호텔 중앙 대형 모델을 사용하는지 구분한다.
		_bool bUseHotelModel = false;

		// 별바라기 텔레포트 UI에 표시할 지역 이름
		_wstring strStargazerLocationName = {};
		// 별바라기 텔레포트 UI 우측에 표시할 프리뷰 텍스처 경로
		string strStargazerPreviewTexturePath = {};

		_bool bStartOpen = false;

		// Item_Lily와 TreasureChest 내부 Item이 전달할 보상 정보다.
		ITEM_TYPE eItemType = ITEM_TYPE::ERGO;
		_wstring strItemName = L"Ergo";
		_uint iItemCount = 1;
		_uint iErgoMinAmount = 100;
		_uint iErgoMaxAmount = 500;

		// Monastery_WallDeco01의 끝 피해 Trigger와 원통 본체 Blocking Capsule 설정이다.
		_float fMapTrapDamage = 10.f;
		_float fMapTrapPushDistance = 0.5f;
		_float3 vMapTrapHitboxHalfExtents = { 0.5f, 0.5f, 0.5f };
		_float3 vMapTrapHitboxLocalOffset = { 0.f, 0.f, 0.f };
		_float fMapTrapBlockingRadius = 0.9f;
		_float fMapTrapBlockingHalfHeight = 2.44f;
		_float3 vMapTrapBlockingLocalOffset = { 3.14f, 0.f, 0.f };
	} MAP_GAMEPLAY_SPAWN_DATA;

	// MapGameplay 활성화 장치가 연결 대상 SpawnID를 전달하는 이벤트 데이터
	typedef struct tagMapGameplayActivateEventDesc : public Engine::Event_Desc
	{
		_uint iTargetSpawnID = UINT_MAX;
		_float fActivatorHeight = 0.f;
	} MAP_GAMEPLAY_ACTIVATE_EVENT_DESC;

	// Cage가 연결된 Monster 하나에 열림 상태를 전달할 때 사용하는 이벤트 데이터다.
	typedef struct tagCageOpenEventDesc : public Engine::Event_Desc
	{
		_uint iTargetMonsterSpawnID = UINT_MAX;
	} CAGE_OPEN_EVENT_DESC;

	typedef struct tagMapTriggerData
	{
		_uint iSectorID = UINT_MAX;
		_uint iTriggerID = UINT_MAX;
		_uint iTargetSpawnID = UINT_MAX;
		_float3 vPosition = {};
		_float3 vRotation = {};
		_float3 vScale = { 1.f, 1.f, 1.f };
		_float3 vExtents = { 0.5f, 0.5f, 0.5f };
		_uint iTargetFilter = ETOUI(Engine::FILTER_GROUP::PLAYER);
		_bool bOnceTrigger = false;
		string strName = "Trigger";
		string strEventKey = "";
		string strCutSceneAssetName = "";
		_bool bCutScenePlayOnce = false;
	} MAP_TRIGGER_DATA;

	typedef struct tagUIStartEventDesc : public Engine::Event_Desc
	{
		_uint iTargetSpawnID = UINT_MAX;
	} UISTART_EVENT_DESC;

	typedef struct tagCutSceneRequest : public Engine::Event_Desc
	{
		_uint iTriggerID = UINT_MAX;
		_uint iTargetSpawnID = UINT_MAX;
		string strAssetName = "";
		_bool bPlayOnce = false;
		HRESULT hResult = E_FAIL;
	} CUTSCENE_REQUEST;


	typedef struct tagRespawnPoint
	{
		_uint	iChapterIndex = { 0 };
		_float4 vPosition = {};
		//Radian 각도
		_float	fYaw = { 0.f };
	} Respawn_Point;

	typedef struct tagChangeChaptetEventDesc : public Event_Desc
	{
		_uint iChapterIndex = { 0 };
		Respawn_Point SpawnPoint = {};
	} ChangeChapter_EventDesc;

	typedef struct tagRespawnPoint_EventDesc : public Event_Desc
	{
		Respawn_Point SpawnPoint = {};
	} RespawnPoint_EventDesc;


	typedef struct tagActorDespawnEventDesc : public Event_Desc
	{
		SPAWN_MONSTER_TYPE eMonsterType = { SPAWN_MONSTER_TYPE::END };
	} BossClear_EventDesc;

	typedef struct tagActivateEventDesc : public Event_Desc
	{
		// Player가 상호작용 대상에 맞는 동작을 선택할 때 사용하는 공용 타입이다.
		INTERACTION_TYPE eInteractionType = { INTERACTION_TYPE::END };
		_float4 vPosition = {};
		_float4 vQuat = {};
	} Activate_EventDesc;
}
