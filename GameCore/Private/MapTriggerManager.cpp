#include "MapTriggerManager.h"
#include "GameInstance.h"
#include "Layer.h"
#include "Trigger.h"
#include "MapGameplayObjectManager.h"
#include "MapGameplayObject.h"
#include "Elevator.h"

CMapTriggerManager::CMapTriggerManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CMapGameplayObjectManager* pMapGameplayObjectManager , LEVEL eLevel)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_pGameInstance{ Engine::CGameInstance::Get_Instance() }
	, m_pMapGameplayObjectManager{ pMapGameplayObjectManager }
	, m_eLevel{eLevel}
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
	Safe_AddRef(m_pMapGameplayObjectManager);
}

HRESULT CMapTriggerManager::Initialize_Triggers(const vector<MAP_TRIGGER_DATA>& Triggers)
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	// Chapter Reset에서 제거된 이전 Trigger 비소유 포인터와 Overlap 기록을 먼저 비운다.
	m_TriggerDataByObject.clear();
	m_OverlapsByTrigger.clear();

	// Chapter Reset 이전에 예약된 순간이동 요청을 제거한다.
	m_pPendingElevatorTransferInstigator = nullptr;
	m_iPendingElevatorTransferSpawnID = UINT_MAX;
	m_fElevatorTransferCooldown = 0.f;
	m_PendingCutSceneRequest = {};

	m_pGameInstance->Bind_TriggerCallback(
		[this](PxTriggerPair* pPairs, PxU32 iCount)
		{
			Handle_TriggerCallback(pPairs, iCount);
		});

	for (const MAP_TRIGGER_DATA& Data : Triggers)
	{
		// 획득 완료로 제거된 Item/Chest Trigger는 같은 Chapter Reset에서도 다시 만들지 않는다.
		if (m_DeactivatedTargetSpawnIDs.end() != m_DeactivatedTargetSpawnIDs.find(Data.iTargetSpawnID))
			continue;

		if (FAILED(Add_TriggerObject(Data)))
			return E_FAIL;
	}

	return S_OK;
}

_bool CMapTriggerManager::Handle_EventKey(const MAP_TRIGGER_DATA& Data, Engine::CGameObject* pOther, _bool bBegin)
{
	if (Data.strEventKey.empty() || nullptr == pOther)
		return false;

	// BOSSDIALOG도 대상 Spawn 없이 동작하는 Begin 전용 이벤트다.
	if ("UI_START" == Data.strEventKey)
	{
		if (true == bBegin && nullptr != m_pGameInstance)
		{
			UISTART_EVENT_DESC EventDesc{};
			EventDesc.iTargetSpawnID = Data.iTargetSpawnID;

			m_pGameInstance->Broadcast(EVENT_TYPE::UI_START, &EventDesc);
		}

		return true;
	}

	if ("CHANGE_ELEVATOR" == Data.strEventKey)
	{
		// End에서는 순간이동 요청을 생성하지 않는다.
		if (false == bBegin)
			return true;

		// 도착 Trigger 재진입과 같은 프레임 중복 요청을 차단한다.
		if (m_fElevatorTransferCooldown > 0.f || nullptr != m_pPendingElevatorTransferInstigator)
			return true;

		if (UINT_MAX == Data.iTargetSpawnID)
			return true;

		// PhysX Callback에서는 Player와 목적지 SpawnID만 예약한다.
		m_pPendingElevatorTransferInstigator = pOther;
		m_iPendingElevatorTransferSpawnID = Data.iTargetSpawnID;

		return true;
	}

	// Ladder 상호작용은 TargetSpawnID로 연결된 CLadder가 체류 상태와 E 입력을 처리한다.
	return false;
}

EVENT_TYPE_MAP_OBJECT CMapTriggerManager::Resolve_InteractionEvent(const string& strEventKey) const
{
	// 컷신 Trigger의 문자열 데이터는 여기서만 타입 이벤트로 변환한다.
	if ("START_CUTSCENE_FESTIVAL" == strEventKey)
		return EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_FESTIVAL;

	if ("START_CUTSCENE_HOTEL" == strEventKey)
		return EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_HOTEL;

	if ("START_CUTSCENE_RAXASIA_P1" == strEventKey)
		return EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_P1;

	if ("START_CUTSCENE_RAXASIA_P2" == strEventKey)
		return EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_P2;

	if ("START_CUTSCENE_RAXASIA_DEAD" == strEventKey)
		return EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_DEAD;

	if ("START_CUTSCENE_SOPHIA_DEAD" == strEventKey)
		return EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_SOPHIA_DEAD;

	return EVENT_TYPE_MAP_OBJECT::END;
}

void CMapTriggerManager::Queue_CutSceneRequest(const MAP_TRIGGER_DATA& Data)
{
	if (Data.strCutSceneAssetName.empty() || false == m_PendingCutSceneRequest.strAssetName.empty())
		return;

	// 컷신만 일회성으로 소비하며 Door의 Begin/End 및 물리 Trigger 수명은 그대로 유지한다.
	if (Data.bCutScenePlayOnce && m_ConsumedCutSceneTriggerIDs.end() != m_ConsumedCutSceneTriggerIDs.find(Data.iTriggerID))
		return;

	m_PendingCutSceneRequest.iTriggerID = Data.iTriggerID;
	m_PendingCutSceneRequest.strAssetName = Data.strCutSceneAssetName;
	m_PendingCutSceneRequest.bPlayOnce = Data.bCutScenePlayOnce;
}

HRESULT CMapTriggerManager::Add_TriggerObject(const MAP_TRIGGER_DATA& Data)
{
	if (nullptr == m_pGameInstance)
		return E_FAIL; 

	Engine::CTrigger::TRIGGER_DESC Desc{};
	Desc.vTranslate = _float4(Data.vPosition.x, Data.vPosition.y, Data.vPosition.z, 1.f);
	Desc.vRotationDegree = _float3(XMConvertToRadians(Data.vRotation.x), XMConvertToRadians(Data.vRotation.y), XMConvertToRadians(Data.vRotation.z));
	Desc.vHalfExtents = _float3(fabsf(Data.vExtents.x * Data.vScale.x), fabsf(Data.vExtents.y * Data.vScale.y), fabsf(Data.vExtents.z * Data.vScale.z));
	Desc.iTargetFilter = Data.iTargetFilter;
	Desc.bOnceTrigger = Data.bOnceTrigger;

	const EVENT_TYPE_MAP_OBJECT eInteractionEvent = Resolve_InteractionEvent(Data.strEventKey);

	// Player 상호작용형 컷신 이벤트는 연결할 객체 ID와 재생할 Asset이 모두 필요하다.
	if (EVENT_TYPE_MAP_OBJECT::END != eInteractionEvent && (UINT_MAX == Data.iTargetSpawnID || Data.strCutSceneAssetName.empty()))
		return E_FAIL;

	// TargetSpawnID가 있는 외부 Trigger는 실제 대상 존재 여부를 확인한다.
	if (UINT_MAX != Data.iTargetSpawnID && "UI_START" != Data.strEventKey)
	{
		if (nullptr == m_pMapGameplayObjectManager)
			return E_FAIL;

		CMapGameplayObject* pTargetObject = m_pMapGameplayObjectManager->Find_SpawnedObject(Data.iTargetSpawnID);

		// TargetSpawnID에 대응하는 오브젝트가 없으면 잘못된 Trigger 데이터다.
		if (nullptr == pTargetObject)
			return E_FAIL;

		if (EVENT_TYPE_MAP_OBJECT::END != eInteractionEvent)
			pTargetObject->Configure_InteractionEvent(eInteractionEvent, Data.strCutSceneAssetName, Data.bCutScenePlayOnce);

		if ("CHANGE_ELEVATOR" == Data.strEventKey)
		{
			// 순간이동 Trigger는 Player만 진입할 수 있도록 제한한다.
			if (ETOUI(FILTER_GROUP::PLAYER) != Data.iTargetFilter)
				return E_FAIL;

			// CHANGE_ELEVATOR의 TargetSpawnID는 이동할 목적지 Elevator를 직접 가리킨다.
			if (nullptr == dynamic_cast<CElevator*>(pTargetObject))
				return E_FAIL;
		}
		else
		{
			// 일반 외부 Trigger는 기존처럼 실제 MapGameplayObject를 논리 오너로 전달한다.
			Desc.pLogicalOwner = pTargetObject;
		}
	}
	
	Desc.OnBeginOverlap = [this, Data](Engine::CGameObject* pOther)
		{
			Dispatch_Begin(Data, pOther);
		};

	Desc.OnEndOverlap = [this, Data](Engine::CGameObject* pOther)
		{
			Dispatch_End(Data, pOther);
		};

	Engine::CTrigger* pTrigger = m_pGameInstance->Clone_Prototype<Engine::CTrigger>(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Trigger"), &Desc);

	if (nullptr == pTrigger)
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(m_eLevel), TEXT("Layer_Trigger"), pTrigger)))
	{
		Safe_Release(pTrigger);
		return E_FAIL;
	}

	// 논리 오너가 소비될 때 같은 TargetSpawnID의 외부 Trigger를 GameCore Manager가 제거한다.
	CMapGameplayObject* pMapGameplayObject = dynamic_cast<CMapGameplayObject*>(Desc.pLogicalOwner);

	if (nullptr != pMapGameplayObject && false == Data.bOnceTrigger)
	{
		m_TriggerDataByObject.emplace(pTrigger, Data);
		m_OverlapsByTrigger.emplace(pTrigger, vector<Engine::CGameObject*>{});
		pMapGameplayObject->Bind_LinkedTriggerDeactivate([this, iTargetSpawnID = Data.iTargetSpawnID]() { Request_Deactivate_TargetTriggers(iTargetSpawnID); });
	}

	return S_OK;
}

void CMapTriggerManager::Handle_TriggerCallback(PxTriggerPair* pPairs, PxU32 iCount)
{
	if (nullptr == pPairs)
		return;

	for (PxU32 i = 0; i < iCount; ++i)
	{
		if (pPairs[i].flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
			continue;

		PxActor* pTriggerActor = pPairs[i].triggerActor;
		PxActor* pOtherActor = pPairs[i].otherActor;

		if (nullptr == pTriggerActor || nullptr == pOtherActor)
			continue;

		Engine::CGameObject* pTriggerOwner = static_cast<Engine::CGameObject*>(pTriggerActor->userData);
		Engine::CGameObject* pOtherOwner = static_cast<Engine::CGameObject*>(pOtherActor->userData);

		if (nullptr == pTriggerOwner || nullptr == pOtherOwner)
			continue;

		if (true == pTriggerOwner->Is_InActive())
			continue;

		// 외부 CTrigger가 아닌 기존 자체 Physics Trigger는 물리 Owner 자신을 사용한다.
		Engine::CGameObject* pLogicalOwner = pTriggerOwner;

		// 외부 CTrigger는 Player에게 물리 Trigger 대신 실제 MapGameplayObject를 전달한다.
		Engine::CTrigger* pExternalTrigger = dynamic_cast<Engine::CTrigger*>(pTriggerOwner);

		if (nullptr != pExternalTrigger)
			pLogicalOwner = pExternalTrigger->Get_LogicalOwner();

		if (pPairs[i].status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			auto OverlapIter = m_OverlapsByTrigger.find(pExternalTrigger);

			if (m_OverlapsByTrigger.end() != OverlapIter && OverlapIter->second.end() == find(OverlapIter->second.begin(), OverlapIter->second.end(), pOtherOwner))
				OverlapIter->second.push_back(pOtherOwner);

			// Trigger 자체에는 충돌 상대를 전달하여 바인딩된 콜백을 실행한다.
			pTriggerOwner->Begin_Overlap(pOtherOwner);

			// Player 등 충돌 상대에게는 실제 논리 오너를 전달한다.
			if (nullptr != pLogicalOwner)
				pOtherOwner->Begin_Overlap(pLogicalOwner);
		}
		else if (pPairs[i].status & PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			auto OverlapIter = m_OverlapsByTrigger.find(pExternalTrigger);

			if (m_OverlapsByTrigger.end() != OverlapIter)
			{
				auto OtherIter = find(OverlapIter->second.begin(), OverlapIter->second.end(), pOtherOwner);

				if (OverlapIter->second.end() != OtherIter)
					OverlapIter->second.erase(OtherIter);
			}

			pTriggerOwner->End_Overlap(pOtherOwner);

			// Begin에서 전달한 동일한 논리 오너를 제거 대상으로 전달한다.
			if (nullptr != pLogicalOwner)
				pOtherOwner->End_Overlap(pLogicalOwner);
		}
	}
}

void CMapTriggerManager::Request_Deactivate_TargetTriggers(_uint iTargetSpawnID)
{
	if (UINT_MAX == iTargetSpawnID)
		return;

	m_DeactivatedTargetSpawnIDs.emplace(iTargetSpawnID);
	m_PendingDeactivateTargetSpawnIDs.emplace(iTargetSpawnID);
}

HRESULT CMapTriggerManager::Flush_PendingActions(_float fTimeDelta, _float3* pOutTransferPosition)
{
	const unordered_set<_uint> PendingTargetSpawnIDs = m_PendingDeactivateTargetSpawnIDs;
	m_PendingDeactivateTargetSpawnIDs.clear();
	CUTSCENE_REQUEST PendingCutSceneRequest = m_PendingCutSceneRequest;
	m_PendingCutSceneRequest = {};
	HRESULT hCutSceneResult = S_OK;

	// 기존 Item 및 Chest Trigger 제거 처리를 유지한다.
	for (const _uint iTargetSpawnID : PendingTargetSpawnIDs)
		Deactivate_TargetTriggers(iTargetSpawnID);

	// PhysX Callback 밖에서 동기 EventBus로 재생을 요청하고 수신자가 기록한 결과를 확인한다.
	if (false == PendingCutSceneRequest.strAssetName.empty())
	{
		PendingCutSceneRequest.hResult = E_FAIL;
		m_pGameInstance->Broadcast(EVENT_TYPE::PLAY_CUTSCENE, &PendingCutSceneRequest);
		hCutSceneResult = PendingCutSceneRequest.hResult;

		// 실제 재생에 성공한 요청만 소비하여 실패 후 재진입 시 다시 시도할 수 있게 한다.
		if (SUCCEEDED(hCutSceneResult) && PendingCutSceneRequest.bPlayOnce && UINT_MAX != PendingCutSceneRequest.iTriggerID)
			m_ConsumedCutSceneTriggerIDs.emplace(PendingCutSceneRequest.iTriggerID);

		if (FAILED(hCutSceneResult))
			OutputDebugStringA(("[Map Trigger CutScene Error] " + PendingCutSceneRequest.strAssetName + "\n").c_str());
	}

	if (m_fElevatorTransferCooldown > 0.f && fTimeDelta > 0.f)
		m_fElevatorTransferCooldown = max(0.f, m_fElevatorTransferCooldown - fTimeDelta);

	if (nullptr == m_pPendingElevatorTransferInstigator || UINT_MAX == m_iPendingElevatorTransferSpawnID)
		return FAILED(hCutSceneResult) ? hCutSceneResult : S_FALSE;

	Engine::CGameObject* pInstigator = m_pPendingElevatorTransferInstigator;
	const _uint iTargetElevatorSpawnID = m_iPendingElevatorTransferSpawnID;

	// 성공 여부와 관계없이 동일 요청이 다음 프레임에 반복되지 않도록 먼저 비운다.
	m_pPendingElevatorTransferInstigator = nullptr;
	m_iPendingElevatorTransferSpawnID = UINT_MAX;

	if (nullptr == m_pMapGameplayObjectManager)
		return E_FAIL;

	_float3 vLandingPosition{};

	if (FAILED(m_pMapGameplayObjectManager->Transfer_PlayerToElevator(iTargetElevatorSpawnID, pInstigator, &vLandingPosition)))
		return E_FAIL;

	// 목적지 Trigger가 다음 PhysX Step에서 Begin을 발생시켜도 재이동하지 않는다.
	m_fElevatorTransferCooldown = ELEVATOR_TRANSFER_COOLDOWN;

	if (nullptr != pOutTransferPosition)
		*pOutTransferPosition = vLandingPosition;

	return FAILED(hCutSceneResult) ? hCutSceneResult : S_OK;
}

void CMapTriggerManager::Deactivate_TargetTriggers(_uint iTargetSpawnID)
{
	if (UINT_MAX == iTargetSpawnID)
		return;

	vector<Engine::CTrigger*> TargetTriggers = {};

	for (const auto& Pair : m_TriggerDataByObject)
	{
		if (Pair.second.iTargetSpawnID == iTargetSpawnID)
			TargetTriggers.push_back(Pair.first);
	}

	for (Engine::CTrigger* pTrigger : TargetTriggers)
	{
		auto DataIter = m_TriggerDataByObject.find(pTrigger);

		if (m_TriggerDataByObject.end() == DataIter)
			continue;

		const MAP_TRIGGER_DATA Data = DataIter->second;
		auto OverlapIter = m_OverlapsByTrigger.find(pTrigger);
		const vector<Engine::CGameObject*> OverlappingObjects = m_OverlapsByTrigger.end() != OverlapIter ? OverlapIter->second : vector<Engine::CGameObject*>{};
		CMapGameplayObject* pLogicalOwner = nullptr != pTrigger ? dynamic_cast<CMapGameplayObject*>(pTrigger->Get_LogicalOwner()) : nullptr;

		for (Engine::CGameObject* pOther : OverlappingObjects)
		{
			if (nullptr == pOther)
				continue;

			// Removed Shape 콜백이 생략돼도 대상 상태와 Player 후보군을 GameCore에서 직접 정리한다.
			Dispatch_End(Data, pOther);

			if (nullptr != pLogicalOwner)
				pOther->End_Overlap(pLogicalOwner);
		}

		if (nullptr != pTrigger)
			pTrigger->Set_InActive();

		m_OverlapsByTrigger.erase(pTrigger);
		m_TriggerDataByObject.erase(pTrigger);
	}
}

void CMapTriggerManager::Dispatch_Begin(const MAP_TRIGGER_DATA& Data, Engine::CGameObject* pOther)
{
	if (nullptr == pOther)
		return;

	// 상호작용 이벤트가 없는 일반 컷신 Trigger만 진입 시점에 즉시 재생한다.
	if (EVENT_TYPE_MAP_OBJECT::END == Resolve_InteractionEvent(Data.strEventKey))
		Queue_CutSceneRequest(Data);

	if (true == Handle_EventKey(Data, pOther, true))
		return;

	if (nullptr == m_pMapGameplayObjectManager || UINT_MAX == Data.iTargetSpawnID)
		return;

	// 대상 타입에 상관없이 SpawnID로 Begin을 전달한다.
	m_pMapGameplayObjectManager->Dispatch_LinkedTrigger(Data.iTargetSpawnID, pOther, true);
}

void CMapTriggerManager::Dispatch_End(const MAP_TRIGGER_DATA& Data, Engine::CGameObject* pOther)
{
	if (nullptr == pOther)
		return;

	if (true == Handle_EventKey(Data, pOther, false))
		return;

	if (nullptr == m_pMapGameplayObjectManager || UINT_MAX == Data.iTargetSpawnID)
		return;

	// 연결 대상이 필요한 경우에만 공통 End Hook을 처리한다.
	m_pMapGameplayObjectManager->Dispatch_LinkedTrigger(Data.iTargetSpawnID, pOther, false);
}


CMapTriggerManager* CMapTriggerManager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CMapGameplayObjectManager* pMapGameplayObjectManager, LEVEL eLevel)
{
	CMapTriggerManager* pInstance = new CMapTriggerManager(pDevice, pContext, pMapGameplayObjectManager,eLevel);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CMapTriggerManager");
		return nullptr;
	}

	return pInstance;
}

void CMapTriggerManager::Free()
{
	m_OverlapsByTrigger.clear();
	m_TriggerDataByObject.clear();
	m_DeactivatedTargetSpawnIDs.clear();
	m_PendingDeactivateTargetSpawnIDs.clear();

	// 보관 중인 비소유 Player 포인터와 목적지 정보를 제거한다.
	m_pPendingElevatorTransferInstigator = nullptr;
	m_iPendingElevatorTransferSpawnID = UINT_MAX;
	m_fElevatorTransferCooldown = 0.f;
	m_PendingCutSceneRequest = {};
	m_ConsumedCutSceneTriggerIDs.clear();

	if (nullptr != m_pGameInstance)
		m_pGameInstance->Bind_TriggerCallback(nullptr);

	Safe_Release(m_pMapGameplayObjectManager);
	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);

	__super::Free();
}
