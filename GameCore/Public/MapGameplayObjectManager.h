#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "ChapterMapData.h"
#include "IListener.h"
#include "ElevatorSettingData.h"
#include "RollingBallSettingData.h"

NS_BEGIN(Engine)

class CGameInstance;
class CGameObject;

NS_END

NS_BEGIN(GameCore)

class CMapGameplayObject;
class CStargazerManager;

class GAMECORE_DLL CMapGameplayObjectManager final : public CBase, public Engine::IListener
{
private:
	CMapGameplayObjectManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLevel, LEVEL ePrototypeLevel, CStargazerManager* pStargazerManager, _uint iChapterIndex);
	virtual ~CMapGameplayObjectManager() = default;

public:
	HRESULT Initialize(const CHAPTER_DESC& ChapterDesc);

	HRESULT Spawn_All();
	HRESULT Spawn_ForSector(_uint iSectorID);

	// Trigger가 TargetSpawnID로 실제 MapGameplayObject를 찾을 때 사용한다.
	CMapGameplayObject* Find_SpawnedObject(_uint iSpawnID) const;

	// 기존 Player Teleport를 사용해 지정 Elevator로 이동한다.
	HRESULT Transfer_PlayerToElevator(_uint iElevatorSpawnID, Engine::CGameObject* pInstigator, _float3* pOutLandingPosition);

	void Dispatch_LinkedTrigger(_uint iSpawnID, Engine::CGameObject* pInstigator, _bool bBegin);

	virtual void OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;
	virtual void OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;
private:
	HRESULT Load_ElevatorSettings(const _wstring& strChapterName);
	HRESULT Validate_ElevatorData() const;
	HRESULT Resolve_ElevatorLinks(_uint iSectorID);
	HRESULT Load_RollingBallSettings(const _wstring& strChapterName);
	HRESULT Validate_RollingBallData() const;
	HRESULT Validate_SteppingStoneBridgeData() const;
	HRESULT Resolve_BossBarrierLinks(_uint iSectorID);
	SPAWN_MONSTER_TYPE Resolve_BossBarrierClearType() const;
	SPAWN_MONSTER_TYPE Resolve_BossBarrierStartType() const;
	_wstring Resolve_BossBarrierLegacyDoorActorType() const;

	HRESULT Spawn_MapGameplayObject(const MAP_GAMEPLAY_SPAWN_DATA& SpawnData);
	HRESULT Spawn_Ladder(const MAP_GAMEPLAY_SPAWN_DATA& SpawnData);
	HRESULT Spawn_NPC(const MAP_GAMEPLAY_SPAWN_DATA& SpawnData);
	HRESULT Add_SpawnedObject(const MAP_GAMEPLAY_SPAWN_DATA& SpawnData, const _wstring& strPrototypeTag, void* pDesc);

	void Clear_SpawnedObjects();

private:
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };
	Engine::CGameInstance* m_pGameInstance = { nullptr };

private:
	// Sector별 MapGameplayObject Spawn 데이터를 보관한다.
	unordered_map<_uint, ELEVATOR_SETTING_DATA> m_ElevatorSettingsBySpawnID = {};
	unordered_map<_uint, ROLLING_BALL_SETTING_DATA> m_RollingBallSettingsBySpawnID = {};
	unordered_map<_uint, vector<MAP_GAMEPLAY_SPAWN_DATA>> m_MapGameplaySpawnsBySector = {};
	unordered_set<_uint> m_SpawnedSectorIDs = {};

	// ChapterMapManager가 수명을 소유하는 비소유 참조다.
	CStargazerManager* m_pStargazerManager = { nullptr };
	_uint m_iChapterIndex = UINT_MAX;

	// Layer와 별도로 SpawnID 조회용 참조를 보유한다.
	unordered_map<_uint, CMapGameplayObject*> m_SpawnedObjectsByID = {};

	_bool m_bLadderOpenSubscribed = { false };
	_bool m_bElevatorStartSubscribed = { false };
	_bool m_bTrapOpenSubscribed = { false };
	_bool m_bSteppingStoneSubscribed = { false };
	_bool m_bDoorActivateSubscribed = { false };

	LEVEL m_eCurrentLevel = { LEVEL::GAMEPLAY };
	// Logo와 Gameplay가 각자 소유한 Model/GameObject Prototype 레벨을 구분한다.
	LEVEL m_ePrototypeLevel = { LEVEL::GAMEPLAY };

public:
	static CMapGameplayObjectManager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CHAPTER_DESC& ChapterDesc, LEVEL eLevel = LEVEL::GAMEPLAY, LEVEL ePrototypeLevel = LEVEL::GAMEPLAY, CStargazerManager* pStargazerManager = nullptr, _uint iChapterIndex = UINT_MAX);
	virtual void Free() override;
};

NS_END
