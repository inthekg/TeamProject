#include "ActorSectorManager.h"
#include "GameInstance.h"
#include "AIObject.h"

CActorSectorManager::CActorSectorManager()
	: m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CActorSectorManager::Initialize()
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	// MapWorld에서 발생하는 섹터 변경 이벤트를 받음 
	m_pGameInstance->Subscribe(EVENT_TYPE::SECTOR_CHANGED, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::ACTOR_DESPAWN, this);

	return S_OK;
}

void CActorSectorManager::Register_Monster(_uint iSectorID, CAIObject* pMonster)
{
	if (UINT_MAX == iSectorID || nullptr == pMonster)
		return;

	m_MonstersBySector[iSectorID].push_back(pMonster);

	// 새로 스폰된 몬스터도 현재 활성 섹터 기준으로 즉시 Sleep 상태
	if (false == m_ActiveSectorIDs.empty())
	{
		const _bool bActive = m_ActiveSectorIDs.end() != m_ActiveSectorIDs.find(iSectorID);
		pMonster->Set_AISleep(false == bActive); 
	}
}

void CActorSectorManager::Update_MonsterSleep(const unordered_set<_uint>* pActiveSectorIDs)
{
	if (nullptr == pActiveSectorIDs)
		return;

	m_ActiveSectorIDs = *pActiveSectorIDs;

	for (auto& Pair : m_MonstersBySector)
	{
		const _uint iSectorID = Pair.first;
		const _bool bActive = m_ActiveSectorIDs.end() != m_ActiveSectorIDs.find(iSectorID);

		for (CAIObject* pMonster : Pair.second)
		{
			if (nullptr == pMonster)
				continue;

			pMonster->Set_AISleep(false == bActive); 
		}
	}
}

//수정
//void CActorSectorManager::OnEvent(EVENT_TYPE etype, Event_Desc* pData)
//{
//	if (EVENT_TYPE::SECTOR_CHANGED != etype || nullptr == pData)
//		return;
//	
//	SECTOR_CHANGED_EVENT_DESC* pSectorChangedDesc = static_cast<SECTOR_CHANGED_EVENT_DESC*>(pData);
//	Update_MonsterSleep(pSectorChangedDesc->pActiveSectorIDs);
//}


void CActorSectorManager::OnEvent(EVENT_TYPE etype, Event_Desc* pData)
{
	if (EVENT_TYPE::SECTOR_CHANGED == etype)
	{
		if(pData == nullptr) return;

		SECTOR_CHANGED_EVENT_DESC* pSectorChangedDesc = static_cast<SECTOR_CHANGED_EVENT_DESC*>(pData);
		// 옛 ActiveSectorIDs와 동일하게 이웃을 포함한 Render 섹터 기준으로 몬스터를 깨운다.
		Update_MonsterSleep(pSectorChangedDesc->pActiveRenderSectorIDs);
	}
	else if (EVENT_TYPE::ACTOR_DESPAWN == etype)
	{
		if (pData == nullptr) return;
		ActorDespawn_EventDesc* Desc = static_cast<ActorDespawn_EventDesc*>(pData);

		CGameObject* pActor = Desc->pActor;
		if (pActor == nullptr) return;

		ACTOR_TYPE eActorType = Desc->eActorType;

		if (eActorType == ACTOR_TYPE::MONSTER || eActorType == ACTOR_TYPE::BOSS)
			UnRegister_Monster(pActor);

	}


}

//추가 
void CActorSectorManager::UnRegister_Monster(CGameObject* pMonster)
{
	for(auto&Pair : m_MonstersBySector)
	{
		auto& Monsters = Pair.second;
		auto it = std::find(Monsters.begin(), Monsters.end(), pMonster);
		if (it != Monsters.end())
		{
			Monsters.erase(it);
			return;
		}
	}
}

CActorSectorManager* CActorSectorManager::Create()
{
	CActorSectorManager* pInstance = new CActorSectorManager();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CActorSectorManager");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CActorSectorManager::Free()
{
	if (nullptr != m_pGameInstance)
		m_pGameInstance->UnSubscribe_AllEvent(this);

	m_ActiveSectorIDs.clear();
	m_MonstersBySector.clear();

	__super::Free();
}