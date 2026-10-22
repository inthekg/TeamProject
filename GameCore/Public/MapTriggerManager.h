#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "Core_Struct.h"

NS_BEGIN(Engine)
class CGameInstance;
class CGameObject;
class CTrigger;
NS_END

NS_BEGIN(GameCore)
class CMapGameplayObjectManager;

class CMapTriggerManager final : public CBase
{
private:
	CMapTriggerManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CMapGameplayObjectManager* pMapGameplayObjectManager,LEVEL eLevel);
	virtual ~CMapTriggerManager() = default;

public:
	HRESULT Initialize_Triggers(const vector<MAP_TRIGGER_DATA>& Triggers);
	void Handle_TriggerCallback(PxTriggerPair* pPairs, PxU32 iCount);

	// PhysX Callback에서 예약한 제거 및 Elevator 순간이동을 처리한다.
	HRESULT Flush_PendingActions(_float fTimeDelta, _float3* pOutTransferPosition);

private:
	_bool Handle_EventKey(const MAP_TRIGGER_DATA& Data, Engine::CGameObject* pOther, _bool bBegin);
	EVENT_TYPE_MAP_OBJECT Resolve_InteractionEvent(const string& strEventKey) const;
	void Queue_CutSceneRequest(const MAP_TRIGGER_DATA& Data);
	HRESULT Add_TriggerObject(const MAP_TRIGGER_DATA& Data);
	void Dispatch_Begin(const MAP_TRIGGER_DATA& Data, Engine::CGameObject* pOther);
	void Dispatch_End(const MAP_TRIGGER_DATA& Data, Engine::CGameObject* pOther);
	void Request_Deactivate_TargetTriggers(_uint iTargetSpawnID);
	void Deactivate_TargetTriggers(_uint iTargetSpawnID);

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	Engine::CGameInstance* m_pGameInstance = nullptr;
	CMapGameplayObjectManager* m_pMapGameplayObjectManager = nullptr;

	LEVEL m_eLevel = { LEVEL::END };

	// GameCore가 생성한 외부 Trigger와 현재 Overlap을 TargetSpawnID 수명에 맞춰 관리한다.
	unordered_map<Engine::CTrigger*, MAP_TRIGGER_DATA> m_TriggerDataByObject = {};
	unordered_map<Engine::CTrigger*, vector<Engine::CGameObject*>> m_OverlapsByTrigger = {};
	unordered_set<_uint> m_DeactivatedTargetSpawnIDs = {};
	unordered_set<_uint> m_PendingDeactivateTargetSpawnIDs = {};

	// PhysX Callback 안에서는 실제 순간이동을 하지 않고 요청만 보관한다.
	Engine::CGameObject* m_pPendingElevatorTransferInstigator = { nullptr };
	_uint m_iPendingElevatorTransferSpawnID = { UINT_MAX };
	_float m_fElevatorTransferCooldown = { 0.f };
	CUTSCENE_REQUEST m_PendingCutSceneRequest = {};
	unordered_set<_uint> m_ConsumedCutSceneTriggerIDs = {};

	static constexpr _float ELEVATOR_TRANSFER_COOLDOWN = { 0.5f };

public:
	static CMapTriggerManager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CMapGameplayObjectManager* pMapGameplayObjectManager, LEVEL eLevel);
	virtual void Free() override;
};

NS_END
